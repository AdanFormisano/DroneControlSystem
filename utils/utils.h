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
    WORKING,
    CHARGING,
    WAITING_CHARGE, // Drone is in the base, is idle, is not fully charged and is watching for a charging station
    TO_ZONE,        // Drone is moving to zone
    TO_BASE,        // Drone is moving to base
    SLEEP,          // Drone is sleeping
    TOTAL
};

constexpr std::array<const char *, static_cast<std::size_t>(drone_state_enum::TOTAL)> drone_state_str = {
    "IDLE_IN_BASE",
    "WORKING",
    "CHARGING",
    "WAITING_CHARGE",
    "TO_ZONE",
    "TO_BASE",
    "SLEEP"};

namespace utils {
const char *CaccaPupu(drone_state_enum state);
}
#endif // DRONECONTROLSYSTEM_UTILS_H
