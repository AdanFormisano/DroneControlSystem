/* checkDroneCharge(): This monitor checks that the drones' charge is correctly managed by the system. This means that
the drones are swapped when their charge is just enough to return to the base.

Monitor needs to take from Redis db the chargeToBase for each zone and use it to check if the drone's charge is enough
to go back to base as soon as the drone state changes from WORKING to TO_BASE
*/
#include "Monitor.h"
#include <fstream>

std::ofstream charge_log("charge_monitor.log");

void DroneChargeMonitor::checkDroneCharge() {
    try {
        // Sleep for 5 seconds to let the zones calculate and upload the charge_needed to Redis
        charge_log << "CHARGE-MONITOR: Initiated..." << std::endl;
        spdlog::info("CHARGE-MONITOR: Initiated...");
        // boost::this_thread::sleep_for(boost::chrono::seconds(10));
        std::this_thread::sleep_for(std::chrono::seconds(10));

        // Get data needed to run monitor
        getChargeNeededForZones();

        while (true) {
            // Get last tick read
            pqxx::work W(db.getConnection());
            auto r = W.exec("SELECT tick_n FROM drone_logs ORDER BY tick_n DESC");
            last_tick = r[0][0].as<int>();

            charge_log << "CHARGE-MONITOR: last tick read " << last_tick << std::endl;
            spdlog::info("CHARGE-MONITOR: last tick read {}", last_tick);

            auto query = "WITH StatusChange AS ("
                         "    SELECT"
                         "        tick_n,"
                         "        drone_id,"
                         "        zone,"
                         "        charge,"
                         "        status,"
                         "        LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status"
                         "    FROM"
                         "        drone_logs"
                         ")"
                         "SELECT"
                         "    tick_n,"
                         "    drone_id,"
                         "    zone,"
                         "    charge,"
                         "    status "
                         "FROM"
                         "    StatusChange "
                         "WHERE"
                         "    status = 'TO_BASE' AND prev_status = 'WORKING' AND tick_n > " +
                         std::to_string(last_tick) +
                         " ORDER BY"
                         "    drone_id, tick_n;";

            r = W.exec(query);

            // Check if the drone has enough charge to go back to base
            for (const auto &row : r) {
                int tick = row[0].as<int>();
                int drone_id = row[1].as<int>();
                int zone = row[2].as<int>();
                auto charge = row[3].as<float>();
                auto status = row[4].as<std::string>();

                if (charge < charge_needed[zone]) {
                    charge_log << "CHARGE MONITOR: Drone " << drone_id << " going to base at tick " << tick
                               << " with " << charge << "% charge, but it needs " << charge_needed[zone] << "% charge"
                               << std::endl;
                    spdlog::error("CHARGE MONITOR: Drone {} going to base at tick {} with {}% charge, but it needs {}% charge",
                                  drone_id, tick, charge, charge_needed[zone]);

                    // Insert into monitor_logs
                    std::string q = "INSERT INTO monitor_logs (tick_n, charge_drone_id, charge_percentage, charge_needed) "
                                    "VALUES (" +
                                    std::to_string(tick) + ", ARRAY[" + std::to_string(drone_id) + "], ARRAY[" + std::to_string(charge) + ", ARRAY[" + std::to_string(charge_needed[zone]) + "]) "
                                                                                                                                                                                             "ON CONFLICT (tick_n) DO UPDATE SET "
                                                                                                                                                                                             "charge_drone_id = array_append(monitor_logs.charge_drone_id, " +
                                    std::to_string(drone_id) + "), "
                                                               "charge_percentage = array_append(monitor_logs.charge_percentage, " +
                                    std::to_string(charge) + "), "
                                                             "charge_needed = array_append(monitor_logs.charge_needed, " +
                                    std::to_string(charge_needed[zone]) + ");";

                    // WriteToDB(q);    // Commented because it can create an error when accessing the db
                    W.exec(q);
                } else {
                    charge_log << "CHARGE MONITOR: Drone " << drone_id << " going back to base at tick " << tick << " with " << charge << "% charge" << std::endl;
                    spdlog::info("CHARGE MONITOR: Drone {} ({}%) is going back to base", drone_id, charge);
                }
            }
            W.commit();
            // boost::this_thread::sleep_for(boost::chrono::seconds(20));
            std::this_thread::sleep_for(std::chrono::seconds(20));
        }
    } catch (const std::exception &e) {
        charge_log << "CHARGE-MONITOR: " << e.what() << std::endl;
        spdlog::error("CHARGE-MONITOR: {}", e.what());
    }
}

void DroneChargeMonitor::getChargeNeededForZones() {
    // For each zone get charge_needed for each drone from Redis
    for (int z = 0; z < ZONE_NUMBER; z++) {
        if (auto r = shared_redis.get("zone:" + std::to_string(z) + ":charge_needed_to_base"); !r.has_value()) {
            charge_log << "CHARGE-MONITOR: Couldn't get charge_needed_to_base from Redis" << std::endl;
            spdlog::error("Couldn't get charge_needed_to_base from Redis");
        } else {
            charge_needed[z] = std::stof(r.value());
        }
    }
};
