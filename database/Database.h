#pragma once

#include <pqxx/pqxx>
#include <utility>

class Database {
public:
    Database();

    pqxx::connection C;
    pqxx::work W;

    std::unique_ptr<pqxx::connection> connectToDatabase(const std::string &dbname, const std::string &user, const std::string &password, const std::string &hostaddr, const std::string &port);

    pqxx::result executeQuery(const std::string &tableName, const std::shared_ptr<pqxx::connection> &conn);
    void executeQueryAndPrintTable(const std::string &tableName, const std::shared_ptr<pqxx::connection> &conn);

    void getDabase();
    // void TestDatabase();
    void printTable(const std::string &tableName,
                    const pqxx::result &R);
};
