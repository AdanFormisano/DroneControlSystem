#ifndef DRONECONTROLSYSTEM_GLOBALS_H
#define DRONECONTROLSYSTEM_GLOBALS_H

#include <chrono>
#include <string>
#include <array>

#define DRONE_CONSUMPTION 0.00672f
#define TICK_TIME_SIMULATED 2.42f

// Variables used for synchronization of processes
extern std::string sync_counter_key;
extern std::string sync_channel;

// Simulation settings
extern std::chrono::milliseconds tick_duration_ms;  // duration of 1 tick in milliseconds
extern std::chrono::milliseconds sim_duration_ms;   // duration of the simulation in milliseconds

struct drone_data {
    int id;
    std::string status;
    float charge;
    std::pair<float, float> position;
    int zone_id = 0;
    float charge_needed_to_base;
    // TODO: Add the last time the drone was updated
};
#endif // DRONECONTROLSYSTEM_GLOBALS_H
