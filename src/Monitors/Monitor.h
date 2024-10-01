#ifndef MONITOR_H
#define MONITOR_H

#include <set>
#include "../Database/Database.h"
#include "sw/redis++/redis++.h"
#include "../globals.h"

using namespace sw::redis;

class Monitor {
public:
    Redis &shared_redis;

    explicit Monitor(Redis& redis);
    virtual ~Monitor() { JoinThread(); }
    virtual void RunMonitor() {};
    int JoinThread();

protected:
    Database db;
    std::thread t;
    int tick_last_read = 0;

    void WriteToDB(const std::string& query);
};

class RechargeTimeMonitor final : public Monitor {
public:
    explicit RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;

    void checkDroneRechargeTime();
    void getChargingDrones(pqxx::work& W);
    void getChargedDrones(pqxx::work& W);
};

class CoverageMonitor final : public Monitor {
public:
    explicit CoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::set<int> failed_ticks;
    std::vector<std::array<int,3>> failed_data;

    void checkCoverage();
    void checkWaveVerification();
    void checkAreaCoverage();

    struct WaveVerification
    {
        int wave_id;
        int tick_n;
        int drone_id;
    };

    std::vector<WaveVerification> getWaveVerification();   // wave_id, tick_n, drone_id
};

class DroneChargeMonitor final : public Monitor {
public:
    explicit DroneChargeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

    int last_tick = 0;

private:
    std::unordered_map<int, float> charge_needed;

    void getChargeNeededForZones();
    void checkDroneCharge();
};

class TimeToReadDataMonitor final : public Monitor {
public:
    explicit TimeToReadDataMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::vector<int> failed_ticks;

    void checkTimeToReadData();
};

#endif  // MONITOR_H