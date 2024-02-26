#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include <iostream>
#include <chrono>

/*
Drones should update their status every 5 seconds.

There are multiple ways to do this:
1. Each drone has its key in Redis and updates its status every 5 seconds
2. Every drone uses sub/pub to broadcast its status and the DroneControl should be listening to the channel
    and manages all the messages that are being sent
3. The status updates are sent from the drones using a Redis stream that the DroneControl keeps on reading
    to manage the status updates

The best way to choose is to implement a monitor and compare the performance of each method.
*/

namespace drones {
    Drone::Drone(int id, Redis& sharedRedis) : drone_id(id), drone_redis(sharedRedis) {
        redis_id = "drone:" + std::to_string(id);
        drone_charge = 100.0;
        spdlog::info("Drone {} created", id);

        // Adding the drone to the dataset on redis
        drone_redis.hset(redis_id, "status", "idle");
    }

    void Drone::requestCharging() {
        // Example logging, adjust as needed
        spdlog::info("Drone {} requesting charging", drone_id);
        ChargeBase* chargeBase = ChargeBase::getInstance();
        if (chargeBase && chargeBase->takeDrone(*this)) {
            status = "Charging Requested";
            drone_redis.hset(redis_id, "status", status);
        }
    }

    void Drone::onChargingComplete() {
        status = "Charging Complete";
        drone_redis.hset(redis_id, "status", status);
        spdlog::info("Drone {} charging complete", drone_id);
    }

    // This will be the ran in the threads of each drone
    void Drone::Run() {
        // Implementing option 1: each drone updates its status using its key in Redis
        drone_thread_id = std::this_thread::get_id();
        std::cout << "Thread " << drone_thread_id << " started" << std::endl;

        Move();
        int sleep_time = std::abs(static_cast<int>(std::round(position.first * 100)));
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        UpdateStatus();
        std::cout << "Thread " << drone_thread_id << " slept for " << sleep_time << " milliseconds" << std::endl;
    }

    // This will just move the drone to a random position
    void Drone::Move() {
        float x = generateRandomFloat();
        float y = generateRandomFloat();

        position = std::make_pair(x, y);
    }

    void Drone::UpdateStatus() {
        // Implementing option 1: each drone updates its status using its key in Redis and uploading a map with the data
        drone_data = {
                {"status", "moving"},
                {"charge", std::to_string(drone_charge)},
                {"X", std::to_string(position.first)},
                {"Y", std::to_string(position.second)},
        };

        drone_redis.hmset(redis_id, drone_data.begin(), drone_data.end());
        // spdlog::info("Drone {} updated its status", drone_id);
    }

    float Drone::getCharge() const {
        return drone_charge;
    }

    void Drone::setCharge(float newCharge) {
        drone_charge=newCharge;
    }

} // drones
