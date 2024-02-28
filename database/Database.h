#pragma once

#include <pqxx/pqxx>
#include <utility>

class Database {
    public:
        Database();

        pqxx::connection C;
        pqxx::work W;

        void getDabase();
        void TestDatabase();
};
