#ifndef DRONECONTROLSYSTEM_DRONE_H
#define DRONECONTROLSYSTEM_DRONE_H

#include <string>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

// TODO: Check if namespace is necessary
namespace drones {
    class Drone {
    private:
        int id;
        static int nextId;
        std::string status; // TODO: Change to enum
        std::string data;

    public:
        Drone();    // Constructor
        [[nodiscard]] int getId() const;

        // Upload data to Redis

        int uploadData(Redis &redis, std::string& key) const;
    };
} // drones

#endif //DRONECONTROLSYSTEM_DRONE_H