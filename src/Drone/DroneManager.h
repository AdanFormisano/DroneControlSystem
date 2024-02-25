#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H
#include "Drone.h"
#include "DroneZone.h"
#include <array>
#include <thread>

using namespace sw::redis;
namespace drones {


    int Init(Redis& redis); // This initializes the Drones PROCESS

    class DroneManager {
    private:
        std::vector<std::unique_ptr<Drone>> drone_vector;
        std::vector<std::thread> drone_threads;
        std::vector<DroneZone> drone_zones;

    public:
        // List of zones using the global coordinates
        std::array<std::array<std::pair<int, int>, 4>, 600> zones;

        DroneManager() = default;
        ~DroneManager();

        void CreateDrone(int number_of_drones, Redis& shared_redis);
        void CreateGlobalZones();
        void PrintDroneThreadsIDs() const;
    };
} // drones
#endif //DRONECONTROLSYSTEM_DRONEMANAGER_H
