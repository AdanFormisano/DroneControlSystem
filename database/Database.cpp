#include "Database.h"
#include <iomanip>
#include <iostream>

// DB constructor
Database::Database()
    : C("user=postgres \
         password=admin@123 \
         hostaddr=127.0.0.1 \
         port=5432"),
      W(pqxx::work(C)) {
    // // Connect to DB
    // std::cout << "\nConnecting to DB..."
    //           << std::endl
    //           << "-------";
}

// Connect to DB
std::unique_ptr<pqxx::connection>
Database::con_2_DB(
    const std::string &dbname,
    const std::string &user,
    const std::string &password,
    const std::string &hostaddr,
    const std::string &port) {
    std::string
        connectionString = "dbname=" + dbname +
                           " user=" + user +
                           " password=" + password +
                           " hostaddr=" + hostaddr +
                           " port=" + port;
    auto conn = std::make_unique<pqxx::connection>(connectionString);

    if (!conn->is_open()) {
        std::cerr << "DB can't connect" << std::endl;
        return nullptr;
    }

    std::cout << "\nDB connected: " << conn->dbname() << std::endl
              << std::endl;
    return conn;
}

// Get or create DB
void Database::get_DB() {
    try {
        W.commit();

        // Non-transactional way to execute query
        pqxx::nontransaction N(C);

        // Does target DB exist?
        std::string db_name = "dcs";
        pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='" + db_name + "'");

        if (R.empty()) {
            // DB doesn't exist: create it
            std::cout << "DB doesn't exist. Creating it..." << std::endl;

            // Connect again to the DB because you can't create
            // a DB within a transaction block of another DB
            N.exec("CREATE DATABASE " + db_name);
            N.commit();
            pqxx::connection C("dbname=dcs \
                                user=postgres \
                                password=admin@123 \
                                hostaddr=127.0.0.1 \
                                port=5432");
            pqxx::work W(C);

            std::cout << "DB created"
                      << std::endl;
        } else {
            std::cout << "\nDB already exists"
                      << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

// Query
pqxx::result Database::qry(
    const std::string &tableName,
    const std::shared_ptr<pqxx::connection> &conn) {
    pqxx::work W(*conn);
    std::string sql = "SELECT * FROM " + tableName;
    pqxx::result R = W.exec(sql);
    W.commit();
    return R;
}

// Query and print
void Database::qry_prnt(
    const std::string &tableName,
    const std::shared_ptr<pqxx::connection> &conn) {
    pqxx::result R = qry(tableName, conn);
    prnt_tab(tableName, R);
}

// Handle connection
void Database::hndl_con(
    std::unique_ptr<pqxx::connection> &conn,
    const std::string &tableName) {
    if (conn) {
        std::shared_ptr<pqxx::connection> shared_conn = std::move(conn);
        qry_prnt(tableName, shared_conn);
    } else {
        std::cerr << "Connection is not valid." << std::endl;
        // Handle connection error
    }
}

// Print table
void Database::prnt_tab(
    const std::string &tableName,
    const pqxx::result &R) {

    // Columns width
    int colWidth = 12;

    // Table width
    int tableWidth = R.columns() * colWidth;

    // Table title
    std::string tableTitle = "--------------> " + tableName + " <-------------";
    int padding = (tableWidth - tableTitle.length()) / 2;

    // Print table title
    std::cout << std::setw(padding + tableTitle.length())
              << tableTitle << std::endl;

    // Print column names
    for (int i = 0; i < R.columns(); ++i) {
        std::cout << std::left
                  << std::setw(colWidth)
                  << R.column_name(i);
    }
    std::cout << std::endl;

    // Print rows data
    for (const auto &row : R) {
        for (int i = 0; i < row.size(); ++i) {
            std::cout << std::left
                      << std::setw(colWidth)
                      << row[i].c_str();
        }
        std::cout << std::endl;
    }

    // Final row
    std::cout << std::setfill('-')
              << std::setw(tableWidth)
              << "-" << std::endl
              << std::endl;
}

void Database::logDroneData(
    const std::map<std::string, std::string> &droneData,
    const std::shared_ptr<pqxx::connection> &conn) {
    pqxx::work W(*conn);

    // Convert timestamp to proper format
    long long timestamp = std::stoll(droneData.at("latestStatusUpdateTime"));
    auto time_in_s = timestamp / 1000000; // timestamp is in microseconds
    std::time_t time = static_cast<time_t>(time_in_s);

    std::string sql = "INSERT INTO drone_logs \
                       (time, drone_id, status, charge, zone, x, y) VALUES (" +
                      W.quote(time) + ", " +
                      W.quote(droneData.at("id")) + ", " +
                      W.quote(droneData.at("status")) + ", " +
                      W.quote(droneData.at("charge")) + ", " + "NULL, " +
                      W.quote(droneData.at("X")) + ", " +
                      W.quote(droneData.at("Y")) + ")";

    W.exec0(sql);
    W.commit();
}
