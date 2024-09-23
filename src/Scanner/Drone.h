
#ifndef DRONEFST_H
#define DRONEFST_H

#include "../globals.h"
#include "DroneState.h"
#include "ScannerManager.h"

class DroneState;
class Wave;

class Drone {
public:
    Drone(int id, int wave_id, Wave &ctx);

    int id;                              ///< Unique identifier for the drone.
    coords position = {0.0f, 0.0f};      ///< Current position of the drone in global coordinates.
    coords starting_line = {0.0f, 0.0f}; ///< Position of the starting line.
    coords dir = {0.0f, 0.0f};           ///< Direction of the drone.
    // drone_state_enum state = drone_state_enum::IDLE_IN_BASE; ///< Current state of the drone.
    int wave_id;           ///< Identifier for the wave the drone is part of.
    float charge = 100.0f; ///< Current charge level of the drone, in percentage.
    Wave &ctx;
    int tick_drone = 0;
    coords hidden_coords = {0.0f, 0.0f}; ///< Hidden coordinates for the drone.
    int reconnect_tick = 0;              ///< Tick when the drone will reconnect.
    float hidden_charge = 0.0f;          ///< Hidden charge for the drone.
    int disconnected_tick = 0;           ///< Tick when the drone disconnected.

    drone_state_enum previous = drone_state_enum::NONE;

    void run();
    [[nodiscard]] DroneState *getCurrentState() const { return currentState; }
    void setState(DroneState &newState);

private:
    DroneState *currentState = nullptr;
};
#endif // DRONEFST_H
