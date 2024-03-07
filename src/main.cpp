#include "../database/Database.h"
#include "../utils/RedisUtils.h"
#include "Drone/DroneManager.h"
#include "DroneControl/DroneControl.h"
#include "globals.h"
#include <iostream>
#include <pqxx/pqxx>
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <unistd.h>

using namespace sw::redis;

/* The simulation starts after all the process have initiated and are sync to one another.
 * The tick duration is 1 second, and it simulates 2.42 real time seconds (time needed for a drone to move to the next
 * "checkpoint") in 1 tick.
 * */

// FIXME: All simulation should run from the main file
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

        // Create the DroneControl object
        drone_control::DroneControl dc(drone_control_redis);
        // Start simulation
        dc.Run();

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

            // Create the DroneManager object
            drones::DroneManager dm(drone_redis);
            dm.Run();
        } else {
            // In Main process
            auto main_redis = Redis("tcp://127.0.0.1:7777");
            main_redis.incr(sync_counter_key);

            Database db;
            db.getDabase();

            auto conn = db.connectToDatabase("dcs", "postgres", "admin@123", "127.0.0.1", "5432");
            if (conn) {
                std::shared_ptr<pqxx::connection> shared_conn = std::move(conn);
                db.executeQueryAndPrintTable("droni", shared_conn);
                // db.executeQuery("droni", shared_conn);
            } else {
                // TODO: Handle connection error
            }

            // Wait for the other processes to finish initialization
            utils::SyncWait(main_redis);

            // Start simulation
            auto sim_end_after = sim_duration_ms / tick_duration_ms;
            int tick_n = 0;
            while (tick_n < sim_end_after) {
                // Do simulation stuff
                std::cout << "Tick " << tick_n << " started" << std::endl;
                std::this_thread::sleep_for(tick_duration_ms);  // Sleep for 1 tick: 1 second
                std::cout << "Tick " << tick_n << " ended" << std::endl;
                ++tick_n;
            }
            // Use Redis to stop the simulation
            main_redis.set("sim_running", "false");

            // FIXME: This is a placeholder for the monitor process, without it the main process will exit and
            //  the children will be terminated
            std::this_thread::sleep_for(std::chrono::seconds(10));
            std::cout << "Exiting..." << std::endl;
        }
    }
    return 0;
}
