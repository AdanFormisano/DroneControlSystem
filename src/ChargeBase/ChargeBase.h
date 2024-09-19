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

class ChargeBase
{
public:
    // Prevent copy construction and assignment
    ChargeBase() = delete;
    // Destructor
    ~ChargeBase() = default;

    int tick_n = 0;

    //not thread safe thought does it really need to be?
    static ChargeBase* getInstance(Redis& redis);
    void SetEngine(std::random_device&);
    void ChargeDrone();
    void ChargeDroneMegaSpeed();
    void ReleaseDrone(ChargingDroneData& drone_data);
    void Run();

private:
    std::mt19937 engine;
    Redis& redis;
    std::string current_stream_id = "0-0";
    std::unordered_map<int, ChargingDroneData> charging_drones;

    ChargeBase(Redis& redis);

    void TickCompleted();
    void ReadChargeStream();
    void SetChargeData(const std::vector<std::pair<std::string, std::string>>& data);
    float getChargeRate();
};

#endif //DRONECONTROLSYSTEM_CHARGEBASE_H
