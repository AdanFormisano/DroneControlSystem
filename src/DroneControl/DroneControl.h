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
        Redis redis = Redis("tcp://127.0.0.1:7777");
        DroneControl();
        void start();
    };
} // drone_control

#endif //DRONECONTROLSYSTEM_DRONECONTROL_H