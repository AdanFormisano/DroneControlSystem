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

#include "../utils/LogUtils.h"
#include "../utils/RedisUtils.h"
#include "../utils/utils.h"
#include "ChargeBase/ChargeBase.h"
#include "DroneControl/DroneControl.h"
#include "Monitors/Monitor.h"
#include "Scanner/ScannerManager.h"
#include "TestGenerator/TestGenerator.h"
#include "globals.h"
#include <iostream>
#include <sw/redis++/redis++.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace sw::redis;

int main() {
    // utils_sync::restart_postgresql();

    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 7777;
    connection_options.socket_timeout = std::chrono::milliseconds(3000);

    // Create the DroneControl process
    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1) {
        log_error("Main", "Fork for DroneControl failed");
        // std::cerr << "Fork for DroneControl failed" << std::endl;
        return 1;
    } else if (pid_drone_control == 0) {
        // In child DroneControl process
        // spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");

        // Create the Redis object for DroneControl
        auto drone_control_redis = Redis(connection_options);

        // Create the DroneControl object
        DroneControl dc(drone_control_redis);

        // Start simulation
        dc.Run();
    } else {
        // In parent process create new child Drones process
        pid_t pid_drone = fork();
        if (pid_drone == -1) {
            log_error("Main", "Fork for Drone failed");
            // std::cerr << "Fork for Drone failed" << std::endl;
            return 1;
        } else if (pid_drone == 0) {
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
        } else {
            // In parent process create new child ChargeBase process
            pid_t pid_charge_base = fork();
            if (pid_charge_base == -1) {
                log_error("Main", "Fork for ChargeBase failed");
                // std::cerr << "Fork for ChargeBase failed" << std::endl;
                return 1;
            } else if (pid_charge_base == 0) {
                // In child ChargeBase process
                // Create the Redis object for ChargeBase
                auto charge_base_redis = Redis(connection_options);

                // Create the ChargeBase object
                auto cb = ChargeBase::getInstance(charge_base_redis);

                // Set the charge rate for the charging slots
                thread_local std::random_device rd;
                cb->SetEngine(); // TODO: Check if every slot has the same charge rate

                // Start simulation
                cb->Run();
            } else {
                // In parent process create new child TestGenerator process
                pid_t pid_test_generator = fork();
                if (pid_test_generator == -1) {
                    log_error("Main", "Fork for TestGenerator failed");
                    // std::cerr << "Fork for TestGenerator failed" << std::endl;
                    return 1;
                } else if (pid_test_generator == 0) {
                    // In child TestGenerator process
                    // spdlog::set_pattern("[%T.%e][%^%l%$][TestGenerator] %v");

                    // Create the Redis object for TestGenerator
                    auto test_redis = Redis(connection_options);

                    // Create the TestGenerator object
                    TestGenerator tg(test_redis);

                    log_tg("TestGenerator started");
                    // std::cout << "TestGenerator started" << std::endl;

                    // Start test generation
                    tg.Run();
                } else {
                    pid_t pid_monitors = fork();
                    // In Main process
                    if (pid_monitors == -1) {
                        log_error("Main", "Fork for Monitors failed");
                        // std::cerr << "Fork for Monitors failed" << std::endl;
                    } else if (pid_monitors == 0) {
                        // In child Monitors process
                        log_monitor("Monitors started");
                        // std::cout << "[Monitor] Monitors started" << std::endl;

                        auto monitors_redis = Redis(connection_options);

                        // Create the Monitors
                        WaveCoverageMonitor cm(monitors_redis);
                        RechargeTimeMonitor rm(monitors_redis);
                        DroneChargeMonitor dm(monitors_redis);

                        // Run the Monitors' thread
                        cm.RunMonitor();
                        rm.RunMonitor();
                        dm.RunMonitor();

                        cm.JoinThread();
                        rm.JoinThread();
                        dm.JoinThread();

                        AreaCoverageMonitor am(monitors_redis);
                        SystemPerformanceMonitor sm(monitors_redis);
                        sm.RunMonitor();
                        am.RunMonitor();
                        sm.JoinThread();
                        am.JoinThread();
                    } else {
                        auto main_redis = Redis(connection_options);

                        // SystemPerformanceMonitor sm(main_redis);
                        // sm.RunMonitor();

                        // Create named semaphore for synchronization
                        sem_t *sem_sync_dc = utils_sync::create_or_open_semaphore("/sem_sync_dc", 0);
                        sem_t *sem_sync_sc = utils_sync::create_or_open_semaphore("/sem_sync_sc", 0);
                        sem_t *sem_sync_cb = utils_sync::create_or_open_semaphore("/sem_sync_cb", 0);
                        sem_t *sem_dc = utils_sync::create_or_open_semaphore("/sem_dc", 0); // Used for DroneControl end of tick synchronization
                        sem_t *sem_sc = utils_sync::create_or_open_semaphore("/sem_sc", 0); // Used for ScannerManager end of tick synchronization
                        sem_t *sem_cb = utils_sync::create_or_open_semaphore("/sem_cb", 0); // Used for ChargeBase end of tick synchronization

                        // Start simulation
                        int tick_n = 0;

                        // Start timewatch
                        auto start_time = std::chrono::high_resolution_clock::now();

                        // log("=====================================");
                        log_dcsa("=====================================");
                        // std::cout << "=====================================" << std::endl;

                        // Simulation loop
                        while (tick_n < sim_duration_ticks) {
                            log_main("TICK: " + std::to_string(tick_n));
                            // std::cout << "[Main] TICK: " << tick_n << std::endl;
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
                            log_dcsa("=====================================");
                        }
                        // Stop time watch
                        auto sim_end_time = std::chrono::high_resolution_clock::now();

                        // Use Redis to stop the simulation
                        main_redis.set("sim_running", "false");

                        // Wait for child processes to finish
                        kill(pid_test_generator, SIGTERM);
                        waitpid(pid_drone_control, nullptr, 0);
                        waitpid(pid_drone, nullptr, 0);
                        waitpid(pid_monitors, nullptr, 0);
                        waitpid(pid_charge_base, nullptr, 0);

                        // sm.JoinThread();

                        // Get shutoff time
                        auto shutoff_time = std::chrono::high_resolution_clock::now();
                        auto shutoff_duration = std::chrono::duration_cast<std::chrono::milliseconds>(shutoff_time - start_time).count();
                        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(sim_end_time - start_time).count();
                        log_main("Simulation duration: " + std::to_string(duration) + " ms");
                        // std::cout << "Simulation duration: " << duration << " ms" << std::endl;
                        log_main("Entire system duration: " + std::to_string(shutoff_duration) + " ms");
                        // std::cout << "Entire system duration: " << shutoff_duration << " ms" << std::endl;

                        std::this_thread::sleep_for(std::chrono::seconds(5));
                        log_main("Finished. You can exit now or stay for logs");
                        // std::cout << "Exiting..." << std::endl;

                        close_logs();
                    }
                }
            }
        }
    }
    return 0;
}
