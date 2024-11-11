#include "Database.h"
#include "../../libs/nlohmann/json.hpp"
#include "../../utils/LogUtils.h"
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>
#include <thread>



void Database::ConnectToDB ()
{
    auto [dbname, user, password, hostaddr, port] = ReadCredentialsFromConfig();

    std::string connectionString = "dbname=" + dbname +
                                   " user=" + user +
                                   " password=" + password +
                                   " hostaddr=" + hostaddr +
                                   " port=" + port;

    conn = std::make_unique<pqxx::connection>(connectionString);

    if (!conn->is_open()) {
        throw std::runtime_error("DB can't connect");
    }
}


// Read credentials from config file
Database::DBCredentials Database::ReadCredentialsFromConfig()
{
    std::ifstream configFile("../src/db_config.json");
    if (!configFile.is_open())
    {
        throw std::runtime_error("Could not open db_config.json. Check file or path.");
    }

    // Parso JSON
    nlohmann::json config;
    configFile >> config;

    // Estraggo credenziali da JSON
    const std::string dbname = config["database"]["name"];
    const std::string user = config["database"]["user"];
    const std::string password = config["database"]["password"];
    const std::string hostaddr = config["database"]["host"];
    const std::string port = config["database"]["port"];

    return {dbname, user, password, hostaddr, port};
}

// Create the DB
void Database::CreateDB(const DBCredentials& credentials) {
    // Connessione al server PostgreSQL (senza specificare un DB)
    std::string serverConnectionString = "user=" + credentials.user +
                                         " password=" + credentials.password +
                                         " hostaddr=" + credentials.hostaddr +
                                         " port=" + credentials.port;
    pqxx::connection C(serverConnectionString);
    pqxx::nontransaction N(C);

    // Se il DB non esiste, crealo
    if (pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='" + credentials.dbname + "'"); R.empty()) {
        log_db("Creating " + credentials.dbname + " database");
        N.exec("CREATE DATABASE " + credentials.dbname);
    }
}

// Create tables
void Database::CreateTables() {
    if (conn && conn->is_open()) {
        pqxx::work W(*conn);
        W.exec("DROP TABLE IF EXISTS drone_logs");
        W.exec("DROP TABLE IF EXISTS wave_coverage_logs");
        W.exec("DROP TABLE IF EXISTS area_coverage_logs");
        W.exec("DROP TABLE IF EXISTS system_performance_logs");
        W.exec("DROP TABLE IF EXISTS drone_charge_logs");
        W.exec("DROP TABLE IF EXISTS drone_recharge_logs");

        W.exec(
            "CREATE TABLE drone_logs ("
            "tick_n INT, "
            "drone_id INT NOT NULL, "
            "status VARCHAR(255), "
            "charge FLOAT, "
            "wave_id INT, "
            "x FLOAT, "
            "y FLOAT, "
            "checked BOOLEAN, "
            "created_at TIME DEFAULT CURRENT_TIME, "
            "CONSTRAINT PK_drone_logs PRIMARY KEY (tick_n, drone_id))");

        W.exec("CREATE INDEX working_only ON drone_logs (drone_id, wave_id, tick_n) WHERE status = 'WORKING';");
        W.exec("CREATE INDEX idx_dead_unread ON drone_logs (drone_id, wave_id, tick_n) WHERE status = 'DEAD';");
        W.exec("CREATE INDEX idx_disconnected_unread ON drone_logs (tick_n, wave_id, drone_id) WHERE status = 'DISCONNECTED';");
        W.exec("CREATE INDEX idx_charging_unread ON drone_logs (tick_n, wave_id, drone_id) WHERE status = 'CHARGING';");
        W.exec("CREATE INDEX idx_charging_completed_unread ON drone_logs (tick_n, wave_id, drone_id) WHERE status = 'CHARGING_COMPLETED';");



        W.exec(
            "CREATE TABLE wave_coverage_logs ("
            "tick_n INT, "
            "wave_id INT, "
            "drone_id INT, "
            "fault_type VARCHAR(255), "
            "CONSTRAINT PK_wave_coverage_logs PRIMARY KEY (tick_n, drone_id));");

        W.exec(
            "CREATE TABLE area_coverage_logs ("
            "checkpoint VARCHAR(20) PRIMARY KEY, "
            "unverified_ticks INT[]);");

        W.exec(
            "CREATE TABLE system_performance_logs ("
            "tick_n INT PRIMARY KEY, "
            "working_drones_count INT, "
            "waves_count INT, "
            "performance FLOAT);");

        W.exec(
            "CREATE TABLE drone_charge_logs ("
            "drone_id INT PRIMARY KEY, "
            "consumption FLOAT, "
            "consumption_factor FLOAT, "
            "arrived_at_base BOOLEAN);");

        W.exec(
            "CREATE TABLE drone_recharge_logs ("
            "drone_id INT PRIMARY KEY, "
            "recharge_duration_ticks INT, "
            "recharge_duration_min FLOAT,"
            "start_tick INT, "
            "end_tick INT);");

        W.commit();
    }
    else
    {
        log_error("Database", "Failed to connect to DB");
    }
}

// Get or create db
void Database::get_DB() {
    const int max_retries = 5;       // Numero massimo di tentativi
    const int retry_delay_ms = 1000; // Ritardo tra i tentativi (in millisecondi)
    int retry_count = 0;

    while (retry_count < max_retries)
    {
        try
        {
            // Legge le credenziali dal db_config.json
            auto [dbname, user, password, hostaddr, port] = ReadCredentialsFromConfig();
            CreateDB({dbname, user, password, hostaddr, port});
            ConnectToDB();
            CreateTables();
            log_db("Successfully connected to DB on attempt " + std::to_string(retry_count + 1));
            break; // Esci dal loop se la connessione è riuscita
        } catch (const pqxx::sql_error &e) {
            log_error("Database", "SQL error: " + std::string(e.what()));
        } catch (const pqxx::broken_connection &e) {
            log_error("Database", "Connection error: " +
                                      std::string(e.what()) + " (Attempt " +
                                      std::to_string(retry_count + 1) + " of " +
                                      std::to_string(max_retries) + ")");
        } catch (const std::exception &e) {
            log_error("Database", "Failed to get DB: " +
                                      std::string(e.what()) + " (Attempt " +
                                      std::to_string(retry_count + 1) + " of " +
                                      std::to_string(max_retries) + ")");
        }

        retry_count++;
        if (retry_count < max_retries) {
            std::this_thread::sleep_for(std::chrono::milliseconds(retry_delay_ms));
        } else {
            log_error("Database", "Maximum retry attempts reached. Could not connect to DB.");
        }
    }
}

void Database::ExecuteQuery(const std::string &query) const {
    if (!conn || !conn->is_open()) {
        log_error("Database", "DB connection not established for query execution");
        return;
    }

    try {
        pqxx::work W(*conn);
        W.exec(query);
        W.commit();
    } catch (const pqxx::sql_error &e) {
        log_error("Database", "SQL error: " + std::string(e.what()));
    } catch (const std::exception &e) {
        log_error("Database", "Failed to execute query: " + std::string(e.what()));
    }
}
