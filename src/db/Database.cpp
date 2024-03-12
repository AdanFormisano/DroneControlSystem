#include "Database.h"
#include <iomanip>
#include <iostream>
#include <pqxx/pqxx>

// Constructor
Database::Database() {
    // La connessione non viene più
    // stabilita nel costruttore
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
    conn = std::
        make_shared<pqxx::connection>(connectionString);

    if (!conn->is_open()) {
        std::cerr << "DB can't connect" << std::endl;
        throw std::runtime_error("DB can't connect");
    }
    std::cout << "DB connected: " << conn->dbname()
              << std::endl;
}

// Get or create db
void Database::get_DB() {
    if (!conn || !conn->is_open()) {
        connect_to_db("dcs", "postgres", "admin@123",
                      "127.0.0.1", "5432");
    }
    pqxx::work W(*conn);
    W.exec("CREATE TABLE IF NOT EXISTS drone_logs ("
           "time TIMESTAMP, "
           "drone_id INT NOT NULL PRIMARY KEY, "
           "status VARCHAR(255), "
           "charge INT, "
           "zone VARCHAR(255), " // Vuota per ora
           "x INT, "
           "y INT"
           ")");
    W.commit();
}

// Do query
pqxx::result Database::qry(
    const std::string &tableName) {
    pqxx::work W(*conn);
    pqxx::result R = W.exec("SELECT * FROM " + tableName);
    W.commit();
    return R;
}

// Do query and print table
void Database::qry_prnt(
    const std::string &tableName) {
    pqxx::result R = qry(tableName);
    prnt_tab(tableName, R);
}

// Handle connection
void Database::hndl_con(
    std::unique_ptr<pqxx::connection> &conn,
    const std::string &tableName) {
    if (!conn) {
        std::cerr << "Invalid database connection."
                  << std::endl;
        return;
    }
    this->conn = std::move(conn); // Assume this overwrites the shared_ptr conn
    qry_prnt(tableName);
}

// Print table
void Database::prnt_tab(
    const std::string &tableName,
    const pqxx::result &R) {
    int colWidth = 12;
    int tableWidth = R.columns() * colWidth;
    std::string tableTitle = "------> " + tableName + " <------";
    int padding = (tableWidth - tableTitle.length()) / 2;

    std::cout << std::setw(padding + tableTitle.length())
              << tableTitle << std::endl;

    for (int i = 0; i < R.columns(); ++i) {
        std::cout << std::left
                  << std::setw(colWidth)
                  << R.column_name(i);
    }

    std::cout << std::endl;

    for (const auto &row : R) {
        for (int i = 0; i < row.size(); ++i) {
            std::cout << std::left
                      << std::setw(colWidth)
                      << row[i].c_str();
        }
        std::cout << std::endl;
    }

    std::cout << std::setfill('-')
              << std::setw(tableWidth)
              << "-" << std::endl
              << std::endl;
}

// Print all from table
void Database::prnt_tab_all(const std::string &tableName) {
    // Assumi che la connessione sia stata già stabilita da get_DB().
    if (!conn || !conn->is_open()) {
        std::cerr << "Database connection is not open."
                  << std::endl;
        return;
    }

    try {
        pqxx::work W(*conn);
        pqxx::result R = W.exec("SELECT * FROM " + tableName);
        W.commit();
        prnt_tab(tableName, R);
    } catch (const std::exception &e) {
        std::cerr << "Failed to print table: "
                  << e.what() << std::endl;
    }
}

// Log drone data
void Database::logDroneData(const drone_control::drone_data &drone) {
    if (!conn || !conn->is_open()) {
        std::cerr << "Db connection not established for logging."
                  << std::endl;
        return;
    }

    pqxx::work W(*conn);
    try {
        // Query prep to insert drone data in drone_logs
        std::string query = "INSERT INTO drone_logs (drone_id, status, charge, x, y) VALUES (" +
                            std::to_string(drone.id) + ", " +
                            W.quote(drone.status) + ", " +
                            W.quote(drone.charge) + ", " +
                            std::to_string(drone.position.first) + ", " +
                            std::to_string(drone.position.second) + ");";

        W.exec(query);
        W.commit();
    } catch (const std::exception &e) {
        std::cerr << "Error logging drone data: "
                  << e.what() << std::endl;
        W.abort();
    }
}