#ifndef MONITOR_H
#define MONITOR_H

#include <set>
#include "../Database/Database.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;

/**
 * \brief Base Monitor class.
 */
class Monitor {
public:
    Redis &shared_redis; ///< Redis connection shared with the main thread

    /**
     * \brief Constructor for Monitor.
     * \param redis Redis connection object.
     */
    explicit Monitor(Redis& redis);

    /**
     * \brief Destructor for Monitor.
     */
    virtual ~Monitor() { JoinThread(); }

    /**
     * \brief Main execution function of thread, overridden by each monitor.
     */
    virtual void RunMonitor() {};

    /**
     * \brief Joins the thread if it is joinable.
     * \return 0 if the thread was joined, 1 otherwise.
     */
    int JoinThread();

protected:
    Database db;        ///< Connection to Database
    boost::thread t;    ///< Thread for execution of monitor
    int tick_last_read = 0;

    void WriteToDB(const std::string& query);
};

/**
 * \brief Class for the monitor that checks the recharge duration for each drone.
 */
class RechargeTimeMonitor : public Monitor {
public:
    /**
     * \brief Constructor for RechargeTimeMonitor.
     * \param redis Redis connection object.
     */
    explicit RechargeTimeMonitor(Redis& redis) : Monitor(redis) {};

    /**
     * \brief Main execution function of the monitor.
     */
    void RunMonitor() override;

private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;   ///< tick_n: <start, end> ticks of the recharge

    /**
     * \brief Checks the drone recharge time.
     */
    void checkDroneRechargeTime();

    /**
     * \brief Gets the drones that are currently charging.
     * \param W A work object for executing the query.
     */
    void getChargingDrones(pqxx::work& W);

    /**
     * \brief Gets the drones that are done charging.
     * \param W A work object for executing the query.
     */
    void getChargedDrones(pqxx::work& W);
};

/**
 * \brief Class for the monitors that check the coverage of each zone and of the total 6x6 km area.
 */
class CoverageMonitor : public Monitor {
public:
    /**
     * \brief Constructor for CoverageMonitor.
     * \param redis Redis connection object.
     */
    explicit CoverageMonitor(Redis& redis) : Monitor(redis) {};

    /**
     * \brief Main execution function of the monitor.
     */
    void RunMonitor() override;

    int last_tick = 0;  ///< Last tick that was checked/read from DB

private:
    std::set<int> failed_ticks;
    std::vector<std::array<int,3>> failed_data;

    /**
     * \brief Checks the coverage.
     */
    void checkCoverage();

    /**
     * \brief Checks the verification status of each zone.
     * \param N A nontransaction object for executing the query.
     */
    void checkZoneVerification();

    /**
     * \brief Checks the coverage of the entire area.
     */
    void checkAreaCoverage();

    /**
     * \brief Retrieves zones that were not verified.
     * \param N A nontransaction object for executing the query.
     * \return A vector of arrays, each containing the zone ID, tick number, and drone ID.
     */
    std::vector<std::array<int,3>> getZoneVerification();
};

/**
 * \brief Class for the monitor that checks the drone charge.
 */
class DroneChargeMonitor : public Monitor {
public:
    /**
     * \brief Constructor for DroneChargeMonitor.
     * \param redis Redis connection object.
     */
    explicit DroneChargeMonitor(Redis& redis) : Monitor(redis) {};

    /**
     * \brief Main execution function of the monitor.
     */
    void RunMonitor() override;

    int last_tick = 0;  ///< Last tick that was checked/read from DB

private:
    std::unordered_map<int, float> charge_needed;

    /**
     * \brief Retrieves the charge needed for each zone from Redis.
     */
    void getChargeNeededForZones();

    /**
     * \brief Monitors the drone charge and ensures drones have enough charge to return to base.
     */
    void checkDroneCharge();
};

/**
 * \brief Class for the monitor that checks the time to read data.
 */
class TimeToReadDataMonitor : public Monitor {
public:
    /**
     * \brief Constructor for TimeToReadDataMonitor.
     * \param redis Redis connection object.
     */
    explicit TimeToReadDataMonitor(Redis& redis) : Monitor(redis) {};

    /**
     * \brief Main execution function of the monitor.
     */
    void RunMonitor() override;

private:
    std::vector<int> failed_ticks;

    /**
     * \brief Checks the time to read data.
     */
    void checkTimeToReadData();
};

#endif  // MONITOR_H
