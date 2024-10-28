#ifndef MONITOR_H
#define MONITOR_H

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
    int tick_last_read = 0;

    void WriteToDB(const std::string& query);
};

class RechargeTimeMonitor final : public Monitor
{
public:
    explicit RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;
    std::unordered_set<int> drone_id_written;

    void checkDroneRechargeTime();
    void getChargingDrones(pqxx::work& W);
    void getChargedDrones(pqxx::work& W);
};

class CoverageMonitor final : public Monitor
{
public:
    explicit CoverageMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:

    void checkCoverage();
    void checkCoverageVerification();

    struct WaveVerification
    {
        int wave_id;
        int tick_n;
        int drone_id;
        std::string issue_type;
        int X;
        int Y;
    };

    std::vector<WaveVerification> getWaveVerification(); // wave_id, tick_n, drone_id
};

class DroneChargeMonitor final : public Monitor
{
public:
    explicit DroneChargeMonitor(Redis& redis) : Monitor(redis) {};
    void RunMonitor() override;

private:
    struct DroneData
    {
        int drone_id{};
        float consumption_factor{};
        bool arrived_at_base{};

        // Overload the equality operator for comparison in unordered_set
        bool operator==(const DroneData& other) const
        {
            return drone_id == other.drone_id &&
                consumption_factor == other.consumption_factor &&
                arrived_at_base == other.arrived_at_base;
        }
    };

    struct DroneDataHash {
        std::size_t operator()(const DroneData& drone) const {
            // Combine the hash of all fields
            std::size_t hash1 = std::hash<int>()(drone.drone_id);
            std::size_t hash2 = std::hash<float>()(drone.consumption_factor);
            std::size_t hash3 = std::hash<bool>()(drone.arrived_at_base);

            // Combine the hashes using bitwise XOR and shifts to reduce collisions
            return hash1 ^ (hash2 << 1) ^ (hash3 << 2);
        }
    };

    bool write_based_drones = false;
    bool write_dead_drones = false;
    std::set<int> failed_drones;
    void checkDroneCharge();
    void checkBasedDrones(std::unordered_set<DroneData, DroneDataHash>& based_drones, pqxx::work& W);
    void checkDeadDrones(std::unordered_set<DroneData, DroneDataHash>& dead_drones, pqxx::work& W);
    std::unordered_set<DroneData, DroneDataHash> getBasedDrones(pqxx::work& W);
    std::unordered_set<DroneData, DroneDataHash> getDeadDrones(pqxx::work& W);
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
    // void getPerformanceData(int last_processed_tick);
    // void simulateQueryExecution(size_t num_rows);
};

#endif  // MONITOR_H
