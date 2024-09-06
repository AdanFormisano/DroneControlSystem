#include "TestGenerator.h"
#include <spdlog/spdlog.h>

TestGenerator::TestGenerator(Redis& redis) :
    test_redis(redis), mq(open_only, "drone_status_queue"), gen(rd()), dis(0, 1), dis_zone(0, ZONE_NUMBER - 1),
    dis_tick(1, 20)
{
    spdlog::info("Creating TestGenerator object");
    // message_queue::remove("test_generator_queue");

    // Everything_is_fine scenario [80%]
    scenarios[0.9f] = []()
    {
        spdlog::info("Everything is fine");
    };

    // Drone_failure scenario (drone stops working) [10%]
    scenarios[0.95f] = [this]()
    {
        // Choose a random drone to explode
        auto [zone_id, drone_id] = ChooseRandomDrone();

        // Set the drone's status to "Exploded"
        // test_redis.hset("drone:" + std::to_string(drone_id), "status", "DEAD");
        DroneStatusMessage msg = {zone_id, drone_state_enum::DEAD};
        mq.send(&msg, sizeof(msg), 0);

        spdlog::warn("Drone {} exploded", drone_id);
    };

    // Connection_lost scenario (drone loses connection to the DroneControl system) [10%]
    scenarios[1.0f] = [this]()
    {
        auto [zone_id, drone_id] = ChooseRandomDrone();

        // Set the drone's status to "Connection lost"
        // test_redis.hset("drone:" + std::to_string(drone_id), "status", "NOT_CONNECTED");
        DroneStatusMessage msg = {zone_id, drone_state_enum::NOT_CONNECTED};
        mq.send(&msg, sizeof(msg), 0);

        // Probability of reconnecting (70%) [for testing purposes 50%]
        float reconnect = generateRandomFloat();
        if (reconnect < 0.7f)
        {
            // Calculate when the drone will reconnect
            spdlog::info("Drone {} will reconnect", drone_id);
            int tick = ChooseRandomTick();
            test_redis.hset("drones_fault:" + std::to_string(drone_id), "reconnect_tick", std::to_string(tick));
        }
        else
        {
            // Set -1 to indicate that the drone will not reconnect
            spdlog::info("Drone {} will not reconnect", drone_id);
            test_redis.hset("drones_fault:" + std::to_string(drone_id), "reconnect_tick", "-1");
        }
    };

    // Sleep for 5 seconds to allow the DroneControlSystem to start
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

void TestGenerator::Run()
{
    while (true)
    {
        // Generate a random float between 0 and 1 to decide the scenario
        float randomValue = generateRandomFloat();

        // Find the first key in the map that is greater than the random float
        auto it = scenarios.upper_bound(randomValue);

        // If such a key exists, execute the corresponding function
        if (it != scenarios.end())
        {
            it->second();
        }

        // Sleep for 5 seconds
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

DroneInfo TestGenerator::ChooseRandomDrone()
{
    // Choose a random zone
    int zone_id = dis_zone(gen);

    // Get a random drone from the zone's redis db
    auto v = test_redis.srandmember("zone:" + std::to_string(zone_id) + ":drones_alive");
    if (!v.has_value())
    {
        spdlog::error("No drones in zone {}", zone_id);
        return {0, 0};
    }
    if (v == std::nullopt)
    {
        spdlog::error("No alive drones found for zone {}", zone_id);
        return {0, 0};
    }
    spdlog::info("Drone chosen: {}", v.value());
    return {zone_id, std::stoi(v.value())};
}

int TestGenerator::ChooseRandomTick()
{
    return dis_tick(gen);
}
