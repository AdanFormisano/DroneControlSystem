#include "../../utils/LogUtils.h"
#include "Monitor.h"
#include <iostream>

void SystemPerformanceMonitor::checkPerformance() {
    log_system("Initiated...");
    std::cout << "[Monitor-SP] Initiated..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(10));

    try {
        // Get performance data
        getPerformanceData();

        if (performance_data.empty()) {
            log_system("No performance data to write to the database.");
            return;
        }

        // Write performance data to the database
        std::string query =
            "INSERT INTO system_performance_logs (tick_n, working_drones_count, waves_count, performance) VALUES ";

        for (const auto &[tick_n, working_drones_count, waves_count, performance] : performance_data) {
            query += "(" + std::to_string(tick_n) + ", " + std::to_string(working_drones_count) + ", " +
                     std::to_string(waves_count) + ", " + std::to_string(performance) + "), ";
        }
        query = query.substr(0, query.size() - 2);
        query += ";";

        WriteToDB(query);

        // Clear the performance data
        performance_data.clear();

        std::this_thread::sleep_for(std::chrono::seconds(10));
    } catch (const std::exception &e) {
        log_error("SystemPerformanceMonitor", e.what());
        std::cerr << "[Monitor-CV] " << e.what() << std::endl;
    }
}

void SystemPerformanceMonitor::getPerformanceData() {
    pqxx::work W(db.getConnection());

    // Get the number of working drones and their respective waves
    const std::string q =
        "SELECT tick_n, COUNT(drone_id) AS working_drones_count, COUNT(DISTINCT wave) AS waves_count "
        "FROM drone_logs "
        "WHERE status = 'WORKING' "
        "GROUP BY tick_n "
        "ORDER BY tick_n;";

    auto R = W.exec(q);

    for (const auto &row : R) {
        const int tick_n = row["tick_n"].as<int>();
        const int working_drones_count = row["working_drones_count"].as<int>();
        const int waves_count = row["waves_count"].as<int>();

        // Calculate the performance in percentage
        const double performance = (working_drones_count / static_cast<double>(waves_count * 300)) * 100;

        // std::cout << "[Monitor-SP] Performance at tick " << tick_n << ": " << performance << "%" << std::endl;

        performance_data.push_back({tick_n, working_drones_count, waves_count, performance});
    }
}
