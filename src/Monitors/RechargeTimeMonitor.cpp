/* checkDroneRechargeTime(): This monitor checks that the number of ticks that a drone has been charging is between [2,3] hours.
 *
 * The monitor reads the DB looking for new drone in 'CHARGING' state saving the tick in drone_recharge_time[tick_n][0].
 * Once the monitor finds a drone with the 'CHARGE_COMPLETE' state, it checks if delta_time >= 3214 && delta_time <= 4821,
 * where:
 * - delta_time = end_tick - start_tick and
 * - 2975 = number of ticks for 2 hours
 * - 4463 = number of ticks for 3 hours
 */

#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include "spdlog/spdlog.h"
#include "sw/redis++/redis++.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <sstream>

using namespace sw::redis;

// std::ofstream recharge_log("recharge_monitor.log");

void RechargeTimeMonitor::checkDroneRechargeTime() {
    log_recharge("Initiated...");
    // spdlog::info("RECHARGE-MONITOR: Initiated...");
    std::this_thread::sleep_for(std::chrono::seconds(10));

    try {
        while (true) {
            log_recharge("Checking drone recharge time...");
            // spdlog::info("RECHARGE-MONITOR: Checking drone recharge time...");
            pqxx::work W(db.getConnection());

            // Get drones that are charging
            getChargingDrones(W);

            // Get drones that are charged
            getChargedDrones(W);

            // Check if drones are charging for the right amount of time
            for (const auto &drone : drone_recharge_time) {
                int drone_id = drone.first;
                const int start_tick = drone.second.first;

                if (const int end_tick = drone.second.second; end_tick == -1) {
                    log_recharge("Drone " + std::to_string(drone_id) + " is still charging");
                    // spdlog::warn("RECHARGE-MONITOR: Drone {} is still charging", drone_id);
                } else {
                    if (const int delta_time = end_tick - start_tick; delta_time >= 2975 && delta_time <= 4463) {
                        log_recharge("Drone " + std::to_string(drone_id) + " has charged for " +
                                     std::to_string((static_cast<float>(delta_time) * TICK_TIME_SIMULATED) / 60) + " minutes");
                        // spdlog::info("RECHARGE-MONITOR: Drone {} has charged for {} minutes", drone_id, (static_cast<float>(delta_time) * TICK_TIME_SIMULATED) / 60);
                    } else {
                        log_recharge("Drone " + std::to_string(drone_id) + " has charged for " +
                                     std::to_string((static_cast<float>(delta_time) * TICK_TIME_SIMULATED) / 60) + " minutes...wrong amount of time");
                        // spdlog::warn("RECHARGE-MONITOR: Drone {} has charged for {} minutes...wrong amount of time", drone_id, (static_cast<float>(delta_time) * TICK_TIME_SIMULATED) / 60);

                        std::string q = "INSERT INTO monitor_logs (tick_n, recharge_drone_id, recharge_duration) "
                                        "VALUES (" +
                                        std::to_string(end_tick) + ", ARRAY[" + std::to_string(drone_id) + "], ARRAY[" +
                                        std::to_string(delta_time) + "]) "
                                                                     "ON CONFLICT (tick_n) DO UPDATE SET "
                                                                     "recharge_drone_id = array_append(monitor_logs.recharge_drone_id, " +
                                        std::to_string(drone_id) + "), "
                                                                   "recharge_duration = array_append(monitor_logs.recharge_duration, " +
                                        std::to_string(delta_time) + ");";

                        W.exec(q);
                    }
                }
            }
            W.commit();

            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    } catch (const std::exception &e) {
        log_error("RechargeTimeMonitor", e.what());
        log_recharge("Error: " + std::string(e.what()));
        // spdlog::error("RECHARGE-MONITOR: {}", e.what());
    }
}

void RechargeTimeMonitor::getChargingDrones(pqxx::work &W) {
    pqxx::result r = W.exec("SELECT drone_id, tick_n FROM drone_logs "
                            "WHERE status = 'CHARGING' AND tick_n > " +
                            std::to_string(tick_last_read) +
                            " ORDER BY tick_n DESC");

    if (r.empty()) {
        log_recharge("No new drones charging");
        spdlog::info("RECHARGE-MONITOR: No new drones charging");
    } else {
        tick_last_read = r[0][1].as<int>(); // Update last read tick from DB

        for (const auto &row : r) {
            int drone_id = row[0].as<int>();
            int tick_n = row[1].as<int>();
            if (!drone_recharge_time.contains(drone_id)) {
                drone_recharge_time[drone_id] = std::make_pair(tick_n, -1);
            }
        }
    }
}

void RechargeTimeMonitor::getChargedDrones(pqxx::work &W) {
    pqxx::result r = W.exec("SELECT drone_id, tick_n FROM drone_logs "
                            "WHERE status = 'CHARGE_COMPLETE' AND tick_n > " +
                            std::to_string(tick_last_read) +
                            " ORDER BY tick_n DESC");

    if (r.empty()) {
        log_recharge("No drones are done charging");
        spdlog::info("RECHARGE-MONITOR: No drones are done charging");
    } else {
        tick_last_read = r[0][1].as<int>();

        for (const auto &row : r) {
            int drone_id = row[0].as<int>();
            int tick_n = row[1].as<int>();

            if (drone_recharge_time.contains(drone_id)) {
                drone_recharge_time[drone_id].second = tick_n;
            } else {
                log_recharge("Drone " + std::to_string(drone_id) + " is not charging");
                // spdlog::error("RECHARGE-MONITOR: Drone {} is not charging", drone_id);
            }
        }
    }
}
