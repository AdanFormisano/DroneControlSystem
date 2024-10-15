#include <iostream>

#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <fstream>

void DroneChargeMonitor::checkDroneCharge() {
    log_charge("Initiated...");
    // std::cout << "[Monitor-DC] Initiated..." << std::endl;

    std::unordered_set<DroneData, DroneDataHash> based_drones;
    std::unordered_set<DroneData, DroneDataHash> dead_drones;

    while (tick_last_read < sim_duration_ticks - 5) {
        try {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            pqxx::work W(db.getConnection());

            auto max_tick_result = W.exec("SELECT MAX(tick_n) AS max_tick_in_db FROM drone_logs;");
            tick_last_read = max_tick_result[0]["max_tick_in_db"].as<int>();

            auto crt_based_drones = getBasedDrones(W);
            auto crt_dead_drones = getDeadDrones(W);

            // Add drones to based_drones vector if they have wrong charge
            based_drones.merge(crt_based_drones);
            checkBasedDrones(based_drones, W);

            // Check charge for drones that died before reaching the base
            dead_drones.merge(crt_dead_drones);
            checkDeadDrones(dead_drones, W);

            W.commit();
        } catch (...) {
            log_error("DroneChargeMonitor", "Error occurred");
            // std::cerr << "[Monitor-DC] Error occurred" << std::endl;
        }
    }
}

void DroneChargeMonitor::checkBasedDrones(const std::unordered_set<DroneData, DroneDataHash> &based_drones, pqxx::work &W) {
    if (!based_drones.empty() && write_based_drones) {
        std::string query = "INSERT INTO drone_charge_logs (drone_id, consumption_factor, arrived_at_base) VALUES ";
        for (const auto &[drone_id, consumption_factor, arrived_at_base] : based_drones) {
            log_charge("Drone " + std::to_string(drone_id) + " has wrong consumption " + std::to_string(consumption_factor));
            // std::cout << "[Monitor-DC] Drone " << drone_id << " has wrong consumption " << consumption_factor << std::endl;
            query += "(" + std::to_string(drone_id) + ", " + std::to_string(consumption_factor) + ", " +
                     std::to_string(arrived_at_base) + "), ";
        }
        query = query.substr(0, query.size() - 2);

        W.exec(query);

        write_based_drones = false;

        // Remove from based_drones the drones that were written to the DB
        // for (auto drone_id : failed_drones)
        // {
        //     based_drones.erase({drone_id, 0.0f, true});
        // }
    } else {
        log_charge("No abnormal consumption of based drones until tick " + std::to_string(tick_last_read));
        // std::cout << "[Monitor-DC] No abnormal consumption of based drones until tick " << tick_last_read << std::endl;
    }
}

void DroneChargeMonitor::checkDeadDrones(const std::unordered_set<DroneData, DroneDataHash> &dead_drones, pqxx::work &W) {
    if (!dead_drones.empty() && write_dead_drones) {
        std::string query = "INSERT INTO drone_charge_logs (drone_id, consumption_factor, arrived_at_base) VALUES ";
        for (const auto &[drone_id, consumption_factor, arrived_at_base] : dead_drones) {
            log_charge("DEAD Drone " + std::to_string(drone_id) + " has wrong consumption " + std::to_string(consumption_factor));
            // std::cout << "[Monitor-DC] DEAD Drone " << drone_id << " has wrong consumption " << consumption_factor << std::endl;

            query += "(" + std::to_string(drone_id) + ", " + std::to_string(consumption_factor) + ", " +
                     std::to_string(arrived_at_base) + "), ";
        }
        query = query.substr(0, query.size() - 2);
        query += " ON CONFLICT DO NOTHING;";

        // auto test_q = "TEST QUERY";

        // std::cout << "QUERY: " << query << std::endl;

        W.exec(query);

        write_dead_drones = false;
    } else {
        log_charge("No abnormal consumption of DEAD drones until tick " + std::to_string(tick_last_read));
        // std::cout << "[Monitor-DC] No abnormal consumption of DEAD drones until tick " << tick_last_read << std::endl;
    }
}

std::unordered_set<DroneChargeMonitor::DroneData, DroneChargeMonitor::DroneDataHash> DroneChargeMonitor::getBasedDrones(pqxx::work &W) {
    std::unordered_set<DroneData, DroneDataHash> based_drones;

    // A based drone is a drone with CHARGING status. We need to check if the value he has is the same as the expected one

    auto r = W.exec(
        "SELECT "
        "fal.first_tick_alive, "
        "lta.last_tick_alive, "
        "fal.drone_id, "
        "lta.wave, "
        "lta.charge, "
        "lta.last_tick_alive - fal.first_tick_alive AS ticks_alive "
        "FROM ("
        "SELECT DISTINCT ON (drone_id) drone_id, tick_n AS first_tick_alive "
        "FROM drone_logs "
        "WHERE status = 'TO_STARTING_LINE' "
        "ORDER BY drone_id, tick_n ASC "
        ") fal "
        "JOIN ("
        "SELECT tick_n AS last_tick_alive, drone_id, wave, charge "
        "FROM drone_logs "
        "WHERE status = 'CHARGING'"
        ") lta "
        "ON fal.drone_id = lta.drone_id;");

    for (const auto &row : r) {
        const int drone_id = row["drone_id"].as<int>();
        const auto charge = row["charge"].as<float>();
        const int ticks_alive = row["ticks_alive"].as<int>();
        const float optimal_chg = 100.0f - (ticks_alive * DRONE_CONSUMPTION_RATE);

        // Calculate how much charge was used per tick
        const float used_per_tick = (100.0f - charge) / ticks_alive;
        DroneData tmp_data = {drone_id, used_per_tick, true};

        if (charge < optimal_chg && !failed_drones.contains(drone_id)) {
            // based_drones.push_back({drone_id, used_per_tick, true});
            based_drones.insert(tmp_data);

            // std::cerr << "[Monitor-DC] Drone " << drone_id << " has wrong charge: " << charge
            //     << " instead of " << optimal_chg << " [" << used_per_tick << "]" << std::endl;

            write_based_drones = true;
            failed_drones.insert(drone_id);
        }
    }

    return based_drones;
}

std::unordered_set<DroneChargeMonitor::DroneData, DroneChargeMonitor::DroneDataHash> DroneChargeMonitor::getDeadDrones(pqxx::work &W) {
    std::unordered_set<DroneData, DroneDataHash> dead_drones;

    std::string query = R"(
SELECT
    fal.first_tick_alive,
    lta.last_tick_alive,
    fal.drone_id,
    lta.wave,
    lta.charge,
    lta.last_tick_alive - fal.first_tick_alive AS ticks_alive
FROM (
    SELECT DISTINCT ON (drone_id) drone_id, tick_n AS first_tick_alive
    FROM drone_logs
    WHERE status = 'TO_STARTING_LINE'
    ORDER BY drone_id, tick_n ASC
) fal
JOIN (
    SELECT tick_n AS last_tick_alive, drone_id, wave, charge
    FROM drone_logs
    WHERE status = 'DEAD'
) lta
ON fal.drone_id = lta.drone_id;
)";

    try {
        auto r = W.exec(query);

        for (const auto &row : r) {
            const int drone_id = row["drone_id"].as<int>();
            const auto charge = row["charge"].as<float>();
            const int ticks_alive = row["ticks_alive"].as<int>();
            const float optimal_chg = 100.0f - (ticks_alive * DRONE_CONSUMPTION_RATE);

            // Calculate how much charge was used per tick
            const float used_per_tick = (100.0f - charge) / ticks_alive;

            DroneData tmp_data = {drone_id, used_per_tick, false};

            if (charge < optimal_chg && !failed_drones.contains(drone_id)) {
                // dead_drones.push_back({drone_id, used_per_tick, false});
                dead_drones.insert(tmp_data);

                // std::cerr << "[Monitor-DC] DEAD Drone " << drone_id << " has wrong charge: " << charge
                //     << " instead of " << optimal_chg << " [" << used_per_tick << "]" << std::endl;

                write_dead_drones = true;
                failed_drones.insert(drone_id);
            }
        }
    } catch (...) {
        log_error("DroneChargeMonitor", "Error executing query");
        // std::cerr << "[Monitor-DC] Error executing query" << std::endl;
    }

    return dead_drones;
}
