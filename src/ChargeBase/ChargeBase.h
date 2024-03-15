//
// Created by roryu on 23/02/2024.
//

#ifndef DRONECONTROLSYSTEM_CHARGEBASE_H
#define DRONECONTROLSYSTEM_CHARGEBASE_H
#include <vector>
#include <optional>
#include <spdlog/spdlog.h>
#include "../globals.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;

namespace charge_base {
// Forward declaration of Drone to resolve circular dependency
//  Singleton pattern
    class ChargeBase {
    private:
        // std::unordered_map<std::string, std::string> drone_data;
        struct ext_drone_data {
            drone_data base_data;
            float charge_rate{};
        };

        std::mt19937 engine;
        Redis& redis;
        std::string current_stream_id = "0-0";
        std::vector<ext_drone_data> charging_drones;
        int tick_n = 0;

        ChargeBase(Redis& redis) : redis(redis){}

        void ReadChargeStream();
        void SetChargeData(const std::vector<std::pair<std::string, std::string>>& data);
        float getChargeRate();

    public:
        // Prevent copy construction and assignment
        ChargeBase() = delete;

        //not thread safe thought does it really need to be?
        static ChargeBase* getInstance(Redis& redis);

        void SetEngine(std::random_device&);
        void ChargeDrone();
        void releaseDrone(ext_drone_data &);
        void Run();

        // Destructor
        ~ChargeBase() = default;
    };
}
#endif //DRONECONTROLSYSTEM_CHARGEBASE_H
