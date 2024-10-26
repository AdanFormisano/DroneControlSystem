#pragma once
#include <memory>
#include <pqxx/pqxx>
#include <string>

class Database {

public:
    void ConnectToDB(
        const std::string &dbname,
        const std::string &user,
        const std::string &password,
        const std::string &hostaddr,
        const std::string &port);

    void ExecuteQuery(
        const std::string &query) const;

    pqxx::connection &getConnection() { return *conn; }

    void CreateDB(
        const std::string &dbname,
        const std::string &user,
        const std::string &password,
        const std::string &hostaddr,
        const std::string &port);

    void CreateTables();

    void get_DB();

    std::tuple<std::string,
               std::string,
               std::string,
               std::string,
               std::string>
    ReadCredentialsFromConfig();

private:
    std::unique_ptr<pqxx::connection> conn;

    void ConnectToDB_();
};
