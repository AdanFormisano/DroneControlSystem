#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include "../globals.h"
#include <iostream>
#include <spdlog/spdlog.h>

namespace drones {
    void DroneManager::Run() {
        spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
        spdlog::info("Initializing Drone process");

        drone_zones.reserve(300);
        drone_vector.reserve(300);
        // drone_threads.reserve(300);

        // Calculate the zones' vertex_coords
        CalculateGlobalZoneCoords();

        int zone_id = 0;    // Needs to be 0 because is used in DroneControl::setDroneData()

        // Create the DroneZones objects for every zone calculated
        for (auto& zone : zones) {
            CreateDroneZone(zone, zone_id);
            ++zone_id;
        }
        spdlog::info("All zones created");

        utils::SyncWait(shared_redis);

        CreateThreadBlocks();

        // Exists for the duration of the simulation
        try {
            bool sim_running = (shared_redis.get("sim_running") == "true");
            // Run the simulation
            while(sim_running) {
                // Get the time_point
                auto tick_start = std::chrono::steady_clock::now();

                // Sleep for the remaining time

                // Check if there is time left in the tick
                auto tick_now = std::chrono::steady_clock::now();
                if (tick_now < tick_start + tick_duration_ms) {
                    // Sleep for the remaining time
                    std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
                } else if (tick_now > tick_start + tick_duration_ms) {
                    // Log if the tick took too long
                    spdlog::warn("DroneManager tick took too long");
                    break;
                }

                // std::cout << n_data_sent << " data sent" << std::endl;
                // Get sim_running from Redis
                sim_running = (shared_redis.get("sim_running") == "true");
                ++tick_n;
            }
        } catch (const sw::redis::IoError& e) {
            spdlog::error("Couldn't get sim_running: {}", e.what());
        }
    }

    DroneManager::DroneManager(Redis& redis) : shared_redis(redis) {}

    DroneManager::~DroneManager() {
        for (auto& thread : drone_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    // Creates the zones' global vertex_coords
    void DroneManager::CalculateGlobalZoneCoords() {
        int i = 0;
        int width = 62;
        int height = 2;

        for (int x = -124; x <= 124 - width; x += width) {
            for (int y = -75; y <= 75 - height; y += height) {
                zones[i][0] = {x, y};
                zones[i][1] = {x + width, y};
                zones[i][2] = {x, y + height};
                zones[i][3] = {x + width, y + height};
                ++i;
            }
        }
    }

    // For a set of vertex_coords creates a DroneZone object
    void DroneManager::CreateDroneZone(std::array<std::pair<int, int>, 4>& zone, int zone_id) {
        drone_zones.emplace_back(zone_id, zone, this);
    }

    void DroneManager::CreateThreadBlocks() {
        // For every "block" of zones, create the threads
        int width = 62;
        int height = 2;
        int n_drone = 0;
        for (int x = -124; x <= 124 - width; x += width) {
            // Creates threads for a column of zones
            for (int y = -75; y <= 75 - height; y += height) {
                // Create the threads
                drone_threads.emplace_back(&Drone::Run, drone_vector[n_drone].get());
                ++n_drone;
            }
            std::this_thread::sleep_for(tick_duration_ms * 50);
        }
    }
} // drones

