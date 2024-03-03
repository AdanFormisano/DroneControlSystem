#pragma once

#include <pqxx/pqxx>
#include <utility>

class Database {
public:
    Database();

    pqxx::connection C;
    pqxx::work W;

    std::unique_ptr<pqxx::connection>
    con_2_DB(
        const std::string &dbname,
        const std::string &user,
        const std::string &password,
        const std::string &hostaddr,
        const std::string &port);

    pqxx::result qry(
        const std::string &tableName,
        const std::shared_ptr<pqxx::connection> &conn);

    void qry_prnt(
        const std::string &tableName,
        const std::shared_ptr<pqxx::connection> &conn);

    void get_DB();

    void hndl_con(
        std::unique_ptr<pqxx::connection> &conn,
        const std::string &tableName);

    void prnt_tab(const std::string &tableName,
                  const pqxx::result &R);
};
