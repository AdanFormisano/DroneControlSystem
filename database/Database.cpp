#include "Database.h"
#include <iomanip>
#include <iostream>

Database::Database() : C("user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432"), W(pqxx::work(C)) {
    // // Connect to the maintenance database
    // std::cout << "\nConnecting to DB..."
    //           << std::endl
    //           << "-------";
}

// Connect to DB
std::unique_ptr<pqxx::connection> Database::connectToDatabase(const std::string &dbname, const std::string &user, const std::string &password, const std::string &hostaddr, const std::string &port) {
    auto conn = std::make_unique<pqxx::connection>("dbname = " + dbname +
                                                   " user = " + user + " password = " + password +
                                                   " hostaddr = " + hostaddr +
                                                   " port = " + port);
    if (conn && conn->is_open()) {
        std::cout << "\nDB connected: "
                  << conn->dbname() << std::endl
                  << std::endl;
        return conn;
    } else {
        std::cout << "\nDB can't connect" << std::endl;
        return nullptr;
    }
}

void Database::getDabase() {
    try {
        W.commit();

        // Non-transactional way to execute query
        pqxx::nontransaction N(C);

        // Check if the target database exists
        std::string db_name = "dcs";
        pqxx::result R = N.exec("SELECT 1 FROM pg_database WHERE datname='" + db_name + "'");

        if (R.empty()) {
            // The database does not exist, so create it
            std::cout << "Database does not exist. Creating database..." << std::endl;

            // Disconnect from the current database to avoid issues with creating a new database in a transaction block

            // Connect again to the maintenance database because you can't create a database within a transaction block of another database
            N.exec("CREATE DATABASE " + db_name);
            N.commit();
            pqxx::connection C("dbname=dcs user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432");
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

pqxx::result Database::executeQuery(const std::string &tableName, const std::shared_ptr<pqxx::connection> &conn) {
    pqxx::work W(*conn);
    std::string sql = "SELECT * FROM " + tableName;
    pqxx::result R = W.exec(sql);
    W.commit();
    return R;
}

void Database::executeQueryAndPrintTable(const std::string &tableName, const std::shared_ptr<pqxx::connection> &conn) {
    pqxx::result R = executeQuery(tableName, conn);
    printTable(tableName, R);
}

void Database::printTable(const std::string &tableName, const pqxx::result &R) {
    // Columns width
    int colWidth = 12;

    // Table width
    int tableWidth = R.columns() * colWidth;

    // Table title
    std::string tableTitle = "--------------> " + tableName + " <-------------";
    int padding = (tableWidth - tableTitle.length()) / 2;

    // Print table title
    std::cout << std::setw(padding + tableTitle.length()) << tableTitle << std::endl;

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

// pqxx::work W(C);

// // SQL query
// std::string sql = "SELECT * FROM " + tableName;

// // Execute query
// pqxx::result R = W.exec(sql);

// printTable(tableName, R);

// // Close transaction
// W.commit();

// // Close connection
// C.disconnect();
// }

// void Database::TestDatabase() {
//     Postgres connection try {
//         pqxx::connection C("dbname = dcs\
//                                 user = postgres\
//                                 password = admin@123 \
//                                 hostaddr = 127.0.0.1\
//                                 port = 5432");
//         if (C.is_open()) {
//             std::cout << "\nDB connected: "
//                       << C.dbname()
//                       << std::endl
//                       << std::endl;
//         } else {
//             std::cout << "\nDB can't connect"
//                       << std::endl;
//         }

//         // SQL transaction
//         pqxx::work W(C);

//         // Table name
//         std::string tab_name = "droni";

//         // SQL query
//         std::string sql = "SELECT * FROM " + tab_name;

//         // Exec query
//         pqxx::result R = W.exec(sql);

//         // printTable(tab_name, R);

//         // Close transaction
//         W.commit();

//         // Close connection
//         C.disconnect();

//     } catch (const std::exception &e) {
//         std::cerr << e.what() << std::endl;
//     }
// }
