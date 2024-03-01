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

#endif //DRONECONTROLSYSTEM_GLOBALS_H
