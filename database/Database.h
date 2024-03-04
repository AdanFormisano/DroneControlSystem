#pragma once

#include <map>
#include <memory>
#include <pqxx/pqxx>
#include <string>

class Database {
public:
    Database();

    void get_DB();

    void prnt_tab_all(const std::string &tableName);

    void hndl_con(
        std::unique_ptr<pqxx::connection> &conn,
        const std::string &tableName);

    void logDroneData(
        const std::map<std::string, std::string> &droneData);

private:
    std::shared_ptr<pqxx::connection> conn;

    void connect_to_db(
        const std::string &dbname,
        const std::string &user,
        const std::string &password,
        const std::string &hostaddr,
        const std::string &port);

    pqxx::result qry(
        const std::string &tableName);

    void qry_prnt(
        const std::string &tableName);

    void prnt_tab(
        const std::string &tableName,
        const pqxx::result &R);
};
