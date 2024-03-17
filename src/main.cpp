#include "../utils/RedisUtils.h"
#include "Drone/DroneManager.h"
#include "DroneControl/DroneControl.h"
#include "ChargeBase/ChargeBase.h"
#include "db/Database.h"
#include "globals.h"
#include "GUI/UI.h"
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
            // In parent process create new child ChargeBase process
            pid_t pid_charge_base = fork();
            if (pid_charge_base == -1) {
                spdlog::error("Fork for ChargeBase failed");
                return 1;
            } else if (pid_charge_base == 0) {
                // In child ChargeBase process
                auto charge_base_redis = Redis("tcp://127.0.0.1:7777");
                charge_base_redis.incr(sync_counter_key);

                // Create the ChargeBase object
                auto cb = charge_base::ChargeBase::getInstance(charge_base_redis);
                // cb->SetRedis(charge_base_redis);

                // Set the charge rate for the charging slots
                thread_local std::random_device rd;
                cb->SetEngine(rd);

                utils::SyncWait(charge_base_redis);

                // Start simulation
                cb->Run();

            } else {

                // In Main process
                auto main_redis = Redis("tcp://127.0.0.1:7777");
                main_redis.incr(sync_counter_key);


                // DB obj
                Database db;

                // DB get or create
                // db.get_DB();

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

                // FIXME: This is a placeholder for the monitor process,
                // without it the main process will exit and
                // the children will be terminated
                std::this_thread::sleep_for(std::chrono::seconds(10));

                std::cout << "Exiting..." << std::endl;
            }
        }
    }
    return 0;
}
