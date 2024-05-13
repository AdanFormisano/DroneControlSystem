#include "TestGenerator.h"
#include <spdlog/spdlog.h>

TestGenerator::TestGenerator(Redis &redis) :
    gen(rd()), dis(0, 1), dis_zone(0, ZONE_NUMBER - 1), test_redis(redis) {
    // Populate the map with cumulative probabilities and corresponding functions

    spdlog::info("Creating TestGenerator object");

    // Everything_is_fine scenario [80%]
    scenarios[0.2f] = []() {
        spdlog::info("Everything is fine");
    };

    // Drone_failure scenario (drone stops working) [10%]
    scenarios[0.9f] = [this]() {
        // Choose a random drone to explode
        int drone_id = ChooseRandomDrone();

        // Set the drone's status to "Exploded"
        test_redis.hset("drone:" + std::to_string(drone_id), "status", "EXPLODED");

        spdlog::warn("Drone {} exploded", drone_id);
    };

    // Connection_lost scenario (drone loses connection to the DroneControl system) [10%]
    scenarios[1.0f] = []() {
        spdlog::warn("Connection lost");
    };

    // Sleep for 5 seconds
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void TestGenerator::Run() {
    while (true) {

        // Generate a random float between 0 and 1 to decide the scenario
        float randomValue = generateRandomFloat();

        // Find the first key in the map that is greater than the random float
        auto it = scenarios.upper_bound(randomValue);

        // If such a key exists, execute the corresponding function
        if (it != scenarios.end()) {
            it->second();
        }

        // Sleep for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

int TestGenerator::ChooseRandomDrone() {
    // Choose a random zone
    int zone_id = dis_zone(gen);

    // Get a random drone from the zone's redis db
    auto drone_id = test_redis.srandmember("zone:" + std::to_string(zone_id) + ":drones");

    return std::stoi(drone_id.value());
}