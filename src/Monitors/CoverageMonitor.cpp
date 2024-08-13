/* checkCoverage(): Thread function for both the zone and area coverage monitors.
 *
 * checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 *
 * checkAreaCoverage(): This monitor checks that all the zones of the 6x6 Km area are verified.
 */
#include "Monitor.h"

void CoverageMonitor::checkCoverage()
{
    // spdlog::set_pattern("[%T.%e][%^%l%$][M-ZoneCoverage] %v");
    spdlog::info("Coverage monitors initiated...");

    pqxx::nontransaction N(db.getConnection());

    while(true)
    {
        checkZoneVerification(N);
        checkAreaCoverage();

        // Sleep for 20 seconds
        boost::this_thread::sleep_for(boost::chrono::seconds(20));
    }
}

void CoverageMonitor::checkZoneVerification(pqxx::nontransaction &N)
{
    spdlog::info("Checking zone verification");
    auto _ = getZoneVerification(N);

    for (const auto &zone : _)
    {
        int zone_id = zone[0];
        int tick_n = zone[1];
        int drone_id = zone[2];

        // Enque failed checks
        failed_ticks.insert(tick_n);

        spdlog::warn("Zone {} was not verified by drone {} at tick {}", zone_id, drone_id, tick_n);
    }
}

void CoverageMonitor::checkAreaCoverage()
{
    spdlog::info("Checking area coverage");

    // Print list of failed ticks
    if (!failed_ticks.empty())
    {
        std::string f;
        for (const auto &tick : failed_ticks)
        {
            f += std::to_string(tick) + ", ";
        }
        f.resize(f.size() - 2);
        spdlog::warn("Failed ticks: {}", f);

        failed_ticks.clear();
    } else {
        spdlog::info("All zones were verified");
    }
}

// Get the zones that were not verified
std::vector<std::array<int,3>>CoverageMonitor::getZoneVerification(pqxx::nontransaction &N)
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


