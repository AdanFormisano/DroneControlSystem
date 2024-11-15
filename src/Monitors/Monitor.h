#ifndef MONITOR_H
#define MONITOR_H

#include <bitset>

#include "../Database/Database.h"
#include "../globals.h"
#include "sw/redis++/redis++.h"
#include <set>

using namespace sw::redis;

class Monitor
{
public:
    Redis& shared_redis;

    explicit Monitor(Redis& redis);
    virtual ~Monitor() { JoinThread(); }
    virtual void RunMonitor() {};
    int JoinThread();

protected:
    Database db;
    std::thread t;
    int tick_last_read = 0; // TODO: Deprecated, use lastProcessedTime instead
    std::string old_processed_time = "-1:-1:-1";
    std::string latest_processed_time = "00:00:00";
    bool sim_running = true;

    void WriteToDB(const std::string& query);
    void CheckSimulationEnd();
};

class RechargeTimeMonitor final : public Monitor
{
public:
    explicit RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;
    std::unordered_set<int> drone_id_written;
    std::string temp_time = "00:00:00";

    void checkDroneRechargeTime();
    void getChargingDrones(pqxx::work& W);
    void getChargedDrones(pqxx::work& W);
};

class WaveCoverageMonitor final : public Monitor
{
public:
    explicit WaveCoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    struct WaveVerification
    {
        int wave_id;
        int tick_n;
        int drone_id;
        std::string issue_type;
    };

    std::vector<WaveVerification> getWaveVerification(); // wave_id, tick_n, drone_id
    void checkWaveCoverage();
    void checkCoverageVerification();
};

class AreaCoverageMonitor final : public Monitor
{
public:
    explicit AreaCoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    int tick_n = 0;

    struct AreaData
    {
        int tick_n = -1;
        int wave_id = -1;
        int drone_id = -1;
    };

    // TODO: A single nested map could be used: the first value for each coords is always the "timer", the rest are the list of ticks that were unverified
    std::unordered_map<int, std::array<AreaData, 300>> area_coverage_data; // X, (tick, drone_id) index of array is Y
    std::map<int, std::map<int, std::bitset<6000>>> unverified_ticks;

    void checkAreaCoverage(); // Reads area coverage data
    void readAreaCoverageData(const AreaData& area_data, int X, int Y); // Reads area coverage data
    void InsertUnverifiedTicks();
};

class DroneChargeMonitor final : public Monitor
{
public:
    explicit DroneChargeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    struct NewDroneData
    {
        int first_tick = -1;
        int last_tick = -1;
        float final_charge{};
        bool arrived_at_base = false;
    };

    void checkDroneCharge();
    void getDroneData();
    void ElaborateData();

    bool write_based_drones = false;
    bool write_dead_drones = false;
    std::unordered_map<int, NewDroneData> drones_data;
    std::vector<int> data_ready;    // Drone IDs that have all data ready
    std::set<int> failed_drones;
    std::string temp_time = "00:00:00";
};

class SystemPerformanceMonitor final : public Monitor
{
public:
    explicit SystemPerformanceMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    struct PerformanceData
    {
        int tick_n;
        int working_drones_count;
        int waves_count;
        double performance;
    };

    std::vector<PerformanceData> performance_data;

    void checkPerformance();
    void getPerformanceData();
};

#endif  // MONITOR_H
