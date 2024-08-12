/* checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 */
#include "Monitor.h"

void ZoneCoverageMonitor::checkZoneVerification()
{
    // spdlog::set_pattern("[%T.%e][%^%l%$][M-ZoneCoverage] %v");
    spdlog::info("Monitor initiated...");

    pqxx::nontransaction N(db.getConnection());

    while(true)
    {
        spdlog::info("Checking zone verification");
        auto _ = getZoneVerification(N);

        for (const auto &zone : _)
        {
            int zone_id = zone[0];
            int tick_n = zone[1];
            int drone_id = zone[2];

            spdlog::warn("Zone {} was not verified by drone {} at tick {}", zone_id, drone_id, tick_n);
        }

        // Sleep for 20 seconds
        boost::this_thread::sleep_for(boost::chrono::seconds(20));
    }
}

// Get the zones that were not verified
std::vector<std::array<int,3>>ZoneCoverageMonitor::getZoneVerification(pqxx::nontransaction &N)
{
    // Get the zones that were verified
    auto r = N.exec("SELECT zone, tick_n, drone_id FROM drone_logs "
               "WHERE status = 'WORKING' AND checked = FALSE AND tick_n > " + std::to_string(last_tick) +
               " ORDER BY tick_n DESC");

    if (!r.empty())
    {
        last_tick = r[0][1].as<int>();
        spdlog::info("Last tick checked: {}", last_tick);
        std::vector<std::array<int,3>> zones_not_verified; // zone_id, tick_n, drone_id
        for (const auto &row : r)
        {
            zones_not_verified.emplace_back(std::array{row[0].as<int>(), row[1].as<int>(), row[2].as<int>()});
        }

        return zones_not_verified;
    }
    return {};
}
