#pragma once
#include <spdlog/spdlog.h>

#include <memory>
#include <pqxx/pqxx>
#include <string>

class Database
{
public:
    void ConnectToDB(const std::string& dbname,
                     const std::string& user,
                     const std::string& password,
                     const std::string& hostaddr,
                     const std::string& port);
    void ExecuteQuery(const std::string& query) const;
    pqxx::connection& getConnection() { return *conn; }
    void get_DB();

private:
    std::unique_ptr<pqxx::connection> conn;

    void ConnectToDB_();
};
