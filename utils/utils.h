#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <array>
#include <random>
#include <unordered_map>

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
    TOTAL
};

constexpr std::array<const char *, static_cast<std::size_t>(drone_state_enum::TOTAL)> drone_state_str = {
    "IDLE_IN_BASE",
    "TO_ZONE",
    "WORKING",
    "TO_BASE",
    "WAITING_CHARGE",
    "TO_ZONE_FOLLOWING",
    "FOLLOWING",};

namespace utils {
const char *CaccaPupu(drone_state_enum state);
}
#endif // DRONECONTROLSYSTEM_UTILS_H
