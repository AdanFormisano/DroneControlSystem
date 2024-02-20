#include <spdlog/spdlog.h>
#include <unistd.h>
#include "DroneControl/DroneControl.h"
#include <sw/redis++/redis++.h>

using namespace sw::redis;

int main() {
    // Fork to create the Drone and DroneControl processes
    pid_t pidDroneControl = fork();

    if (pidDroneControl == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pidDroneControl == 0) {
        // In child DroneControl process
        drone_control::DroneControl droneControl;
        droneControl.start();
    } else {
        // In parent process create new child Drone process
        pid_t pidDrone = fork();

        if (pidDrone == -1) {
            spdlog::error("Fork for Drone failed");
            return 1;
        } else if (pidDrone == 0) {
            // In child Drone process
            spdlog::info("Drone process starting");
            // "Drone process" class here
        } else {
            // In parent process
            // Using Redis sub/pu, parent should wait for completed initialization of DroneControl and Drone
            // Then parent should start the monitor and simulation processes

            // Maybe check if connection is needed to create a subscriber
            Redis redis("tcp://127.0.0.1:7777");
            auto sub = redis.subscriber();

//            sub.on_message([](std::string channel, std::string msg) {
//                if
//            });

            // Here should be the monitor and simulation processes (should stay in the main process?)
        }
    }

    return 0;
}
