#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <spdlog/spdlog.h>

namespace drones {
// Initializes the Drone process
int Init(Redis &redis) {
    spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
    spdlog::info("Initializing Drone process");

    // Create DroneManager
    drones::DroneManager dm(redis);

    // Calculate the zones' coordinates
    dm.CreateGlobalZones();

    int zone_id = 0; // Needs to be 0 because is used in DroneControl::setDroneData()
    // Create the DroneZones objects for every zone calculated
    for (auto &zone : dm.zones) {
        dm.CreateDroneZone(zone, zone_id);
        ++zone_id;
    }

    // Initialization finished
    utils::SyncWait(redis);

    return 0;
}

DroneManager::DroneManager(Redis &redis) : shared_redis(redis) {
}

DroneManager::~DroneManager() {
    for (auto &thread : drone_threads) {
        if (thread.joinable()) {
            // std::cout << "Joining thread " << thread.get_id() << std::endl;
            thread.join();
        }
    }
}

// Creates the zones' global coordinates
void DroneManager::CreateGlobalZones() {
    int i = 0;
    int width = 62;
    int height = 2;

    for (int x = -124; x <= 124 - width; x += width) {
        for (int y = -75; y <= 75 - height; y += height) {
            zones[i][0] = {x, y};
            zones[i][1] = {x + width, y};
            zones[i][2] = {x, y + height};
            zones[i][3] = {x + width, y + height};
            ++i;
        }
    }
}

// For a set of coords creates a DroneZone object
void DroneManager::CreateDroneZone(std::array<std::pair<int, int>, 4> zone, int zone_id) {
    drone_zones.emplace_back(zone_id, zone, shared_redis, this);
}

void DroneManager::PrintDroneThreadsIDs() const {
    for (const auto &drone : drone_vector) {
        std::cout << "Drone thread ID: " << drone->getThreadId() << std::endl;
    }
}
} // namespace drones
