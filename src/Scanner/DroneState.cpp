#include "DroneState.h"
#include "../../utils/LogUtils.h"

DroneState &Idle::getInstance() {
    static Idle instance;
    return instance;
}

void Idle::run(Drone *drone) {
    // TODO: First implement wave drone recycling
}

DroneState &ToStartingLine::getInstance() {
    static ToStartingLine instance;
    return instance;
}

void ToStartingLine::enter(Drone *drone) {
    // spdlog::info("Drone {} is moving to the starting line", drone->id);
}

void ToStartingLine::run(Drone *drone) {
    if (drone->charge - DRONE_CONSUMPTION_RATE * drone->high_consumption_factor <= 0.0f) {
        drone->ctx->incrDronesNotWaiting();
        drone->setState(Dead::getInstance());
        return;
    }

    // Check if the drone has reached the starting line
    if (drone->position.x + drone->dir.x * DRONE_STEP_SIZE <= -2990) {
        drone->position.x = -2990;
        drone->position.y = drone->starting_line.y; // TODO check if this is correct
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        drone->setState(Ready::getInstance());
    } else {
        drone->position.x += drone->dir.x * DRONE_STEP_SIZE;
        drone->position.y += drone->dir.y * DRONE_STEP_SIZE;
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
    }
}

void ToStartingLine::exit(Drone *drone) {
    // spdlog::info("TICK {} Drone {} has reached the starting line", drone->tick_drone, drone->id);
}

DroneState &Ready::getInstance() {
    static Ready instance;
    return instance;
}

void Ready::enter(Drone *drone) {
    // spdlog::info("TICK {} Drone {} is ready to start working", drone->tick_drone, drone->id);
    drone->ctx->incrReadyDrones();
}

void Ready::run(Drone *drone) {
    if (drone->charge - DRONE_CONSUMPTION_RATE * drone->high_consumption_factor <= 0.0f) {
        drone->setState(Dead::getInstance());
        return;
    }

    if (drone->ctx->getReadyDrones() - drone->ctx->getDronesNotWaiting() < 300) {
        // Wait for all drones to be ready
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        // spdlog::info("TICK {} Drone {} is waiting for all drones to be ready", drone->tick_drone, drone->id);
    } else {
        // Change the state to WORKING
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        drone->setState(Working::getInstance());
        return;
    }
}

DroneState &Working::getInstance() {
    static Working instance;
    return instance;
}

void Working::enter(Drone *drone) {
    // spdlog::info("TICK {} Drone {} is starting work", drone->tick_drone, drone->id);
}

void Working::run(Drone *drone) {
    if (drone->charge - DRONE_CONSUMPTION_RATE * drone->high_consumption_factor <= 0.0f) {
        drone->setState(Dead::getInstance());
        return;
    }

    // Move to the right by 20 units
    if (drone->position.x >= 2990) {
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        drone->setState(ToBase::getInstance());
        return;
    } else {
        drone->position.x += DRONE_STEP_SIZE;
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        // spdlog::info("Drone {} ({}, {})", drone->id, drone->position.x, drone->position.y);
    }
}

DroneState &ToBase::getInstance() {
    static ToBase instance;
    return instance;
}

void ToBase::enter(Drone *drone) {
    // spdlog::info("TICK {} Drone {} is moving to the base", drone->tick_drone, drone->id);
}

void ToBase::run(Drone *drone) {
    if (drone->charge - DRONE_CONSUMPTION_RATE * drone->high_consumption_factor <= 0.0f) {
        drone->setState(Dead::getInstance());
        return;
    }

    // Check if the drone has reached the starting line
    if (drone->dir.x * DRONE_STEP_SIZE + drone->position.x <= 0) {
        drone->position.x = 0;
        drone->position.y = 0; // TODO check if this is correct
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        drone->setState(Charging::getInstance());
    } else {
        drone->position.x += drone->dir.x * DRONE_STEP_SIZE;
        drone->position.y += -drone->dir.y * DRONE_STEP_SIZE;
        drone->charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
        // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
    }
}

void ToBase::exit(Drone *drone) {
    // spdlog::info("TICK {} Drone {} has reached the base", drone->tick_drone, drone->id);
}

DroneState &Charging::getInstance() {
    static Charging instance;
    return instance;
}

void Charging::enter(Drone *drone) {
    // Upload to Redis drones' information for ChargeBase
    ChargingStreamData data(drone->id, drone->wave_id, drone->charge,
                            utils::droneStateToString(drone_state_enum::CHARGING));
    auto v = data.toVector();
    drone->ctx->redis.xadd("charge_stream", "*", v.begin(), v.end());

    // Drone has sent its data to Redis, now it can charge (die)
    drone->ctx->drones_to_delete.push_back(drone->id);
}

DroneState &Disconnected::getInstance() {
    static Disconnected instance;
    return instance;
}

void Disconnected::enter(Drone *drone) {
    drone->hidden_coords = drone->position;
    drone->hidden_charge = drone->charge;
    // spdlog::info("[[DroneCH]] Drone {} disconnected at TICK {}. Previous state: {}", drone->id, drone->tick_drone, utils::droneStateToString(drone->previous));
    drone->disconnected_tick = drone->tick_drone;
}

void Disconnected::hidden_to_starting_line(Drone *drone) {
    drone->hidden_charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
    if (drone->dir.x * DRONE_STEP_SIZE + drone->hidden_coords.x <= -2990) {
        drone->hidden_coords.x = -2990;
        drone->hidden_coords.y = drone->starting_line.y;
        drone->ctx->incrReadyDrones();
        drone->previous = drone_state_enum::READY;
    } else {
        drone->hidden_coords.x += drone->dir.x * DRONE_STEP_SIZE;
        drone->hidden_coords.y += drone->dir.y * DRONE_STEP_SIZE;
    }
}

void Disconnected::hidden_ready(Drone *drone) {
    drone->hidden_charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
    if (drone->ctx->getReadyDrones() >= 300) {
        drone->previous = drone_state_enum::WORKING;
    }
}

void Disconnected::hidden_working(Drone *drone) {
    drone->hidden_charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
    drone->hidden_coords.x += DRONE_STEP_SIZE;
    if (drone->hidden_coords.x >= 2990) {
        drone->previous = drone_state_enum::TO_BASE;
    }
}

void Disconnected::hidden_to_base(Drone *drone) {
    drone->hidden_charge -= DRONE_CONSUMPTION_RATE * drone->high_consumption_factor;
    drone->hidden_coords.x += drone->dir.x * DRONE_STEP_SIZE;
    drone->hidden_coords.y += -drone->dir.y * DRONE_STEP_SIZE; // Movimento "indietro"
    if (drone->hidden_coords.x <= 0) {
        drone->setState(Charging::getInstance());
        return;
    }
}

void Disconnected::run(Drone *drone) {
    if (drone->reconnect_tick != -1) {
        if (drone->charge - DRONE_CONSUMPTION_RATE * drone->high_consumption_factor <= 0.0f) {
            drone->setState(Dead::getInstance());
            return;
        }

        switch (drone->previous) {
        case drone_state_enum::TO_STARTING_LINE:
            hidden_to_starting_line(drone);
            break;
        case drone_state_enum::READY:
            hidden_ready(drone);
            break;
        case drone_state_enum::WORKING:
            hidden_working(drone);
            break;
        case drone_state_enum::TO_BASE:
            hidden_to_base(drone);
            break;
        default:
            break;
        }

        if (drone->tick_drone >= drone->reconnect_tick + drone->disconnected_tick) {
            drone->setState(Reconnected::getInstance());
            return;
        }
    } else {
        // spdlog::info("[[DroneCH]] Drone {} is disconnected, coming from {}, waiting to die...", drone->id, utils::droneStateToString(drone->previous));
        if (drone->tick_drone >= drone->disconnected_tick + 20) {
            // Check if the drone was in a READY or TO_STARTING_LINE state
            if (drone->previous == drone_state_enum::READY || drone->previous == drone_state_enum::TO_STARTING_LINE) {
                drone->ctx->incrDronesNotWaiting();
            }
            drone->setState(Dead::getInstance());
            return;
        }
    }
}

DroneState &Reconnected::getInstance() {
    static Reconnected instance;
    return instance;
}

void Reconnected::enter(Drone *drone) {
    // spdlog::info("[TestGenerator] Drone {} reconnected at TICK {}", drone->id, drone->tick_drone);
    drone->position = drone->hidden_coords;
    drone->charge = drone->hidden_charge;
}

void Reconnected::run(Drone *drone) {
    if (drone->previous != drone_state_enum::NONE) {
        // Riporta il drone allo stato prec. la disconnessione
        switch (drone->previous) {
        case drone_state_enum::TO_STARTING_LINE:
            dcs_log << "Drone " << drone->id << " is returning to its previous state: " << utils::droneStateToString(drone->previous) << std::endl;
            // spdlog::info("[DroneCH] Drone {} is returning to its previous state: {}", drone->id, utils::droneStateToString(drone->previous));
            drone->setState(ToStartingLine::getInstance());
            dcs_log << "BOOOTO_STARTING_LINEOOOM" << std::endl;
            // spdlog::info("BOOOTO_STARTING_LINEOOOM");
            break;
        case drone_state_enum::READY:
            dcs_log << "Drone " << drone->id << " is returning to its previous state: " << utils::droneStateToString(drone->previous) << std::endl;
            // spdlog::info("[DroneCH] Drone {} is returning to its previous state: {}", drone->id, utils::droneStateToString(drone->previous));
            drone->setState(Ready::getInstance());
            dcs_log << "BOOOOOOOOREADYOOOOOOOOOOM" << std::endl;
            // spdlog::info("BOOOOOOOOREADYOOOOOOOOOOM");
            break;
        case drone_state_enum::WORKING:
            dcs_log << "Drone " << drone->id << " is returning to its previous state: " << utils::droneStateToString(drone->previous) << std::endl;
            // spdlog::info("[DroneCH] Drone {} is returning to its previous state: {}", drone->id, utils::droneStateToString(drone->previous));
            drone->setState(Working::getInstance());
            dcs_log << "BOOOOOOOWORKINGOOOOOOOM" << std::endl;
            // spdlog::info("BOOOOOOOWORKINGOOOOOOOM");
            break;
        case drone_state_enum::TO_BASE:
            dcs_log << "Drone " << drone->id << " is returning to its previous state: " << utils::droneStateToString(drone->previous) << std::endl;
            // spdlog::info("[DroneCH] Drone {} is returning to its previous state: {}", drone->id, utils::droneStateToString(drone->previous));
            drone->setState(ToBase::getInstance());
            dcs_log << "BOOOOOOOTO_BASEOOOOOOOM" << std::endl;
            // spdlog::info("BOOOOOTO_BASEOOOOOOOM");
            break;
        case drone_state_enum::CHARGING:
            dcs_log << "Drone " << drone->id << " is returning to its previous state: " << utils::droneStateToString(drone->previous) << std::endl;
            // spdlog::info("[DroneCH] Drone {} is returning to its previous state: {}", drone->id, utils::droneStateToString(drone->previous));
            drone->setState(Charging::getInstance());
            dcs_log << "BOOOOOOCHARGINGOOOOOOOM" << std::endl;
            // spdlog::info("BOOOOOOCHARGINGOOOOOOOM");
            break;
        default:
            dcs_log << "Drone " << drone->id << " has a None state where it shouldn't ⇒ Dead" << std::endl;
            // spdlog::error("[DroneCH] Drone {}'s last state chip broken, can't recover its state before disconnection ⇒ Dead", drone->id);
            drone->setState(Dead::getInstance());
            break;
        }
    } else {
        dcs_log << "Drone " << drone->id << " has a None state where it shouldn't ⇒ Dead" << std::endl;
        // spdlog::error("Drone {} has a None state where it shouldn't", drone->id);
        drone->setState(Dead::getInstance()); // Garbage Collection
        return;
    }
}

DroneState &Dead::getInstance() {
    static Dead instance;
    return instance;
}

void Dead::enter(Drone *drone) {
    drone->ctx->incrDronesNotWaiting();
    drone->ctx->drones_to_delete.push_back(drone->id);
}

void Dead::run(Drone *drone) {
}
