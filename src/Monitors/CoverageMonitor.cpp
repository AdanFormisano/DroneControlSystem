/* checkCoverage(): Thread function for both the zone and area coverage monitors.
 *
 * checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 *
 * checkAreaCoverage(): This monitor checks that all the zones of the 6x6 Km area are verified for every given tick.
 */
#include <iostream>
#include <sstream>

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
            checkCoverageVerification();

            std::this_thread::sleep_for(std::chrono::seconds(15));
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Monitor-CV] " << e.what() << std::endl;
    }
}

void CoverageMonitor::checkCoverageVerification()
{
    std::cout << "[Monitor-CV] Checking wave verification..." << std::endl;
    auto failed_data = getWaveVerification();

    if (failed_data.empty())
    {
        std::cout << "[Monitor-CV] All waves were verified until tick " << tick_last_read << std::endl;
    }
    else
    {
        struct area_data
        {
            int drone_id;
            int wave_id;
            int X;
            int Y;
        };

        std::unordered_map<int, std::vector<area_data>> failed_ticks;

        std::string q_wave = "INSERT INTO wave_coverage_logs (tick_n, wave_id, drone_id, issue_type) VALUES ";
        std::string q_area = "INSERT INTO area_coverage_logs (tick_n, wave_ids, drone_ids, X, Y) VALUES ";

        std::ostringstream wave_stream;

        // Parse failed data and construct the query
        for (const auto& data : failed_data)
        {
            int wave_id = data.wave_id;
            int tick_n = data.tick_n;
            int drone_id = data.drone_id;
            auto issue_type = data.issue_type;
            int x = data.X;
            int y = data.Y;

            // Enqueue failed checks
            failed_ticks[tick_n].push_back({drone_id, wave_id, x, y});

            // Append the values to the wave query stream
            wave_stream << "(" << tick_n << ", " << wave_id << ", " << drone_id << ", '" << issue_type << "'), ";
        }

        // Only append if the stream has content
        std::string wave_values = wave_stream.str();
        if (!wave_values.empty())
        {
            // Remove the trailing comma and space, then add the semicolon
            wave_values = wave_values.substr(0, wave_values.size() - 2) + ";";
            q_wave += wave_values;

            // Write to the database
            WriteToDB(q_wave);
        }
        std::cout << "[Monitor-CV] Wave coverage data written to DB" << std::endl;

        // Parse Area Coverage data
        for (const auto& failed_tick : failed_ticks)
        {
            std::ostringstream wave_id_stream, drone_stream, x_stream, y_stream;

            wave_id_stream << "ARRAY[";
            drone_stream << "ARRAY[";
            x_stream << "ARRAY[";
            y_stream << "ARRAY[";

            for (const auto& data : failed_tick.second)
            {
                wave_id_stream << data.wave_id << ", ";
                drone_stream << data.drone_id << ", ";
                x_stream << data.X << ", ";
                y_stream << data.Y << ", ";
            }

            // Remove the last ", " from each stream and close the arrays
            std::string waves_str = wave_id_stream.str();
            std::string drones_str = drone_stream.str();
            std::string x_str = x_stream.str();
            std::string y_str = y_stream.str();

            // Remove the last ", " from each and close the array
            waves_str = waves_str.substr(0, waves_str.size() - 2) + "]";
            drones_str = drones_str.substr(0, drones_str.size() - 2) + "]";
            x_str = x_str.substr(0, x_str.size() - 2) + "]";
            y_str = y_str.substr(0, y_str.size() - 2) + "]";

            // Build the final string for this tick
            q_area += "(" + std::to_string(failed_tick.first) + ", " + waves_str + ", " + drones_str + ", " + x_str +
                ", " + y_str + "), ";
        }

        // End the query correctly by removing the last ", " and adding a semicolon
        if (!failed_ticks.empty())
        {
            q_area.resize(q_area.size() - 2);
            q_area += R"(
ON CONFLICT (tick_n)
DO UPDATE SET
    drone_ids = area_coverage_monitor.drone_ids || EXCLUDED.drone_ids,
    x = area_coverage_monitor.x + EXCLUDED.x,
    y = area_coverage_monitor.y + EXCLUDED.y;)";
            WriteToDB(q_area);
        }

        std::cout << "[Monitor-CV] Area coverage data written to DB" << std::endl;
    }
}

std::vector<CoverageMonitor::WaveVerification> CoverageMonitor::getWaveVerification()
{
    pqxx::work txn(db.getConnection()); // Begin a transaction
    std::cout << "[Monitor-CV] Getting wave verification" << std::endl;

    std::string new_q = R"(
WITH working_waves AS (
    -- Identify waves that are currently in a WORKING state
    SELECT DISTINCT tick_n, wave_id
    FROM drone_logs
    WHERE status = 'WORKING'
),
dead_drones AS (
    -- Record the tick when each drone transitioned to DEAD
    SELECT drone_id, wave_id, MIN(tick_n) AS death_tick
    FROM drone_logs
    WHERE status = 'DEAD'
    GROUP BY drone_id, wave_id
),
drones_in_working_waves AS (
    -- Find drones that are in waves that are currently WORKING and include coordinates
    SELECT l.tick_n, l.wave_id, l.drone_id, l.status, l.checked, l.x, l.y
    FROM drone_logs l
    JOIN working_waves w ON l.tick_n = w.tick_n AND l.wave_id = w.wave_id
),
working_unchecked_drones AS (
    -- Drones that are in the WORKING state but have checked = FALSE
    SELECT tick_n, wave_id, drone_id, x, y
    FROM drones_in_working_waves
    WHERE status = 'WORKING' AND checked = FALSE
),
dead_drones_in_still_working_waves AS (
    -- Drones that died in a previous tick and whose wave is still WORKING in subsequent ticks
    SELECT w.tick_n, w.wave_id, d.drone_id, l.x, l.y
    FROM dead_drones d
    JOIN working_waves w ON d.wave_id = w.wave_id AND w.tick_n > d.death_tick
    LEFT JOIN drone_logs l ON l.drone_id = d.drone_id AND l.wave_id = d.wave_id AND l.tick_n = d.death_tick
),
disconnected_drones_in_working_waves AS (
    -- Drones that are in a DISCONNECTED state while their wave is WORKING
    SELECT tick_n, wave_id, drone_id, x, y
    FROM drones_in_working_waves
    WHERE status = 'DISCONNECTED'
)
-- Combine all the results into a single output
SELECT
    'WORKING_UNCHECKED' AS issue_type, tick_n, wave_id, drone_id, x, y
FROM working_unchecked_drones
UNION ALL
SELECT
    'DIED_WHILE_WORKING' AS issue_type, tick_n, wave_id, drone_id, x, y
FROM dead_drones_in_still_working_waves
UNION ALL
SELECT
    'DISCONNECTED' AS issue_type, tick_n, wave_id, drone_id, x, y
FROM disconnected_drones_in_working_waves;
)";

    auto out = txn.exec(new_q);
    std::cout << "[Monitor-CV] Executed new query" << std::endl;

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

    if (!out.empty())
    {
        for (const auto& row : out)
        {
            const int tick_n = row["tick_n"].as<int>();
            const int wave_id = row["wave_id"].as<int>();
            const int drone_id = row["drone_id"].as<int>();
            const auto issue_type = row["issue_type"].as<std::string>();
            const int x = row["x"].as<int>();
            const int y = row["y"].as<int>();

            WaveVerification wv = {wave_id, tick_n, drone_id, issue_type, x, y};
            wave_not_verified.push_back(wv);
        }
    }

    return wave_not_verified;
}
