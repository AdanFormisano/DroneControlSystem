#include "Database.h"
#include <iostream>

Database::Database() : C("user=postgres password=admin@123 hostaddr=127.0.0.1 port=5432"), W(pqxx::work(C)) {
    // Connect to the maintenance database
    std::cout << "Connecting to postgres database" << std::endl;
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

            std::cout << "Database created successfully" << std::endl;
        } else {
            std::cout << "Database already exists." << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

void Database::TestDatabase() {
    // Postgres connection
    {
        try {
            pqxx::connection C("dbname = dcs user = postgres password = admin@123 hostaddr = 127.0.0.1 port = 5432");
            if (C.is_open()) {
                std::cout << "DB: opened: " << C.dbname() << std::endl;
            } else {
                std::cout << "DB: can't open" << std::endl;
            }

            // SQL transaction
            pqxx::work W(C);

            // SQL query
            std::string sql = "SELECT * FROM droni";

            // Execute SQL query
            pqxx::result R = W.exec(sql);

            // Print result
            {
                // for (const auto &row : R) {
                //     std::cout << "DB: " << row[0].c_str() << " " << row[1].c_str() << " " << row[2].c_str() << " " << row[3].c_str() << std::endl;
                // }
            }

            // Print alternative
            for (const auto &row : R) {
                std::cout << "Column 1: " << row[0].as<std::string>() << std::endl;
                std::cout << "Column 2: " << row[1].as<std::string>() << std::endl;
                std::cout << "Column 3: " << row[2].as<std::string>() << std::endl;
            }

            // Close transaction
            W.commit();

            // Close connection
            C.disconnect();

            // Print DB interaction success
            std::cout << "DB interaction succesfully" << std::endl;

        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }
}
