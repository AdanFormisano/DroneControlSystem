#include <iostream>

#include "../../utils/RedisUtils.h"
#include "DroneManager.h"
#include "spdlog/spdlog.h"

/* Drones are created by their zone, but its thread is created after the system has ended the initialization process.
The drones' threads are created in groups, where every group is a "column" of the zone. This is done to avoid overloading the system.
Each thread immediately starts running the drone's simulation.
*/

namespace drones {
Drone::Drone(int id, DroneZone &drone_zone) : drone_id(id), dz(drone_zone), drone_redis(drone_zone.zone_redis) {
    // Drone's base data initialization
    redis_id = "drone:" + std::to_string(id);

    drone_data = {
        {"id", std::to_string(drone_id)},
        {"status", utils::CaccaPupu(drone_state)},  // FIXME: name of function
        {"charge", std::to_string(drone_charge)},
        {"X", std::to_string(drone_position.first)},
        {"Y", std::to_string(drone_position.second)},
        {"zone_id", std::to_string(dz.getZoneId())},
        {"charge_needed_to_base", std::to_string(0)},
        {"tick_n", std::to_string(tick_n)}};

    // Add the drone to the zone's queue of drones
    drone_redis.rpush("zone:" + std::to_string(dz.getZoneId()) + ":drones", std::to_string(drone_id));
    drone_redis.sadd("zone:" + std::to_string(dz.getZoneId()) + ":drones_alive", std::to_string(drone_id));

    // Check if the drone already exists in the Redis DB
    if (!Exists()) {
#ifdef DEBUG
        spdlog::warn("Drone {} does not exist in the Redis DB", drone_id);
#endif
        SetChargeNeededToBase();
    } else {
        // Take the charge_needed_to_base from the Redis DB
        drone_charge_to_base = std::stof(drone_redis.hget(redis_id, "charge_needed_to_base").value_or("0"));
        drone_data[6].second = std::to_string(drone_charge_to_base);

        // Get the drone's final coords for the swap
        swap_final_coords = {std::stof(drone_redis.hget(redis_id, "swap_final_x").value_or("0")),
                             std::stof(drone_redis.hget(redis_id, "swap_final_y").value_or("0"))};
    }

    // Set initial status in Redis
    //        tick_n = dz.tick_n;
    UploadStatus(); // TODO: Check if needed
}

// Drone is going to execute this function every tick, this is the main function of the drone
void Drone::Run() {
    try {
        // TODO: Implement charge checking: if <= 0, set drone_state to DEAD

        std::optional<std::string> cmd;  // Command received from the Redis DB
        std::optional<std::string> r;
        drone_state_enum last_state;
        tick_n = dz.tick_n;
        drone_data[7].second = std::to_string(tick_n);
        drone_redis.hincrby("zone:" + std::to_string(dz.getZoneId()) + ":drones_alive_history", std::to_string(tick_n), 1);

        // Check its status in the Redis DB
        drone_state = CheckDroneStateOnRedis();

        if (!connected_to_sys) {
            auto status = utils::CaccaPupu(drone_state);
            spdlog::warn("Tick {} Drone {} is not connected (status: {})", tick_n, drone_id, status);
        }

        // Run the drone's state machine
        switch (drone_state) {
            case drone_state_enum::IDLE_IN_BASE:
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is idle in base", tick_n, drone_id, drone_charge);
#endif
                // Must wait for DroneControl to release the drone
                cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");

                if (connected_to_sys) {
                    UploadStatusOnStream();

                    if (cmd == "work") {
                        // Remove the drone from the queue of zone drones
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_ZONE);
                        drone_redis.lpop("zone:" + std::to_string(dz.getZoneId()) + ":drones");
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_state = drone_state_enum::TO_ZONE;
                        drone_redis.hset(redis_id, "status", "TO_ZONE");
                    } else if (cmd == "follow") {
                        // Remove the drone from the queue of zone drones
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_ZONE_FOLLOWING);
                        drone_redis.lpop("zone:" + std::to_string(dz.getZoneId()) + ":drones");
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_redis.hset(redis_id, "status", "TO_ZONE_FOLLOWING");
                        drone_state = drone_state_enum::TO_ZONE_FOLLOWING;
                    }
                }
                break;

            case drone_state_enum::TO_ZONE:
                // Move to the zone
                Move(dz.drone_path[0].first, dz.drone_path[0].second);
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is moving to zone {} {}", tick_n, drone_id, drone_charge,
                             drone_position.first, drone_position.second);
#endif
                if (drone_position.first == dz.drone_path[0].first &&
                    drone_position.second == dz.drone_path[0].second) {
                    drone_data[1].second = utils::CaccaPupu(drone_state_enum::WORKING);
                    // Set the drone as the working drone in DroneZone
                    drone_redis.hset(redis_id, "status", "WORKING");

                    // Set the working drone id in the zone
//                    drone_redis.set("zone:" + std::to_string(dz.getZoneId()) + ":working_drone",
//                                    std::to_string(drone_id));

                    path_index = dz.drone_path_index;
                    drone_state = drone_state_enum::WORKING;
                }
                break;

            case drone_state_enum::TO_ZONE_FOLLOWING:
                // Move to the zone
                Move(swap_final_coords.first, swap_final_coords.second);
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is moving to zone (following) {} {}", tick_n, drone_id, drone_charge,
                             drone_position.first, drone_position.second);
#endif
                if (drone_position.first == swap_final_coords.first && drone_position.second == swap_final_coords.second) {
                    // Arrived to the zone
                    drone_data[1].second = utils::CaccaPupu(drone_state_enum::FOLLOWING);
                    drone_redis.hset(redis_id, "status", "FOLLOWING");
                    drone_state = drone_state_enum::FOLLOWING;
                }
                break;

            case drone_state_enum::FOLLOWING:
                // TODO: The drone should still upload to redis its status
                // Swap has started: the drone must follow the working drone (zone:id:swap is "started")
                UseCharge(20.0f);

                // If the working drone still hasn't arrived to the swapping drone's position wait for it
                if (drone_position != dz.drones[0]->getDronePosition()) {
#ifdef DEBUG
                    spdlog::info("TICK {}: Drone {} [{}%] is waiting for Drone {}", tick_n, drone_id, drone_charge, dz.drones[0]->getDroneId());
                    spdlog::info("Drone {} is at {} {}", dz.drones[0]->getDroneId(), dz.drones[0]->getDronePosition().first, dz.drones[0]->getDronePosition().second);
                    spdlog::info("Drone {} is at {} {}", drone_id, drone_position.first, drone_position.second);
#endif
                    drone_data[2].second = std::to_string(drone_charge);
//                    drone_data[3].second = std::to_string(drone_position.first);
//                    drone_data[4].second = std::to_string(drone_position.second);
//                    UploadStatusOnStream();
                } else {
                    // The working drone has arrived to the swapping drone's position
                    //                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::SWAPPING);
                    drone_data[2].second = std::to_string(drone_charge);
//                    drone_data[3].second = std::to_string(drone_position.first);
//                    drone_data[4].second = std::to_string(drone_position.second);
                    drone_redis.hset(redis_id, "status", "SWAPPING");
//                    UploadStatusOnStream();
                }

                if (connected_to_sys) {
                    UploadStatusOnStream();
                }
                break;

            case drone_state_enum::WORKING:
                // TODO: Implement the swap of the drones and all the checks needed
                Work();

                cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");

                if (connected_to_sys) {
                    if (cmd == "charge") {
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_BASE);
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_redis.hset(redis_id, "status", "TO_BASE");
                        swap = true;
                        dz.drone_path_index = path_index;
                        // Remove the old drones from the active drones in redis
                        drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_active",
                                         std::to_string(drone_id));
                        drone_state = drone_state_enum::TO_BASE;
    #ifdef DEBUG
                        spdlog::info("Drone {} [{}%] received charge command", drone_id, drone_charge);
    #endif
                    }
                }
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is working ({} {})", tick_n, drone_id, drone_charge,
                             drone_position.first, drone_position.second);
#endif
                break;

            case drone_state_enum::TO_BASE:
                // Simulate the drone moving to the base
                Move(0, 0);
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is moving to base {} {}", tick_n, drone_id, drone_charge,
                             drone_position.first, drone_position.second);
#endif
                if (drone_position.first == 0 && drone_position.second == 0) {
                    // If the drone has reached the base, change its state
                    drone_data[1].second = utils::CaccaPupu(drone_state_enum::WAITING_CHARGE);
                    drone_state = drone_state_enum::WAITING_CHARGE;
                    drone_redis.hset(redis_id, "status", "WAITING_CHARGE");
                }
                break;

            case drone_state_enum::WAITING_CHARGE:
#ifdef DEBUG
                spdlog::info("TICK {}: Drone {} [{}%] is waiting for charge", tick_n, drone_id, drone_charge);
#endif
                drone_data[2].second = std::to_string(drone_charge);

                if (connected_to_sys) {
                    SendChargeRequest();  // Uploads the drone status to Redis before sleeping
                    UploadStatusOnStream();

                    drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_alive", std::to_string(drone_id));

                    destroy = true;
                }
                break;

            case drone_state_enum::DEAD:
                // The drone is dead
                drone_data[1].second = utils::CaccaPupu(drone_state_enum::DEAD);
                drone_redis.hset(redis_id, "status", "DEAD");
                drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_alive", std::to_string(drone_id));
                drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_active", std::to_string(drone_id));
                // Create a drone fault in DZ
                dz.CreateDroneFault(drone_id, drone_state_enum::DEAD, drone_position, tick_n, tick_n + 20, -1);

                destroy = true;
                spdlog::warn("Drone {} is dead", drone_id);
                break;

            case drone_state_enum::NOT_CONNECTED:
                // The drone is not connected
                drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_active", std::to_string(drone_id));

                // Get the reconnect tick if it exists
                r = drone_redis.hget("drones_fault:" + std::to_string(drone_id), "reconnect_tick");
                // TODO: Add error creation if the reconnect tick is not found
                reconnect_tick = std::stoi(r.value_or("-2"));   // -2: couldn't get the reconnect tick

                // Create a drone fault in DZ
                dz.CreateDroneFault(drone_id, drone_state_enum::NOT_CONNECTED, drone_position, tick_n, tick_n + 20, reconnect_tick);

                // Set on redis the previous state
                last_state = utils::stringToDroneStateEnum(drone_data[1].second);
                drone_redis.hset(redis_id, "status", utils::CaccaPupu(last_state));

                connected_to_sys = false;
                fault_managed = true;

                spdlog::warn("Drone {} has lost connection", drone_id);

                break;

            case drone_state_enum::NONE:
                break;
        }
    } catch (const std::exception &e) {
        spdlog::error("Drone {} tick {} error: {}", drone_id, tick_n, e.what());
    }
}

bool Drone::Exists() {
    return drone_redis.hexists("drone:" + std::to_string(drone_id), "id");
}

void Drone::Work() {
    try {
        FollowPath();

        // Calculate and update charge needed to go back to the base
        //            drone_data[6].second = std::to_string(dz.drone_path_charge[path_index]);

        if (connected_to_sys) {
            UploadStatusOnStream();
        }
    } catch (const std::exception &e) {
        spdlog::error("Drone {} failed to move: {}", drone_id, e.what());
    }
}

// Move the drone to the target position
void Drone::Move(float final_x, float final_y) {
    // Calculate the distance between the current point and the target
    float dx = final_x - drone_position.first;
    float dy = final_y - drone_position.second;
    float distance = std::sqrt(dx * dx + dy * dy);

    // If the distance is less than or equal to 20 units, move directly to the target
    if (distance <= 20.0) {
        drone_position.first = final_x;
        drone_position.second = final_y;

        UseCharge(distance);
    } else {
        // Calculate the direction vector from the current point to the target
        float dirX = dx / distance;
        float dirY = dy / distance;

        // Move the point 20 units in the direction of the target
        drone_position.first += 20.0f * dirX;
        drone_position.second += 20.0f * dirY;

        UseCharge(20.0f);
    }

    drone_data[3].second = std::to_string(drone_position.first);
    drone_data[4].second = std::to_string(drone_position.second);
    drone_data[2].second = std::to_string(drone_charge);

    if (connected_to_sys) {
        UploadStatusOnStream();
    }
}

// The drone will follow a path indicated by the zone
void Drone::FollowPath() {
    // Every drone will start in the top left "square" of its zone.

    // Check if the drone is at the end of the path
    if (path_index == dz.drone_path.size() - 1) {
        drone_position = dz.drone_path[path_index];
        path_index = 0;
    } else {
        drone_position = dz.drone_path[path_index];
        ++path_index;
    }

    // Use the drone's charge
    UseCharge(20.0f);

    dz.drone_path_index = path_index;

    drone_data[3].second = std::to_string(drone_position.first);
    drone_data[4].second = std::to_string(drone_position.second);
    drone_data[2].second = std::to_string(drone_charge);
}

// The drone will follow the path of the working drone
void Drone::FollowDrone() {
    drone_position = dz.drones[0]->getDronePosition();
    path_index = dz.drone_path_index;
    UseCharge(20.0f);
    drone_data[3].second = std::to_string(drone_position.first);
    drone_data[4].second = std::to_string(drone_position.second);
    drone_data[2].second = std::to_string(drone_charge);
}

// Update drone status in db
void Drone::UploadStatusOnStream() {
    // Updating the drone's status in Redis using streams
    try {
        auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(),
                                                drone_data.end());  // Returns the ID of the message
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}

// Upload the drone status to the Redis DB
void Drone::UploadStatus() {
    try {
        // Upload the drone status to redis
        drone_redis.hmset("drone:" + std::to_string(drone_id), drone_data.begin(), drone_data.end());
        // Add to charge_stream the drone id
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}

// Upload the drone status and send a charge request using a stream
void Drone::SendChargeRequest() {
    try {
        auto charge_data = drone_data;
        //            charge_data.pop_back();  // Remove the charge_needed_to_base field
        // Upload the drone status to redis
        //            drone_redis.hmset("drone:" + std::to_string(drone_id), charge_data.begin(), charge_data.end());
        // Add to charge_stream the drone id
        drone_redis.xadd("charge_stream", "*", drone_data.begin(), drone_data.end());
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}

// Calculate the charge needed to go back to the base
void Drone::SetChargeNeededToBase() {
    // Coords of the working zone
    float dx = dz.path_furthest_point.first;
    float dy = dz.path_furthest_point.second;

    // Calculate the distance in meters between the current point and the target
    float distance = std::sqrt(dx * dx + dy * dy);

    // Calculate the charge needed to go back to the base
    drone_charge_to_base = distance * DRONE_CONSUMPTION;

    // Add the charge needed to the drone data
    drone_data[6].second = std::to_string(drone_charge_to_base);
}

void Drone::UseCharge(float move_distance) {
    drone_charge -= DRONE_CONSUMPTION * move_distance;
}

void Drone::SetDroneState(drone_state_enum state) {
    drone_state = state;
    drone_data[1].second = utils::CaccaPupu(state);
}

// Calculate where the swapping drone needs to move in the zone, this will optimize the swapping process
void Drone::CalculateSwapFinalCoords() {
    // Calculate the # ticks needed to reach the furthest point of the zone
    auto [dx, dy] = dz.path_furthest_point;
    auto distance_to_furthest_point = std::sqrt(dx * dx + dy * dy);
    int ticks_needed = static_cast<int>(distance_to_furthest_point / DRONE_STEP_SIZE);

    // Calculate where in the path the working drone will be after # ticks
    int current_index = dz.drone_path_index;  // Current index of the working drone
    int new_index = (current_index + ticks_needed) % static_cast<int>(dz.drone_path.size());

    // Calculate the final coords of the swap
    auto [fx, fy] = dz.drone_path[new_index];
    // Set the final coords of the swap
    swap_final_coords = {fx, fy};

    // Upload on Redis the final coords of the swap
    drone_redis.hset(redis_id, "swap_final_x", std::to_string(fx));
    drone_redis.hset(redis_id, "swap_final_y", std::to_string(fy));
}

// Check the status of the drone in the Redis DB
drone_state_enum Drone::CheckDroneStateOnRedis() {
    auto state = drone_redis.hget("drone:" + std::to_string(drone_id), "status");
    drone_state_enum new_state = drone_state_enum::NONE;

    if (!state.has_value() || state.value() == utils::CaccaPupu(drone_state)) {
        new_state = drone_state;
    } else if (state.value() == "DEAD") {
        new_state = drone_state_enum::DEAD;
        spdlog::warn("Drone {} is NOW dead", drone_id);
    } else if (state.value() == "NOT_CONNECTED") {
        new_state = drone_state_enum::NOT_CONNECTED;
        spdlog::warn("Drone {} is NOW not connected", drone_id);
    } else {
        new_state = utils::stringToDroneStateEnum(state.value());
    }

    return new_state;
}
}  // namespace drones
