#ifndef DRONECONTROLSYSTEM_DRONEZONE_H
#define DRONECONTROLSYSTEM_DRONEZONE_H
#include "Drone.h"
#include <utility>

namespace drones {


    class DroneZone {
    public:
        DroneZone(int x, int y);
        ~DroneZone() = default;

        // Defined in the requirements
        int size_x = 62;
        int size_y = 2;

//        std::unique_ptr<Drone> getDrone1() { return std::move(drone1); }
//        std::unique_ptr<Drone> getDrone2() { return std::move(drone2); }
        Drone* getDrone1() { return drone1; }
        Drone* getDrone2() { return drone2; }

    private:
        // Global coordinates of   the zone
        const std::pair<int, int> global_coordinates; // TODO: Check if it can be calculated instead of hard-coding it

        // Each zone has 2 drones for redundancy
        Drone* drone1{};
        Drone* drone2{};
//        std::unique_ptr<Drone> drone1;
//        std::unique_ptr<Drone> drone2;
    };
}
#endif //DRONECONTROLSYSTEM_DRONEZONE_H
