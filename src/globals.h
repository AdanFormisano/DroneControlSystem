#ifndef DRONECONTROLSYSTEM_GLOBALS_H
#define DRONECONTROLSYSTEM_GLOBALS_H
#include <string>
#include <chrono>

// Variables used for synchronization of processes
extern std::string sync_counter_key;
extern std::string sync_channel;

// Simulation settings
extern std::chrono::milliseconds tick_duration_ms;  // duration of 1 tick in milliseconds
extern std::chrono::milliseconds sim_duration_ms;   // duration of the simulation in milliseconds

struct drone_data {
    int id;
    std::string status;
    int charge;
    std::pair<int, int> position;
    // std::string latest_update;  // TODO: Add the last time the drone was updated
    // TODO: Add zoneId
};

#endif //DRONECONTROLSYSTEM_GLOBALS_H
