#include "DroneManager.h"
#include <spdlog/spdlog.h>
#include <utility>
#include <iostream>

/* The 6x6 km area is divided into zones_vertex of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones_vertex in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones_vertex,

For testing purposes, the right column will be ignored for now.*/

namespace drones {
    // The zone is created with vertex_coords_sqr set.
    DroneZone::DroneZone(int zone_id, std::array<std::pair<float, float>, 4> &coords, Redis &redis)
            : zone_id(zone_id), vertex_coords(coords), zone_redis(redis) {
        // Create drone path
        CreateDronePath();
        UploadPathToRedis();

        // Calculate the furthest point of the drone path
        path_furthest_point = CalculateFurthestPoint();
#ifdef DEBUG
        spdlog::info("Furthest point of zone {}: ({}, {})", zone_id, path_furthest_point.first,
                     path_furthest_point.second);
#endif

        // Upload the zone to the Redis server
        std::string redis_zone_id = "zone:" + std::to_string(zone_id);
        zone_redis.set(redis_zone_id + ":id", std::to_string(zone_id)); // TODO: Maybe remove this
        zone_redis.set("zone:" + std::to_string(zone_id) + ":swap", "none");
    }

    // Thread function for the zone-thread
    void DroneZone::Run() {
        // Set initial drone to working
        zone_redis.set("drone:" + std::to_string(drones[0]->getDroneId()) + ":command", "work");

        // Calculate first drone swap coords
        drones[0]->CalculateSwapFinalCoords();

        // Get sim_running from Redis
        bool sim_running = (zone_redis.get("sim_running") == "true");

        // Simulate until sim_running is false
        while (sim_running) {
            try {
                auto tick_start = std::chrono::steady_clock::now();

                // Check if a drone is working
                // CheckDroneWorking();

                // Execute each drone in the zone
                auto number_of_drones = drones.size();
//                spdlog::info("DroneZone {} tick {}: number of drones {}", zone_id, tick_n, number_of_drones);
                for (auto &drone: drones) {
                    drone->Run();
                }

                // Manage drone faults
                ManageFaults();
//                spdlog::info("DroneZone {} tick {}: drones ran and fault managed", zone_id, tick_n);

                if (!drones.empty()) {
                    // Check if the drone needs to be swapped
                    if (drones[0]->getSwap()) {
                        // Set the new drone to working
                        // Set the drone to working
                        drones[1]->SetDronePathIndex(drone_path_index);
                        drones[1]->SetDroneState(drone_state_enum::WORKING);
                        zone_redis.hset("drone:" + std::to_string(drones[1]->getDroneId()), "status", "WORKING");
                        zone_redis.set("zone:" + std::to_string(zone_id) + ":swap", "none");
                        zone_redis.set("zone:" + std::to_string(zone_id) + ":working_drone", std::to_string(drones[1]->getDroneId()));
    #ifdef DEBUG
                        spdlog::info("Drone {} is now working: path index {}", drones[1]->getDroneId(), drones[1]->GetDronePathIndex());
    #endif
                        drones[0]->setSwap(false);
                    }

                    if (drones[0]->getDestroy()) {
    //                    spdlog::info("Drone {} getting destroyed", drones[0]->getDroneId());
                        // Remove the drone from the vector
                        drones.erase(drones.begin());

    //                    spdlog::info("DroneZone {} now has {} drones", zone_id, drones.size());
                    }
                }

                // Check if there is time left in the tick
                auto tick_now = std::chrono::steady_clock::now();
                if (tick_now <= tick_start + tick_duration_ms) {
                    // Sleep for the remaining time
                    std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
                } else if (tick_now > tick_start + tick_duration_ms) {
                    // Log if the tick took too long
                    spdlog::warn("DroneZone {} tick took too long", zone_id);
//                    break;
                }
                // Get sim_running from Redis
                sim_running = (zone_redis.get("sim_running") == "true");
                ++tick_n;
            } catch (const std::exception &e) {
                spdlog::error("DroneZone {} tick {} error: {}", zone_id, tick_n, e.what());
            }
        }
        spdlog::info("DroneZone {} finished", zone_id);
    }

    void DroneZone::SpawnThread(int tick_n_dm) {
        tick_n = tick_n_dm;
        zone_thread = boost::thread(&DroneZone::Run, this);
    }

    void DroneZone::CreateDrone(int drone_id) {
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
        spdlog::info("Drone {} created in zone {}", drone_id, zone_id);
        zone_redis.sadd("zone:" + std::to_string(zone_id) + ":drones_active", std::to_string(drone_id));
    }

    void DroneZone::CreateNewDrone() {
        auto drone_id = new_drone_id + zone_id * 10;
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
        zone_redis.sadd("zone:" + std::to_string(zone_id) + ":drones_active", std::to_string(drone_id));
        ++new_drone_id;
    }

    // Creates the drone path for the zone using global coords
    void DroneZone::CreateDronePath() {
        // Determine the boundaries of the drone path
        std::array<std::pair<float, float>, 4> drone_boundaries;
        drone_boundaries[0] = {vertex_coords[0].first + 10, vertex_coords[0].second - 10};
        drone_boundaries[1] = {vertex_coords[1].first - 10, vertex_coords[1].second - 10};
        drone_boundaries[2] = {vertex_coords[2].first - 10, vertex_coords[2].second + 10};
        drone_boundaries[3] = {vertex_coords[3].first + 10, vertex_coords[3].second + 10};

        auto step_size = 20.0f; // In meters
        int i = 0;

        using namespace std;

        // From the top left corner to the top right corner
        for (float x = drone_boundaries[0].first; x <= drone_boundaries[1].first; x += step_size) {
//            drone_path[i] = {x, drone_boundaries[0].second};
//            drone_path_charge[i] = CalculateChargeNeeded(drone_path[i]);
            drone_path.emplace_back(x, drone_boundaries[0].second);
            ++i;
        }
        // From the bottom right corner to the bottom left corner
        for (float x = drone_boundaries[2].first; x >= drone_boundaries[3].first; x -= step_size) {
//            drone_path[i] = {x, drone_boundaries[2].second};
//            drone_path_charge[i] = CalculateChargeNeeded(drone_path[i]);
            drone_path.emplace_back(x, drone_boundaries[2].second);
            ++i;
        }
    }

    float DroneZone::CalculateChargeNeeded(std::pair<float, float> path_position) {
        // Get the distance from the drone to the base
        float distance = std::sqrt(path_position.first * path_position.first +
                                      path_position.second * path_position.second);
        return distance * DRONE_CONSUMPTION;
    }

    // Calculates the furthest point of the drone path from the base
    std::pair<float, float> DroneZone::CalculateFurthestPoint() {
        std::pair<float, float> furthest_point = {0, 0};

        for (const auto &point: vertex_coords) {
            if (abs(point.first) > abs(furthest_point.first)) {
                furthest_point.first = point.first;
            }
            if (abs(point.second) > abs(furthest_point.second)) {
                furthest_point.second = point.second;
            }
        }

        return furthest_point;
    }

    // Uploads the path to the Redis server
    void DroneZone::UploadPathToRedis() {
        std::string redis_path_id = "path:" + std::to_string(zone_id);
        // Convert the path array to a vector
        std::vector<std::pair<float, float>> drone_path_v(drone_path.begin(), drone_path.end());

        zone_redis.rpush(redis_path_id, drone_path_v.begin(), drone_path_v.end());
    }

    void DroneZone::CheckDroneWorking() {
        // Check if the state has changed
        auto current_val = drones[0]->getDroneState() == drone_state_enum::WORKING ? true : false;
        if (drone_is_working != current_val) {
            // The state has changed
            if (current_val) {
                drone_is_working = true;
                if (drone_working_id != drones[0]->getDroneId()) {
                    drone_working_id = drones[0]->getDroneId();
                    // The key for the zone:id:working_drone is set by the working drone itself
                }
            } else {
                drone_is_working = false;
                drone_working_id = 0;
                zone_redis.set("zone:" + std::to_string(zone_id) + ":working_drone", "none");
                spdlog::warn("DroneZone {} has no working drones", zone_id);
            }
        }
    }

    void DroneZone::CreateDroneFault(int drone_id, drone_state_enum fault_state,
                                     std::pair<float, float> fault_coords, int tick_start,
                                     int tick_end, int reconnect_tick) {
        drone_fault new_fault = {
                .drone_id = drone_id,
                .fault_state = fault_state,
                .fault_coords = fault_coords,
                .zone_id = zone_id,
                .tick_start = tick_start,
                .tick_end = tick_end,
                .reconnect_tick = reconnect_tick};
        drone_faults.push_back(new_fault);
    }

    // When a drone fault is created, the DC needs to be notified and an acknowledgement needs to be received to continue
    // the thread's execution.
    void DroneZone::ManageFaults() {
        if (drone_faults.empty()) {
            return;
        } else {
            // If DC has not been notified of the NEW fault
            if (!drone_fault_ack) {

                for (auto &fault: drone_faults) {
                    // Upload to Redis the fault data
                    std::unordered_map<std::string, std::string> fault_data = {
                            {"drone_id", std::to_string(fault.drone_id)},
                            {"zone_id", std::to_string(fault.zone_id)},
                            {"fault_state", utils::CaccaPupu(fault.fault_state)},
                            {"fault_coords_X", std::to_string(fault.fault_coords.first)},
                            {"fault_coords_Y", std::to_string(fault.fault_coords.second)},
                            {"tick_start", std::to_string(fault.tick_start)},
                            {"tick_end", std::to_string(fault.tick_end)},
                            {"reconnect_tick", std::to_string(fault.reconnect_tick)}
                    };
                    zone_redis.hmset("drones_fault:" + std::to_string(fault.drone_id), fault_data.begin(), fault_data.end());
//                    spdlog::info("Fault data uploaded to Redis for drone {}", fault.drone_id);
                    zone_redis.sadd("drones_faults", std::to_string(fault.drone_id));
                }
        //        ConnectionOptions ack_opts;
        //        ack_opts.host = "127.0.0.1";
        //        ack_opts.port = 7777;
        //        ack_opts.socket_timeout = std::chrono::seconds (5);
                auto sub = zone_redis.subscriber();
                sub.on_message([this](const std::string &channel, const std::string &msg) {
                    if (msg == "ack") {
                        drone_fault_ack = true;
                        spdlog::info("{}'s fault acknowledged by DC", zone_id);
                    }
                });
                sub.subscribe("fault_ack");

                // Wait for the acknowledgement
                while (!drone_fault_ack) {
                    // Check if the faults have been acknowledged
                    try {
                        sub.consume();
                    } catch (const Error &e) {
                        spdlog::error("Acknowledgement not received: {}", e.what());
                    }
                }
            }

            // The DC has acknowledged the fault, now manage the fault
            for (auto &fault: drone_faults) {
                if (fault.fault_state == drone_state_enum::DEAD) {
                    if (tick_n >= fault.tick_end) {
                        // The drone is dead
                        spdlog::warn("Drone {} has died at tick {}", fault.drone_id, tick_n);

                        // Remove fault from drone_faults vector
                        drone_faults.erase(std::remove_if(drone_faults.begin(), drone_faults.end(), [fault](const drone_fault &in_vector_fault) {
                            return fault.drone_id == in_vector_fault.drone_id;
                        }), drone_faults.end());

                        // Set fault status
                        zone_redis.hset("drones_fault:" + std::to_string(fault.drone_id), "fault_state", "DONE");
                    }
                } else {
                    // Drone is in NOT_CONNECTED fault state
                    if (fault.reconnect_tick == -1) {
                        // The drone will not reconnect
                        if (tick_n >= fault.tick_end) {
                            // Destroy the drone
                            spdlog::warn("Drone {} has lost connection and died without reconnecting at tick {}", fault.drone_id, tick_n);

                            // Find the drone in the drones vector
                            for (auto &drone: drones) {
                                if (drone->getDroneId() == fault.drone_id) {
                                    // Reconnect the drone
                                    drone->setDestroy(true);
                                }
                            }

                            // Remove fault from drone_faults vector
                            drone_faults.erase(std::remove_if(drone_faults.begin(), drone_faults.end(), [fault](const drone_fault &in_vector_fault) {
                                return fault.drone_id == in_vector_fault.drone_id;
                            }), drone_faults.end());

                            // Set fault status
                            zone_redis.hset("drones_fault:" + std::to_string(fault.drone_id), "fault_state", "DONE");
                        }
                    } else {
                        // The drone will reconnect
                        if (tick_n >= fault.tick_start + fault.reconnect_tick) {
                            // The drone has reconnected
                            spdlog::warn("Drone {} has reconnected at tick {}", fault.drone_id, tick_n);

                            // Find the drone in the drones vector
                            for (auto &drone: drones) {
                                if (drone->getDroneId() == fault.drone_id) {
                                    // Reconnect the drone
                                    drone->setConnectedToSys(true);
                                }
                            }

                            // Remove fault from drone_faults vector
                            drone_faults.erase(std::remove_if(drone_faults.begin(), drone_faults.end(), [fault](const drone_fault &in_vector_fault) {
                                return fault.drone_id == in_vector_fault.drone_id;
                            }), drone_faults.end());

                            // Set fault status
                            zone_redis.hset("drones_fault:" + std::to_string(fault.drone_id), "fault_state", "RECONNECTED");
                        }
                    }
                }
            }
        }
    }
} // namespace drones
