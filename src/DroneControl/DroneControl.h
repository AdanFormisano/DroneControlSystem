#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H

#include <vector>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace drone_control {
    struct DroneData {
        int id;
        std::string status;
    };

    class DroneControl {
    public:
        Redis& redis;
        explicit DroneControl(Redis& shared_redis);

        void Run(); // This runs the drone control PROCESS
    };

    void Init(Redis& redis); // This initializes the drone control PROCESS
} // drone_control

#endif //DRONECONTROLSYSTEM_DRONECONTROL_H