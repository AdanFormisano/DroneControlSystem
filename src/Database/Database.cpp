#include "Database.h"

#include <iostream>

// Constructor
Database::Database() {
    spdlog::info("Database object created");
}

void Database::ConnectToDB(const std::string &dbname,
                           const std::string &user,
                           const std::string &password,
                           const std::string &hostaddr,
                           const std::string &port) {
    connect_to_db(dbname, user, password, hostaddr, port);
}

// Connect to db
void Database::connect_to_db(const std::string &dbname,
                             const std::string &user,
                             const std::string &password,
                             const std::string &hostaddr,
                             const std::string &port) {
    std::string connectionString = "dbname=" + dbname +
                                   " user=" + user +
                                   " password=" + password +
                                   " hostaddr=" + hostaddr +
                                   " port=" + port;
    conn = std::make_unique<pqxx::connection>(connectionString);

    if (!conn->is_open()) {
#ifdef DEBUG
        spdlog::error("DB can't connect");
#endif
        throw std::runtime_error("DB can't connect");
    }
#ifdef DEBUG
    spdlog::info("DB connected to {}", dbname);
#endif
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
            pqxx::work W(C);
            W.exec("CREATE DATABASE dcs");
            W.commit();
        }
        // Connession al DB 'dcs'
        connect_to_db("dcs", "postgres", "admin@123", "127.0.0.1", "5432");

        // Se la connessione è stata stabilita, sovrascrivere la tabella 'drone_logs'
        if (conn && conn->is_open()) {
            pqxx::work W(*conn);
            // Eliminare la tabella se esiste
            W.exec("DROP TABLE IF EXISTS drone_logs");
            // Ricreare la tabella
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
