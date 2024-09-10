#include "SyncedScannerManager.h"

#include "spdlog/spdlog.h"

Wave::Wave(const int wave_id, Redis& shared_redis, TickSynchronizer& synchronizer) : redis(shared_redis),
    tick_sync(synchronizer)
{
    id = wave_id;

    int i = 0;
    float y = -2990; // Drone has a coverage radius of 10.0f
    for (auto& drone : drones)
    {
        drone.id = id * 1000 + i;
        drone.position.x = static_cast<float>(X);
        drone.position.y = y;
        drone.state = drone_state_enum::IDLE_IN_BASE;
        drone.wave_id = id;

        y += 20.0f;
        i++;
    }
    spdlog::info("Wave {} created all drones", id);
}

void Wave::Move()
{
    X += 20;
    for (auto& drone : drones)
    {
        drone.position.x = static_cast<float>(X);
        drone.charge -= DRONE_CONSUMPTION_RATE;
    }
}

void Wave::UploadData()
{
    try
    {
        // A redis pipeline is used to upload the data to the redis server
        // Create a pipeline from the group of redis connections
        auto pipe = redis.pipeline(false);

        // For each drone a redis function will be added to the pipeline
        // When every command has been added, the pipeline will be executed

        for (auto& drone : drones)
        {
            // Create a DroneData object
            DroneData data(tick, drone.id, utils::droneStateToString(drone.state), drone.charge, drone.position, drone.wave_id);
            auto v = data.toVector();

            // Add the command to the pipeline
            pipe.xadd("scanner_stream", "*", v.begin(), v.end());
            // spdlog::info("Drone {} data uploaded to Redis", drone.id);
        }

        // Redis connection is returned to the pool after the pipeline is executed
        pipe.exec();
    }
    catch (const ReplyError& e)
    {
        spdlog::error("Redis pipeline error: {}", e.what());
    } catch (const IoError& e)
    {
        spdlog::error("Redis pipeline error: {}", e.what());
    }
}

void Wave::Run()
{
    tick_sync.thread_started();
    auto start_time = std::chrono::high_resolution_clock::now();

    // TODO: Implement states for the waves
    while (tick < 1000)
    {
        spdlog::info("Wave {} tick {} started", id, tick);
        // Before the "normal" execution, check if SM did put any "input" inside the "queue" (aka TestGenerator scenarios' input)

        Move();
        UploadData();

        // Use atomic variable to sync with other threads
        // Each tick of the execution will be synced with the other threads. This will make writing to the DB much easier
        // because the data will be consistent/historical
        tick_sync.tick_completed();
        spdlog::info("Wave {} tick {} ended", id, tick);
        tick++;
    }
    tick_sync.thread_finished();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    spdlog::info("Wave {} duration: {}ms", id, duration.count());
    spdlog::info("Wave {} finished", id);
}

SyncedScannerManager::SyncedScannerManager(Redis& shared_redis) : shared_redis(shared_redis)
{
    spdlog::set_pattern("[%T.%e][%^%l%$][ScannerManager] %v");
    spdlog::info("ScannerManager created");
}

void SyncedScannerManager::Run()
{
    spdlog::info("ScannerManager running");

    // Wait for all the processes to be ready
    utils::NamedSyncWait(shared_redis, "Drone");

    int wave_id = 0;

    TickSynchronizer synchronizer;
    ThreadPool pool(6);

    synchronizer.thread_started();
    for (int i = 0; i < 5; i++)
    {
        waves[wave_id] = std::make_shared<Wave>(wave_id, shared_redis, synchronizer);
        pool.enqueue_task([this, wave_id] { waves[wave_id]->Run(); });
        wave_id++;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    // ScannerManager needs sto be synced with the wave-threads
    while (true)
    {
        spdlog::info("TICK {}", tick);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        synchronizer.tick_completed();
        spdlog::info("TICK {} completed", tick);
        tick++;
    }
    synchronizer.thread_finished();
}
