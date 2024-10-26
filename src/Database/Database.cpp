#include "Database.h"
#include "../../libs/nlohmann/json.hpp"
#include "../../utils/LogUtils.h"
#include <fstream>
#include <iostream>
#include <pqxx/pqxx>

void Database::ConnectToDB(const std::string &dbname,
                           const std::string &user,
                           const std::string &password,
                           const std::string &hostaddr,
                           const std::string &port) {
    ConnectToDB_();
}

// Function to read credentials from config file
std::tuple<std::string, std::string, std::string, std::string, std::string> Database::ReadCredentialsFromConfig() {
    // Leggo credenziali
    std::ifstream configFile("../res/doc/config.json");
    if (!configFile.is_open()) {
        throw std::runtime_error("Could not open config.json. Check file or path.");
    }

    // Parse JSON
    nlohmann::json config;
    configFile >> config;

    // Estrai le credenziali dal JSON
    const std::string dbname = config["database"]["name"];
    const std::string user = config["database"]["user"];
    const std::string password = config["database"]["password"];
    const std::string hostaddr = config["database"]["host"];
    const std::string port = config["database"]["port"];

    return std::make_tuple(dbname, user, password, hostaddr, port);
}

// Connect to db
void Database::ConnectToDB_() {
    auto [dbname, user, password, hostaddr, port] = ReadCredentialsFromConfig();

    // Crea la stringa di connessione
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

// Get or create db
void Database::get_DB() {
    try {
        // Leggi le credenziali dal file di configurazione
        auto [dbname, user, password, hostaddr, port] = ReadCredentialsFromConfig();

        // Connessione al server PostgreSQL (senza specificare un DB)
        std::string serverConnectionString = "user=" + user +
                                             " password=" + password +
                                             " hostaddr=" + hostaddr +
                                             " port=" + port;
        pqxx::connection C(serverConnectionString);
        pqxx::nontransaction N(C);

        // Se il DB non esiste, crearlo
        if (pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='" + dbname + "'"); R.empty()) {
            log_db("Creating " + dbname + " database");
            N.exec("CREATE DATABASE " + dbname);
        }

        // Connessione al DB specificato
        ConnectToDB_();

        if (conn && conn->is_open()) {
            pqxx::work W(*conn);
            W.exec("DROP TABLE IF EXISTS drone_logs");
            W.exec("DROP TABLE IF EXISTS monitor_logs");
            W.exec("DROP TABLE IF EXISTS system_performance_logs");
            W.exec("DROP TABLE IF EXISTS drone_charge_logs");

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

            W.exec(
                "CREATE TABLE system_performance_logs ("
                "tick_n INT PRIMARY KEY, "
                "working_drones_count INT, "
                "waves_count INT, "
                "performance FLOAT);");

            W.exec(
                "CREATE TABLE drone_charge_logs ("
                "drone_id INT PRIMARY KEY, "
                "consumption_factor FLOAT, "
                "arrived_at_base BOOLEAN);");

            W.commit();
        } else {
            log_error("Database", "Failed to connect to DB");
        }
    } catch (const std::exception &e) {
        log_error("Database", "Failed to get DB: " + std::string(e.what()));
    }
}

void Database::ExecuteQuery(const std::string &query) const {
    if (!conn || !conn->is_open()) {
        log_error("Database", "DB connection not established for query execution");
        return;
    }

    pqxx::work W(*conn);
    W.exec(query);
    W.commit();
}