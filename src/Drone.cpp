#include "Drone.h"
#include <spdlog/spdlog.h>
#include "../utils/RedisUtils.h"
#include <iostream>

using namespace sw::redis;

// TODO: Check if namespace is necessary
namespace drones {
    Drone::Drone(int id) : id(id), redis("tcp://127.0.0.1:7777") {
        // id = 1;
        data = "Wendy is a sleepy cat.";

        spdlog::info("Drone {} starting", id);
        key = "drone:" + std::to_string(id);

        // TODO: This should be a sub/pub connection
        utils::RedisConnectionCheck(redis, key);

        // Add a drone to Redis
        // This should ONLY be called when a drone is ADDED to the system
        // Redis should always have all the drones data in it
        redis.hset(key, "status", "idle");
    }


    int Drone::getId() const {
        return id;
    }

    // Maybe better way to upload data to Redis?
    int Drone::uploadData() {
        // TODO: Use try/catch, maybe?
        redis.hset(key, "data", data);

        return 0;
    }
} // drones