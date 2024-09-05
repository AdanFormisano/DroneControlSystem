/* DroneManager is the "controller" class for the drones. It creates all the DroneZones, and their threads, that simulate
 * the larger 6x6 km area.
 */

#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include <spdlog/spdlog.h>

namespace drones {
    // Main execution loop for the process
    void DroneManager::Run() {
        // Initial setup
        spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
        spdlog::info("Drone process starting");

        // Calculate coords for DroneZones
        CalculateGlobalZoneCoords();
        spdlog::info("Global zone coords calculated");

        // Create the Zones objects for every zone calculated
        CreateZones();
        spdlog::info("Zones created");

        // Wait for all the processes to be ready
        utils::NamedSyncWait(shared_redis, "Drone");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        // Run the simulation
        bool sim_running = (shared_redis.get("sim_running") == "true");
        while (sim_running) {
            try {
                // Get the real-time start of the tick
                auto tick_start = std::chrono::steady_clock::now();
                // Create the threads for the zones_vertex every 5 ticks
                // This is done to avoid creating all the threads at once
                if (tick_n <= 20 && tick_n % 5 == 0) {
                    CreateZoneThreads();
                }

                // Dispatch any DroneStatus messages
                DroneStatusDispatcher();

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

    DroneManager::DroneManager(Redis &redis) : shared_redis(redis), mq(open_or_create, "drone_status_queue", 100, sizeof(DroneStatusMessage)) {}

    // For every zone calculated create a Zone object
    void DroneManager::CreateZones() {
        int zone_id = 0;

        for (auto &zone: zones_vertex) {
            // Create Zone object
            auto z = std::make_shared<DroneZone>(zone_id, zone, shared_redis);
            // Create the first drone for the zone
            z->CreateNewDrone();

            // Move the zone object to the zones map
            zones[zone_id] = std::move(z);

            ++zone_id;
        }
    }

    // Create the global coords (each of the 4 verticies) for each DroneZone
    void DroneManager::CalculateGlobalZoneCoords() {
        int i = 0;
        constexpr int width = 1240;
        constexpr int height = 40;

        // First 4 "normal" columns
        for (int x = -3000; x <= 1960 - width; x += width) {
            for (int y = -3000; y <= 3000 - height; y += height) {
                zones_vertex[i][0] = {x, (y + height)};             // Top left
                zones_vertex[i][1] = {(x + width), (y + height)};    // Top right
                zones_vertex[i][2] = {(x + width), y};              // Bottom right
                zones_vertex[i][3] = {x, y};                       // Bottom left
                ++i;
            }
        }

        // Special case for the last column
        constexpr int width_special = 1040;
        for (int y = -3000; y <= 3000 - height; y += height) {
            zones_vertex[i][0] = {1960, (y + height)};                  // Top left
            zones_vertex[i][1] = {1960 + width_special, (y + height)};  // Top right
            zones_vertex[i][2] = {1960 + width_special, y};            // Bottom right
            zones_vertex[i][3] = {1960, y};                            // Bottom left
            ++i;
        }
    }

    // Check if DC asked for new drones and create them
    void DroneManager::CheckNewDrones() {
        long long list_len = shared_redis.scard("zones_to_swap");   // Get the number of zones_vertex to swap from Redis

        // Check if DC asked for new drones
        if (list_len > 0) {
            std::unordered_set<std::string> zones_to_swap;
            std::vector<std::string> ids;

            // For every zone that sent a request create a drone object
            for (int i = 0; i < list_len; ++i) {
                auto zone_id = shared_redis.spop("zones_to_swap");

                // Check if the zone_id is valid
                if (zone_id) {
                    ids.emplace_back(zone_id.value());
                    int z = std::stoi(zone_id.value());

                    // Get if available a drone from the zone:id:drones list
                    auto drone_id = shared_redis.lpop("zone:" + zone_id.value() + ":charged_drones");
                    if (drone_id) {
                        spdlog::info("Drone {} fully available", drone_id.value());

                        // Create the drone object
                        zones[z]->CreateDrone(std::stoi(drone_id.value()));

                        // Set the drone to work
                        shared_redis.set("drone:" + std::to_string(zones[z]->drones.back()->getDroneId()) + ":command", "follow");
                    } else {
                        // If there are no drones available, create a new drone for that zone

#ifdef DEBUG
                        spdlog::info("No drones available in DroneZone {}, creating new drone", z);
#endif
                        zones[z]->CreateNewDrone();

                        // Set the drone to work
                        zones[z]->drones.back()->CalculateSwapFinalCoords();
                        shared_redis.set("drone:" + std::to_string(zones[z]->drones.back()->getDroneId()) + ":command", "follow");
                    }
                    // Update zone swap status
                    shared_redis.set("zone:" + zone_id.value() + ":swap", "started");
                }
            }

            // Logs the zones swapped
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

    void DroneManager::DroneStatusDispatcher()
    {
        try
        {
        DroneStatusMessage msg;
        unsigned int priority;
        message_queue::size_type recvd_size;

        if (!mq.try_receive(&msg, sizeof(msg), recvd_size, priority))
        {
        } else
        {
            if(!zones[msg.target_zone]->drone_status_queue.push(msg.status))
            {
                spdlog::error("IPC: Error pushing message to queue from TestGenerator");
            }
        }
        } catch (interprocess_exception &e)
        {
            spdlog::error("IPC: Error in DroneStatusDispatcher: {}", e.what());
        }
    }
} // namespace drones
