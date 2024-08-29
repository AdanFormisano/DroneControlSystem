#include "Database.h"

#include <iostream>

// Constructor
Database::Database() {
    // try {
    //     // Connect to PostgreSQL server
    //     pqxx::connection C("user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432");
    //     if (!C.is_open()) {
    //         spdlog::error("Can't open connection to PostgreSQL server");
    //         return;
    //     }
    //     spdlog::info("Connected to PostgreSQL server");
    //
    //     // Check if dcs exists
    //     pqxx::nontransaction N(C);
    //     auto R = N.exec("SELECT 1 FROM pg_database WHERE datname='dcs'");
    //     if (R.empty()) {
    //         // Create dcs database and connect to it
    //         spdlog::warn("Databse not found, creating dcs...");
    //         pqxx::work W(C);
    //         W.exec("CREATE DATABASE dcs");
    //         W.commit();
    //         ConnectToDB_(); // This sets the conn object
    //
    //         // Se la connessione è stata stabilita, sovrascrivere la tabella 'drone_logs'
    //         if (conn && conn->is_open()) {
    //             pqxx::work W(*conn);
    //             // Eliminare la tabella se esiste
    //             W.exec("DROP TABLE IF EXISTS drone_logs");
    //             // Ricreare la tabella
    //             W.exec(
    //                 "CREATE TABLE drone_logs ("
    //                 "tick_n INT, "
    //                 "drone_id INT NOT NULL, "
    //                 "status VARCHAR(255), "
    //                 "charge FLOAT, "
    //                 "zone VARCHAR(255), "  // TODO: Change to int
    //                 "x FLOAT, "
    //                 "y FLOAT, "
    //                 "checked BOOLEAN, "
    //                 "CONSTRAINT PK_drone_logs PRIMARY KEY (tick_n, drone_id))");
    //             W.commit();
    //         }
    //     }
    //
    //     spdlog::info("Database object created");
    // } catch (const std::exception &e) {
    //     spdlog::error("Database object creation failed: {}", e.what());
    // }
}

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
        pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='dcs'");
        // Se il DB non esiste, crearlo
        if (R.empty()) {
            spdlog::warn("Creating dcs database");
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

            // Ricreare la tabella per i log dei droni
            W.exec(
                "CREATE TABLE drone_logs ("
                "tick_n INT, "
                "drone_id INT NOT NULL, "
                "status VARCHAR(255), "
                "charge FLOAT, "
                "zone VARCHAR(255), "  // TODO: Change to int
                "x FLOAT, "
                "y FLOAT, "
                "checked BOOLEAN, "
                "CONSTRAINT PK_drone_logs PRIMARY KEY (tick_n, drone_id))");

            // Create table for monitor logs
            W.exec(
                "CREATE TABLE monitor_logs ("
                "tick_n INT PRIMARY KEY, "
                "zone_cover INT[], "
                "area_cover VARCHAR(255), "
                "charge_drone_id INT[], "
                "charge_percentage INT[], "
                "charge_needed INT[], "
                "recharge_drone_id INT[], "
                "recharge_duration INT[], "
                "time_to_read INT[]);");

            W.commit();
        }
    } catch (const std::exception &e) {
        spdlog::error("Failed to get DB: {}", e.what());
    }
}

void Database::ExecuteQuery(const std::string &query) {
    if (!conn || !conn->is_open()) {
        spdlog::error("DB connection not established for query execution");
        return;
    }

    //    spdlog::info("Executing query: {}", query);
    pqxx::work W(*conn);
    W.exec(query);
    W.commit();
}
