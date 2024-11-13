/* checkCoverage(): Thread function for both the zone and area coverage monitors.
 *
 * checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes.
 * DroneControl checks for every drone update if the coords are the expected ones then adds the result of the verification
 * to the database (for now this only works when the drone is WORKING).
 *
 * checkAreaCoverage(): This monitor checks that all the zones of the 6x6 Km area are verified for every given tick.
 */
#include <fstream>
#include <iostream>
#include <sstream>

#include "../../utils/LogUtils.h"
#include "../../utils/utils.h"
#include "Monitor.h"

/**
 * \brief Main function to monitor zone and area coverage.
 *
 * This function runs in a loop, periodically calling checkZoneVerification() and checkAreaCoverage().
 * It sleeps for 20 seconds between each iteration.
 */
void WaveCoverageMonitor::checkWaveCoverage()
{
    // std::cout << "[Monitor-CV] Initiated..." << std::endl;
    log_wave_coverage("Initiated...");

    try
    {
        while (sim_running)
        {
            std::this_thread::sleep_for(std::chrono::seconds(10));

            checkCoverageVerification();
            log_wave_coverage("Latest processed time: " + latest_processed_time + " - Old processed time: " + old_processed_time);

            // If in the first execution the monitor hasn't read any data, don't update the old_processed_time
            if (latest_processed_time != "00:00:00")
            {
                CheckSimulationEnd();
                old_processed_time = latest_processed_time;
            }

        }
        log_wave_coverage("Finished");
    }
    catch (const std::exception& e)
    {
        // std::cerr << "[Monitor-CV] " << e.what() << std::endl;
        log_wave_coverage(e.what());
    }
}

void WaveCoverageMonitor::checkCoverageVerification()
{
    log_wave_coverage("Checking wave verification...");
    auto failed_data = getWaveVerification();

    if (failed_data.empty())
    {
        log_wave_coverage("All waves were verified until tick " + std::to_string(tick_last_read));
        return;
    }

    std::string q_wave = "INSERT INTO wave_coverage_logs (tick_n, wave_id, drone_id, fault_type) VALUES ";

    // Parse failed data and construct the query
    for (const auto& data : failed_data)
    {
        int wave_id = data.wave_id;
        int tick_n = data.tick_n;
        int drone_id = data.drone_id;
        auto fault_type = data.issue_type;

        log_wave_coverage(
            "Wave " + std::to_string(wave_id) + " was not verified by drone " + std::to_string(drone_id) +
            " at tick " + std::to_string(tick_n));

        // Append the values directly to the wave query string
        q_wave += "(" + std::to_string(tick_n) + ", " + std::to_string(wave_id) + ", " + std::to_string(drone_id) +
            ", '" + fault_type + "'), ";
    }

    // Remove the trailing comma and space, then add the conflict clause
    q_wave.erase(q_wave.size() - 2);
    q_wave += " ON CONFLICT (tick_n, drone_id) DO UPDATE SET fault_type = EXCLUDED.fault_type;";

    // Write to the database
    WriteToDB(q_wave);
    log_wave_coverage("Wave coverage data written to DB");
}


std::vector<WaveCoverageMonitor::WaveVerification> WaveCoverageMonitor::getWaveVerification()
{
    pqxx::work txn(db.getConnection()); // Begin a transaction
    // std::cout << "[Monitor-CV] Getting wave verification" << std::endl;
    log_wave_coverage("Getting wave verification");

    std::string new_q = R"(
WITH working_waves AS (
    SELECT DISTINCT tick_n, wave_id
    FROM drone_logs
    WHERE status = 'WORKING' AND created_at > $1
),
dead_drones AS (
    SELECT drone_id, wave_id, MIN(tick_n) AS death_tick
    FROM drone_logs
    WHERE status = 'DEAD' AND created_at > $1
    GROUP BY drone_id, wave_id
),
drones_in_working_waves AS (
    SELECT l.tick_n, l.wave_id, l.drone_id, l.status, l.checked, l.created_at
    FROM drone_logs l
    JOIN working_waves w ON l.tick_n = w.tick_n AND l.wave_id = w.wave_id
    WHERE l.created_at > $1
),
working_unchecked_drones AS (
    SELECT tick_n, wave_id, drone_id, created_at
    FROM drones_in_working_waves
    WHERE status = 'WORKING' AND checked = FALSE
),
dead_drones_in_still_working_waves AS (
    SELECT w.tick_n, w.wave_id, d.drone_id, l.created_at
    FROM dead_drones d
    JOIN working_waves w ON d.wave_id = w.wave_id AND w.tick_n > d.death_tick
    LEFT JOIN drone_logs l ON l.drone_id = d.drone_id AND l.wave_id = d.wave_id AND l.tick_n = d.death_tick
    WHERE l.created_at > $1
),
disconnected_drones_in_working_waves AS (
    SELECT tick_n, wave_id, drone_id, created_at
    FROM drones_in_working_waves
    WHERE status = 'DISCONNECTED'
)
SELECT
    'WORKING_UNCHECKED' AS issue_type, tick_n, wave_id, drone_id, created_at
FROM working_unchecked_drones
UNION ALL
SELECT
    'DIED_WHILE_WORKING' AS issue_type, tick_n, wave_id, drone_id, created_at
FROM dead_drones_in_still_working_waves
UNION ALL
SELECT
    'DISCONNECTED' AS issue_type, tick_n, wave_id, drone_id, created_at
FROM disconnected_drones_in_working_waves;
)";


    auto out = txn.exec_params(new_q, latest_processed_time);
    // std::cout << "[Monitor-CV] Executed new query" << std::endl;
    log_wave_coverage("Executed new query");

//     std::string update_query = R"(
//     UPDATE drone_logs
//     SET is_read = TRUE
//     WHERE created_at <= $1
// )";
//     txn.exec_params(update_query, lastProcessedTime);

    txn.commit();

    std::vector<WaveVerification> wave_not_verified;

    if (!out.empty())
    {
        for (const auto& row : out)
        {
            int tick_n = row["tick_n"].as<int>();
            int wave_id = row["wave_id"].as<int>();
            int drone_id = row["drone_id"].as<int>();
            auto issue_type = row["issue_type"].as<std::string>();
            // auto x = row["x"].as<float>();
            // auto y = row["y"].as<float>();
            auto read_time = row["created_at"].as<std::string>();

            // Update lastProcessedTime to the maximum timestamp
            if (read_time > latest_processed_time)
            {
                latest_processed_time = read_time;
            }

            wave_not_verified.push_back({wave_id, tick_n, drone_id, issue_type});
        }
    }
    return wave_not_verified;
}
