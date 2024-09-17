#include "ScannerManager.h"

#include "spdlog/spdlog.h"

Wave::Wave(int tick_n, const int wave_id, Redis& shared_redis, TickSynchronizer& synchronizer) : tick(tick_n),
    redis(shared_redis), tick_sync(synchronizer)
{
    id = wave_id;
    starting_tick = tick_n;

    float y = -2990; // Drone has a coverage radius of 10.0f
    // for (auto& drone : drones)
    // {
    //     drone.id = id * 1000 + i;
    //     drone.position.x = static_cast<float>(X);
    //     drone.position.y = y;
    //     // drone.state = drone_state_enum::IDLE_IN_BASE;
    //     drone.setState(ToStartingLine::getInstance());
    //     drone.wave_id = id;
    //
    //     y += 20.0f;
    //     i++;
    // }

    for(int i = 0; i < 300; i++)
    {
        int drone_id = id * 1000 + i;
        drones.emplace_back(drone_id, id, *this);
        drones[i].position.x = 0;
        drones[i].position.y = 0;
        drones[i].starting_line.x = -2990.0f;
        drones[i].starting_line.y = y;
        drones[i].setState(ToStartingLine::getInstance());
        drones[i].tick_drone = tick;

        // Calculate drones' direction
        float dx = -2990.0f;
        float dy = y;
        float distance = std::sqrt(dx * dx + dy * dy);

        drones[i].dir.x = dx / distance;
        drones[i].dir.y = dy / distance;

        y += 20.0f;
    }

    // Self add alive wave on Redis
    redis.sadd("waves_alive", std::to_string(id));

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
            DroneData data(tick, drone.id, utils::droneStateToString(drone.getCurrentState()->getState()), drone.charge, drone.position, drone.wave_id);
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
    spdlog::info("Wave {} started", id);
    tick_sync.thread_started();
    auto start_time = std::chrono::high_resolution_clock::now();

    // A redis pipeline is used to upload the data to the redis server
    // Create a pipeline from the group of redis connections
    auto pipe = redis.pipeline(false);

    // TODO: Implement states for the waves
    while (tick < starting_tick + 1000)
    {
        spdlog::info("Wave {} tick {}", id, tick);
        // Before the "normal" execution, check if SM did put any "input" inside the "queue" (aka TestGenerator scenarios' input)
        // Check if there is any message in the queue
        if (!tg_data.empty())
        {
            // TODO: In theory there will only ever be a single message in the queue, maybe we can optimize this
            // Get the message from the queue
            auto msg = tg_data.pop().value();
            spdlog::info("[TestGenerator] Drone has new state {}", msg.drone_id, utils::droneStateToString(msg.new_state));
        }


        // Move();
        // UploadData();

        for (auto& drone : drones)
        {
            // Execute the current state
            drone.run();

            // Create a DroneData object
            DroneData data(tick, drone.id, utils::droneStateToString(drone.getCurrentState()->getState()), drone.charge, drone.position, drone.wave_id);
            auto v = data.toVector();

            // Add the command to the pipeline
            pipe.xadd("scanner_stream", "*", v.begin(), v.end());
            // spdlog::info("Drone {} data uploaded to Redis", drone.id);
        }
        pipe.exec();

        // Each tick of the execution will be synced with the other threads. This will make writing to the DB much easier
        // because the data will be consistent/historical
        tick_sync.tick_completed();
        tick++;
    }

    // Remove self from alive waves on Redis
    redis.srem("waves_alive", std::to_string(id));

    tick_sync.thread_finished();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    spdlog::info("Wave {} duration: {}ms", id, duration.count());
    spdlog::info("Wave {} finished", id);
}

bool ScannerManager::CheckSyncTickAck()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
        auto v = shared_redis.get("scanner_tick_ack");
        int current_tick = std::stoi(v.value_or("-2"));
        // spdlog::info("Checking scanner_tick_ack: {} - {}", current_tick, tick);

        if (current_tick == tick)
        {
            return true;
        }

        auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
        // spdlog::info("Waited for {}", std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count());
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() > timeout_ms)
        {
            spdlog::error("Timeout waiting for scanner_tick_ack");
            if (current_tick == -1)
            {
                spdlog::error("Error getting scanner_tick_ack");
                return false;
            }
            spdlog::error("scanner_tick_ack is not in sync with the ScannerManager");
            return false;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

bool ScannerManager::CheckSpawnWave()
{
    auto start_time = std::chrono::high_resolution_clock::now();

    // TODO: Better while condition
    while (true)
    {
        auto v = shared_redis.get("spawn_wave");
        int spawn_wave = std::stoi(v.value_or("-1"));
        // spdlog::info("Checking spawn_wave: {}", spawn_wave);

        if (spawn_wave == 1)
        {
            // spdlog::info("Spawn wave = {}", spawn_wave);
            shared_redis.decr("spawn_wave");
            return true;
        }

        auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() > timeout_ms)
        {
            if (spawn_wave == -1)
            {
                spdlog::error("Error getting spawn_wave");
                return false;
            }
            spdlog::error("Timeout waiting for spawn_wave");
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void ScannerManager::SpawnWave()
{
    // Capture the current wave_id by value
    int current_wave_id = wave_id;

    // waves[wave_id] = std::make_shared<Wave>(wave_id, shared_redis, synchronizer);
    waves.emplace(wave_id, std::make_shared<Wave>(tick, wave_id, shared_redis, synchronizer));
    pool.enqueue_task([this, current_wave_id] { waves[current_wave_id]->Run(); });
    wave_id++;
}

ScannerManager::ScannerManager(Redis& shared_redis) : shared_redis(shared_redis), pool(10)
{
    spdlog::set_pattern("[%T.%e][%^%l%$][ScannerManager] %v");
    spdlog::info("ScannerManager created");
}

void ScannerManager::Run()
{
    spdlog::info("ScannerManager running");

    // Create IPC message queue
    auto mq = message_queue(open_or_create, "drone_fault_queue", 100, sizeof(TG_data));
    unsigned int priority;
    message_queue::size_type recvd_size;

    // Wait for all the processes to be ready
    utils::NamedSyncWait(shared_redis, "Drone");

    synchronizer.thread_started();
    // for (int i = 0; i < 5; i++)
    // {
    //     waves[wave_id] = std::make_shared<Wave>(wave_id, shared_redis, synchronizer);
    //     pool.enqueue_task([this] { waves[wave_id]->Run(); });
    //     wave_id++;
    //     std::this_thread::sleep_for(std::chrono::milliseconds(500));
    // }

    // ScannerManager needs sto be synced with the wave-threads
    while (true)
    {
        spdlog::info("TICK {}", tick);

        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        // Syncing with the DC
        if ((tick % 150) == 0 && CheckSpawnWave())
        {
            SpawnWave();
        };

        // Get syze of the message queue
        auto size = mq.get_num_msg();

        if (size > 0)
        {
            // Iterate over the message queue and receive all the messages
            for (int i = 0; i < size; i++)
            {
                TG_data msg{};
                mq.receive(&msg, sizeof(msg), recvd_size, priority);

                // Append the message to the wave's queue
                waves[msg.wave_id]->tg_data.push(msg);
            }
        }

        // Try to receive a message from the message queue

        tick++;

        // TODO: Combine in a single function
        utils::UpdateSyncTick(shared_redis, tick);
        CheckSyncTickAck();

        synchronizer.tick_completed();
    }
    synchronizer.thread_finished();
}
