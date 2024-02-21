#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <thread>
#include <chrono>

namespace drones {
    Drone::Drone(int id, Redis& sharedRedis) : id(id), redis(sharedRedis) {
        // spdlog::info("Drone {} starting", id);
        data = "Wendy is a sleepy cat.";
        key = "drone:" + std::to_string(id);

        utils::RedisConnectionCheck(redis, key);

        // Add a drone to Redis
        // This should ONLY be called when a drone is ADDED to the system
        // Redis should always have all the drones data in it
        redis.hset(key, "status", "idle");
    }


    int Drone::getId() const {
        return id;
    }


    int Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e] [Drone] [%^%l%$] %v");
        spdlog::info("Initializing drone process");

        // TESTING: Create 10 drones
        spdlog::info("Creating 10 drones");
        for (int i = 0; i < 10; i++) {
            drones::Drone drone(i, redis);
            spdlog::info("Drone {} created (Client ID {})", i, utils::RedisGetClientID(redis));
        }
        spdlog::info("----------All drones created----------");

        // Initialization finished
        utils::SyncWait(redis);

        return 0;
    }
} // drones