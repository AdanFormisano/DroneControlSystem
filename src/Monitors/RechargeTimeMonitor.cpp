#include "Monitor.h"

void RechargeTimeMonitor::checkDroneRechargeTime() {
    spdlog::set_pattern("[%T.%e][%^%l%$][M-RechargeTime] %v");
    spdlog::info("Monitor initiated...");

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
