#pragma once
#include <spdlog/spdlog.h>

#include <boost/thread.hpp>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <queue>
#include <string>
#include <thread>

#include "../globals.h"

class Database
{
public:
    void ConnectToDB(const std::string& dbname,
                     const std::string& user,
                     const std::string& password,
                     const std::string& hostaddr,
                     const std::string& port);
    void ExecuteQuery(const std::string& query);
    pqxx::connection& getConnection() { return *conn; }
    void get_DB();

private:
    std::unique_ptr<pqxx::connection> conn;

    void ConnectToDB_();
};
