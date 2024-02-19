#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H

#include <vector>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

struct DroneData {
    int id;
    std::string status;
};

class DroneControl {
private:
    Redis redis;

public:
    DroneControl();

    // Get drone status from Redis
    DroneData getDroneStatusFromRedis(int id);

};

#endif //DRONECONTROLSYSTEM_DRONECONTROL_H