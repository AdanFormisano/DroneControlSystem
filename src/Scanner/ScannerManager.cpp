#include "ScannerManager.h"
#include "../../utils/LogUtils.h"
#include <thread>

bool ScannerManager::CheckSpawnWave() const
{
    try
    {
        auto start_time = std::chrono::high_resolution_clock::now();

        // TODO: Better while condition
        while (true)
        {
            auto v = shared_redis.get("spawn_wave");
            int spawn_wave = std::stoi(v.value_or("-1"));

            if (spawn_wave == 1)
            {
                // log_sm("Spawn wave = 1");
                shared_redis.decr("spawn_wave");
                return true;
            }

            auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() > timeout_ms)
            {
                if (spawn_wave == -1)
                {
                    log_sm("Error getting spawn_wave");
                    return false;
                }
                log_sm("Timeout waiting for spawn_wave");
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    } catch (const TimeoutError &e)
    {
        log_sm("Timeout spawning wave: " + std::string(e.what()));
    } catch (const IoError &e)
    {
        log_sm("IoError spawning wave: " + std::string(e.what()));
    } catch (const std::exception &e)
    {
        log_sm("Exception in CheckSpawnWave: " + std::string(e.what()));
    } catch (...)
    {
        log_sm("Unknown error spawning wave");
    }
    return false;
}

void ScannerManager::SpawnWave()
{
    // Capture the current wave_id by value
    int current_wave_id = wave_id;

    // waves[wave_id] = std::make_shared<Wave>(wave_id, shared_redis, synchronizer);
    waves.emplace(wave_id, std::make_shared<Wave>(tick, wave_id, shared_redis, &synchronizer));
    pool.enqueue_task([this, current_wave_id] { waves[current_wave_id]->Run(); });
    wave_id++;
}

ScannerManager::ScannerManager(Redis& shared_redis) : pool(8), shared_redis(shared_redis)
{
    // std::cout << "[ScannerManager] ScannerManager created" << std::endl;
    log_sm("ScannerManager created");
}

void scannerManagerSignalHandler(int signal) {
    std::cerr << "[ScannerManager] Error: Segmentation fault (signal " << signal << ")" << std::endl;
    // Perform any cleanup here if necessary
    exit(signal);
}

void ScannerManager::Run()
{
    try
    {
        std::cout << "[ScannerManager] Running" << std::endl;
        // std::signal(SIGSEGV, scannerManagerSignalHandler);

        // Create IPC message queue
        auto mq = message_queue(open_or_create, "drone_fault_queue", 100, sizeof(TG_data));
        unsigned int priority;
        message_queue::size_type recvd_size;

        // Create or open the semaphore for synchronization
        sem_t* sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);
        sem_t* sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0);

        if (sem_sync == SEM_FAILED || sem_sc == SEM_FAILED)
        {
            std::cerr << "Failed to initialize semaphores" << std::endl;
            return;
        }
        std::cout << "[ScannerManager] Semaphores initialized successfully" << std::endl;

        // synchronizer.thread_started();
        synchronizer.add_thread();

        // ScannerManager needs sto be synced with the wave-threads
        while (tick < sim_duration_ticks)
        {
                // Wait for the semaphore to be released
                // std::this_thread::sleep_for(std::chrono::milliseconds(5));
                sem_wait(sem_sync);

                // std::cout << "[ScannerManager] TICK: " << tick << std::endl;
                log_sm("TICK: " + std::to_string(tick));

                try
                {
                    // Syncing with the DC
                    if ((tick % WAVE_DISTANCE_TICKS) == 0 && CheckSpawnWave())
                    {
                        SpawnWave();
                    };
                }
                catch (const std::exception &e)
                {
                    std::cerr << "Error spawning wave: " << e.what() << std::endl;
                }

                // Get size of the message queue
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

                // utils::UpdateSyncTick(shared_redis, tick);
                // CheckSyncTickAck();
                // std::cout << "[ScannerManager] TICK: " << tick << " waiting synchronizer" << std::endl;
                synchronizer.sync();
                // std::cout << "[ScannerManager] TICK: " << tick << " synchronizer done" << std::endl;

                // Release the semaphore to signal the end of the tick
                sem_post(sem_sc);
                // std::cout << "[ScannerManager] TICK: " << tick << " finished" << std::endl;
                tick++;
        }
        synchronizer.remove_thread();
        pool.shutdown();

        // Close the semaphore
        sem_close(sem_sync);
        sem_close(sem_sc);

        // Close the message queue
        message_queue::remove("drone_fault_queue");

        // std::cout << "[ScannerManager] finished" << std::endl;
        log_sm("Finished");
    }
    catch (const std::exception& e)
    {
        // std::cerr << "Error running ScannerManager: " << e.what() << std::endl;
        log_error("ScannerManager", "Error running ScannerManager");
    }
}
