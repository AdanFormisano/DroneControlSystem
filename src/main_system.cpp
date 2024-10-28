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
#include <sys/wait.h>
#include "Scanner/ScannerManager.h"

using namespace sw::redis;


int main()
{
    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 7777;
    connection_options.socket_timeout = std::chrono::milliseconds(3000);

    pid_t pid_test_generator = fork();
    if (pid_test_generator == -1)
    {
        // spdlog::error("Fork for TestGenerator failed");
        std::cerr << "Fork for TestGenerator failed" << std::endl;
        return 1;
    }
    else if (pid_test_generator == 0)
    {
        // In child TestGenerator process
        spdlog::set_pattern("[%T.%e][%^%l%$][TestGenerator] %v");

        // Create the Redis object for TestGenerator
        auto test_redis = Redis(connection_options);

        // Create the TestGenerator object
        TestGenerator tg(test_redis);

        // spdlog::info("TestGenerator started");
        std::cout << "[TestGenerator] Started" << std::endl;

        // Start test generation
        tg.Run();
    }
    else
    {
        pid_t pid_monitors = fork();
        // In Main process
        if (pid_monitors == -1)
        {
            std::cerr << "Fork for Monitors failed" << std::endl;
        }
        else if (pid_monitors == 0)
        {
            // In child Monitors process
            std::cout << "[Monitor] Monitors started" << std::endl;

            auto monitors_redis = Redis(connection_options);

            // Create the Monitors
            CoverageMonitor cm(monitors_redis);
            DroneChargeMonitor dcm(monitors_redis);
            RechargeTimeMonitor rtm(monitors_redis);

            // Run the Monitors' thread
            cm.RunMonitor();
            dcm.RunMonitor();
            rtm.RunMonitor();


            // Wait for the Monitors to finish
            cm.JoinThread();
            dcm.JoinThread();
            rtm.JoinThread();

            std::cout << "[Monitor] Monitors mostly finished..." << std::endl;
            SystemPerformanceMonitor spm(monitors_redis);
            spm.RunMonitor();
            spm.JoinThread();
        }
        else
        {
            auto main_redis = Redis(connection_options);

            // Create named semaphore for synchronization
            sem_t* sem_sync_dc = utils_sync::create_or_open_semaphore("/sem_sync_dc", 0);
            sem_t* sem_sync_sc = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);
            sem_t* sem_sync_cb = utils_sync::create_or_open_semaphore("/sem_sync_cb", 0);
            sem_t* sem_dc = utils_sync::create_or_open_semaphore("/sem_dc", 0);
            // Used for DroneControl end of tick synchronization
            sem_t* sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0);
            // Used for ScannerManager end of tick synchronization
            sem_t* sem_cb = utils_sync::create_or_open_semaphore("/sem_cb", 0);
            // Used for ChargeBase end of tick synchronization

            // Start simulation
            int tick_n = 0;

            // Start timewatch
            auto start_time = std::chrono::high_resolution_clock::now();

            // std::cout << "=====================================" << std::endl;

            // Simulation loop
            while (tick_n < sim_duration_ticks)
            {
                // spdlog::info("TICK: {}", tick_n);
                std::cout << "[Main] TICK: " << tick_n << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                // Release sem_sync to start the next tick
                sem_post(sem_sync_dc); // Signal DroneControl
                sem_post(sem_sync_sc); // Signal ScannerManager
                sem_post(sem_sync_cb); // Signal ChargeBase

                // Wait for all processes to finish the tick
                sem_wait(sem_dc);
                sem_wait(sem_sc);
                sem_wait(sem_cb);

                ++tick_n;
                // std::cout << "=====================================" << std::endl;
            }
            // Stop time watch
            auto sim_end_time = std::chrono::high_resolution_clock::now();

            // Remove semaphores
            sem_unlink("/sem_sync_dc");
            sem_unlink("/sem_sync_sc");
            sem_unlink("/sem_sync_cb");
            sem_unlink("/sem_dc");
            sem_unlink("/sem_sc");
            sem_unlink("/sem_cb");

            // Use Redis to stop the simulation
            main_redis.set("sim_running", "false");


            // Wait for child processes to finish
            kill(pid_test_generator, SIGTERM);

            int status_monitors;
            waitpid(pid_monitors, &status_monitors, 0);

            if (WIFEXITED(status_monitors))
            {
                std::cout << "Monitors exited with status " << WEXITSTATUS(status_monitors) << std::endl;
            }

            // Get shutoff time
            auto shutoff_time = std::chrono::high_resolution_clock::now();
            auto shutoff_duration = std::chrono::duration_cast<std::chrono::milliseconds>(shutoff_time - start_time).
                count();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sim_end_time - start_time).count();
            std::cout << "Simulation duration: " << duration << " ms" << std::endl;
            std::cout << "Entire system duration: " << shutoff_duration << " ms" << std::endl;

            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Exiting..." << std::endl;
        }
    }
    return 0;
}
