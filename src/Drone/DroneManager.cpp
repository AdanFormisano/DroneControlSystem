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
        spdlog::info("Initializing Drone process");

        bool threads_created = false;

        // Calculate the zones_vertex' vertex_coords
        CalculateGlobalZoneCoords();

        // Create the Zones objects for every zone calculated
        CreateZones();

        // Wait for all the processes to be ready
        utils::SyncWait(shared_redis);
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
                if (!threads_created && tick_n < 20 && tick_n % 5 == 0) {
                    CreateZoneThreads();

                    // Check if all the threads were created
                    if (zone_threads.size() == ZONE_NUMBER) {
                        threads_created = true;
                    }
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
                    break;
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

    DroneManager::DroneManager(Redis &redis) : shared_redis(redis) {
    }

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

    // Creates the zones_vertex' global vertex_coords
    void DroneManager::CalculateGlobalZoneCoords() {
        int i = 0;
        int width = 62;
        int height = 2;

        for (int x = -124; x <= 124 - width; x += width) {
            for (int y = -150; y <= 150 - height; y += height) {
                zones_vertex[i][0] = {x * 20, (y + height) * 20};              // Top left
                zones_vertex[i][1] = {(x + width) * 20, (y + height) * 20};    // Top right
                zones_vertex[i][2] = {(x + width) * 20, y * 20};                    // Bottom right
                zones_vertex[i][3] = {x * 20, y * 20};                         // Bottom left
                ++i;
            }
        }
    }

    // Check if DC asked for new drones and create them
    void DroneManager::CheckNewDrones() {
        std::unordered_set<std::string> zones_to_swap;
        long long list_len = shared_redis.scard("zones_to_swap");   // Get the number of zones_vertex to swap from Redis

        // Check if DC asked for new drones
        if (list_len == 0) {
            return;
        } else {
            // Drones to swap have been requested
            spdlog::info("{} zones to swap", list_len);

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
                        shared_redis.set("drone:" + drone_id.value() + ":command", "follow");
                    } else {
                        // If there are no drones available, create a new drone for that zone

                        // Create the drone
                        auto new_drone_id = zones[z]->getNewDroneId() + z * 10;
                        spdlog::info("No drones available in zone {}, creating Drone {}", z, new_drone_id);
                        zones[z]->CreateDrone(new_drone_id);

                        // Set the drone to work
                        shared_redis.set("drone:" + std::to_string(new_drone_id) + ":command", "follow");
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
        }
    }

    // Creates a column of 150 threads
    void DroneManager::CreateZoneThreads() {
        size_t zone_threads_size = zone_threads.size();

        // Loops for the next 150 zones_vertex starting from the last zone thread created
        for (size_t i = zone_threads_size; i < zone_threads_size + 150; ++i) {
            zone_threads.emplace_back(&DroneZone::Run, zones[static_cast<int>(i)]);
        }
    }
} // namespace drones
