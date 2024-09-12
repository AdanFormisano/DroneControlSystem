/* The simulation is diveded in 4 processes: Main, DroneControl, Drone, ChargeBase, TestGenerator.
 *  - Main: is the parent process, creates the other 4 processes and starts the monitors.
 *  - DroneControl: manages the system and always has knowledge of the drones' state, it also sends commands to the drones.
 *  - Drone: manages the simulation of the drones, and sends their state to the DroneControl.
 *  - ChargeBase: manages the charging slots and assigns drones to them, manages the charging process.
 *  - TestGenerator: generates test data for the simulation, simulating every possible outcome.
 *
 *  Every process has a Redis connection which is used to sync the processes and to store the simulation status.
 *  Once the process is ready to start the simulation, it adds itself to a set in Redis, and waits for the other processes to do the same.
 *  The simulation starts when all the processes are ready.
 *  The Main process is the one that stops the simulation, by setting the "sim_running" key in Redis to "false".
 *
 * The simulation starts after all the processes have initiated and are sync to one another.
 * The tick duration is 1 second (can be modified in globals.cpp), and it simulates 2.42 real time seconds
 * (time needed for a drone to move to the next "checkpoint") in 1 tick.
 */

#include "../utils/RedisUtils.h"
#include "ChargeBase/ChargeBase.h"
#include "Drone/DroneManager.h"
#include "DroneControl/DroneControl.h"
#include "Monitors/Monitor.h"
#include "TestGenerator/TestGenerator.h"
#include "globals.h"
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <iostream>

#include "DroneControl/SyncedDroneControl.h"
#include "Scanner/SyncedScannerManager.h"

using namespace sw::redis;


int main()
{
    spdlog::set_pattern("[%T.%e][%^%l%$][Main] %v");

    // Create the DroneControl process
    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1)
    {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    }
    else if (pid_drone_control == 0)
    {
        // In child DroneControl process
        spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");

        // Create the Redis object for DroneControl
        auto drone_control_redis = Redis("tcp://127.0.0.1:7777");

        // Sync of processes
        utils::AddThisProcessToSyncCounter(drone_control_redis, "DroneControl");

        // Create the DroneControl object
        drone_control::DroneControl dc(drone_control_redis);
        SyncedDroneControl sdc(drone_control_redis);

        // Start simulation
        // dc.Run();
        sdc.Run();
    }
    else
    {
        // In parent process create new child Drones process
        pid_t pid_drone = fork();
        if (pid_drone == -1)
        {
            spdlog::error("Fork for Drone failed");
            return 1;
        }
        else if (pid_drone == 0)
        {
            // In child Drones process
            // Create the Redis object for Drone
            ConnectionOptions drone_connection_options;
            drone_connection_options.host = "127.0.0.1";
            drone_connection_options.port = 7777;

            ConnectionPoolOptions drone_connection_pool_options;
            drone_connection_pool_options.size = 8;
            drone_connection_pool_options.wait_timeout = std::chrono::milliseconds(1000);

            auto drone_redis = Redis(drone_connection_options, drone_connection_pool_options);

            // Sync of processes
            utils::AddThisProcessToSyncCounter(drone_redis, "Drone");

            // Create the DroneManager object
            // drones::DroneManager dm(drone_redis);
            // ScannerManager sm(drone_redis);
            SyncedScannerManager ssm(drone_redis);

            // Start simulation
            // dm.Run();
            ssm.Run();
        }
        else
        {
            // In parent process create new child ChargeBase process
            pid_t pid_charge_base = fork();
            if (pid_charge_base == -1)
            {
                spdlog::error("Fork for ChargeBase failed");
                return 1;
            }
            else if (pid_charge_base == 0)
            {
                // In child ChargeBase process
                // Create the Redis object for ChargeBase
                auto charge_base_redis = Redis("tcp://127.0.0.1:7777");

                // Sync of processes
                // utils::AddThisProcessToSyncCounter(charge_base_redis, "ChargeBase");

                // Create the ChargeBase object
                auto cb = charge_base::ChargeBase::getInstance(charge_base_redis);

                // Set the charge rate for the charging slots
                thread_local std::random_device rd;
                cb->SetEngine(rd); // TODO: Check if every slot has the same charge rate

                // Sync of processes
                utils::NamedSyncWait(charge_base_redis, "ChargeBase");

                // Start simulation
                // cb->Run();
            }
            // else {    // In parent process create new child TestGenerator process
            //     pid_t pid_test_generator = fork();
            //     if (pid_test_generator == -1) {
            //         spdlog::error("Fork for TestGenerator failed");
            //         return 1;
            //     } else if (pid_test_generator == 0) {   // In child TestGenerator process
            //         spdlog::set_pattern("[%T.%e][%^%l%$][TestGenerator] %v");
            //
            //         // Create the Redis object for TestGenerator
            //         auto test_redis = Redis("tcp://127.0.0.1:7777");
            //
            //         // Create the TestGenerator object
            //         TestGenerator tg(test_redis);
            //
            //         spdlog::info("TestGenerator started");
            //
            //         // Start test generation
            //         tg.Run();
            //     }
            else
            {
                // In Main process
                auto main_redis = Redis("tcp://127.0.0.1:7777");

                // Sync of processes
                utils::AddThisProcessToSyncCounter(main_redis, "Main");
                utils::NamedSyncWait(main_redis, "Main");

                // Start monitors
                RechargeTimeMonitor rtm(main_redis);
                CoverageMonitor zcm(main_redis);
                DroneChargeMonitor dcm(main_redis);
                TimeToReadDataMonitor trd(main_redis);
                // rtm.RunMonitor(); //TODO: Bring out the thread from inside the function
                // zcm.RunMonitor(); //TODO: Bring out the thread from inside the function
                // dcm.RunMonitor();
                // trd.RunMonitor();

                // Start simulation
                auto sim_end_after = sim_duration_ms / tick_duration_ms;
                int tick_n = 0;
                while (tick_n < sim_end_after)
                {
                    // Do simulation stuff
                    // std::cout << "Tick " << tick_n << " started" << std::endl;
                    std::this_thread::sleep_for(tick_duration_ms); // Sleep for 1 tick: 1 second
                    // std::cout << "Tick " << tick_n << " ended" << std::endl;
                    ++tick_n;
                }

                // Use Redis to stop the simulation
                main_redis.set("sim_running", "false");

                // Join monitor's thread
                rtm.JoinThread();
                zcm.JoinThread();
                dcm.JoinThread();
                trd.JoinThread();

                // FIXME: This is a placeholder for the monitor process,
                // without it the main process will exit and
                // the children will be terminated
                std::this_thread::sleep_for(std::chrono::seconds(10));
                std::cout << "Exiting..." << std::endl;
                // }
            }
        }
    }
    return 0;
}
