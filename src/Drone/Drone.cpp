#include "spdlog/spdlog.h"
#include "../../database/Database.h"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include "DroneManager.h"
#include "../globals.h"
#include <iostream>
#include <iomanip>
#include <cstdlib>

/* Drones are created by their zone, but its thread is created after the system has ended the initialization process.
The drones' threads are created in groups, where every group is a "collumn" of the zone. This is done to avoid overloading the system.
Each thread immediately starts running the drone's simulation.
*/

namespace drones {
Drone::Drone(int id, DroneZone* drone_zone)
    : drone_id(id), dz(drone_zone), drone_redis(dz->dm->shared_redis) {
    redis_id = "drone:" + std::to_string(id);
    drone_charge = 100.0;
drone_position = {0,0};
    }

// This will be the ran in the threads of each drone
void Drone::Run() {
    tick_n = 0;
    // spdlog::info("Drone {} bound: 1 {},{}, 2 {},{}, 3 {},{}, 4 {},{}",
    // drone_id, dz->vertex_coords_glb[0].first, dz->vertex_coords_glb[0].second,
    // dz->vertex_coords_glb[1].first, dz->vertex_coords_glb[1].second,
        // dz->vertex_coords_glb[2].first, dz->vertex_coords_glb[2].second,
    // dz->vertex_coords_glb[3].first, dz->vertex_coords_glb[3].second);

    // Get drone_path length

    int path_length = dz->drone_path.size();

    path_index = -1;

    // Get sim_running from Redis
    bool sim_running = (dz->dm->shared_redis.get("sim_running") == "true");

    // Run the simulation
    while (sim_running) {
        // Get the time_point
        auto tick_start = std::chrono::steady_clock::now();

        // Work
        try {Move();
        UpdateStatus();} catch (const std::exception& e) {
                spdlog::error("Drone {} failed to move: {}", drone_id, e.what());
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

// The drone will follow a path indicated by the zone
void Drone::Move() {
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
        //Implementing option 1: each drone updates its status using its key in Redis and uploading a map with the data
    std::map<std::string, std::string> drone_data = {
        {"id", std::to_string(drone_id)},
        {"status", "moving"}, // FIXME: This is a placeholder, it should take Drone.status
        {"charge", std::to_string(drone_charge)},
        {"X", std::to_string(drone_position.first)},
        {"Y", std::to_string(drone_position.second)},
        };
        // TODO: Find better way to get the time

    Database db;
    db.logDroneData(drone_data);

    // Updating the drone's status in Redis using streams
    try {auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message
}catch (const sw::redis::IoError& e) {
            spdlog::error("Couldn't update status: {}", e.what());
        }
    }

// Duplicate of UpdateStatus, with old implementation
// void Drone::UpdateStatus() {
//     // Option 1: each drone updates its status using its key in Redis and uploading a map with the data
//     drone_data = {
//         {"id", std::to_string(drone_id)},
//         {"status", "moving"}, // FIXME: This is a placeholder, it should take Drone.status
//         {"charge", std::to_string(drone_charge)},
//         {"X", std::to_string(position.first)},
//         {"Y", std::to_string(position.second)},
//         {"latestStatusUpdateTime", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}};

//     // Updating the drone's status in Redis using streams
//     auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message
// }

void Drone::requestCharging() {
    // Example logging, adjust as needed
    spdlog::info("Drone {} requesting charging", drone_id);
    ChargeBase *chargeBase = ChargeBase::getInstance();
    if (chargeBase && chargeBase->takeDrone(*this)) {
        drone_status = "Charging Requested";
        drone_redis.hset(redis_id, "status", drone_status);
    }
}

void Drone::onChargingComplete() {
    drone_status = "Charging Complete";
    drone_redis.hset(redis_id, "status", drone_status);
    spdlog::info("Drone {} charging complete", drone_id);
}

float Drone::getCharge() const {
    return drone_charge;
}

void Drone::setCharge(float newCharge) {
    drone_charge = newCharge;
}

} // namespace drones
