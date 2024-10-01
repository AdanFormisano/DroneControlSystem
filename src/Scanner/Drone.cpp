#include "Drone.h"
#include "DroneState.h"

Drone::Drone(int id, const int wave_id, Wave &ctx) : id(id), wave_id(wave_id), ctx(ctx) {
    currentState = &ToStartingLine::getInstance();
}

void Drone::setState(DroneState &newState) {
    if (currentState != nullptr) {
        currentState->exit(this);
    }
    currentState = &newState;
    currentState->enter(this);
}

void Drone::run() {
    currentState->run(this);
    tick_drone++;
}