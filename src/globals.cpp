#include "globals.h"


// Variables used for synchronization of processes
std::string sync_counter_key = "sync_process_count";
std::string sync_channel = "SYNC";

// Simulation settings
std::chrono::milliseconds tick_duration_ms = std::chrono::milliseconds(500);
std::chrono::milliseconds sim_duration_ms = std::chrono::milliseconds(5000);   // duration of the simulation in milliseconds
