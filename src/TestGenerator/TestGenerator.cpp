#include "TestGenerator.h"
#include "../globals.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>

TestGenerator::TestGenerator(Redis &redis) : test_redis(redis), mq(open_only, "drone_fault_queue"), gen(rd()), dis(0, 1), dis_charge(1, 2), dis_drone(0, 299),
                                             dis_tick(1, 20){
    // spdlog::info("Creating TestGenerator object");
    std::cout << "Creating TestGenerator" << std::endl;
    // message_queue::remove("test_generator_queue");

    // Everything_is_fine scenario [80%]
    scenarios[0.4f] = []() {
        // spdlog::info("Everything is fine");
    };

    // High_consumption [5%]
    scenarios[0.6f] = [this]() {
        // Choose a random drone to increase its consumption rate
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Generate a random high consumption factor between 1 and 2
        float high_consumption_factor = dis_charge(gen);

        // Send a message to ScannerManager to set the drone's high consumption factor
        TG_data msg = {drone_id, wave_id, drone_state_enum::NONE, -1, high_consumption_factor};
        mq.send(&msg, sizeof(msg), 0);

        spdlog::warn("Drone {} has high consumption factor of {}", drone_id, high_consumption_factor);
    };


    // Drone_failure scenario (drone stops working) [10%]
    scenarios[0.8f] = [this]() {
        // Choose a random drone to explode
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Send a message to ScannerManager to set the drone's status to "DEAD"
        TG_data msg = {drone_id, wave_id, drone_state_enum::DEAD, -1, 1};
        mq.send(&msg, sizeof(msg), 0);

        // spdlog::warn("Drone {} exploded", drone_id);
        std::cout << "[TestGenerator] Drone " << drone_id << " exploded" << std::endl;
    };

    // Connection_lost scenario (drone loses connection to the DroneControl system) [10%]
    scenarios[1.0f] = [this]() {
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Probability of reconnecting (70%) [for testing purposes 50%]
        float reconnect = generateRandomFloat();

        if (reconnect < 0.7f) {
            // Calculate when the drone will reconnect
            // Send a message to ScannerManager to set the drone's status to "RECONECTED"
            auto reconnect_tick = ChooseRandomTick();
            TG_data msg = {drone_id, wave_id, drone_state_enum::DISCONNECTED, reconnect_tick, 1};
            mq.send(&msg, sizeof(msg), 0);
            // spdlog::warn("Drone {} disconnected and will reconnect after {} tick", drone_id, reconnect_tick);
            std::cout << "[TestGenerator] Drone " << drone_id << " disconnected and will reconnect after " << reconnect_tick << " tick" << std::endl;
        } else {
            // Send a message to ScannerManager to set the drone's status to "DISCONNECTED"
            TG_data msg = {drone_id, wave_id, drone_state_enum::DISCONNECTED, -1, 1};
            mq.send(&msg, sizeof(msg), 0);
            // spdlog::warn("Drone {} disconnected", drone_id);
            std::cout << "[TestGenerator] Drone " << drone_id << " disconnected" << std::endl;
        }
    };

    // Sleep for 5 seconds to allow the DroneControlSystem to start
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

// Signal handler function
void signalHandler(int signum) {
    std::cout << "[TestGenerator] Received signal " << signum << ", exiting gracefully..." << std::endl;
    // Clean up and terminate the process
    exit(signum);
}

void TestGenerator::Run() {
    signal(SIGTERM, signalHandler);

    while (true) {
        // Generate a random float between 0 and 1 to decide the scenario
        float randomValue = generateRandomFloat();

        // Find the first key in the map that is greater than the random float
        auto it = scenarios.upper_bound(randomValue);

        // If such a key exists, execute the corresponding function
        if (it != scenarios.end()) {
            it->second();
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

DroneInfo TestGenerator::ChooseRandomDrone() {

    // Get a random alive wave from Redis
    auto wave_id = std::stoi(test_redis.srandmember("waves_alive").value_or("-1"));
    spdlog::info("Wave ID: {}", wave_id);

    if (wave_id == -1) {
        // Create exception if the wave is not found
        throw std::runtime_error("Wave not found");
        return {};
    }

    // Get a random drone from the wave
    auto _ = dis_drone(gen);

    int drone_id = wave_id * 1000 + _;

    return {wave_id, drone_id};
}

int TestGenerator::ChooseRandomTick() {
    return dis_tick(gen);
}
