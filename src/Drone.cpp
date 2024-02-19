#include "Drone.h"
#include <spdlog/spdlog.h>
#include "../utils/RedisUtils.h"
#include <iostream>

using namespace sw::redis;

// TODO: Check if namespace is necessary
namespace drones {
    int Drone::nextId = 0;


    Drone::Drone() : id(nextId++) {
        spdlog::info("Drone {} starting", id);
        const std::string key = "drone:" + std::to_string(id);
        data = "Wendy is a sleepy cat.";

        // TODO: This should be a sub/pub connection
        auto redis = Redis("tcp://127.0.0.1:7777");
        utils::RedisConnectionCheck(redis, key);

        // Add a drone to Redis
        // This should ONLY be called when a drone is ADDED to the system
        // Redis should always have all the drones data in it
        redis.hset(key, "status", "idle");
    }

    int Drone::getId() const {
        return id;
    }

    int Drone::uploadData(Redis &redis, std::string& key) const {
        // TODO: Use try/catch, maybe?
        redis.hset(key, "data", data);

        return 0;
    }
} // drones