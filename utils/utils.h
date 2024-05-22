#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <array>
#include <random>
#include <unordered_map>
#include <iostream>
#include <csignal>
#include <cstdlib>

extern std::default_random_engine generator;
extern std::uniform_real_distribution<float> float_distribution_mten_to_ten;

float generateRandomFloat();

enum class drone_state_enum {
    IDLE_IN_BASE, // Drone is in the base, is idle and is fully charged (stopped charging)
    TO_ZONE,        // Drone is moving to zone
    WORKING,
    TO_BASE,        // Drone is moving to base
    WAITING_CHARGE, // Drone is in the base, is idle, is not fully charged and is watching for a charging station
    TO_ZONE_FOLLOWING,
    FOLLOWING,
    NOT_CONNECTED,
    DEAD,
    NONE
};

constexpr std::array<const char *, static_cast<std::size_t>(drone_state_enum::NONE)> drone_state_str = {
    "IDLE_IN_BASE",
    "TO_ZONE",
    "WORKING",
    "TO_BASE",
    "WAITING_CHARGE",
    "TO_ZONE_FOLLOWING",
    "FOLLOWING",
    "NOT_CONNECTED",
    "DEAD"};

namespace utils {
    const char *CaccaPupu(drone_state_enum state);
    drone_state_enum stringToDroneStateEnum(const std::string& stateStr);
    void signalHandler(int signal);
}
#endif // DRONECONTROLSYSTEM_UTILS_H
