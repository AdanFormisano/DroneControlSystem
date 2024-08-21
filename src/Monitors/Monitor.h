#ifndef MONITOR_H
#define MONITOR_H

#include <set>

#include "../Database/Database.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;

// Base Monitor class
class Monitor {
public:
    Redis &shared_redis; // Redis connection shared with the main thread

    explicit Monitor(Redis&);
    virtual ~Monitor() { JoinThread(); }

    virtual void RunMonitor() {};   // Main execution function of thread, overitten by each monitor

    int JoinThread() {
        if (t.joinable()) {
            t.join();
            return 0;
        } else {
            return 1;
        }
    };

protected:
    Database db;        // Connection to Database
    boost::thread t;    // Thread for execution of monitor
};

class RechargeTimeMonitor : public Monitor {
public:
    RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;   // Used for storing the recharge time of drones

    void checkDroneRechargeTime();          // Thread's function
    void getChargingDrones(pqxx::work& W);  // Get the drones that are currently charging
    void getChargedDrones(pqxx::work& W);   // Get drone done charging
};

class CoverageMonitor : public Monitor {
public:
    CoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;
    int last_tick = 0;  // Last tick that was checked/read from DB

private:
    std::set<int> failed_ticks;
    std::vector<std::array<int,3>> failed_data;

    void checkCoverage();                                   // Thread's function
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
