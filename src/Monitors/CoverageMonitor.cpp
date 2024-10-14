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
#include "../../utils/utils.h"

/**
 * \brief Main function to monitor zone and area coverage.
 *
 * This function runs in a loop, periodically calling checkZoneVerification() and checkAreaCoverage().
 * It sleeps for 20 seconds between each iteration.
 */
void CoverageMonitor::checkCoverage()
{
    // spdlog::info("COVERAGE-MONITOR: Initiated...");
    std::cout << "[Monitor-CV] Initiated..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(10));

    try
    {
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
        std::cerr << "[Monitor-CV] " << e.what() << std::endl;
    }
}

void CoverageMonitor::checkWaveVerification()
{
    std::cout << "[Monitor-CV] Checking wave verification..." << std::endl;

    if (auto failed_waves = getWaveVerification(); failed_waves.empty())
    {
        std::cout << "[Monitor-CV] All waves were verified until tick " << tick_last_read << std::endl;
    }
    else
    {
        for (const auto& wave : failed_waves)
        {
            int wave_id = wave.wave_id;
            int tick_n = wave.tick_n;
            int drone_id = wave.drone_id;

            // Enqueue failed checks
            failed_ticks.insert(tick_n);

            // spdlog::warn("COVERAGE-MONITOR: Wave {} was not verified by drone {} at tick {}", wave_id, drone_id,
            //              tick_n);

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
    // spdlog::info("COVERAGE-MONITOR: Checking area coverage...");
    std::cout << "[Monitor-CV] Checking area coverage..." << std::endl;

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
        // spdlog::warn("COVERAGE-MONITOR: Failed ticks: {}", f);

        failed_ticks.clear();
    }
    else
    {
        // spdlog::info("COVERAGE-MONITOR: All waves were verified until tick {}", tick_last_read);
        std::cout << "[Monitor-CV] All waves were verified until tick " << tick_last_read << std::endl;
    }
}

std::vector<CoverageMonitor::WaveVerification> CoverageMonitor::getWaveVerification()
{
    pqxx::work txn(db.getConnection()); // Begin a transaction
    std::cout << "[Monitor-CV] Getting wave verification" << std::endl;
    //Run the main query
    // auto r = txn.exec(
    //     "WITH StatusChange AS ("
    //     "SELECT drone_id, tick_n, status, checked, wave, "
    //     "LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status "
    //     "FROM drone_logs "
    //     "WHERE tick_n > " + std::to_string(tick_last_read) +
    //     ") "
    //     "SELECT * "
    //     "FROM StatusChange "
    //     "WHERE status IN ('DEAD', 'DISCONNECTED') "
    //     "AND prev_status = 'WORKING';"
    // );

    auto work_disconnect_reconnect = txn.exec(
        "WITH StatusChange AS ("
        "SELECT drone_id, tick_n, status, wave, "
        "LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status "
        "FROM drone_logs "
        "WHERE tick_n > " + std::to_string(tick_last_read) +
        "), "
        "TransitionToDisconnected AS ("
        "SELECT drone_id, MIN(tick_n) AS first_disconnected_tick "
        "FROM StatusChange "
        "WHERE status = 'DISCONNECTED' "
        "AND prev_status = 'WORKING' "
        "GROUP BY drone_id"
        "), "
        "TransitionToReconnected AS ("
        "SELECT drone_id, tick_n AS reconnect_tick, wave "
        "FROM StatusChange "
        "WHERE status = 'RECONNECTED' "
        "AND prev_status = 'DISCONNECTED' "
        "AND drone_id IN (SELECT drone_id FROM TransitionToDisconnected)"
        ") "
        "SELECT tr.wave, tr.drone_id, td.first_disconnected_tick, tr.reconnect_tick "
        "FROM TransitionToReconnected tr "
        "JOIN TransitionToDisconnected td "
        "ON tr.drone_id = td.drone_id;"
    );
    std::cout << "[Monitor-CV] Got work_disconnect_reconnect" << std::endl;


    auto work_disconnect_dead = txn.exec(
        "WITH StatusChange AS ("
        "SELECT drone_id, tick_n, status, wave, "
        "LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status "
        "FROM drone_logs "
        "WHERE tick_n > " + std::to_string(tick_last_read) +
        "), "
        "TransitionToWorking AS ("
        "SELECT drone_id, MIN(tick_n) AS first_working_tick, wave "
        "FROM StatusChange "
        "WHERE status = 'WORKING' "
        "AND prev_status = 'READY' "
        "GROUP BY drone_id, wave"
        "), "
        "TransitionToDisconnected AS ("
        "SELECT drone_id, MIN(tick_n) AS first_disconnected_tick "
        "FROM StatusChange "
        "WHERE status = 'DISCONNECTED' "
        "AND prev_status = 'WORKING' "
        "GROUP BY drone_id"
        "), "
        "TransitionToDead AS ("
        "SELECT drone_id, wave, MIN(tick_n) AS dead_tick "
        "FROM StatusChange "
        "WHERE status = 'DEAD' "
        "AND prev_status = 'DISCONNECTED' "
        "AND drone_id IN (SELECT drone_id FROM TransitionToDisconnected) "
        "GROUP BY drone_id, wave"
        ") "
        "SELECT tw.wave, tw.drone_id, tw.first_working_tick, tdd.first_disconnected_tick "
        "FROM TransitionToWorking tw "
        "JOIN TransitionToDead td "
        "ON tw.drone_id = td.drone_id "
        "JOIN TransitionToDisconnected tdd "
        "ON tdd.drone_id = tw.drone_id;"
    );
    std::cout << "[Monitor-CV] Got work_disconnect_dead" << std::endl;


    std::string q_working_dead = R"(
WITH StatusChange AS (
    SELECT
        drone_id,
        tick_n,
        status,
        wave,
        LAG(status) OVER (PARTITION BY drone_id ORDER BY tick_n) AS prev_status
    FROM
        drone_logs
    WHERE
        tick_n > 0
),
TransitionToWorking AS (
    SELECT
        drone_id,
        MIN(tick_n) AS first_working_tick,
        wave
    FROM
        StatusChange
    WHERE
        status = 'WORKING'
        AND prev_status = 'READY'
    GROUP BY
        drone_id,
        wave
),
TransitionToDead AS (
    SELECT
        sc.drone_id,
        sc.tick_n AS dead_tick,
        sc.wave
    FROM
        StatusChange sc
    WHERE
        sc.status = 'DEAD'
        AND sc.prev_status = 'WORKING'
        AND EXISTS (SELECT 1 FROM TransitionToWorking tw WHERE tw.drone_id = sc.drone_id)
)
SELECT
    tw.wave,
    tw.drone_id,
    tw.first_working_tick,
    td.dead_tick
FROM
    TransitionToWorking tw
JOIN
    TransitionToDead td
ON
    tw.drone_id = td.drone_id;
)";


    auto work_dead = txn.exec(q_working_dead);
    std::cout << "[Monitor-CV] Got work_dead" << std::endl;


    // Get the maximum tick in the entire table
    auto max_tick_result = txn.exec("SELECT MAX(tick_n) AS max_tick_in_db FROM drone_logs;");
    // if (!max_tick_result[0]["max_tick_in_db"].is_null())
    // {
    //     tick_last_read = max_tick_result[0]["max_tick_in_db"].as<int>();
    // }

    std::cout << "[Monitor-CV] Got latest tick" << std::endl;

    tick_last_read = max_tick_result[0]["max_tick_in_db"].is_null()
                         ? 0
                         : max_tick_result[0]["max_tick_in_db"].as<int>();

    // Commit the transaction
    std::cout << "[Monitor-CV] About to commit queries" << std::endl;
    txn.commit();
    std::cout << "[Monitor-CV] Commited queries" << std::endl;

    // Now you can process the results
    std::cout << "[Monitor-CV] Max Tick in DB: " << tick_last_read << std::endl;

    std::vector<WaveVerification> wave_not_verified;

    if (!work_disconnect_reconnect.empty())
    {
        for (const auto& row : work_disconnect_reconnect)
        {
            const int drone_id = row["drone_id"].as<int>();
            const int wave_id = row["wave"].as<int>();
            const int first_disconnected_tick = row["first_disconnected_tick"].as<int>();
            const int reconnect_tick = row["reconnect_tick"].as<int>();

            for (int i = first_disconnected_tick; i <= reconnect_tick; ++i)
            {
                WaveVerification wv = {wave_id, i, drone_id};
                wave_not_verified.push_back(wv);
            }
        }
    }

    if (!work_disconnect_dead.empty())
    {
        for (const auto& row : work_disconnect_dead)
        {
            const int drone_id = row["drone_id"].as<int>();
            const int wave_id = row["wave"].as<int>();
            const int first_working_tick = row["first_working_tick"].as<int>();
            const int first_disconnected_tick = row["first_disconnected_tick"].as<int>();
            const int last_working_tick = first_working_tick + 300;

            for (int i = first_disconnected_tick; i < last_working_tick; i++)
            {
                WaveVerification wv = {wave_id, i, drone_id};
                wave_not_verified.push_back(wv);
            }
        }
    }

    if (!work_dead.empty())
    {
        for (const auto& row : work_dead)
        {
            const int drone_id = row["drone_id"].as<int>();
            const int wave_id = row["wave"].as<int>();
            const int first_working_tick = row["first_working_tick"].as<int>();
            const int dead_tick = row["dead_tick"].as<int>();
            const int last_working_tick = first_working_tick + 300;

            for (int i = dead_tick; i < last_working_tick; ++i)
            {
                WaveVerification wv = {wave_id, i, drone_id};
                wave_not_verified.push_back(wv);
            }
        }
    }

    return wave_not_verified;
}
