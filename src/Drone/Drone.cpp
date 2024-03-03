#include "Drone.h"

#ifndef SPDLOG_GUARD_H
#define SPDLOG_GUARD_H
#include "spdlog/spdlog.h"
#endif

#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include <iostream>

namespace drones {

    Drone::Drone(int id, Redis& sharedRedis) : drone_id(id), drone_redis(sharedRedis) {
        redis_id = "drone:" + std::to_string(id);
        drone_charge = 100.0;
    }

    // This will be the ran in the threads of each drone
    void Drone::Run() {
        // Initialization
        // TODO: Implement a Init for the threads
        drone_thread_id = std::this_thread::get_id();
        drone_redis.incr("sync_process_count");

        float x = generateRandomFloat();
        float y = generateRandomFloat();

        position = std::make_pair(x, y);

        int sleep_time = std::abs(static_cast<int>(std::round(position.first * 100)));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        UpdateStatus();

        // Initialization finished
        utils::SyncWait(drone_redis);

        // Get sim_running from Redis
        bool sim_running = (drone_redis.get("sim_running") == "true");
        tick_n = 0;

        // TESTING: placeholder where drone moves by 5 units every tick
        // Run the simulation
        while(sim_running) {
            // Get the time_point
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            Move();
            UpdateStatus();

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

    // This will just move the drone to a random position
    void Drone::Move() {
        // TESTING: move the drone 5 units on the x axis
        position.first += 5;
        // spdlog::info("Tick: {} - Drone ID {} moved to ({}, {})",tick_n, drone_id, position.first, position.second);
    }

    void Drone::UpdateStatus() {
        // Implementing option 1: each drone updates its status using its key in Redis and uploading a map with the data
        drone_data = {
                {"id", std::to_string(drone_id)},
                {"status", "moving"},                           // FIXME: This is a placeholder, it should take Drone.status
                {"charge", std::to_string(drone_charge)},
                {"X", std::to_string(position.first)},
                {"Y", std::to_string(position.second)},
                {"latestStatusUpdateTime", std::to_string(std::chrono::system_clock::now().time_since_epoch().count())}
        };

        // Updating the drone's status in Redis using streams
        auto redis_stream_id = drone_redis.xadd("drone_stream", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message
    }



    void Drone::onChargingComplete() {
        status = "Charging Complete";
        drone_redis.hset(redis_id, "status", status);
        spdlog::info("Drone {} charging complete", drone_id);
    }
    void Drone::onCharging() {
        status = "Charging";
        drone_redis.hset(redis_id, "status", status);
        spdlog::info("Drone {} charging", drone_id);
    }

    float Drone::getCharge() const {
        return drone_charge;
    }

    void Drone::setCharge(float newCharge) {
        drone_charge=newCharge;
    }

    const std::string &Drone::getRedisId() const {
        return redis_id;
    }

} // drones
