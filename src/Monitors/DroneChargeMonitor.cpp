#include <iostream>

#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <fstream>
#include <nlohmann/json.hpp>

void DroneChargeMonitor::checkDroneCharge()
{
    // std::cout << "[Monitor-DC] Initiated..." << std::endl;
    log_charge("Initiated...");

    while (sim_running)
    {
        try
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            // auto crt_based_drones = getBasedDrones(W);
            // auto crt_dead_drones = getDeadDrones(W);

            // Check if any new drone has been created or has finished (DEAD or CHARGING)
            getDroneData();

            if (!data_ready.empty())
            {
                ElaborateData();
                data_ready.clear();
            }

            latest_processed_time = temp_time;
            log_charge(
                "Latest processed time: " + latest_processed_time + " - Old processed time: " + old_processed_time);

            // Update old_processed_time after the first successful data read
            if (latest_processed_time != "00:00:00")
            {
                CheckSimulationEnd();
                old_processed_time = latest_processed_time;
            }
        }
        catch (const std::exception& e)
        {
            // std::cerr << "[Monitor-DC] Error occurred: " << e.what() << std::endl;
            log_error("DroneChargeMonitor", e.what());
            log_charge("Error occurred in DroneChargeMonitor::checkDroneCharge's try block");
        }
    }
    //std::cout << "[Monitor-DC] Finished" << std::endl;
    log_charge("Finished");
}

void DroneChargeMonitor::ElaborateData()
{
    bool write = false;
    std::string q = "INSERT INTO drone_charge_logs (drone_id, consumption, consumption_factor, arrived_at_base) VALUES ";
    // Elaborate data
    for (const auto& drone_id : data_ready)
    {
        auto& drone_data = drones_data[drone_id];
        const int ticks_alive = drone_data.last_tick - drone_data.first_tick;
        const float optimal_chg = 100.0f - (ticks_alive * DRONE_CONSUMPTION_RATE);
        const float used_per_tick = (100.0f - drone_data.final_charge) / ticks_alive;
        const auto consumption_factor = used_per_tick / DRONE_CONSUMPTION_RATE;

        if (used_per_tick > 0.1347)
        {
            log_charge("Drone " + std::to_string(drone_id) + " has wrong consumption " + std::to_string(used_per_tick)
                + " - " + std::to_string(consumption_factor) + " with final charge: " + std::to_string(drone_data.final_charge)
                + " OPT: " + std::to_string(optimal_chg) + " in " + std::to_string(ticks_alive) + " ticks [" + std::to_string(
                    drone_data.first_tick) + " - " + std::to_string(drone_data.last_tick) + "]");

            q += "(" + std::to_string(drone_id) + ", " + std::to_string(used_per_tick) + ", " + std::to_string(
                consumption_factor) + ",";

            if (drone_data.arrived_at_base)
            {
                q += "true), ";
            }
            else
            {
                q += "false), ";
            }
            write = true;
        }
    }

    if (write)
    {
        pqxx::work W(db.getConnection());
        q.pop_back(); q.pop_back();
        q += ";";
        W.exec(q);
        W.commit();
    }
}

void DroneChargeMonitor::getDroneData()
{
    pqxx::work W(db.getConnection());

    std::string q_new_drones = R"(
SELECT tick_n, drone_id, created_at
FROM (
    SELECT tick_n, drone_id, created_at,
           ROW_NUMBER() OVER (PARTITION BY drone_id ORDER BY tick_n ASC, created_at ASC) AS rn
    FROM drone_logs
    WHERE created_at > $1
) AS ranked_logs
WHERE rn = 1;
)";

    auto R = W.exec_params(q_new_drones, latest_processed_time);
    for (const auto& row : R)
    {
        auto drone_id = row["drone_id"].as<int>();
        auto tick_start = row["tick_n"].as<int>();
        auto read_time = row["created_at"].as<std::string>();

        if (!drones_data.contains(drone_id))
        {
            drones_data[drone_id].first_tick = tick_start;
        }

        if (temp_time < read_time)
        {
            temp_time = read_time;
        }
    }

    std::string q_ended_drones = R"(
SELECT tick_n, drone_id, charge, created_at,
       CASE WHEN status = 'CHARGING' THEN true ELSE false END AS arrived_at_base
FROM (
    SELECT tick_n, drone_id, charge, status,created_at,
           ROW_NUMBER() OVER (PARTITION BY drone_id ORDER BY tick_n ASC, created_at ASC) AS rn
    FROM drone_logs
    WHERE created_at > $1
      AND (status = 'CHARGING' OR status = 'DEAD')
) AS ranked_logs
WHERE rn = 1;
)";

    auto S = W.exec_params(q_ended_drones, latest_processed_time);
    for (const auto& row : S)
    {
        auto drone_id = row["drone_id"].as<int>();
        auto tick_end = row["tick_n"].as<int>();
        auto final_charge = row["charge"].as<float>();
        auto arrived_at_base = row["arrived_at_base"].as<bool>();
        auto read_time = row["created_at"].as<std::string>();

        // Check if the drone is in the drones_data vector
        if (drones_data.contains(drone_id))
        {
            drones_data[drone_id].last_tick = tick_end;
            drones_data[drone_id].final_charge = final_charge;
            drones_data[drone_id].arrived_at_base = arrived_at_base;
            data_ready.push_back(drone_id);
            // log_charge("Drone " + std::to_string(drone_id) + " first tick: " + std::to_string(drones_data[drone_id].first_tick)
            //     + " last tick: " + std::to_string(drones_data[drone_id].last_tick) + " final charge: " + std::to_string(
            //         drones_data[drone_id].final_charge) + " arrived at base: " + std::to_string(drones_data[drone_id].arrived_at_base));
        }
        else
        {
            log_charge("Found end of drone " + std::to_string(drone_id) + " without start");
        }

        if (temp_time < read_time)
        {
            temp_time = read_time;
        }
    }
}
