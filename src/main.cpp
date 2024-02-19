#include "DroneControl.h"
#include <spdlog/spdlog.h>
#include "Drone.h"

using namespace sw::redis;

int main() {
    // Start DroneControl
    DroneControl droneControl;

    // For testing purposes create a single drone
    spdlog::info("Creating a test drone");
    drones::Drone testDrone(1);
    spdlog::info("Test drone {} created", testDrone.getId());
    
    // Get drone data from Redis
    DroneData data = droneControl.getDroneData(testDrone.getId());
    spdlog::info("Drone {} status: {}", data.id, data.status);

    // Upload data from drone to Redis
    testDrone.uploadData();

    // Start ChargingBase

    //Start MovementControl

    return 0;
}
