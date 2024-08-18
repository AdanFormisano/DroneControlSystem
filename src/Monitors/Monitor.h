#ifndef MONITOR_H
#define MONITOR_H

#include <set>

#include "../Database/Database.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;

/* To avoid extra work reading the hole database, we can make sure that the monitors only read the data that they need.
 * This way, we can avoid reading the whole database every time we need to check something.
 * This can be achived by saving the latest ordered row every time we read from the db, and save it as an offset.
 */

class Monitor {
public:
    Redis &shared_redis;

    explicit Monitor(Redis&);
    virtual ~Monitor() { JoinThread(); }

    virtual void RunMonitor() {};

    int JoinThread() {
        if (t.joinable()) {
            t.join();
            return 0;
        } else {
            return 1;
        }
    };

protected:
    Database db;
    boost::thread t;
};

class RechargeTimeMonitor : public Monitor {
public:
    RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;

    void checkDroneRechargeTime();  // Thread's function
    void getChargingDrones(pqxx::work& W);
    void getChargedDrones(pqxx::work& W);
};

class CoverageMonitor : public Monitor {
public:
    CoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;
    int last_tick = 0;  // Last tick that was checked/read from DB

private:
    std::set<int> failed_ticks;
    std::vector<std::array<int,3>> failed_data;

    void checkCoverage();  // Thread's function
    void checkZoneVerification(pqxx::nontransaction& N);
    void checkAreaCoverage();

    std::vector<std::array<int,3>> getZoneVerification(pqxx::nontransaction& N);
};

class DroneChargeMonitor : public Monitor
{
public:
    DroneChargeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;
    int last_tick = 0;  // Last tick that was checked/read from DB

private:
    std::unordered_map<int, float> charge_needed;

    void getChargeNeededForZones();
    void checkDroneCharge();
};

class TimeToReadDataMonitor : public Monitor
{
public:
    TimeToReadDataMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::vector<int> failed_ticks;

    void checktimeToReadData();
};
#endif  // MONITOR_H
