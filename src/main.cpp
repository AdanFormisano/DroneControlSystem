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
    spdlog::set_pattern("[%T.%e] [Main] [%^%l%$] %v");

    // Forks to create the Drone and DroneControl processes
    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pid_drone_control == 0) {
        // In child DroneControl process

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

            // FIXME: This is a placeholder for the monitor process, without it the main process will exit and
            //  the children will be terminated
            std::this_thread::sleep_for(std::chrono::seconds(15));
            std::cout << "Exiting..." << std::endl;
        }
    }

    return 0;
}
