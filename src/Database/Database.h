#pragma once
#include <memory>
#include <pqxx/pqxx>
#include <string>

class Database {

    struct DBCredentials {
        std::string dbname;
        std::string user;
        std::string password;
        std::string hostaddr;
        std::string port;
    };

public:
    void get_DB();
    void CreateDB(const DBCredentials&);
    void CreateTables();
    void ConnectToDB();
    void ExecuteQuery(const std::string &query) const;
    pqxx::connection &getConnection() { return *conn; }
    DBCredentials ReadCredentialsFromConfig();

private:
    std::unique_ptr<pqxx::connection> conn;
};
