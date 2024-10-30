#include "TestGenerator.h"
#include "../../utils/LogUtils.h"
#include "../globals.h"
#include <csignal>
#include <iostream>
#include <spdlog/spdlog.h>

TestGenerator::TestGenerator(Redis& redis) : test_redis(redis),
                                             mq(open_or_create, "drone_fault_queue", 100, sizeof(TG_data)),
                                             mq_charge(open_or_create, "charge_fault_queue", 100, sizeof(TG_charge_data)),
                                             gen(rd()), dis(0, 1), dis_consumption(1.5, 2), dis_drone(0, 299),
                                             dis_tick(1, 20),
                                             dis_charge_rate(0.01, 2)
{
    // std::cout << "[TestGenerator] TestGenerator created" << std::endl;
    log_tg("TestGenerator created");
    // message_queue::remove("test_generator_queue");

    // Everything_is_fine scenario [80%]
    scenarios[0.2f] = []()
    {
        // spdlog::info("Everything is fine");
    };

    // Charge_rate malfunction scenario
    scenarios[0.4f] = [this]()
    {
        // Choose a random drone to increase its charge rate from Redis
        auto drone_id = std::stoi(test_redis.srandmember("charging_drones").value_or("-1"));

        if (drone_id != -1)
        {
            // Generate a random charge rate between 1.5 and 2 times faster than normal
            float charge_rate_factor = dis_charge_rate(gen);

            // Send a message to ChargeBase to set the drone's charge rate
            TG_charge_data msg = {
                drone_id,
                charge_rate_factor
            };
            mq_charge.send(&msg, sizeof(msg), 0);

            // std::cout << "[TestGenerator] Drone " << drone_id << " has charge rate factor of " << charge_rate_factor << std::endl;
            log_tg("Drone " + std::to_string(drone_id) + " has charge rate factor of " + std::to_string(charge_rate_factor));
        }
        else
        {
            // std::cout << "[TestGenerator] No drones found in the charging list" << std::endl;
            log_tg("No drones found in the charging list");
        }

    };

    // High_consumption [5%]
    scenarios[0.6f] = [this]()
    {
        // Choose a random drone to increase its consumption rate
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Generate a random high consumption factor between 1.5 and 2c
        float high_consumption_factor = dis_consumption(gen);

        // Send a message to ScannerManager to set the drone's high consumption factor
        TG_data msg = {
            drone_id, wave_id,
            drone_state_enum::NONE, -1, high_consumption_factor
        };
        mq.send(&msg, sizeof(msg), 0);

        // std::cout << "[TestGenerator] Drone " << drone_id << " has high consumption factor of " <<
        //    high_consumption_factor << std::endl;
        log_tg("Drone " + std::to_string(drone_id) + " has high consumption factor of " + std::to_string(high_consumption_factor));
    };

    // Drone_failure scenario (drone stops working) [5%]
    scenarios[0.8f] = [this]()
    {
        // Choose a random drone to explode
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Send a message to ScannerManager to set the drone's status to "DEAD"
        TG_data msg = {drone_id, wave_id, drone_state_enum::DEAD, -1, 1};
        mq.send(&msg, sizeof(msg), 0);

        // std::cout << "[TestGenerator] Drone " << drone_id << " exploded" << std::endl;
        log_tg("Drone " + std::to_string(drone_id) + " exploded");
    };

    // Connection_lost scenario (drone loses connection to the DroneControl system) [10%]
    scenarios[1.0f] = [this]()
    {
        auto [wave_id, drone_id] = ChooseRandomDrone();

        // Probability of reconnecting (70%) [for testing purposes 50%]
        float reconnect = generateRandomFloat();

        if (reconnect < 0.7f)
        {
            // Calculate when the drone will reconnect
            // Send a message to ScannerManager to set the drone's status to "RECONECTED"
            auto reconnect_tick = ChooseRandomTick();
            TG_data msg = {drone_id, wave_id, drone_state_enum::DISCONNECTED, reconnect_tick, 1};
            mq.send(&msg, sizeof(msg), 0);

            // std::cout << "[TestGenerator] Drone " << drone_id << " disconnected and will reconnect after " <<
            //    reconnect_tick << " tick" << std::endl;
            log_tg("Drone " + std::to_string(drone_id) + " disconnected and will reconnect after " + std::to_string(reconnect_tick) + " tick");
        }
        else
        {
            // Send a message to ScannerManager to set the drone's status to "DISCONNECTED"
            TG_data msg = {drone_id, wave_id, drone_state_enum::DISCONNECTED, -1, 1};
            mq.send(&msg, sizeof(msg), 0);

            // std::cout << "[TestGenerator] Drone " << drone_id << " disconnected and will NOT reconnect" << std::endl;
            log_tg("Drone " + std::to_string(drone_id) + " disconnected and will NOT reconnect");
        }
    };

    // Sleep for 5 seconds to allow the DroneControlSystem to start
    std::this_thread::sleep_for(std::chrono::seconds(5));
}

// Signal handler function
void signalHandler(int signum)
{
    // std::cout << "[TestGenerator] Received signal " << signum << ", exiting gracefully..." << std::endl;
    log_tg("Received signal " + std::to_string(signum) + ", exiting gracefully...");
    // Clean up and terminate the process
    exit(signum);
}

void TestGenerator::Run()
{
    // std::cout << "[TestGenerator] Running" << std::endl;
    log_tg("Running");
    signal(SIGTERM, signalHandler);

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

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

DroneInfo TestGenerator::ChooseRandomDrone()
{
    // Get a random alive wave from Redis
    auto wave_id = std::stoi(test_redis.srandmember("waves_alive").value_or("-1"));

    if (wave_id == -1)
    {
        // throw std::runtime_error("Wave not found");
        // std::cout << "[TestGenerator] Wave not found" << std::endl;
        log_tg("Wave not found");
        return {};
    }

    // Get a random drone from the wave
    auto _ = dis_drone(gen);
    int drone_id = wave_id * 1000 + _;

    return {wave_id, drone_id};
}

int TestGenerator::ChooseRandomTick()
{
    return dis_tick(gen);
}
