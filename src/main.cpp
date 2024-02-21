#include <spdlog/spdlog.h>
#include <unistd.h>
#include <sw/redis++/redis++.h>
#include <iostream>
#include "DroneControl/DroneControl.h"
#include "Drone/Drone.h"
#include "globals.h"
#include "../utils/RedisUtils.h"

using namespace sw::redis;

int main() {
    std::cout << "PID: " << getpid() << std::endl;
    spdlog::set_pattern("[%T.%e] [Main] [%^%l%$] %v");

    // Fork to create the Drone and DroneControl processes
    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pid_drone_control == 0) {
        // In child DroneControl process
        std::cout << "PID: " << getpid() << std::endl;

        auto drone_control_redis = Redis("tcp://127.0.0.1:7777");
        drone_control_redis.incr(sync_counter_key);

        drone_control::Init(drone_control_redis);
    } else {
        // In parent process create new child Drone process
        pid_t pid_drone = fork();
        if (pid_drone == -1) {
            spdlog::error("Fork for Drone failed");
            return 1;
        } else if (pid_drone == 0) {
            // In child Drone process
            std::cout << "PID: " << getpid() << std::endl;

            auto drone_redis = Redis("tcp://127.0.0.1:7777");
            drone_redis.incr(sync_counter_key);

            drones::Init(drone_redis);
        } else {
            // In parent process
            auto main_redis = Redis("tcp://127.0.0.1:7777");
            main_redis.incr(sync_counter_key);

            // Initialization finished
            utils::SyncWait(main_redis);

            // Here should be the monitor and simulation processes (should stay in the main process?)
        }
    }

    return 0;
}
