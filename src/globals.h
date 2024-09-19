#ifndef DRONECONTROLSYSTEM_GLOBALS_H
#define DRONECONTROLSYSTEM_GLOBALS_H

#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#define DRONE_CONSUMPTION_RATE 0.00672f // Normal consumption rate
// #define DRONE_CONSUMPTION 0.01072f  // Higher consumption rate used for testing purposes
#define TICK_TIME_SIMULATED 2.42f
#define DRONE_STEP_SIZE 20.0f
#define ZONE_NUMBER 750

// Variables used for synchronization of processes
inline std::string sync_counter_key = "sync_process_count";
inline std::string sync_channel = "SYNC";

// Simulation settings CHANGE THIS FIRST IF PC IS MELTING!!!!
inline auto tick_duration_ms = std::chrono::milliseconds(300);
// duration of 1 tick in milliseconds. 50ms is minimum I could set without the simulation breaking
inline auto sim_duration_ms = std::chrono::milliseconds(3000000); // duration of the simulation in milliseconds

inline float coord_min = -2980.0f;
inline float coord_max = 2980.0f;

struct coords {
    float x;
    float y;
};

struct drone_data {
    int id;
    std::string status;
    float charge;
    std::pair<float, float> position;
    int zone_id = 0;
    float charge_needed_to_base;
    // TODO: Add the last time the drone was updated
};

// Enum for the state of the drone
enum class drone_state_enum {
    IDLE,             // Drone is in the base, is idle and is fully charged (stopped charging)
    TO_STARTING_LINE, // Drone is moving to zone
    READY,
    WORKING,
    TO_BASE, // Drone is moving to base
    DISCONNECTED,
    RECONNECTED,
    DEAD,
    CHARGING,
    NONE
};

constexpr std::array drone_state_str = {
    "IDLE",
    "TO_STARTING_LINE",
    "READY",
    "WORKING",
    "TO_BASE",
    "DISCONNECTED",
    "RECONNECTED",
    "DEAD",
    "CHARGING"};

/**
 * \struct Drone
 * \brief Represents a drone in the system.
 *
 * This structure holds the essential information about a drone, including its
 * unique identifier, current position, state, wave identifier, and charge level.
 */
// struct Drone
// {
//     int id{}; ///< Unique identifier for the drone.
//     coords position = {0.0f, 0.0f}; ///< Current position of the drone in global coordinates.
//     drone_state_enum state = drone_state_enum::IDLE_IN_BASE; ///< Current state of the drone.
//     int wave_id{}; ///< Identifier for the wave the drone is part of.
//     float charge = 100.0f; ///< Current charge level of the drone, in percentage.
// };

struct DroneData {
    std::string tick_n;
    std::string id;
    std::string status;
    std::string charge;
    std::string x;
    std::string y;
    std::string wave_id;
    std::string checked = "false";

    DroneData() = default;

    DroneData(int tick, const int drone_id, const std::string &drone_status, float drone_charge,
              coords position, int drone_wave_id) {
        tick_n = std::to_string(tick);
        id = std::to_string(drone_id);
        status = drone_status;
        charge = std::to_string(drone_charge);
        x = std::to_string(position.x);
        y = std::to_string(position.y);
        wave_id = std::to_string(drone_wave_id);
        checked = "false";
    }

    DroneData(int tick_n, const std::string &id, const std::string &status, const std::string &charge,
              std::pair<float, float> position, const std::string &zone_id, const std::string &checked)
        : tick_n(std::to_string(tick_n)),
          id(id),
          status(status),
          charge(charge),
          x(std::to_string(position.first)),
          y(std::to_string(position.second)),
          wave_id(zone_id),
          checked(checked) {}

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> toVector() const {
        return {
            {"tick_n", tick_n},
            {"id", id},
            {"status", status},
            {"charge", charge},
            {"x", x},
            {"y", y},
            {"wave_id", wave_id},
            {"checked", checked}
        };
    }
};

struct ChargingDroneData
{
    int id;
    int wave_id;
    float charge;
    drone_state_enum state;
    float charge_rate;

    ChargingDroneData() = default;
    ChargingDroneData(int id, int wave_id, float charge, drone_state_enum state, float charge_rate)
        : id(id), wave_id(wave_id), charge(charge), state(state), charge_rate(charge_rate) {};
};

struct ChargingStreamData
{
    std::string id;
    std::string wave_id;
    std::string charge;
    std::string state;

    ChargingStreamData(int drone_id, int drone_wave_id, float drone_charge, std::string drone_state)
    {
        id = std::to_string(drone_id);
        wave_id = std::to_string(drone_wave_id);
        charge = std::to_string(drone_charge);
        state = std::move(drone_state);
    }

    [[nodiscard]] std::vector<std::pair<std::string, std::string>> toVector() const
    {
        return {
            {"id", id},
            {"wave_id", wave_id},
            {"charge", charge},
            {"state", state}
        };
    }
};

struct TG_data {
    int drone_id;               // ID of the drone
    int wave_id;                // ID of the wave
    drone_state_enum new_state; // New state of the drone
    int reconnect_tick; // Contains the number of ticks after the disconnection
    // when the drone reconnected (-1 if not reconnecting)
};

struct drone_fault {
    std::string fault_state;
    int drone_id;
    int zone_id;
    std::pair<float, float> position;
    int fault_tick;
    int reconnect_tick;
};
#endif // DRONECONTROLSYSTEM_GLOBALS_H
