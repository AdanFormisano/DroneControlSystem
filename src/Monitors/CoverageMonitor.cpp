/* checkCoverage(): Thread function for both the zone and area coverage monitors.
 *
 * checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 *
 * checkAreaCoverage(): This monitor checks that all the zones of the 6x6 Km area are verified for every given tick.
 */
#include <iostream>

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
    std::this_thread::sleep_for(std::chrono::seconds(10));

    try
    {
        // TODO: Use better condition
        while (tick_last_read < sim_duration_ticks - 1)
        {
            checkWaveVerification();
            checkAreaCoverage();

            // Sleep for 20 seconds
            // boost::this_thread::sleep_for(boost::chrono::seconds(15));
            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("COVERAGE-MONITOR: {}", e.what());
    }
}

void CoverageMonitor::checkWaveVerification()
{
    spdlog::info("COVERAGE-MONITOR: Checking wave verification...");

    if (auto failed_waves = getWaveVerification(); failed_waves.empty())
    {
        spdlog::info("COVERAGE-MONITOR: No wave failed until tick {}", tick_last_read); // TODO: Better english pls
    }
    else
    {
        for (const auto& wave : failed_waves)
        {
            int wave_id = wave[0];
            int tick_n = wave[1];
            int drone_id = wave[2];

            // Enqueue failed checks
            failed_ticks.insert(tick_n);

            spdlog::warn("COVERAGE-MONITOR: Wave {} was not verified by drone {} at tick {}", wave_id, drone_id,
                         tick_n);

            std::string q = "INSERT INTO monitor_logs (tick_n, wave_cover) "
                "VALUES (" + std::to_string(tick_n) + ", ARRAY[" + std::to_string(drone_id) + "]) "
                "ON CONFLICT (tick_n) DO UPDATE SET "
                "wave_cover = array_append(monitor_logs.wave_cover, " + std::to_string(drone_id) + ");";

            WriteToDB(q);
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
        for (const auto& tick : failed_ticks)
        {
            f += std::to_string(tick) + ", ";

            std::string q = "INSERT INTO monitor_logs (tick_n, area_cover) "
                "VALUES (" + std::to_string(tick) + ", 'FAILED') "
                "ON CONFLICT (tick_n) DO UPDATE SET "
                "area_cover = 'FAILED';";
            WriteToDB(q); // This will maybe cause a lot of overhead
        }
        f.resize(f.size() - 2);
        spdlog::warn("COVERAGE-MONITOR: Failed ticks: {}", f);

        failed_ticks.clear();
    }
    else
    {
        spdlog::info("COVERAGE-MONITOR: All waves were verified until tick {}", tick_last_read);
    }
}

std::vector<std::array<int, 3>> CoverageMonitor::getWaveVerification()
{
    pqxx::work txn(db.getConnection()); // Begin a transaction

    //Run the main query
    // auto r = txn.exec(
    //     "SELECT wave, tick_n, drone_id "
    //     "FROM drone_logs "
    //     "WHERE status = 'WORKING' AND checked = FALSE AND tick_n > " + std::to_string(tick_last_read) + " "
    //     "ORDER BY tick_n DESC;"
    // );

    auto r = txn.exec(
        "WITH StatusChange AS ("
        "SELECT drone_id, tick_n, status, checked, wave, "
        "LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status "
        "FROM drone_logs "
        "WHERE tick_n > " + std::to_string(tick_last_read) +
        ") "
        "SELECT * "
        "FROM StatusChange "
        "WHERE status IN ('DEAD', 'DISCONNECTED') "
        "AND prev_status = 'WORKING';"
    );

    // Get the maximum tick in the entire table
    auto max_tick_result = txn.exec("SELECT MAX(tick_n) AS max_tick_in_db FROM drone_logs;");
    // if (!max_tick_result[0]["max_tick_in_db"].is_null())
    // {
    //     tick_last_read = max_tick_result[0]["max_tick_in_db"].as<int>();
    // }

    tick_last_read = max_tick_result[0]["max_tick_in_db"].is_null()
                         ? 0
                         : max_tick_result[0]["max_tick_in_db"].as<int>();

    // Commit the transaction
    txn.commit();

    // Now you can process the results
    std::cout << "[Monitor] Max Tick in DB: " << tick_last_read << std::endl;

    if (!r.empty())
    {
        std::vector<std::array<int, 3>> wave_not_verified;
        for (const auto& row : r)
        {
            int wave = row["wave"].is_null() ? 0 : row["wave"].as<int>();
            int tick_n = row["tick_n"].is_null() ? 0 : row["tick_n"].as<int>();
            int drone_id = row["drone_id"].is_null() ? 0 : row["drone_id"].as<int>();

            wave_not_verified.emplace_back(std::array{wave, tick_n, drone_id});

            std::cout << "Wave: " << wave << ", Tick: " << tick_n << ", Drone ID: " << drone_id << std::endl;
        }
        return wave_not_verified;
    }
    return {};
}
