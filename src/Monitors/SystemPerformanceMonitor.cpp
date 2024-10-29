#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <chrono>
#include <iostream>
#include <pqxx/pqxx>
#include <thread>

void SystemPerformanceMonitor::checkPerformance() {
    int last_processed_tick = 0;
    bool first_run = true;

    while (true) {
        try {
            log_system("Fetching performance data...");
            getPerformanceData(last_processed_tick);

            if (performance_data.empty()) {
                log_system("No performance data available to process.");
            } else {
                double total_degraded_performance = 0.0;
                int degraded_tick_count = 0;
                int total_tick_count = performance_data.size();

                for (const auto &[tick_n, working_drones_count, waves_count, performance] : performance_data) {
                    if (performance < 100.0) {
                        log_system("Degraded Tick " + std::to_string(tick_n) +
                                   " - Performance: " + std::to_string(performance) + "%");
                        total_degraded_performance += performance;
                        ++degraded_tick_count;
                    }
                }

                // Calculate the average performance of degraded ticks
                if (degraded_tick_count > 0) {
                    double average_degraded_performance = total_degraded_performance / degraded_tick_count;
                    log_system("Average performance of degraded ticks: " +
                               std::to_string(average_degraded_performance) + "%");
                } else {
                    log_system("No degraded ticks detected. All performance values are 100%.");
                }

                // Calculate the overall average performance of the system
                double total_performance = total_degraded_performance + (100.0 * (total_tick_count - degraded_tick_count));
                double average_system_performance = total_performance / total_tick_count;
                log_system("Average system performance: " + std::to_string(average_system_performance) + "%");

                // Update the last processed tick
                if (!performance_data.empty()) {
                    last_processed_tick = performance_data.back().tick_n;
                }

                // Write performance data to the database
                log_system("Writing performance data to the database...");
                try {
                    std::string query = "INSERT INTO system_performance_logs (tick_n, working_drones_count, waves_count, performance) VALUES ";
                    for (const auto &[tick_n, working_drones_count, waves_count, performance] : performance_data) {
                        query += "(" +
                                 std::to_string(tick_n) + ", " +
                                 std::to_string(working_drones_count) + ", " +
                                 std::to_string(waves_count) + ", " +
                                 std::to_string(performance) + "), ";
                    }
                    query = query.substr(0, query.size() - 2);    // Remove the last comma and space
                    query += " ON CONFLICT (tick_n) DO NOTHING;"; // Handle duplicates

                    log_system("Executing query...");
                    pqxx::work W(db.getConnection());
                    W.exec(query);
                    W.commit();

                    log_system("Performance data written to the database.");
                } catch (const std::exception &e) {
                    log_error("SystemPerformance", "Error while writing to the database: " + std::string(e.what()));
                }
            }

            // Clear the processed data
            performance_data.clear();
            // log_system("Performance data processed and cleared. Waiting for new data...");
        } catch (const std::exception &e) {
            log_error("SystemPerformance", e.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait for 10 seconds before the next iteration
    }
}

void SystemPerformanceMonitor::getPerformanceData(int last_processed_tick) {
    log_system("Getting performance data...");
    try {
        pqxx::work W(db.getConnection());

        const std::string q = "SELECT tick_n, COUNT(drone_id) AS working_drones_count, COUNT(DISTINCT wave_id) AS waves_count FROM drone_logs WHERE status = 'WORKING' AND tick_n > " + std::to_string(last_processed_tick) + " GROUP BY tick_n ORDER BY tick_n;";
        log_system("Executing query...");
        auto R = W.exec(q);

        for (const auto &row : R) {
            const int tick_n = row["tick_n"].as<int>();
            const int working_drones_count = row["working_drones_count"].as<int>();
            const int waves_count = row["waves_count"].as<int>();

            if (waves_count == 0) {
                log_system("Waves count is zero for Tick " + std::to_string(tick_n) +
                           ". Skipping this record to avoid division by zero.");
                continue;
            }

            const double performance = (working_drones_count / static_cast<double>(waves_count * 300)) * 100.0;
            performance_data.push_back({tick_n, working_drones_count, waves_count, performance});
        }

        log_system("Finished getting performance data. Retrieved " + std::to_string(R.size()) + " rows.");
    } catch (const std::exception &e) {
        log_error("SystemPerformance", "Error while fetching performance data: " + std::string(e.what()));
    }
}
