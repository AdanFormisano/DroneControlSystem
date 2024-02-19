#include <spdlog/spdlog.h>
#include "../utils/RedisUtils.h"
#include "DroneControl.h"
#include "Drone.h"

//int StartDroneControl() {
//    spdlog::info("Drone Control starting");
//
//    spdlog::info("Connecting to Redis");
//    auto redis = Redis("tcp://127.0.0.1:7777");
//    utils::RedisConnectionCheck(redis);
//
//    return 0;
//}

DroneControl::DroneControl() : redis("tcp://127.0.0.1:7777"){
    spdlog::info("Drone Control starting");
    utils::RedisConnectionCheck(redis, "Drone Control");
}