#include "Drone.h"
#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include <iostream>
#include <chrono>

namespace drones {
    Drone::Drone(int id, Redis& sharedRedis) : id(id), drone_redis(sharedRedis) {
        redis_id = "drone:" + std::to_string(id);
        drone_charge = 100.0;

        // Adding the drone to the dataset on redis
        drone_redis.hset(redis_id, "status", "idle");
    }


    int Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
        spdlog::info("Initializing Drone process");

        // Initialization finished
        utils::SyncWait(redis);

        return 0;
    }

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

    // This will be the ran in the threads of each drone
    void Drone::Run() {
        // Implementing option 1: each drone updates its status using its key in Redis
        drone_thread_id = std::this_thread::get_id();

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
        spdlog::info("Drone {} updated its status", id);
    }


    void DroneManager::CreateDrone(int number_of_drones, Redis& shared_redis) {
    for (int i = 0; i < number_of_drones; i++) {
        // Crea un nuovo obj Drone utilizzando std::make_unique.
        // std::make_unique alloca dinamicamente un obj e ritorna un pointer unico ad esso
        auto drone = std::make_unique<Drone>(i, shared_redis);

        // Aggiunge un nuovo thread alla lista drone_threads.
        // Il thread eseguirà la func membro Run() dell'obj Drone appena creato
        drone_threads.emplace_back(&Drone::Run, drone.get());

        // Aggiunge l'obj Drone appena creato al vettore drone_vector.
        // std::move viene usato per trasf la proprietà dell'obj drone al vettore
        drone_vector.push_back(std::move(drone));
    }
    // Stampa un msg di log usando la lib spdlog.
    // Il msg indica il numero di droni creati.
    spdlog::info("Created {} drones", number_of_drones);
}

    DroneManager::~DroneManager() {
        for (auto& thread : drone_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void DroneManager::PrintDroneThreadsIDs() const {
        for (const auto& drone : drone_vector) {
            std::cout << "Drone thread ID: " << drone->getThreadId() << std::endl;
        }
    }
} // drones
