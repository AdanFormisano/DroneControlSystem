#include "Drone.h"
#include "DroneState.h"

Drone::Drone(int id, const int wave_id, Wave &ctx) : id(id), wave_id(wave_id), ctx(ctx) {
    spdlog::info("Drone {} created", id);
}

void Drone::setState(DroneState &newState) {

    previous = getCurrentStateEnum();

    spdlog::info("[DroneCH] Drone {} is changing state from {} to {}", id, utils::droneStateToString(getCurrentStateEnum()), utils::droneStateToString(newState.getState()));
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

drone_state_enum Drone::getCurrentStateEnum() const {
    if (currentState == &Idle::getInstance())
        return drone_state_enum::IDLE;
    if (currentState == &ToStartingLine::getInstance())
        return drone_state_enum::TO_STARTING_LINE;
    if (currentState == &Ready::getInstance())
        return drone_state_enum::READY;
    if (currentState == &Working::getInstance())
        return drone_state_enum::WORKING;
    if (currentState == &ToBase::getInstance())
        return drone_state_enum::TO_BASE;
    if (currentState == &Charging::getInstance())
        return drone_state_enum::CHARGING;
    if (currentState == &Disconnected::getInstance())
        return drone_state_enum::DISCONNECTED;
    if (currentState == &Reconnected::getInstance())
        return drone_state_enum::RECONNECTED;
    if (currentState == &Dead::getInstance())
        return drone_state_enum::DEAD;
    return drone_state_enum::NONE;
}