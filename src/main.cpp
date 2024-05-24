#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <unistd.h>

#include <iostream>

#include "../utils/RedisUtils.h"
#include "ChargeBase/ChargeBase.h"
#include "Drone/DroneManager.h"
#include "DroneControl/DroneControl.h"
#include "Monitors/Monitor.h"
#include "TestGenerator/TestGenerator.h"
#include "globals.h"

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
        //        drone_control_redis.incr(sync_counter_key);
        utils::AddThisProcessToSyncCounter(drone_control_redis, "DroneControl");
        spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");

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
            //            drone_redis.incr(sync_counter_key);
            utils::AddThisProcessToSyncCounter(drone_redis, "Drone");
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
                //                charge_base_redis.incr(sync_counter_key);
                utils::AddThisProcessToSyncCounter(charge_base_redis, "ChargeBase");
                // Create the ChargeBase object
                auto cb = charge_base::ChargeBase::getInstance(charge_base_redis);
                // cb->SetRedis(charge_base_redis);

                // Set the charge rate for the charging slots
                thread_local std::random_device rd;
                cb->SetEngine(rd);

                //                utils::SyncWait(charge_base_redis);
                utils::NamedSyncWait(charge_base_redis, "ChargeBase");

                // Start simulation
                cb->Run();

            } else {
                // In parent process create new child TestGenerator process
                pid_t pid_test_generator = fork();
                if (pid_test_generator == -1) {
                    spdlog::error("Fork for TestGenerator failed");
                    return 1;
                } else if (pid_test_generator == 0) {
                    // In child TestGenerator process
                    spdlog::set_pattern("[%T.%e][%^%l%$][TestGenerator] %v");
                    auto test_redis = Redis("tcp://127.0.0.1:7777");

                    // Create the TestGenerator object
                    TestGenerator tg(test_redis);

                    spdlog::info("TestGenerator started");

                    // Start test generation
                    tg.Run();
                } else {
                    // In Main process
                    auto main_redis = Redis("tcp://127.0.0.1:7777");
                    //                main_redis.incr(sync_counter_key);
                    utils::AddThisProcessToSyncCounter(main_redis, "Main");

                    // Wait for the other processes to finish initialization
                    //                utils::SyncWait(main_redis);
                    utils::NamedSyncWait(main_redis, "Main");

                    // Start monitors
                    RechargeTimeMonitor rtm;
                    rtm.RunMonitor();   //TODO: Bring out the thread from inside the function

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

                    // Join monitor's thread
                    rtm.JoinThread();

                    // FIXME: This is a placeholder for the monitor process,
                    // without it the main process will exit and
                    // the children will be terminated
                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    std::cout << "Exiting..." << std::endl;
                }
            }
        }
    }
    return 0;
}
