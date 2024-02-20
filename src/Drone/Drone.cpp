#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include <iostream>

// TODO: Check if namespace is necessary
namespace drones {
    Drone::Drone(int id, Redis& sharedRedis) : id(id), redis(sharedRedis) {
        // redis = sharedRedis;
        data = "Wendy is a sleepy cat.";

        spdlog::info("Drone {} starting", id);
        key = "drone:" + std::to_string(id);

        utils::RedisConnectionCheck(redis, key);

        // Add a drone to Redis
        // This should ONLY be called when a drone is ADDED to the system
        // Redis should always have all the drones data in it
        redis.hset(key, "status", "idle");

        // Create Redis subscriber, using the shared connection
//        Subscriber sub = redis.subscriber();
//        sub.psubscribe("drones");
//        sub.on_pmessage([](std::string pattern, std::string channel, std::string msg) {
//            spdlog::info("Pattern: {}, Channel: {}, Message: {}", pattern, channel, msg);
//        });

//        // Consume messages in a loop.
//        while (true) {
//            try {
//                sub.consume();
//            } catch (const Error &err) {
//                // Handle exceptions.
//            }
//        }
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