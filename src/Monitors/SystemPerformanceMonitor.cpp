#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <chrono>
#include <iostream>
#include <pqxx/pqxx>
#include <thread>

void SystemPerformanceMonitor::checkPerformance()
{
    log_system("Fetching performance data...");

    getPerformanceData();

    if (performance_data.empty())
    {
        log_system("No performance data available to process.");
    }
    else
    {
        // Write performance data to the database
        log_system("Writing performance data to the database...");
        try
        {
            std::string query = "INSERT INTO system_performance_logs (tick_n, working_drones_count, waves_count, performance) VALUES ";
            for (const auto& [tick_n, working_drones_count, waves_count, performance] : performance_data)
            {
                if (performance < 100.0f)
                {
                log_system("Degraded Tick " + std::to_string(tick_n) + " - Performance: " + std::to_string(performance) + "%");
                }
                query += "(" +
                    std::to_string(tick_n) + ", " +
                    std::to_string(working_drones_count) + ", " +
                    std::to_string(waves_count) + ", " +
                    std::to_string(performance) + "), ";
            }
            query = query.substr(0, query.size() - 2); // Remove the last comma and space
            query += " ON CONFLICT (tick_n) DO NOTHING;"; // Handle duplicates

            log_system("Executing query...");
            pqxx::work W(db.getConnection());
            W.exec(query);
            W.commit();

            log_system("Finished");
        }
        catch (const std::exception& e)
        {
            log_error("SystemPerformance", "Error while writing to the database: " + std::string(e.what()));
        }
    }
}

void SystemPerformanceMonitor::getPerformanceData()
{
    log_system("Getting performance data...");
    try
    {
        pqxx::work W(db.getConnection());

        const std::string q =
            "SELECT tick_n, COUNT(drone_id) AS working_drones_count, COUNT(DISTINCT wave_id) AS waves_count FROM drone_logs WHERE status = 'WORKING' GROUP BY tick_n;";
        log_system("Executing query...");
        auto R = W.exec(q);

        for (const auto& row : R)
        {
            const int tick_n = row["tick_n"].as<int>();
            const int working_drones_count = row["working_drones_count"].as<int>();
            const int waves_count = row["waves_count"].as<int>();

            const double performance = (working_drones_count / static_cast<double>(waves_count * 300)) * 100.0;
            performance_data.push_back({tick_n, working_drones_count, waves_count, performance});
        }
    }
    catch (const std::exception& e)
    {
        log_error("SystemPerformance", "Error while fetching performance data: " + std::string(e.what()));
    }
}
