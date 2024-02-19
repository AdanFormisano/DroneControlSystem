#ifndef DRONECONTROLSYSTEM_DRONE_H
#define DRONECONTROLSYSTEM_DRONE_H

#include <string>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

// TODO: Check if namespace is necessary
namespace drones {
    class Drone {
    private:
        const int id; // TODO: it should be generated by the system
        std::string status; // TODO: Change to enum
        std::string data;
        Redis redis;
        std::string key;

    public:
        Drone(int);    // Constructor
        [[nodiscard]] int getId() const;
        int uploadData();
    };
} // drones

#endif //DRONECONTROLSYSTEM_DRONE_H