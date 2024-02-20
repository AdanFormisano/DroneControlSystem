#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "DroneControl.h"

namespace drone_control {
    DroneControl::DroneControl() = default;

    void DroneControl::start() {
        spdlog::info("Drone Control starting");
        utils::RedisConnectionCheck(redis, "Drone Control");
    }

    // TODO: Check if this is the right spot for this function
//    DroneData DroneControl::getDroneData(int id) {
//        DroneData droneData{
//                .id = id
//        };
//
//        std::unordered_map<std::string, std::string> redisData;
//        std::string key = "drone:" + std::to_string(id);
//
//        auto clientID = redis.command<long long>("CLIENT", "ID");
//        spdlog::info("Using clientID: {}", clientID);
//        // TODO: Use try/catch
//        redis.hgetall(key, std::inserter(redisData, redisData.begin()));
//
//        for (const auto &pair: redisData) {
//            spdlog::info("Key: {}, Value: {}", pair.first, pair.second);
//        }
//
//        droneData.status = redisData["status"];
//        redisData.clear();
//
//        return droneData;
//    }
} // drone_control
