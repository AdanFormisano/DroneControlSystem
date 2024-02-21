#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <thread>
#include <chrono>

// TODO: Check if namespace is necessary
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

    int Init() {
        spdlog::info("Initializing drone process");
        auto redis = Redis("tcp://127.0.0.1:7777");
        spdlog::info("Using clientID: {}", utils::RedisGetClientID(redis));

        // TESTING: Create 10 drones
        spdlog::info("Creating 10 drones");
        for (int i = 0; i < 10; i++) {
            drones::Drone drone(i, redis);
            spdlog::info("Drone {} created (Client ID {})", i, utils::RedisGetClientID(redis));
        }
        spdlog::info("----------All drones created----------");

        // Waits for the main process to be ready
        auto sub = redis.subscriber();
        bool main_init = false;

        sub.on_message([](const std::string& channel, const std::string& msg) {
            std::cout << "Channel: " << channel << ", Message: " << msg << std::endl;
        });

        sub.subscribe("Main");
        int i = 0;
        while (true) {
            std::cout << i << std::endl;
            try {
                sub.consume();
                redis.publish("Drone", "WAITING");
                std::cout << "Waiting Main msg" << std::endl;
            } catch (const Error &err) {
                spdlog::error("Error consuming messages");
            }
            i++;
        }

        return 0;
    }
} // drones