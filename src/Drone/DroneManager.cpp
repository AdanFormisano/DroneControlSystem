#include "DroneManager.h"
#include "../../utils/RedisUtils.h"
#include <iostream>
#include <spdlog/spdlog.h>

namespace drones {
    int Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e][%^%l%$][Drone] %v");
        spdlog::info("Initializing Drone process");

        // TESTING: Create 10 drones and each is a thread
        drones::DroneManager dm;
        dm.CreateGlobalZones();
        // FIXME: DroneManager should first create the DroneZones
        // dm.CreateDrone(10, drone_redis);
        // dm.PrintDroneThreadsIDs();

        // Initialization finished
        utils::SyncWait(redis);

        return 0;
    }

    DroneManager::~DroneManager() {
        for (auto& thread : drone_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
    }

    void DroneManager::CreateDrone(int number_of_drones, Redis& shared_redis) {
        for (int i = 0; i < number_of_drones; i++) {
            auto drone = std::make_unique<Drone>(i, shared_redis);
            drone_threads.emplace_back(&Drone::Run, drone.get());
            drone_vector.push_back(std::move(drone));
        }
        spdlog::info("Created {} drones", number_of_drones);
    }

    // Creates the zones using the global coordinates
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
                spdlog::info("Zone {} created 1:({},{}) 2:({},{}) 3:({},{}) 4:({},{})", i,
                             zones[i][0].first, zones[i][0].second,
                             zones[i][1].first, zones[i][1].second,
                             zones[i][2].first, zones[i][2].second,
                             zones[i][3].first, zones[i][3].second);
                ++i;
            }
        }
    }

    void DroneManager::PrintDroneThreadsIDs() const {
        for (const auto& drone : drone_vector) {
            std::cout << "Drone thread ID: " << drone->getThreadId() << std::endl;
        }
    }
} // drones

