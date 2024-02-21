#include <spdlog/spdlog.h>
#include <unistd.h>
#include "DroneControl/DroneControl.h"
#include "Drone/Drone.h"
#include <sw/redis++/redis++.h>
#include <iostream>
#include "../utils/RedisUtils.h"

using namespace sw::redis;

int main() {
    std::cout << "PID: " << getpid() << std::endl;
    spdlog::set_pattern("[%T.%e] [Main] [%^%l%$] %v");
    // Fork to create the Drone and DroneControl processes
    pid_t pid_drone_control = fork();
    if (pid_drone_control == -1) {
        spdlog::error("Fork for DroneControl failed");
        return 1;
    } else if (pid_drone_control == 0) {
        // In child DroneControl process
        std::cout << "PID: " << getpid() << std::endl;
        spdlog::set_pattern("[%T.%e] [DroneControl] [%^%l%$] %v");
        spdlog::info("DroneControl process starting");
        drone_control::DroneControl drone_control;
        drone_control.Init();
    } else {
        // In parent process create new child Drone process
        pid_t pid_drone = fork();
        if (pid_drone == -1) {
            spdlog::error("Fork for Drone failed");
            return 1;
        } else if (pid_drone == 0) {
            // In child Drone process
            std::cout << "PID: " << getpid() << std::endl;
            spdlog::set_pattern("[%T.%e] [Drone] [%^%l%$] %v");
            spdlog::info("Drone process starting");
            drones::Init();
            // "Drone process" class here
        } else {
            // In parent process
            // Using Redis sub/pu, parent should wait for completed initialization of DroneControl and Drone
            // Then parent should start the monitor and simulation processes

            // Maybe check if connection is needed to create a subscriber

            // TODO: Maybe use implement a function for process synchronization
            Redis redis("tcp://127.0.0.1:7777");

            auto main_sub = redis.subscriber();
            bool drone_control_init, drone_init = false;

            main_sub.on_message([&drone_control_init, &drone_init](const std::string& channel, const std::string& msg) mutable {
                if (channel == "DroneControl") {
                    if (msg == "INIT_DONE") {
                        spdlog::info("DroneControl initialization message received");
                        drone_control_init = true;
                    } else {
                        spdlog::error("DroneControl failed to initialize");
                    }
                } else if (channel == "Drone") {
                    if (msg == "INIT_DONE") {
                        spdlog::info("Drone initialization message received");
                        drone_init = true;
                    } else {
                        spdlog::error("Drone failed to initialize");
                    }
                }
            });

            main_sub.subscribe("DroneControl", "Drone");
            // Main process should tell DroneControl and Drone when it's ready
            redis.publish("Main", "INIT_DONE");

            while (!drone_init) {
                try {
                    main_sub.consume();
                    std::cout << "Waiting Drone Init" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                } catch (const Error &err) {
                    spdlog::error("Error consuming messages");
                }
            }

            std::cout << "Drone initialization complete" << std::endl;

            // Here should be the monitor and simulation processes (should stay in the main process?)
        }
    }

    return 0;
}
