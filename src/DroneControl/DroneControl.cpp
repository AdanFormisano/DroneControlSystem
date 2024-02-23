#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "DroneControl.h"
#include "../globals.h"

namespace drone_control {
    DroneControl::DroneControl(Redis& shared_redis) : redis(shared_redis) {};

    void Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e] [DroneControl] [%^%l%$] %v");
        spdlog::info("DroneControl process starting");

        utils::RedisConnectionCheck(redis, "Drone Control");

        drone_control::DroneControl droneControl(redis);

        // Finished initialization
        utils::SyncWait(redis);
    }

    void DroneControl::Run() {
        // spdlog::info("DroneControl process running");
    }
} // drone_control
