
#ifndef DRONEFST_H
#define DRONEFST_H

#include "../globals.h"
#include "DroneState.h"
#include "ScannerManager.h"

class DroneState;
class Wave;

class Drone {
public:
    Drone(int id, int wave_id, Wave& ctx);

    int id; ///< Unique identifier for the drone.
    coords position = {0.0f, 0.0f}; ///< Current position of the drone in global coordinates.
    coords starting_line = {0.0f, 0.0f}; ///< Position of the starting line.
    coords dir = {0.0f, 0.0f}; ///< Direction of the drone.
    // drone_state_enum state = drone_state_enum::IDLE_IN_BASE; ///< Current state of the drone.
    int wave_id; ///< Identifier for the wave the drone is part of.
    float charge = 100.0f; ///< Current charge level of the drone, in percentage.
    Wave& ctx;
    int tick_drone = 0;

    void run();
    [[nodiscard]] DroneState* getCurrentState() const { return currentState; }
    void setState(DroneState& newState);

private:
    DroneState* currentState = nullptr;
};



#endif //DRONEFST_H
