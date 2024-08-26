/* checkDroneCharge(): This monitor checks that the drones' charge is correctly managed by the system. This means that
the drones are swapped when their charge is just enough to return to the base.

Monitor needs to take from Redis db the chargeToBase for each zone and use it to check if the drone's charge is enough
to go back to base as soon as the drone state changes from WORKING to TO_BASE
*/
#include "Monitor.h"

void DroneChargeMonitor::checkDroneCharge()
{
    // Sleep for 5 seconds to let the zones calculate and upload the charge_needed to Redis
    // boost::this_thread::sleep_for(boost::chrono::seconds(5));

    spdlog::info("CHARGE-MONITOR: Initiated...");
    pqxx::work W(db.getConnection());

    // Get data needed to run monitor
    getChargeNeededForZones();

    while (true)
    {
        // Get last tick read
        auto r = W.exec("SELECT tick_n FROM drone_logs ORDER BY tick_n DESC");
        last_tick = r[0][0].as<int>();

        spdlog::info("CHARGE-MONITOR: last tick read {}", last_tick);

        auto query =  "WITH StatusChange AS ("
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
            "    status = 'TO_BASE' AND prev_status = 'WORKING' AND tick_n > " + std::to_string(last_tick) +
            " ORDER BY"
            "    drone_id, tick_n;";

        r = W.exec(query);

        // Check if the drone has enough charge to go back to base
        for (const auto &row : r)
        {
            int tick = row[0].as<int>();
            int drone_id = row[1].as<int>();
            int zone = row[2].as<int>();
            auto charge = row[3].as<float>();
            auto status = row[4].as<std::string>();

            if (charge < charge_needed[zone])
            {
                spdlog::error("CHARGE MONITOR: Drone {} going to base at tick {} with {}% charge, but it needs {}% charge",
                              drone_id, tick, charge, charge_needed[zone]);
            } else
            {
                spdlog::info("CHARGE MONITOR: Drone {} ({}%) is going back to base", drone_id, charge);
            }
        }
        boost::this_thread::sleep_for(boost::chrono::seconds(20));
    }
}


void DroneChargeMonitor::getChargeNeededForZones()
{
    // For each zone get charge_needed for each drone from Redis
    for (int z = 0; z < ZONE_NUMBER; z++)
    {
        auto r = shared_redis.get("zone:" + std::to_string(z) + ":charge_needed_to_base");
        if (!r.has_value())
        {
            spdlog::error("Couldn't get charge_needed_to_base from Redis");
        } else
        {
            charge_needed[z] = std::stof(r.value());
        }
    }
}

