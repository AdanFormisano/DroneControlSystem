/* Each monitor will run on a different thread, all will be started by the main thread

All monitors are going to be executed in parallel during the simulation. In particular there are 3 functional monitors
and 2 non-functional monitors.

The functional monitors are:
    - checkDroneRechargeTime(): This monitor checks that the number of ticks that a drone has been charging is between
        [2,3] hours.
    - checkAreaCoverage(): This monitor checks that all the points of the 6x6 Km area are verified (covered by the drones)
        every 5 minutes.
    - checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes (this has some internal
        implications).

The non-functional monitors are:
    - checkTimeToReadDroneData(): This monitor checks that the amount of time that the system takes to read one set of
        data entries from the drones is less than 2.24 seconds (this is the amount of time that a tick simulates).
    - checkDroneCharge(): This monitor checks that the drones' charge is correctly managed by the system. This means that
        the drones are swapped when their charge is just enough to return to the base.
*/

#include "Monitor.h"

Monitor::Monitor() {
    spdlog::info("Monitor object created");

    // Create db connection
    db.ConnectToDB("dcs", "postgres", "admin@123", "127.0.0.1", "5432");
}

void RechargeTimeMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = boost::thread(&RechargeTimeMonitor::checkDroneRechargeTime, this);
    // boost::thread t(&RechargeTimeMonitor::checkDroneRechargeTime, this);
}

void RechargeTimeMonitor::checkDroneRechargeTime() {

    while (true) {
        spdlog::info("Checking drone recharge time");
        pqxx::work W(db.getConnection());

        // Get drones that are charging
        getChargingDrones(W);

        // Get drones that are charged
        getChargedDrones(W);

        // Check if drones are charging for the right amount of time
        for (const auto &drone : drone_recharge_time) {
            int drone_id = drone.first;
            int start_tick = drone.second.first;
            int end_tick = drone.second.second;

            if (end_tick == -1) {
                spdlog::warn("Drone {} is still charging", drone_id);
            } else {
                int delta_time = end_tick - start_tick;
                if (delta_time >= 3214 && delta_time <= 4821) {
                    spdlog::info("Drone {} has been charging for {} minutes", drone_id, (delta_time * 2.24) / 60);
                } else {
                    spdlog::warn("Drone {} has been charging for {} minutes...wrong amount of time", drone_id, (delta_time * 2.24) / 60);
                }
            }
        }

        // Sleep for 20 seconds
        boost::this_thread::sleep_for(boost::chrono::seconds(20));
    }
}

void RechargeTimeMonitor::getChargingDrones(pqxx::work& W) {
    // Get drones that are charging
    pqxx::result r = W.exec("SELECT drone_id, tick_n FROM drone_logs WHERE status = 'CHARGING'");
    for (const auto &row : r) {
        int drone_id = row[0].as<int>();
        int tick_n = row[1].as<int>();

        // Check if drone is in the map
        if (drone_recharge_time.find(drone_id) == drone_recharge_time.end()) {
            drone_recharge_time[drone_id] = std::make_pair(tick_n, -1);
        }
    }
}

void RechargeTimeMonitor::getChargedDrones(pqxx::work& W) {
    // Get drones that are charged
    pqxx::result r = W.exec("SELECT drone_id, tick_n FROM drone_logs WHERE status = 'CHARGE_COMPLETE'");

    for (const auto &row : r) {
        int drone_id = row[0].as<int>();
        int tick_n = row[1].as<int>();

        // Check if drone is in the map
        if (drone_recharge_time.find(drone_id) != drone_recharge_time.end()) {
            drone_recharge_time[drone_id].second = tick_n;
        } else {
            spdlog::error("Drone {} is not charging", drone_id);
        }
    }
}