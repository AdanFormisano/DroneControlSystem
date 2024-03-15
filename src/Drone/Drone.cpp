#include <iostream>
#include "../../utils/RedisUtils.h"
#include "DroneManager.h"
#include "spdlog/spdlog.h"

/* Drones are created by their zone, but its thread is created after the system has ended the initialization process.
The drones' threads are created in groups, where every group is a "collumn" of the zone. This is done to avoid overloading the system.
Each thread immediately starts running the drone's simulation.
*/

namespace drones {
Drone::Drone(int id, const DroneZone *drone_zone) : drone_id(id), dz(drone_zone), drone_redis(dz->dm->shared_redis) {
    redis_id = "drone:" + std::to_string(id);
    drone_charge = 100.0f;
    drone_position = {0.0f, 0.0f};
    drone_state = drone_state_enum::IDLE_IN_BASE;
}

// This will be the ran in the threads of each drone
void Drone::Run() {
    tick_n = 0;
    // spdlog::info("Drone {} bound: 1 {},{}, 2 {},{}, 3 {},{}, 4 {},{}",
    // drone_id, dz->vertex_coords_glb[0].first, dz->vertex_coords_glb[0].second,
    // dz->vertex_coords_glb[1].first, dz->vertex_coords_glb[1].second,
    // dz->vertex_coords_glb[2].first, dz->vertex_coords_glb[2].second,
    // dz->vertex_coords_glb[3].first, dz->vertex_coords_glb[3].second);

    path_index = -1;

    // Get sim_running from Redis
    bool sim_running = (dz->dm->shared_redis.get("sim_running") == "true");

    std::optional<std::string> cmd;
    std::optional<std::string> d_info;

    // Run the simulation
    while (sim_running) {
        // Get the time_point
        auto tick_start = std::chrono::steady_clock::now();

        switch (drone_state) {
            case drone_state_enum::IDLE_IN_BASE:
                // spdlog::info("TICK {}: Drone {} is idle in base", tick_n, drone_id);
                // Must wait for DroneControl to release the drone
                cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");
                if (cmd == "work") {
                    drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                    drone_state = drone_state_enum::TO_ZONE;
                }
                break;
            case drone_state_enum::WORKING:
                Work();
                // spdlog::info("TICK {}: Drone {} is working. ({} {})", tick_n, drone_id, drone_position.first, drone_position.second);

                cmd = drone_redis.get("drone:" + std::to_string(drone_id) + ":command");
                if (cmd == "charge") {
                    drone_redis.set("drone:" + std::to_string(drone_id) + ":command", "none");
                    drone_state = drone_state_enum::TO_BASE;
                }
                break;
            case drone_state_enum::CHARGING:
                // spdlog::info("TICK {}: Drone {} is charging", tick_n, drone_id);
                // Update the drone's charge
                d_info = drone_redis.hget("drone:" + std::to_string(drone_id), "charge");
                if (d_info == "100") {
                    drone_state = drone_state_enum::IDLE_IN_BASE;
                }
                break;
            case drone_state_enum::WAITING_CHARGE:
                // spdlog::info("TICK {}: Drone {} is waiting for charge",tick_n, drone_id);
                upStatusForIdle();   // Uploads the drone status to Redis before sleeping
                drone_state = drone_state_enum::SLEEP;
                break;
            case drone_state_enum::TO_ZONE:
                // Simulate the drone moving to the destination
                Move(dz->drone_path[0].first, dz->drone_path[0].second);
                // spdlog::info("TICK {}: Drone {} is moving to {} {}",tick_n, drone_id, drone_position.first, drone_position.second);

                if (drone_position.first == dz->drone_path[0].first && drone_position.second == dz->drone_path[0].second) {
                    // If the drone has reached the destination, change its state
                    drone_state = drone_state_enum::WORKING;
                }
                break;
            case drone_state_enum::TO_BASE:
                // Simulate the drone moving to the base
                Move(0, 0);
                // spdlog::info("TICK {}: Drone {} is going to base",tick_n, drone_id);
                if (drone_position.first == 0 && drone_position.second == 0) {
                    // If the drone has reached the base, change its state
                    drone_state = drone_state_enum::WAITING_CHARGE;
                }
                break;
            case drone_state_enum::SLEEP:
                // spdlog::info("TICK {}: Drone {} is sleeping", tick_n, drone_id);
                // Read status from redis
                d_info = drone_redis.hget("drone:" + std::to_string(drone_id), "status");
                if (d_info == "CHARGING") {
                    drone_state = drone_state_enum::CHARGING;
                }
                break;
            case drone_state_enum::TOTAL:
                break;
        }

        // Check if there is time left in the tick
        auto tick_now = std::chrono::steady_clock::now();
        if (tick_now < tick_start + tick_duration_ms) {
            // Sleep for the remaining time
            std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
        } else if (tick_now > tick_start + tick_duration_ms) {
            // Log if the tick took too long
            spdlog::warn("Drone {} tick took too long", drone_id);
            break;
        }
        // Get sim_running from Redis
        sim_running = (drone_redis.get("sim_running") == "true");
        ++tick_n;
    }
}

void Drone::Work() {
    // Work
    try {
        FollowPath();
        UpdateStatus();
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
    }
    else {
        // Calculate the direction vector from the current point to the target
        float dirX = dx / distance;
        float dirY = dy / distance;

        // Move the point 20 units in the direction of the target
        drone_position.first += 20.0f * dirX;
        drone_position.second += 20.0f * dirY;
    }
}

// The drone will follow a path indicated by the zone
void Drone::FollowPath() {
    // Every drone will start in the top left "square" of its zone.

    // Check if the drone is at the end of the path
    if (path_index == dz->drone_path.size() - 1) {
        path_index = 0;
        drone_position = dz->drone_path[path_index];
    } else {
        ++path_index;
        drone_position = dz->drone_path[path_index];
    }
}

// Update drone status in db
void Drone::UpdateStatus() {
    // TODO: There can be a better way to do this: there is no need to build each time the map
    // Implementing option 1: each drone updates its status using its key in Redis and uploading a map with the data
    std::vector<std::pair<std::string, std::string>> drone_data = {
        {"id", std::to_string(drone_id)},
        {"status", utils::CaccaPupu(drone_state)},  // FIXME: This is a placeholder, it should take Drone.status
        {"charge", std::to_string(drone_charge)},
        {"X", std::to_string(drone_position.first)},
        {"Y", std::to_string(drone_position.second)},
        {"zone_id", std::to_string(dz->getZoneId())}
    };
    // TODO: Find better way to get the time

    // Updating the drone's status in Redis using streams
    try {
        auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(), drone_data.end());  // Returns the ID of the message
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}

// Upload the drone status before going to sleep
void Drone::upStatusForIdle() {
    std::unordered_map<std::string, std::string> drone_data{
        {"id", std::to_string(drone_id)},
        {"status", utils::CaccaPupu(drone_state)},
        {"charge", std::to_string(drone_charge)},
        {"X", std::to_string(drone_position.first)},
        {"Y", std::to_string(drone_position.second)},
        {"zone_id", std::to_string(dz->getZoneId())}};
    try {
        // Upload the drone status to redis
        drone_redis.hmset("drone:" + std::to_string(drone_id), drone_data.begin(), drone_data.end());
        // Add to charge_stream the drone id
        drone_redis.xadd("charge_stream", "*", drone_data.begin(), drone_data.end());
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}
}  // namespace drones
