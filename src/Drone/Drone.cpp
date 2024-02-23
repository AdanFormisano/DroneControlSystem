#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include <iostream>
#include <chrono>

namespace drones {
    // TODO: Write/format better this constructor
    Drone::Drone(int id, Redis& sharedRedis) : id(id), drone_redis(sharedRedis) {
        redis_id = "drone:" + std::to_string(id);
        data = "Wendy is a sleepy cat.";

        utils::RedisConnectionCheck(drone_redis, redis_id);

        // Adding the drone to the dataset on redis
        drone_redis.hset(redis_id, "status", "idle");

        // Creating the thread for the drone
        drone_thread = std::thread(&Drone::Run, this);
        std::thread::id thread_id = std::this_thread::get_id();
        std::cout << "Drone ID: " << id << " Thread ID: " << thread_id << std::endl;
    }

    Drone::~Drone() {
        if (drone_thread.joinable()) {
            std::thread::id thread_id = std::this_thread::get_id();
            std::cout << "Joining thread " << thread_id << std::endl;
            drone_thread.join();
        } 
    }


    int Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e] [Drone] [%^%l%$] %v");

        spdlog::info("Initializing Drone process");

        // Initialization finished
        utils::SyncWait(redis);

        // TESTING: Create 10 drones and each is a thread
        spdlog::info("Creating 10 drones");
        for (int i = 0; i < 10; i++) {
            drones::Drone drone(i, redis);
            spdlog::info("Drone {} created (Client ID {})", i, utils::RedisGetClientID(redis));
        }
        spdlog::info("----------All drones created----------");

        std::cout << "Drones Are working" << std::endl;

        return 0;
    }

/*
Drones should update their status every 5 seconds.

Theare are multiple ways to do this:
1. Each drone has its key in Redis and updates its status every 5 seconds
2. Every drone uses sub/pub to broadcast its status and the DroneControl should be listening to the channel
    and manages all the messages that are being sent
3. The status updates are sent from the drones using a Redis stream that the DroneControl keeps on reading
    to manage the status updates

The best way to choose is to implement a monitor and compare the performance of each method.
*/

    // This will be the ran in the threads of each drone
    void Drone::Run() {
        // Implementing option 1: each drone updates its status using its key in Redis
        
        // Sleep
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        Move();

        std::cout << position.first << " " << position.second << std::endl;
    }

    // This will just move the drone to a random position
    void Drone::Move() {
        float x = generateRandomFloat();
        float y = generateRandomFloat();

        position = std::make_pair(x, y);
    }

    int Drone::getId() const {
        return id;
    }
} // drones
