#include "DroneControl.h"
#include <spdlog/spdlog.h>
#include "Drone.h"

int main() {
    // Start DroneControl
    DroneControl droneControl;

    // For testing purposes create a single drone
    spdlog::info("Creating a test drone");
    drones::Drone testDrone;
    spdlog::info("Test drone {} created", testDrone.getId());

    // Start ChargingBase

    //Start MovementControl

    return 0;
}

