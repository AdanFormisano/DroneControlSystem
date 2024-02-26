#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H
#include "Drone.h"
#include <array>
#include <thread>

using namespace sw::redis;

namespace drones{
    class DroneZone;
}

namespace drones {
    int Init(Redis& redis); // This initializes the Drones PROCESS

    class DroneManager {
    public:
        // List of zones using the global coordinates
        std::array<std::array<std::pair<int, int>, 4>, 300> zones;
        std::vector<std::shared_ptr<Drone>> drone_vector;
        std::vector<std::thread> drone_threads;
        std::vector<DroneZone> drone_zones;
        Redis& shared_redis;

        explicit DroneManager(Redis&);
        ~DroneManager();

        // void CreateDrone(int number_of_drones, Redis& shared_redis);
        void CreateGlobalZones();
        // For a set of coords creates a DroneZone object
        void CreateDroneZone(std::array<std::pair<int, int>, 4>, int);
        void PrintDroneThreadsIDs() const;
        // Returns a reference to the vector of drones
        std::vector<std::shared_ptr<Drone>>& getDroneVector() { return drone_vector; }
        // Returns a reference to the vector of threads
        std::vector<std::thread>& getDroneThreads() { return drone_threads; }

    private:
    };

    class DroneZone {
    public:
        DroneZone(int, std::array<std::pair<int, int>, 4>&, Redis&, DroneManager*);
        ~DroneZone() = default;

        // Defined in the requirements
        int width = 62;
        int height = 2;
        std::array<std::pair<int, int>, 4>& coordinates;
        Redis& drone_redis;
        DroneManager* drone_manager;

    private:
        std::shared_ptr<Drone> drone_ptr;
        const int id;
    };
} // drones

#endif //DRONECONTROLSYSTEM_DRONEMANAGER_H
