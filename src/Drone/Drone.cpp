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
            {"id",                    std::to_string(drone_id)},
            {"status",                utils::CaccaPupu(drone_state)},  // FIXME: name
            {"charge",                std::to_string(drone_charge)},
            {"X",                     std::to_string(drone_position.first)},
            {"Y",                     std::to_string(drone_position.second)},
            {"zone_id",               std::to_string(dz.getZoneId())},
            {"charge_needed_to_base", std::to_string(0)}
        };

        // Add the drone to the zone's queue of drones
        drone_redis.rpush("zone:" + std::to_string(dz.getZoneId()) + ":drones", std::to_string(drone_id));

        // Check if the drone already exists in the Redis DB
        if (!Exists()) {
#ifdef DEBUG
            spdlog::warn("Drone {} does not exist in the Redis DB", drone_id);
#endif
            SetChargeNeededToBase();
        }

        // Set initial status in Redis
        UploadStatus();
    }

    // Drone is going to execute this function every tick, this is the main function of the drone
    void Drone::Run() {
        try {
            std::optional<std::string> cmd;     // Command received from the Redis DB
            tick_n = dz.tick_n;

            // Run the drone's state machine
            switch (drone_state) {
                case drone_state_enum::IDLE_IN_BASE:
#ifdef DEBUG
                    spdlog::info("TICK {}: Drone {} [{}%] is idle in base", tick_n, drone_id, drone_charge);
#endif
                    // Must wait for DroneControl to release the drone
                    cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");

                    if (cmd == "work") {
                        // Remove the drone from the queue of zone drones
                        drone_redis.lpop("zone:" + std::to_string(dz.getZoneId()) + ":drones");
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_ZONE);
                        drone_state = drone_state_enum::TO_ZONE;
                    } else if (cmd == "follow") {
                        // Remove the drone from the queue of zone drones
                        drone_redis.lpop("zone:" + std::to_string(dz.getZoneId()) + ":drones");
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_ZONE_FOLLOWING);
                        drone_state = drone_state_enum::TO_ZONE_FOLLOWING;
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
                        // Set the drone as the working drone in DroneZone
                        dz.drone_working.reset(this);
                        drone_redis.hset(redis_id, "status", "WORKING");

                        // Set the working drone id in the zone
                        drone_redis.set("zone:" + std::to_string(dz.getZoneId()) + ":working_drone_id",
                                        std::to_string(drone_id));
                        path_index = dz.drone_path_index;
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::WORKING);
                        drone_state = drone_state_enum::WORKING;
                    }
                    break;

                case drone_state_enum::TO_ZONE_FOLLOWING:
                    // Move to the zone
                    Move(dz.drone_path[0].first, dz.drone_path[0].second);
#ifdef DEBUG
                    spdlog::info("TICK {}: Drone {} [{}%] is moving to zone (following) {} {}", tick_n, drone_id, drone_charge,
                                 drone_position.first, drone_position.second);
#endif
                    if (drone_position.first == dz.drone_path[0].first && drone_position.second == dz.drone_path[0].second) {
                        // Arrived to the zone
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::FOLLOWING);
                        drone_state = drone_state_enum::FOLLOWING;
                    }
                    break;

                case drone_state_enum::FOLLOWING:
                    // Swap has started: the drone must follow the working drone (zone:id:swap is "started")
#ifdef DEBUG
                    spdlog::info("TICK {}: Drone {} [{}%] is following Drone {}", tick_n, drone_id, drone_charge, dz.drone_working->getDroneId());
#endif
                    FollowDrone();
                    break;

                case drone_state_enum::WORKING:
                    // TODO: Implement the swap of the drones and all the checks needed
                    cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");
                    if (cmd == "charge") {
                        drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                        drone_redis.hset(redis_id, "status", "TO_BASE");
                        dz.SetSwap();
                        dz.drone_path_index = path_index;
                        drone_data[1].second = utils::CaccaPupu(drone_state_enum::TO_BASE);
                        // Remove the old drones from the active drones in redis
                        drone_redis.srem("zone:" + std::to_string(dz.getZoneId()) + ":drones_active",
                                        std::to_string(drone_id));
                        drone_state = drone_state_enum::TO_BASE;
#ifdef DEBUG
                        spdlog::info("Drone {} [{}%] received charge command", drone_id, drone_charge);
#endif
                    }
                    Work();
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
                    }
                    break;

                case drone_state_enum::WAITING_CHARGE:
#ifdef DEBUG
                    spdlog::info("TICK {}: Drone {} [{}%] is waiting for charge", tick_n, drone_id, drone_charge);
#endif
                    SendChargeRequest();   // Uploads the drone status to Redis before sleeping
                    dz.SetDestroyDrone();

                case drone_state_enum::TOTAL:
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

            UploadStatusOnStream();
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

        UploadStatusOnStream();
    }

// The drone will follow a path indicated by the zone
    void Drone::FollowPath() {
        // Every drone will start in the top left "square" of its zone.

        // Check if the drone is at the end of the path
        if (path_index == dz.drone_path.size() - 1) {
            path_index = 0;
            drone_position = dz.drone_path[path_index];
        } else {
            ++path_index;
            drone_position = dz.drone_path[path_index];
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
        drone_position = dz.drone_working->getDronePosition();
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
            charge_data.pop_back();  // Remove the charge_needed_to_base field
            // Upload the drone status to redis
//            drone_redis.hmset("drone:" + std::to_string(drone_id), charge_data.begin(), charge_data.end());
            // Add to charge_stream the drone id
            drone_redis.xadd("charge_stream", "*", charge_data.begin(), charge_data.end());
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
    }
}  // namespace drones
