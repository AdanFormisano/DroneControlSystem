#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include "../globals.h"
#include "DroneManager.h"
#include "spdlog/spdlog.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

namespace drones {
Drone::Drone(int id, DroneZone *drone_zone)
    : drone_id(id), dz(drone_zone), drone_redis(dz->dm->shared_redis) {
    redis_id = "drone:" + std::to_string(id);
    drone_charge = 100.0;
    drone_position = {0, 0};
}

// This will be the ran in the threads of each drone
void Drone::Run() {
    // Initialize the thread
    tick_n = 0;
    // InitThread();

    // auto random_float = generateRandomFloat();
    // int random_int = abs(int(random_float) * 100);
    // std::cout << "Drone " << drone_id << " random_int: " << random_int << std::endl;
    // uto random_sleep = std::chrono::milliseconds(random_int);
    // std::this_thread::sleep_for(random_sleep);

    // Get sim_running from Redis
    // std::cout << "Drone " << drone_id << " prima di sim_running" << std::endl;
    bool sim_running = (dz->dm->shared_redis.get("sim_running") == "true");
    std::cout << "Drone " << drone_id << " dopo di sim_running" << std::endl;
    // spdlog::info("Drone {}: {}", drone_id, sim_running);
    // TESTING: placeholder where drone moves by 5 units every tick
    // Run the simulation
    while (sim_running) {
        // Get the time_point
        auto tick_start = std::chrono::steady_clock::now();

        // Work
        try {
            Move();
            UpdateStatus();
        } catch (const std::exception &e) {
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

// Thread initialization
void Drone::InitThread() {
    // Increment the process count in Redis
    drone_redis.incr("sync_process_count");
    // TODO: Get initial zone position
    // TODO: Implement the sleep time that simulates the time from base to first coords
    UpdateStatus();
    utils::SyncWait(drone_redis);
}

// This will just move the drone to a random position
void Drone::Move() {
    // Every drone will start in the top left corner of its zone.
    // Every tick the drone will move 20 meters clockwise, until it reaches the boundary of the zone.

    drone_position.first += 20;
}

void Drone::UpdateStatus() {
    // Implementing option 1: each drone updates its status using its key in Redis and uploading a map with the data
    drone_data = {
        {"id", std::to_string(drone_id)},
        {"status", "moving"}, // FIXME: This is a placeholder, it should take Drone.status
        {"charge", std::to_string(drone_charge)},
        {"X", std::to_string(drone_position.first)},
        {"Y", std::to_string(drone_position.second)},
    };
    // TODO: Find better way to get the time

    // Updating the drone's status in Redis using streams
    try {
        auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message
        // dz->dm->n_data_sent++;
        // std::cout << "Drone " << drone_id << " sent data" << std::endl;
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't update status: {}", e.what());
    }
}

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
