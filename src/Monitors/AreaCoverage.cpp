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

    std::this_thread::sleep_for(std::chrono::seconds(10));

    while (tick_n < sim_duration_ticks)
    {
        log_area_coverage("Getting area coverage data...");

        // Get area coverage data from DB
        pqxx::work W(db.getConnection());

        std::string query = R"(
        SELECT tick_n, wave_id, drone_id, x, y
        FROM Drone_logs
        WHERE status = 'WORKING')";

        auto R = W.exec(query);

        W.commit();

        // Parse area coverage data
        for (const auto& row : R)
        {
            int tick = row["tick_n"].as<int>();
            int drone_id = row["drone_id"].as<int>();
            int wave_id = row["wave_id"].as<int>();
            auto X = row["x"].as<float>();
            auto Y = row["y"].as<float>();

            int index = (Y + 2990) / 20; // Calculate the index for the Y coordinate

            auto [data_tick, data_wave_id, data_drone_id] = area_coverage_data[static_cast<int>(X)][index];
            if (tick > data_tick)
            {
                // Save the new data
                AreaData area_data{tick, wave_id, drone_id};
                // area_data_out.emplace_back(area_data, static_cast<int>(X), static_cast<int>(Y));

                // Elaborate the data
                readAreaCoverageData(area_data, static_cast<int>(X), static_cast<int>(Y));
            }
        }


        // Sleep for 10 seconds
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }

    log_area_coverage("Finished monitoring area coverage");
}

void AreaCoverageMonitor::readAreaCoverageData(const AreaData& area_data, const int X, const int Y)
{
    log_area_coverage("Checking area coverage...");

    pqxx::work W(db.getConnection());

    int index = (Y + 2990) / 20; // Calculate the index for the Y coordinate
    int delta_tick = area_data.tick_n - area_coverage_data[X][index].tick_n;    // Number of ticks since the last checkpoint visit

    if (area_coverage_data[X][index].tick_n == -1) // Special starting case
    {
        // log_area_coverage("Checkpoint (" + std::to_string(X) + ", " + std::to_string(Y) + ") not visited during first " + std::to_string(area_data.tick_n) + " ticks");
        for (int i = 0; i < area_data.tick_n; ++i)
        {
            // Insert the missing checkpoints
            std::string query =
                "INSERT INTO area_coverage_logs (checkpoint, unverified_ticks) "
                "VALUES ('(" + std::to_string(X) + ", " + std::to_string(Y) + ")', "
                "ARRAY[(" + std::to_string(i) + ")]) "
                "ON CONFLICT (checkpoint) DO UPDATE SET "
                "unverified_ticks = area_coverage_logs.unverified_ticks || EXCLUDED.unverified_ticks;";

            W.exec(query);
        }
    }
    else if (delta_tick > 125) // Check if delta tick is greater than 125
    {
        int n_of_missed_drones = std::floor(delta_tick / 125); // Calculate the number of missed drones

        // If the delta is greater than 125, it means that the checkpoint hasn't been visited for delta - 125 ticks
        log_area_coverage("Checkpoint (" + std::to_string(X) + ", " + std::to_string(Y) + ") not visited for more then 125 ticks");
        for (int i = 126; i <= (delta_tick - 125); ++i)
        {
            // Insert the missing checkpoints
            std::string query =
                "INSERT INTO area_coverage_logs (checkpoint, unverified_ticks) "
                "VALUES ('(" + std::to_string(X) + ", " + std::to_string(Y) + ")', "
                "ARRAY[(" + std::to_string(area_coverage_data[X][index].tick_n + i) + ")]) "
                "ON CONFLICT (checkpoint) DO UPDATE SET "
                "unverified_ticks = area_coverage_logs.unverified_ticks || EXCLUDED.unverified_ticks;";

            W.exec(query);
        }
    }

    // Update the area coverage data
    area_coverage_data[X][index] = {area_data.tick_n, area_data.wave_id, area_data.drone_id};
    W.commit();
}
