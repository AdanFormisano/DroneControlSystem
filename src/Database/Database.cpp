#include "Database.h"
#include "../../utils/LogUtils.h"
#include <iostream>

void Database::ConnectToDB(const std::string &dbname,
                           const std::string &user,
                           const std::string &password,
                           const std::string &hostaddr,
                           const std::string &port) {
    ConnectToDB_();
}

// Connect to db
void Database::ConnectToDB_() {
    std::string connectionString = "dbname=dcs user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432";
    conn = std::make_unique<pqxx::connection>(connectionString);

    if (!conn->is_open()) {
        throw std::runtime_error("DB can't connect");
    }
}

// TODO: Separate the function for connecting to the db from the one creating the db
// Get or create db
void Database::get_DB() {
    try {
        // Connessione al server PostgreSQL (senza specificare un DB)
        pqxx::connection C("user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432");
        // Verifica se il database 'dcs' esiste
        pqxx::nontransaction N(C);
        // Se il DB non esiste, crearlo
        if (pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='dcs'"); R.empty()) {
            // spdlog::warn("Creating dcs database");
            log_db("Creating dcs database");
            // std::cout << "Creating dcs database" << std::endl;
            // pqxx::work W(C);
            // W.exec("CREATE DATABASE dcs;");
            // W.commit();
            N.exec("CREATE DATABASE dcs");
        }
        // Connession al DB 'dcs'
        ConnectToDB_();

        // TODO: Check if to create a table it's better to use a nontransaction connection or not
        // Se la connessione è stata stabilita, sovrascrivere la tabella 'drone_logs'
        if (conn && conn->is_open()) {
            pqxx::work W(*conn);
            // Eliminare la tabella se esiste
            W.exec("DROP TABLE IF EXISTS drone_logs");
            W.exec("DROP TABLE IF EXISTS monitor_logs");
            W.exec("DROP TABLE IF EXISTS system_performance_logs");
            W.exec("DROP TABLE IF EXISTS drone_charge_logs");

            // Ricreare la tabella per i log dei droni
            W.exec(
                "CREATE TABLE drone_logs ("
                "tick_n INT, "
                "drone_id INT NOT NULL, "
                "status VARCHAR(255), "
                "charge FLOAT, "
                "wave INT, "
                "x FLOAT, "
                "y FLOAT, "
                "checked BOOLEAN, "
                "CONSTRAINT PK_drone_logs PRIMARY KEY (tick_n, drone_id))");

            // Create table for monitor logs
            W.exec(
                "CREATE TABLE monitor_logs ("
                "tick_n INT PRIMARY KEY, "
                "wave_cover INT[], "
                "area_cover VARCHAR(255), "
                "charge_drone_id INT[], "
                "charge_percentage INT[], "
                "charge_needed INT[], "
                "recharge_drone_id INT[], "
                "recharge_duration INT[], "
                "time_to_read INT[]);");

            // Create table for system performance logs
            W.exec(
                "CREATE TABLE system_performance_logs ("
                "tick_n INT PRIMARY KEY, "
                "working_drones_count INT, "
                "waves_count INT, "
                "performance FLOAT);");

            // Create table for drones that consumed more than the intended charge
            W.exec(
                "CREATE TABLE drone_charge_logs ("
                "drone_id INT PRIMARY KEY, "
                "consumption_factor FLOAT, "
                "arrived_at_base BOOLEAN);");

            W.commit();
        } else {
            log_error("Database", "Failed to connect to DB");
            // std::cerr << "Failed to connect to DB" << std::endl;
        }
    } catch (const std::exception &e) {
        // spdlog::error("Failed to get DB: {}", e.what());
        log_error("Database", "Failed to get DB: " + std::string(e.what()));
        // std::cerr << "Failed to get DB: " << e.what() << std::endl;
    }
}

void Database::ExecuteQuery(const std::string &query) const {
    if (!conn || !conn->is_open()) {
        // spdlog::error("DB connection not established for query execution");
        log_error("Database", "DB connection not established for query execution");
        // std::cerr << "DB connection not established for query execution" << std::endl;
        return;
    }

    //    spdlog::info("Executing query: {}", query);
    pqxx::work W(*conn);
    W.exec(query);
    W.commit();
}
