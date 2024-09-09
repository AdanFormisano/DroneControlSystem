//
// Created by adan on 05/09/24.
//

#include "ScannerManager.h"

#include "../Drone/DroneManager.h"
#include "spdlog/spdlog.h"

ScannerManager::ScannerManager(Redis& shared_redis) : redis(shared_redis)
{
    spdlog::set_pattern("[%T.%e][%^%l%$][ScannerManager] %v");
    spdlog::info("ScannerManager created");
}

void ScannerManager::Run()
{
    spdlog::info("ScannerManager running");

    // Wait for all the processes to be ready
    utils::NamedSyncWait(redis, "Drone");

    // Create a single wave
    auto wave = waves.emplace_back(69, 0, redis);
    waves_threads.emplace_back(&Wave::Run, &wave);

    waves_threads[0].join();
    spdlog::info("Thread joined");
}

void Wave::Run()
{
    try
    {
        spdlog::info("Wave {} created", id);
        createDrones();

        // Set wave to far left of the area
        *coords.x = -2990.0f;

        while (tick_n < 500)
        {
            // Get the real-time start of the tick
            auto tick_start = std::chrono::steady_clock::now();


            moveDrones();
            dumpDroneData();

            // Check if there is time left in the tick
            auto tick_now = std::chrono::steady_clock::now();
            if (tick_now <= tick_start + tick_duration_ms)
            {
                // Sleep for the remaining time
                std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
            }
            else if (tick_now > tick_start + tick_duration_ms)
            {
                // Log if the tick took too long
                spdlog::warn("Wave {} tick took too long", id);
            }
            // Get sim_running from Redis
            // sim_running = (zone_redis.get("sim_running") == "true");
            ++tick_n;
        }
    }
    catch (const IoError& e)
    {
        spdlog::error("Couldn't get sim_running: {}", e.what());
    } catch (const std::exception& e)
    {
        spdlog::error("Error in Wave {}: {}", id, e.what());
    }
}

Wave::Wave(const int wave_id, int tick, Redis& redis) : shared_redis(redis), tick_n(tick), id(wave_id)
{
    spdlog::info("Wave {} created", id);
}

void Wave::createDrones()
{
    float y = -2990; // Drone has a coverage radius of 10.0f
    // Create the drones in the wave
    for (int i = 0; i < 300; ++i)
    {
        // Each drone differs by 20.0f in the y-axis
        drones[i].id = id * 1000 + i;
        drones[i].position.x = 0.0f;
        drones[i].position.y = y;
        drones[i].state = drone_state_enum::IDLE_IN_BASE;
        drones[i].wave_id = id;

        y += 20.0f;
        spdlog::info("Drone {} created with ({}, {}) coords", drones[i].id, drones[i].position.x, drones[i].position.y);
    }
}

void Wave::moveDrones()
{
    *coords.x += 20.0f; // Move the wave to the right

    // Move the drones in the wave
    for (auto& [id, position, state, wave_id, charge] : drones)
    {
        // Move the drone to the right
        position.x = *coords.x;
        charge -= DRONE_CONSUMPTION_RATE;
        spdlog::info("TICK {} Drone {} moved to ({}, {})", tick_n, id, position.x, position.y);
    }
}

void Wave::dumpDroneData()
{
    for (const auto& [id, position, state, wave_id, charge] : drones)
    {
        DroneData data(tick_n, id, utils::droneStateToString(state), charge, position, wave_id);
        auto data_map = data.toVector();
        auto r = shared_redis.xadd("drone_data_dump", "*", data_map.begin(), data_map.end());

        // spdlog::info("Drone {} data dumped to Redis with id {}", id, r);
    }
}
