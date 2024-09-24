#include "Drone.h"
#include "DroneState.h"

Drone::Drone(int id, const int wave_id, Wave &ctx) : id(id), wave_id(wave_id), ctx(ctx) {
}

// Drone::~Drone() {
//     try
//     {
//         delete currentState;
//     }
//     catch (std::exception &e)
//     {
//         spdlog::error("Error deleting drone state: {}", e.what());
//     }
// }

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