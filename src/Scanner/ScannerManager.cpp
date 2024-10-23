#include "ScannerManager.h"
#include "../../utils/LogUtils.h"
#include "spdlog/spdlog.h"
#include <thread>

bool ScannerManager::CheckSpawnWave() const {
    try {
        auto start_time = std::chrono::high_resolution_clock::now();

        // TODO: Better while condition
        while (true) {
            auto v = shared_redis.get("spawn_wave");
            // if (!v.has_value()) {
            //     log_sm("Failed to get spawn_wave from Redis");
            //     std::this_thread::sleep_for(std::chrono::milliseconds(4000));
            // }

            int spawn_wave = std::stoi(v.value_or("-1"));
            // log_sm("Spawn wave = " + std::to_string(spawn_wave));

            if (spawn_wave == 1) {
                // spdlog::info("Spawn wave = {}", spawn_wave);
                // log_sm("Spawn wave = 1");
                shared_redis.decr("spawn_wave");
                return true;
            }

            auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
            if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count() > timeout_ms) {
                if (spawn_wave == -1) {
                    log_sm("Error getting spawn_wave");
                    // spdlog::error("[ScannerManager] Error getting spawn_wave");
                    return false;
                }
                log_sm("Timeout waiting for spawn_wave");
                // spdlog::error("[ScannerManager] Timeout waiting for spawn_wave");
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const TimeoutError &e) {
        log_sm("Timeout spawning wave: " + std::string(e.what()));
        // spdlog::error("Timeout spawning wave: {}", e.what());
    } catch (const IoError &e) {
        log_sm("IoError spawning wave: " + std::string(e.what()));
        // spdlog::error("IoError spawning wave: {}", e.what());
    } catch (const std::exception &e) {
        log_sm("Exception in CheckSpawnWave: " + std::string(e.what()));
        // spdlog::error("Error spawning wave: {}", e.what());
    } catch (...) {
        log_sm("Unknown error spawning wave");
        // spdlog::error("Unknown error spawning wave");
    }
    return false;
}

void ScannerManager::SpawnWave() {
    // Capture the current wave_id by value
    int current_wave_id = wave_id;

    // waves[wave_id] = std::make_shared<Wave>(wave_id, shared_redis, synchronizer);
    waves.emplace(wave_id, std::make_shared<Wave>(tick, wave_id, shared_redis, &synchronizer));
    pool.enqueue_task([this, current_wave_id] { waves[current_wave_id]->Run(); });
    wave_id++;
}

ScannerManager::ScannerManager(Redis &shared_redis) : pool(8), shared_redis(shared_redis) {
    log_sm("ScannerManager created");
    spdlog::set_pattern("[%T.%e][%^%l%$][ScannerManager] %v");
    // std::cout << "[ScannerManager] ScannerManager created" << std::endl;
}

void ScannerManager::Run() {
    log_sm("Running");
    // std::cout << "[ScannerManager] Running" << std::endl;

    // Create IPC message queue
    auto mq = message_queue(open_or_create, "drone_fault_queue", 100, sizeof(TG_data));
    unsigned int priority;
    message_queue::size_type recvd_size;

    // Create or open the semaphore for synchronization
    sem_t *sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);
    sem_t *sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0);

    // synchronizer.thread_started();
    synchronizer.add_thread();
    // ScannerManager needs sto be synced with the wave-threads
    while (tick < sim_duration_ticks) {
        try {
            // Wait for the semaphore to be released
            // std::this_thread::sleep_for(std::chrono::milliseconds(5));
            sem_wait(sem_sync);
            // spdlog::info("TICK: {}", tick);
            log_sm("TICK: " + std::to_string(tick));
            // std::cout << "[ScannerManager] TICK: " << tick << std::endl;

            // Syncing with the DC
            if ((tick % WAVE_DISTANCE_TICKS) == 0 && CheckSpawnWave()) {
                SpawnWave();
            };

            // Get size of the message queue
            auto size = mq.get_num_msg();
            // spdlog::info("Messages in the queue: {}", size);

            if (size > 0) {
                // Iterate over the message queue and receive all the messages
                for (int i = 0; i < size; i++) {
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
        } catch (...) {
            log_error("ScannerManager", "Error running ScannerManager");
            // std::cerr << "Error running ScannerManager" << std::endl;
        }
    }
    synchronizer.remove_thread();

    // Close the semaphore
    sem_close(sem_sync);
    sem_close(sem_sc);

    // Close the message queue
    message_queue::remove("drone_fault_queue");

    // spdlog::info("ScannerManager finished");
    log_sm("Finished");
    // std::cout << "[ScannerManager] finished" << std::endl;
}
