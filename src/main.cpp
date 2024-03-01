#include "../database/Database.h"
#include "../utils/RedisUtils.h"
#include "Drone/Drone.h"
#include "Drone/DroneManager.h"
#include "DroneControl/DroneControl.h"
#include "globals.h"
#include <iostream>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <unistd.h>

using namespace sw::redis;

int main() {
    spdlog::set_pattern("[%T.%e][%^%l%$][Main] %v");

    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pid_drone_control == 0) {
        // In child DroneControl process
        auto drone_control_redis = Redis("tcp://127.0.0.1:7777");
        drone_control_redis.incr(sync_counter_key);

        drone_control::Init(drone_control_redis);
    } else {
        // In parent process create new child Drones process
        pid_t pid_drone = fork();
        if (pid_drone == -1) {
            spdlog::error("Fork for Drone failed");
            return 1;
        } else if (pid_drone == 0) {
            // In child Drones process
            auto drone_redis = Redis("tcp://127.0.0.1:7777");
            drone_redis.incr(sync_counter_key);

            drones::Init(drone_redis);
        } else {
            // In Main process
            auto main_redis = Redis("tcp://127.0.0.1:7777");
            main_redis.incr(sync_counter_key);

            Database db;
            db.getDabase();
            // db.TestDatabase();

            auto conn = db.connectToDatabase("dcs", "postgres", "admin@123", "127.0.0.1", "5432");
            if (conn) {
                std::shared_ptr<pqxx::connection> shared_conn = std::move(conn);
                db.executeQueryAndPrintTable("droni", shared_conn);
                // db.executeQuery("droni", shared_conn);
            } else {
                // Handle connection error
            }

            // Initialization finished
            utils::SyncWait(main_redis);

            // Here should be the monitor and simulation processes (should stay in the main process?)

            // FIXME: This is a placeholder for the monitor process, without it the main process will exit and
            //  the children will be terminated
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << "Exiting..." << std::endl;
        }
    }

    return 0;
}
