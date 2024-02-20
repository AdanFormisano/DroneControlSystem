#include <spdlog/spdlog.h>
#include <unistd.h>
#include "DroneControl.h"

int main() {
    // Fork to create the Drone and DroneControl processes
    pid_t pidDroneControl = fork();

    if (pidDroneControl == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pidDroneControl == 0) {
        // In child DroneControl process
        spdlog::info("Drone Control process starting");
        drone_control::DroneControl droneControl;
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
//            spdlog::info("Parent process waiting for children to finish");
//            waitpid(pidDroneControl, nullptr, 0);
//            waitpid(pidDrone, nullptr, 0);

            // Here should be the monitor and simulation processes (should stay in the main process?)
        }
    }

    return 0;
}
