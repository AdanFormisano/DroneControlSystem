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
#include "sw/redis++/redis++.h"
#include <chrono>
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <sstream>

using namespace sw::redis;

void RechargeTimeMonitor::checkDroneRechargeTime()
{
    // std::cout << "[Monitor-RC] Initiated..." << std::endl;
    log_recharge("Initiated...");

    try
    {
        while (sim_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            // std::cout << "[Monitor-RC] Checking drones recharge time..." << std::endl;
            log_recharge("Checking drone recharge time...");
            pqxx::work W(db.getConnection());

            // Get drones that are charging
            getChargingDrones(W);

            // Get drones that are charged
            getChargedDrones(W);

            // Check if drones are charging for the right amount of time
            for (const auto& drone : drone_recharge_time)
            {
                int drone_id = drone.first;
                const int start_tick = drone.second.first;
                const int end_tick = drone.second.second;

                if (end_tick != -1 && !drone_id_written.contains(drone_id))
                {
                    const int delta_time = end_tick - start_tick;
                    const float duration_minutes = (static_cast<float>(delta_time) * TICK_TIME_SIMULATED) / 60;
                    if (delta_time < 3000 || delta_time > 4500)
                    {
                        log_recharge("Drone " + std::to_string(drone_id) + " has recharged for " + std::to_string(duration_minutes) + " minutes");
                        std::string q =
                            "INSERT INTO drone_recharge_logs (drone_id, recharge_duration_ticks, recharge_duration_min, start_tick, end_tick) VALUES ("
                            + std::to_string(drone_id) + ", "
                            + std::to_string(delta_time) + ", "
                            + std::to_string(duration_minutes) + ", "
                            + std::to_string(start_tick) + ", "
                            + std::to_string(end_tick) + ");";

                        W.exec(q);
                    }
                    drone_id_written.insert(drone_id);
                }
            }
            W.commit();

            latest_processed_time = temp_time;
            log_recharge("Latest processed time: " + latest_processed_time + " - Old processed time: " + old_processed_time);

            // Update old_processed_time after the first successful data read
            if (latest_processed_time != "00:00:00")
            {
                CheckSimulationEnd();
                old_processed_time = latest_processed_time;
            }
        }

        // std::cout << "[Monitor-RC] Finished" << std::endl;
        log_recharge("Finished");
    }
    catch (const std::exception& e)
    {
        // std::cerr << "[Monitor-RC] Error: " << e.what() << std::endl;
        log_error("RechargeTimeMonitor", e.what());
        log_recharge("Error: " + std::string(e.what()));
    }
}

void RechargeTimeMonitor::getChargingDrones(pqxx::work& W)
{
    std::string q = R"(
    SELECT drone_id, tick_n, created_at FROM drone_logs
    WHERE status = 'CHARGING' AND created_at > $1;)";

    auto r = W.exec_params(q, latest_processed_time);

    if (!r.empty())
    {
        for (const auto& row : r)
        {
            int drone_id = row["drone_id"].as<int>();
            int tick_n = row["tick_n"].as<int>();
            auto read_time = row["created_at"].as<std::string>();
            if (!drone_recharge_time.contains(drone_id))
            {
                drone_recharge_time[drone_id] = std::make_pair(tick_n, -1);
            }

            if (temp_time < read_time)
            {
                temp_time = read_time;
            }
        }
    }
    else
    {
        // std::cout << "[Monitor-RC] No new drones charging" << std::endl;
    }
}

void RechargeTimeMonitor::getChargedDrones(pqxx::work& W)
{
    std::string q = R"(
    SELECT drone_id, tick_n, created_at FROM drone_logs
    WHERE status = 'CHARGING_COMPLETED' AND created_at > $1;)";

    auto r = W.exec_params(q, latest_processed_time);

    if (!r.empty())
    {
        for (const auto& row : r)
        {
            int drone_id = row["drone_id"].as<int>();
            int tick_n = row["tick_n"].as<int>();
            auto read_time = row["created_at"].as<std::string>();

            if (drone_recharge_time.contains(drone_id))
            {
                drone_recharge_time[drone_id].second = tick_n;
            }
            else
            {
                // std::cout << "[Monitor-RC] Drone " << drone_id << " is not charging" << std::endl;
                log_recharge("Drone " + std::to_string(drone_id) + " is not charging");
            }

            if (temp_time < read_time)
            {
                temp_time = read_time;
            }
        }
    }
    else
    {
        // std::cout << "[Monitor-RC] No new drone has finished recharging" << std::endl;
        // log_recharge("No drone has finished charging");
    }
}
