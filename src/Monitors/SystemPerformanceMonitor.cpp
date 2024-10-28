#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <iostream>

void SystemPerformanceMonitor::checkPerformance() {
    bool first_run = true;
    bool continue_monitoring = true;
    int last_processed_tick = 0;

    while (continue_monitoring) {
        if (first_run) {
            log_system("Initiated...");
            first_run = false;
        }

        try {
            log_system("Fetching performance data...");
            getPerformanceData(last_processed_tick);

            if (performance_data.size() >= 100) {
                log_system("Writing performance data to the database...");
                std::string query =
                    "INSERT INTO system_performance_logs (tick_n, working_drones_count, waves_count, performance) VALUES ";

                for (const auto &[tick_n, working_drones_count, waves_count, performance] : performance_data) {
                    query += "(" + std::to_string(tick_n) + ", " + std::to_string(working_drones_count) + ", " + std::to_string(waves_count) + ", " + std::to_string(performance) + "), ";
                }
                query = query.substr(0, query.size() - 2);
                query += " ON CONFLICT (tick_n) DO NOTHING;"; // Gestione dei duplicati

                // simulateQueryExecution(performance_data.size());
                log_system("Executing query...");
                WriteToDB(query);

                // Calculate the average performance of the last 25 ticks
                if (performance_data.size() >= 25) {
                    double total_performance = 0.0;
                    size_t start_index = 0;
                    size_t end_index = 25;
                    for (size_t i = start_index; i < end_index; ++i) {
                        total_performance += performance_data[i].performance;
                    }
                    double average_performance = total_performance / 25;
                    log_system("Average performance of the last 25 ticks from Tick " + std::to_string(performance_data[start_index].tick_n) + " to Tick " + std::to_string(performance_data[end_index - 1].tick_n) + ": " + std::to_string(average_performance) + "%");

                    // Update the last processed tick
                    last_processed_tick = performance_data[end_index - 1].tick_n;

                    // Remove the processed data
                    performance_data.erase(performance_data.begin(), performance_data.begin() + 25);
                }

                log_system("Performance data written and cleared.");
                log_system("Waiting to process new data...");
            } else {
                log_system("Not enough data to start processing. Waiting...");
            }
        } catch (const std::exception &e) {
            log_error("SystemPerformance", e.what());
        }

        // Check if the simulation is still running
        auto sim_running = shared_redis.get("sim_running");
        if (sim_running && *sim_running == "false") {
            log_system("Simulation ended. Continuing to monitor until all data is processed.");
            continue_monitoring = false;
        }

        std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait for 10 seconds before the next check
    }

    // Continue monitoring until all data is processed
    while (true) {
        try {
            log_system("Fetching performance data...");
            getPerformanceData(last_processed_tick);

            if (performance_data.size() >= 25) {
                log_system("Writing performance data to the database...");
                std::string query =
                    "INSERT INTO system_performance_logs (tick_n, working_drones_count, waves_count, performance) VALUES ";

                for (const auto &[tick_n, working_drones_count, waves_count, performance] : performance_data) {
                    query += "(" + std::to_string(tick_n) + ", " + std::to_string(working_drones_count) + ", " + std::to_string(waves_count) + ", " + std::to_string(performance) + "), ";
                }
                query = query.substr(0, query.size() - 2);
                query += " ON CONFLICT (tick_n) DO NOTHING;"; // Gestione dei duplicati

                // simulateQueryExecution(performance_data.size());
                log_system("Executing query...");
                WriteToDB(query);

                // Calculate the average performance of the last 25 ticks
                double total_performance = 0.0;
                size_t start_index = 0;
                size_t end_index = 25;
                for (size_t i = start_index; i < end_index; ++i) {
                    total_performance += performance_data[i].performance;
                }
                double average_performance = total_performance / 25;
                log_system("Average performance of the last 25 ticks from Tick " + std::to_string(performance_data[start_index].tick_n) + " to Tick " + std::to_string(performance_data[end_index - 1].tick_n) + ": " + std::to_string(average_performance) + "%");

                // Update the last processed tick
                last_processed_tick = performance_data[end_index - 1].tick_n;

                // Remove the processed data
                performance_data.erase(performance_data.begin(), performance_data.begin() + 25);

                log_system("Performance data written and cleared.");
                log_system("Waiting to process new data...");
            } else {
                log_system("Not enough data to process. Waiting...");
            }
        } catch (const std::exception &e) {
            log_error("SystemPerformance", e.what());
        }

        std::this_thread::sleep_for(std::chrono::seconds(10)); // Wait for 10 seconds before the next check
    }
}

void SystemPerformanceMonitor::getPerformanceData(int last_processed_tick) {
    log_system("Getting performance data...");
    pqxx::work W(db.getConnection());

    // Get the number of working drones and their respective waves
    const std::string q =
        "SELECT tick_n, COUNT(drone_id) AS working_drones_count, COUNT(DISTINCT wave_id) AS waves_count "
        "FROM drone_logs "
        "WHERE status = 'WORKING' AND tick_n > " +
        std::to_string(last_processed_tick) + " "
                                              "GROUP BY tick_n "
                                              "ORDER BY tick_n;";

    // simulateQueryExecution(q.size());
    log_system("Executing query...");
    auto R = W.exec(q);

    for (const auto &row : R) {
        const int tick_n = row["tick_n"].as<int>();
        const int working_drones_count = row["working_drones_count"].as<int>();
        const int waves_count = row["waves_count"].as<int>();
        const double performance = (working_drones_count / static_cast<double>(waves_count * 300)) * 100;

        performance_data.push_back({tick_n, working_drones_count, waves_count, performance});
    }

    log_system("Finished getting performance data.");
}

void SystemPerformanceMonitor::simulateQueryExecution(size_t num_rows) {
    size_t total_steps = 4; // 100% / 25% = 4 steps
    size_t step_size = num_rows / total_steps;
    for (size_t i = 1; i <= total_steps; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Simulate time taken for query execution
        log_system("Executing query: " + std::to_string(i * 25) + "% complete");
    }
}
