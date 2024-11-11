#ifndef DRONECONTROLSYSTEM_CHARGEBASE_H
#define DRONECONTROLSYSTEM_CHARGEBASE_H

#include <vector>
#include <boost/interprocess/ipc/message_queue.hpp>
#include "../globals.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;
using namespace boost::interprocess;

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
    void SetEngine();
    void ChargeDrone();
    void ChargeDroneMegaSpeed();
    void ReleaseDrone(const ChargingDroneData& drone_data) const;
    void Run();

private:
    message_queue mq_charge;
    std::mt19937 engine;
    Redis& redis;
    std::string current_stream_id = "0-0";
    std::unordered_map<int, ChargingDroneData> charging_drones;

    explicit ChargeBase(Redis& redis);

    void ReadChargeStream();
    void SetChargeData(const std::vector<std::pair<std::string, std::string>>& data);
    float getChargeRate();
};

#endif //DRONECONTROLSYSTEM_CHARGEBASE_H
