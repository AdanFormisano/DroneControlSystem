#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <spdlog/spdlog.h>

/* The coords used for the zones_vertex indicates the "square" that the drone is going to cover: it's not a real coordinate.
The real global coordinates are needed for the drone path. They are going to be calculated when the zones_vertex are created.
*/

namespace drones {
    void DroneManager::Run() {
        // Initial setup
        spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
        spdlog::info("Drone process starting");

        // Calculate the zones_vertex' vertex_coords
        CalculateGlobalZoneCoords();
        spdlog::info("Global zone coords calculated");

        // Create the Zones objects for every zone calculated
        CreateZones();
        spdlog::info("Zones created");

        // Wait for all the processes to be ready
//        utils::SyncWait(shared_redis);
        utils::NamedSyncWait(shared_redis, "Drone");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Exists for the duration of the simulation
        bool sim_running = (shared_redis.get("sim_running") == "true");
        spdlog::info("DroneManager sim value: {}", sim_running);

        // Run the simulation
        while (sim_running) {
            try {
                // Get the time_point
                auto tick_start = std::chrono::steady_clock::now();

                // Create the threads for the zones_vertex every 5 ticks
                if (tick_n <= 20 && tick_n % 5 == 0) {
                    CreateZoneThreads();
                }

                // Check if DC asked for new drones
                CheckNewDrones();

                // Check if there is time left in the tick
                auto tick_now = std::chrono::steady_clock::now();
                if (tick_now < tick_start + tick_duration_ms) {
                    // Sleep for the remaining time
                    std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
                } else if (tick_now > tick_start + tick_duration_ms) {
                    // Log if the tick took too long
                    spdlog::warn("DroneManager tick took too long");
//                    break;
                }

                // Get sim_running from Redis
                sim_running = (shared_redis.get("sim_running") == "true");
                ++tick_n;
            } catch (const sw::redis::IoError &e) {
                spdlog::error("Couldn't get sim_running: {}", e.what());
            } catch (const std::exception &e) {
                spdlog::error("Error in DroneManager: {}", e.what());
            }
        }
    }

    DroneManager::DroneManager(Redis &redis) : shared_redis(redis) {}

    // For every zone calculated create a Zone object
    void DroneManager::CreateZones() {
        int zone_id = 0;

        for (auto &zone: zones_vertex) {
            // Create Zone object
            auto z = std::make_shared<DroneZone>(zone_id, zone, shared_redis);
            z->CreateNewDrone();

            // Move the zone object to the zones map
            zones[zone_id] = std::move(z);

            ++zone_id;
        }
    }

    // Creates the zones_vertexglobal vertex_coords. The total area
    void DroneManager::CalculateGlobalZoneCoords() {
        int i = 0;
        int width = 1240;
        int height = 40;

        for (int x = -3000; x <= 1960 - width; x += width) {
            for (int y = -3000; y <= 3000 - height; y += height) {
                zones_vertex[i][0] = {x, (y + height)};                 // Top left
                zones_vertex[i][1] = {(x + width), (y + height)};       // Top right
                zones_vertex[i][2] = {(x + width), y};                  // Bottom right
                zones_vertex[i][3] = {x, y};                            // Bottom left
                ++i;
            }
        }

        // Special case for the last column
        int width_special = 1040;
        for (int y = -3000; y <= 3000 - height; y += height) {
            zones_vertex[i][0] = {1960, (y + height)};                  // Top left
            zones_vertex[i][1] = {1960 + width_special, (y + height)};  // Top right
            zones_vertex[i][2] = {1960 + width_special, y};             // Bottom right
            zones_vertex[i][3] = {1960, y};                             // Bottom left
            ++i;
        }
    }

    // Check if DC asked for new drones and create them
    void DroneManager::CheckNewDrones() {
        long long list_len = shared_redis.scard("zones_to_swap");   // Get the number of zones_vertex to swap from Redis

        // Check if DC asked for new drones
        if (list_len == 0) {
            return;
        } else {
            // Drones to swap have been requested

            std::unordered_set<std::string> zones_to_swap;
            std::vector<std::string> ids;

            // For every zone create a drone object
            for (int i = 0; i < list_len; ++i) {
                auto zone_id = shared_redis.spop("zones_to_swap");

                // Check if the zone_id is valid
                if (zone_id) {
                    ids.emplace_back(zone_id.value());
                    int z = std::stoi(zone_id.value());

                    // Get and available drone from the zone:id:drones list
                    auto drone_id = shared_redis.lpop("zone:" + zone_id.value() + ":charged_drones");

                    // If there is a fully charged drone available in base
                    if (drone_id) {
                        spdlog::info("Drone {} fully available", drone_id.value());

                        // Create the drone object
                        zones[z]->CreateDrone(std::stoi(drone_id.value()));

                        // Set the drone to work
//                        zones[z]->drones.back()->SetDroneState(drone_state_enum::TO_ZONE_FOLLOWING);
                        shared_redis.set("drone:" + std::to_string(zones[z]->drones.back()->getDroneId()) + ":command", "follow");
                    } else {
                        // If there are no drones available, create a new drone for that zone

                        // Create the drone
#ifdef DEBUG
                        spdlog::info("No drones available in DroneZone {}, creating new drone", z);
#endif
                        zones[z]->CreateNewDrone();

                        // Set the drone to work
                        zones[z]->drones.back()->CalculateSwapFinalCoords();
//                        zones[z]->drones.back()->SetDroneState(drone_state_enum::TO_ZONE_FOLLOWING);
                        shared_redis.set("drone:" + std::to_string(zones[z]->drones.back()->getDroneId()) + ":command", "follow");
                    }
                    // Update zone swap status
                    shared_redis.set("zone:" + zone_id.value() + ":swap", "started");
                }
            }
            std::ostringstream oss;
            for(const auto& val : ids) {
                oss << val << " ";
            }

            spdlog::info("Zones swapped: {}", oss.str());
            zones_to_swap.clear();
            ids.clear();
        }
    }

    // Creates a column of 150 threads
    void DroneManager::CreateZoneThreads() {
        for (int i = column_n; i < column_n + 150; ++i) {
            zones[i]->SpawnThread(tick_n);
        }
        column_n += 150;
    }
} // namespace drones
