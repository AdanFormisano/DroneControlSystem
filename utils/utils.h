#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H


#include <random>
#include <unordered_map>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <boost/interprocess/sync/named_semaphore.hpp>

#include "../src/globals.h"

extern std::default_random_engine generator;
extern std::uniform_real_distribution<float> float_distribution_mten_to_ten;

float generateRandomFloat();

// Message structure for IPC TestGenerator <=> DroneManager
struct DroneStatusMessage
{
    int target_zone;
    drone_state_enum status;
};

namespace utils
{
    const char* droneStateToString(drone_state_enum state);
    drone_state_enum stringToDroneStateEnum(const std::string& stateStr);
    void signalHandler(int signal);
}

namespace utils_sync
{
    sem_t* create_or_open_semaphore(const char* name, unsigned int initial_value);
}
#endif // DRONECONTROLSYSTEM_UTILS_H
