#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <array>
#include <random>
#include <unordered_map>
#include <iostream>
#include <csignal>
#include <cstdlib>
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

namespace utils {
    const char *droneStateToString(drone_state_enum state);
    drone_state_enum stringToDroneStateEnum(const std::string& stateStr);
    void signalHandler(int signal);
}
#endif // DRONECONTROLSYSTEM_UTILS_H
