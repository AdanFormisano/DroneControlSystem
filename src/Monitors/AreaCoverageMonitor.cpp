#include "Monitor.h"
#include "../../utils/LogUtils.h"

void AreaCoverageMonitor::checkAreaCoverage()
{
    log_area_coverage("Initiated...");

    // Initialize the area coverage data
    for (int i = -2990; i <= 2990; i += 20) // X
    {
        for (int j = -2990; j <= 2990; j += 20) // Y
        {
            const int index = (j + 2990) / 20; // Calculate the index for the Y coordinate
            area_coverage_data[i][index] = {-1, -1, -1};
        }
    }

    // Sleep for 10 seconds
    std::this_thread::sleep_for(std::chrono::seconds(10));

    log_area_coverage("Getting area coverage data...");

    // Start a transaction for read and write operations
    pqxx::nontransaction W(db.getConnection());
    std::string query = R"(
    SELECT tick_n, wave_id, drone_id, x, y, created_at
    FROM drone_logs
    WHERE status = 'WORKING' ORDER BY tick_n;
    )";

    auto R = W.exec(query);
    W.commit();

    // Parse area coverage data
    for (const auto& row : R)
    {
        int tick = row["tick_n"].as<int>();
        int drone_id = row["drone_id"].as<int>();
        int wave_id = row["wave_id"].as<int>();
        int X = row["x"].as<int>();
        int Y = row["y"].as<int>();
        // auto read_time = row["created_at"].as<std::string>();

        int index = (Y + 2990) / 20; // Calculate the index for the Y coordinate

        auto [data_tick, data_wave_id, data_drone_id] = area_coverage_data[X][index];
        if (tick > data_tick)
        {
            // if (X == 2990)
            // {
            //     log_area_coverage("Drone " + std::to_string(drone_id) + " reached the end of the area");
            // }

            // Elaborate the data with batched updates
            readAreaCoverageData({tick, wave_id, drone_id}, X, Y);
        }

        // if (read_time > latest_processed_time)
        // {
        //     latest_processed_time = read_time;
        // }
    }

    log_area_coverage("Latest processed time: " + latest_processed_time + " - Old processed time: " + old_processed_time);

    // // Update old_processed_time after the first successful data read
    // if (latest_processed_time != "00:00:00")
    // {
    //     CheckSimulationEnd();
    //     old_processed_time = latest_processed_time;
    // }

    // Now insert the unverified ticks for each checkpoint
    log_area_coverage("Inserting unverified ticks for each checkpoint");

    InsertUnverifiedTicks();

    log_area_coverage("Finished monitoring area coverage");
}

void AreaCoverageMonitor::readAreaCoverageData(const AreaData& area_data, const int X, const int Y) {
    int index = (Y + 2990) / 20;
    int delta_tick = area_data.tick_n - area_coverage_data[X][index].tick_n;

    try
    {
        if (area_coverage_data[X][index].tick_n == -1) { // Special starting case
            for (int i = 0; i < area_data.tick_n && i < 6000; i++) {
                unverified_ticks[X][Y].set(i);  // Set bit for each unverified tick
            }
        } else if (delta_tick > 125) {
            log_area_coverage("Checkpoint (" + std::to_string(X) + ", " + std::to_string(Y) + ") not visited for " + std::to_string(delta_tick) + " ticks [" + std::to_string(area_coverage_data[X][index].tick_n) + " -> " + std::to_string(area_data.tick_n) + "]");

            for (int i = 125; i + area_coverage_data[X][index].tick_n < area_data.tick_n && i < 6000; i++) {
                int tick_position = area_coverage_data[X][index].tick_n + i;
                if (tick_position < 6000) {  // Ensure tick_position is within bitset bounds
                    unverified_ticks[X][Y].set(tick_position);
                } else {
                    log_area_coverage("Warning: tick_position out of range: " + std::to_string(tick_position) + " i: " + std::to_string(i));
                    break;  // Exit the loop if tick_position is out of bounds
                }
            }
        }
        // Update area coverage data for the current checkpoint
        area_coverage_data[X][index] = {area_data.tick_n, area_data.wave_id, area_data.drone_id};
    }
    catch (const std::exception& e)
    {
        log_area_coverage("Error in readAreaCoverageData: " + std::string(e.what()));
    }
}

void AreaCoverageMonitor::InsertUnverifiedTicks() {
    log_area_coverage(std::to_string(unverified_ticks.size()));
    pqxx::work W(db.getConnection());  // Start a new transaction here

    try {
        for (auto& column : unverified_ticks) {
            auto X = column.first;
            std::string q = "INSERT INTO area_coverage_logs (checkpoint, unverified_ticks) VALUES ";

            // Iterate through all Y values for the current X
            for (auto& row: column.second) {
                auto Y = row.first;
                const auto& tick_bitset = row.second;

                q += "('(" + std::to_string(X) + ", " + std::to_string(Y) + ")', ARRAY[";

                for (int tick = 0; tick < 6000; ++tick) {
                    if (tick_bitset.test(tick)) {  // Only include set bits
                        q += std::to_string(tick) + ", ";
                    }
                }

                q.pop_back(); q.pop_back();  // Remove trailing comma
                q += "]), ";
            }
            q.pop_back(); q.pop_back();  // Remove trailing comma
            q += ";";

            W.exec(q);
            log_area_coverage("Inserted unverified ticks for checkpoint (" + std::to_string(X) + ", Y)");
        }
        W.commit();  // Commit the transaction after each set of inserts
    } catch (const std::exception& e) {
        log_area_coverage("Error in InsertUnverifiedTicks: " + std::string(e.what()));
        // Optionally add rollback or other error handling here if needed
    }
}



