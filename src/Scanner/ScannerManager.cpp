#include "ScannerManager.h"

#include "spdlog/spdlog.h"



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
