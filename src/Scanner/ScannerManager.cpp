#include "ScannerManager.h"

#include "spdlog/spdlog.h"

bool ScannerManager::CheckSpawnWave()
{
    try
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
    catch (const TimeoutError& e)
    {
        spdlog::error("Timeout spawning wave: {}", e.what());
    } catch (const IoError& e)
    {
        spdlog::error("IoError spawning wave: {}", e.what());
    } catch (const std::exception& e)
    {
        spdlog::error("Error spawning wave: {}", e.what());
    } catch (...)
    {
        spdlog::error("Unknown error spawning wave");
    }
    return false;
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

    // Create or open the semaphore for synchronization
    sem_t* sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);
    sem_t* sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0);

    synchronizer.thread_started();

    // ScannerManager needs sto be synced with the wave-threads
    while (tick < sim_duration_ticks)
    {
        try
        {
            // Wait for the semaphore to be released
            sem_wait(sem_sync);
            // spdlog::info("TICK: {}", tick);
            std::cout << "[ScannerManager] TICK: " << tick << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

            // Syncing with the DC
            if ((tick % WAVE_DISTANCE_TICKS) == 0 && CheckSpawnWave())
            {
                SpawnWave();
            };

            // Get size of the message queue
            auto size = mq.get_num_msg();
            // spdlog::info("Messages in the queue: {}", size);

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

            // utils::UpdateSyncTick(shared_redis, tick);
            // CheckSyncTickAck();
            synchronizer.tick_completed();

            // Release the semaphore to signal the end of the tick
            sem_post(sem_sc);
            std::cout << "[ScannerManager] TICK: " << tick << " finished" << std::endl;
            tick++;
        }
        catch (...)
        {
            std::cerr << "Error running ScannerManager" << std::endl;
        }
    }
    synchronizer.thread_finished();

    // spdlog::info("ScannerManager finished");
    std::cout << "ScannerManager finished" << std::endl;
}
