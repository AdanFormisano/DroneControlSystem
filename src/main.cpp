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
#include "../utils/utils.h"
#include "ChargeBase/ChargeBase.h"
#include "DroneControl/DroneControl.h"
#include "Monitors/Monitor.h"
#include "TestGenerator/TestGenerator.h"
#include "globals.h"
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <iostream>

#include "DroneControl/DroneControl.h"
#include "Scanner/ScannerManager.h"

using namespace sw::redis;


int main()
{
    spdlog::set_pattern("[%T.%e][%^%l%$][Main] %v");

    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 7777;
    connection_options.socket_timeout = std::chrono::milliseconds(3000);

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
        auto drone_control_redis = Redis(connection_options);

        // Create the DroneControl object
        DroneControl dc(drone_control_redis);
        DroneControl sdc(drone_control_redis);

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
            ConnectionPoolOptions drone_connection_pool_options;
            drone_connection_pool_options.size = 8;
            drone_connection_pool_options.wait_timeout = std::chrono::milliseconds(1000);

            auto drone_redis = Redis(connection_options, drone_connection_pool_options);

            // Create the DroneManager object
            // drones::DroneManager dm(drone_redis);
            // ScannerManager sm(drone_redis);
            ScannerManager ssm(drone_redis);

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
                auto charge_base_redis = Redis(connection_options);

                // Create the ChargeBase object
                auto cb = ChargeBase::getInstance(charge_base_redis);

                // Set the charge rate for the charging slots
                thread_local std::random_device rd;
                cb->SetEngine(rd); // TODO: Check if every slot has the same charge rate

                // Start simulation
                cb->Run();
            }
            else
            {
                // In parent process create new child TestGenerator process
                pid_t pid_test_generator = fork();
                if (pid_test_generator == -1) {
                    spdlog::error("Fork for TestGenerator failed");
                    return 1;
                } else if (pid_test_generator == 0) {   // In child TestGenerator process
                    spdlog::set_pattern("[%T.%e][%^%l%$][TestGenerator] %v");

                    // Create the Redis object for TestGenerator
                    auto test_redis = Redis(connection_options);

                    // Create the TestGenerator object
                    TestGenerator tg(test_redis);

                    spdlog::info("TestGenerator started");

                    // Start test generation
                    tg.Run();
                }
                else
                {
                    // In Main process
                    auto main_redis = Redis(connection_options);

                    // Start monitors
                    RechargeTimeMonitor rtm(main_redis);
                    CoverageMonitor zcm(main_redis);
                    DroneChargeMonitor dcm(main_redis);
                    TimeToReadDataMonitor trd(main_redis);
                    // rtm.RunMonitor(); //TODO: Bring out the thread from inside the function
                    // zcm.RunMonitor(); //TODO: Bring out the thread from inside the function
                    // dcm.RunMonitor();
                    // trd.RunMonitor();

                    // Create named semaphore for synchronization
                    sem_t* sem_sync_dc = utils_sync::create_or_open_semaphore("/sem_sync_dc", 0);   // Used for tick synchronization
                    sem_t* sem_sync_sc = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);   // Used for tick synchronization
                    sem_t* sem_sync_cb = utils_sync::create_or_open_semaphore("/sem_sync_cb", 0);   // Used for tick synchronization
                    sem_t* sem_dc = utils_sync::create_or_open_semaphore("/sem_dc", 0);       // Used for DroneControl end of tick synchronization
                    sem_t* sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0);       // Used for ScannerManager end of tick synchronization
                    sem_t* sem_cb = utils_sync::create_or_open_semaphore("/sem_cb", 0);       // Used for ChargeBase end of tick synchronization

                    // Start simulation
                    auto sim_end_after = sim_duration_ms / tick_duration_ms;
                    int tick_n = 0;

                    // Simulation loop
                    while (tick_n < sim_duration_ticks)
                    {
                        // spdlog::info("TICK: {}", tick_n);
                        std::cout << "[Main] TICK: " << tick_n << std::endl;
                        // Release sem_sync to start the next tick
                        sem_post(sem_sync_dc); // Signal DroneControl
                        sem_post(sem_sync_sc); // Signal ScannerManager
                        sem_post(sem_sync_cb); // Signal ChargeBase

                        // Wait for all processes to finish the tick
                        sem_wait(sem_dc);
                        sem_wait(sem_sc);
                        sem_wait(sem_cb);

                        ++tick_n;
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));     // <-- Use this to slow down the simulation and check if the system is actuallyy synced
                        spdlog::info("=====================================");
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

                    // Kill the children
                    kill(pid_drone_control, SIGTERM);
                    kill(pid_drone, SIGTERM);
                    kill(pid_charge_base, SIGTERM);
                    kill(pid_test_generator, SIGTERM);

                    std::this_thread::sleep_for(std::chrono::seconds(10));
                    std::cout << "Exiting..." << std::endl;
                    // }
                }
            }
        }
    }
    return 0;
}
