#include "Drone.h"
#include "DroneState.h"

Drone::Drone(int id, const int wave_id, Wave &ctx) : id(id), wave_id(wave_id), ctx(ctx) {
    spdlog::info("Drone {} created", id);
}

void Drone::setState(DroneState &newState) {

    spdlog::info("[DroneCH] Drone {} is changing state from {} to {}", id, utils::droneStateToString(currentState->getState()), utils::droneStateToString(newState.getState()));
    if (currentState != nullptr) {
        currentState->exit(this);
    }
    currentState = &newState;
    currentState->enter(this);
}

void Drone::run() {
    // spdlog::info("TICK {} of Drone {}", tick_drone, id);
    currentState->run(this);
    tick_drone++;
}