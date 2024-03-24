#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <spdlog/spdlog.h>

/* The coords used for the zones indicates the "square" that the drone is going to cover: it's not a real coordinate.
The real global coordinates are needed for the drone path. They are going to be calculated when the zones are created.
*/

namespace drones {
void DroneManager::Run() {
    spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
    spdlog::info("Initializing Drone process");

    drone_zones.reserve(300);
    drone_vector.reserve(300);
    drone_threads.reserve(300);

    // Calculate the zones' vertex_coords
    // TODO: Sono stronzo e l'ordine e' diverso da quello utilizzato per il path
    CalculateGlobalZoneCoords();

    int zone_id = 0; // Needs to be 0 because is used in DroneControl::setDroneData()

    // Create the DroneZones objects for every zone calculated
    for (auto &zone : zones) {
        DroneZone* dz = CreateDroneZone(zone, zone_id);

        int drone_id = (zone_id * 10) + new_drones_id[zone_id];

        // Create the drone
        CreateDrone(drone_id, dz);

        new_drones_id[zone_id]++;
        ++zone_id;
    }
    spdlog::info("All zones created");

    utils::SyncWait(shared_redis);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    CreateThreadBlocks();

    auto it_thread = drone_threads.begin();

    // Exists for the duration of the simulation
    try {
        bool sim_running = (shared_redis.get("sim_running") == "true");
        // Run the simulation
        while (sim_running) {
            // Get the time_point
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            for (auto &thread : drone_threads) {
                if (thread.try_join_for(boost::chrono::milliseconds(0))) {
#ifdef DEBUG
                    std::cout << "Thread " << thread.get_id() << " joined" << std::endl;
#endif
                    it_thread = std::remove_if(drone_threads.begin(), drone_threads.end(), [&thread](const boost::thread &t) {
                        return thread.get_id() == t.get_id();
                    });
                    thread.join();
                }
            }

            // Check if there is time left in the tick
            auto tick_now = std::chrono::steady_clock::now();
            if (tick_now < tick_start + tick_duration_ms) {
                // Sleep for the remaining time
                std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
            } else if (tick_now > tick_start + tick_duration_ms) {
                // Log if the tick took too long
                spdlog::warn("DroneManager tick took too long");
                break;
            }

            // std::cout << n_data_sent << " data sent" << std::endl;
            // Get sim_running from Redis
            sim_running = (shared_redis.get("sim_running") == "true");
            ++tick_n;
        }
    } catch (const sw::redis::IoError &e) {
        spdlog::error("Couldn't get sim_running: {}", e.what());
    }
}

DroneManager::DroneManager(Redis &redis) : shared_redis(redis) {
    new_drones_id.fill(1);
}

DroneManager::~DroneManager() {
    for (auto &thread : drone_threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

// Creates the zones' global vertex_coords
void DroneManager::CalculateGlobalZoneCoords() {
    int i = 0;
    int width = 62;
    int height = 2;

    for (int x = -124; x <= 124 - width; x += width) {
        for (int y = -75; y <= 75 - height; y += height) {
            zones[i][0] = {x, y};                   // Bottom left
            zones[i][1] = {x + width, y};           // Bottom right
            zones[i][2] = {x + width, y + height};  // Top right
            zones[i][3] = {x, y + height};          // Top left
            ++i;
        }
    }
}

// For a set of vertex_coords creates a DroneZone object
DroneZone* DroneManager::CreateDroneZone(std::array<std::pair<int, int>, 4> &zone, int zone_id) {
    drone_zones.emplace_back(zone_id, zone, this);
    return &drone_zones.back();
}

// Creates a drone for a zone
void DroneManager::CreateDrone(int zone_id, DroneZone* dz) {
    int drone_id = zone_id; // TODO: This is a placeholder, use better drone_id
    auto drone = std::make_shared<Drone>(drone_id, dz, this);

    // Adds the drone to the vector
    drone_vector.push_back(std::move(drone));
}

void DroneManager::CreateThreadBlocks() {
    // For every "block" of zones, create the threads
    int width = 62;
    int height = 2;
    int n_drone = 0;
    for (int x = -124; x <= 124 - width; x += width) {
        // Creates threads for a column of zones
        for (int y = -75; y <= 75 - height; y += height) {
            // Create the threads
            drone_threads.emplace_back(&Drone::Run, drone_vector[n_drone].get());

            ++n_drone;
        }
        std::this_thread::sleep_for(tick_duration_ms * 5);
        spdlog::info("Column of drones created");
    }
}

// Check if DC asked for new drones
//void DroneManager::CheckNewDrones() {
//    std::vector<std::string> zones_to_swap;
//
//    // Check if DC asked for new drones
//    shared_redis.lrange("zones_to_swap", 0, -1, std::back_inserter(zones_to_swap));
//
//    // For every zone create a drone object
//    for (const auto &zone_id : zones_to_swap) {
//        int z = std::stoi(zone_id);
//        // Get and available drone from the zone:id:drones list
//        auto drone_id = shared_redis.lpop("zone:" + zone_id + ":drones");
//
//        // Get the zone object
//        auto dz = &drone_zones[z];
//
//        // Create the drone
//        CreateDrone(z, dz);
//
//        // Set the drone to work
//        shared_redis.set("drone:" + drone_id.value() + ":command", "work");
//
//        // Create the thread
//        drone_threads.emplace_back(&Drone::Run, drone_vector[n_drone].get());
//    }
//}
} // namespace drones
