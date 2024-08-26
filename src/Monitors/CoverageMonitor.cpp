/* checkCoverage(): Thread function for both the zone and area coverage monitors.
 *
 * checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 *
 * checkAreaCoverage(): This monitor checks that all the zones of the 6x6 Km area are verified for every given tick.
 */
#include "Monitor.h"

/**
 * \brief Main function to monitor zone and area coverage.
 *
 * This function runs in a loop, periodically calling checkZoneVerification() and checkAreaCoverage().
 * It sleeps for 20 seconds between each iteration.
 */
void CoverageMonitor::checkCoverage()
{
    spdlog::info("COVERAGE-MONITOR: Initiated...");

    pqxx::nontransaction N(db.getConnection());

    // TODO: Use better condition
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
    spdlog::info("COVERAGE-MONITOR: Checking zone verification...");

    if (auto failed_zones = getZoneVerification(N); failed_zones.empty())
    {
        spdlog::info("COVERAGE-MONITOR: No zone failed until tick {}", tick_last_read); // TODO: Better english pls
    } else
    {
        for (const auto &zone : failed_zones)
        {
            int zone_id = zone[0];
            int tick_n = zone[1];
            int drone_id = zone[2];

            // Enqueue failed checks
            failed_ticks.insert(tick_n);

            spdlog::warn("COVERAGE-MONITOR: Zone {} was not verified by drone {} at tick {}", zone_id, drone_id, tick_n);
        }
    }
}

void CoverageMonitor::checkAreaCoverage()
{
    spdlog::info("COVERAGE-MONITOR: Checking area coverage...");

    // Print list of failed ticks
    if (!failed_ticks.empty())
    {
        std::string f;
        for (const auto &tick : failed_ticks)
        {
            f += std::to_string(tick) + ", ";
        }
        f.resize(f.size() - 2);
        spdlog::warn("COVERAGE-MONITOR: Failed ticks: {}", f);

        failed_ticks.clear();
    } else {
        spdlog::info("COVERAGE-MONITOR: All zones were verified until tick {}", tick_last_read);
    }
}

std::vector<std::array<int,3>>CoverageMonitor::getZoneVerification(pqxx::nontransaction &N)
{
    // Get the zones that were verified
    auto r = N.exec("SELECT zone, tick_n, drone_id FROM drone_logs "
               "WHERE status = 'WORKING' AND checked = FALSE AND tick_n > " + std::to_string(last_tick) +
               " ORDER BY tick_n DESC");

    if (!r.empty())
    {
        last_tick = r[0][1].as<int>();
        // spdlog::info("COVERAGE-MONITOR: Last tick checked: {}", last_tick);
        std::vector<std::array<int,3>> zones_not_verified; // zone_id, tick_n, drone_id
        for (const auto &row : r)
        {
            zones_not_verified.emplace_back(std::array{row[0].as<int>(), row[1].as<int>(), row[2].as<int>()});
        }
        return zones_not_verified;
    }
    return {};
}


