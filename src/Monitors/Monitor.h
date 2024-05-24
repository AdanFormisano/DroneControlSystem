#ifndef MONITOR_H
#define MONITOR_H

#include "../Database/Database.h"

/* To avoid extra work reading the hole database, we can make sure that the monitors only read the data that they need.
 * This way, we can avoid reading the whole database every time we need to check something.
 * This can be achived by saving the latest ordered row every time we read from the db, and save it as an offset.
 */

class Monitor {
public:
    explicit Monitor();

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
    RechargeTimeMonitor() : Monitor() {};

    void RunMonitor() override;

   private:
    std::unordered_map<int, std::pair<int, int>> drone_recharge_time;

    void checkDroneRechargeTime();  // Thread's function
    void getChargingDrones(pqxx::work& W);
    void getChargedDrones(pqxx::work& W);
};
#endif  // MONITOR_H
