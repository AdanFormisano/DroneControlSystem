#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace drones {

    Drone::Drone(int droneId, Redis& sharedRedis) : id(droneId), redis(sharedRedis) {
        spdlog::info("Drone {} starting", id);
        data = "Initial drone data.";
        key = "drone:" + std::to_string(id);

        // Example Redis operation, adjust based on actual utility functions
        redis.hset(key, "status", "idle");
        redis.hset(key, "charge", std::to_string(charge));

        // Additional setup as needed
    }

    void Drone::requestCharging() {
        // Example logging, adjust as needed
        spdlog::info("Drone {} requesting charging", id);
        ChargeBase* chargeBase = ChargeBase::getInstance();
        if (chargeBase && chargeBase->takeDrone(*this)) {
            status = "Charging Requested";
            redis.hset(key, "status", status);
        }
    }

    void Drone::onChargingComplete() {
        status = "Charging Complete";
        redis.hset(key, "status", status);
        spdlog::info("Drone {} charging complete", id);
    }

// Other methods...
} // drones