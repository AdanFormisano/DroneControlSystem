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

class Database {
   public:
    Database();

    void ConnectToDB(const std::string &dbname,
                     const std::string &user,
                     const std::string &password,
                     const std::string &hostaddr,
                     const std::string &port);
    void get_DB();

    void ExecuteQuery(const std::string &query);
    pqxx::connection& getConnection() { return *conn; }

   private:
    std::unique_ptr<pqxx::connection> conn;

    void connect_to_db(
        const std::string &dbname,
        const std::string &user,
        const std::string &password,
        const std::string &hostaddr,
        const std::string &port);
};
