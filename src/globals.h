#ifndef DRONECONTROLSYSTEM_GLOBALS_H
#define DRONECONTROLSYSTEM_GLOBALS_H
#include <chrono>
#include <string>
#include <array>

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
    // TODO: Add the last time the drone was updated
};

// 1200 is 6000/5 = the size of the area is nicely divisible by 5
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 1200

#endif // DRONECONTROLSYSTEM_GLOBALS_H
