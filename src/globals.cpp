#include "globals.h"

// Variables used for synchronization of processes
std::string sync_counter_key = "sync_process_count";
std::string sync_channel = "SYNC";

// Simulation settings CHANGE THIS FIRST IF PC IS MELTING!!!!
std::chrono::milliseconds tick_duration_ms = std::chrono::milliseconds(300);    // duration of 1 tick in milliseconds. 50ms is minimum I could set without the simulation breaking
std::chrono::milliseconds sim_duration_ms = std::chrono::milliseconds(3000000); // duration of the simulation in milliseconds
