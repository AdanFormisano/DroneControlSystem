#include "DroneState.h"

DroneState& Idle::getInstance()
{
    static Idle instance;
    return instance;
}

void Idle::run(Drone* drone)
{
    // TODO: First implement wave drone recycling
}

DroneState& ToStartingLine::getInstance()
{
    static ToStartingLine instance;
    return instance;
}

void ToStartingLine::enter(Drone* drone)
{
    // spdlog::info("Drone {} is moving to the starting line", drone->id);

}

void ToStartingLine::run(Drone* drone)
{
    // Check if the drone has reached the starting line
    if (drone->position.x == -2990)
    {
        // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
        // Change the state to READY
        drone->setState(Ready::getInstance());
    }
    else
    {
        if (drone->dir.x * DRONE_STEP_SIZE + drone->position.x <= -2990)
        {
            drone->position.x = -2990;
            drone->position.y = drone->starting_line.y; // TODO check if this is correct
            drone->charge -= DRONE_CONSUMPTION_RATE;
        } else
        {
            drone->position.x += drone->dir.x * DRONE_STEP_SIZE;
            drone->position.y += drone->dir.y * DRONE_STEP_SIZE;
            drone->charge -= DRONE_CONSUMPTION_RATE;
            // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
        }
    }
}

void ToStartingLine::exit(Drone* drone)
{
    // spdlog::info("TICK {} Drone {} has reached the starting line", drone->tick_drone, drone->id);
}

DroneState& Ready::getInstance()
{
    static Ready instance;
    return instance;
}

void Ready::enter(Drone* drone)
{
    // spdlog::info("TICK {} Drone {} is ready to start working", drone->tick_drone, drone->id);
    drone->ctx.incrReadyDrones();
}

void Ready::run(Drone* drone)
{
    if (drone->ctx.getReadyDrones() < 300)
    {
        // Wait for all drones to be ready
        // spdlog::info("TICK {} Drone {} is waiting for all drones to be ready", drone->tick_drone, drone->id);
    }
    else
    {
        // Change the state to WORKING
        drone->setState(Working::getInstance());
    }
}

DroneState& Working::getInstance()
{
    static Working instance;
    return instance;
}

void Working::enter(Drone* drone)
{
    spdlog::info("TICK {} Drone {} is starting work", drone->tick_drone, drone->id);
}

void Working::run(Drone* drone)
{
    // Move to the right by 20 units
    if (drone->position.x >= 2990)
    {
        drone->setState(ToBase::getInstance());
    } else
    {
        drone->position.x += DRONE_STEP_SIZE;
        drone->charge -= DRONE_CONSUMPTION_RATE;
        // spdlog::info("Drone {} ({}, {})", drone->id, drone->position.x, drone->position.y);
    }
}

DroneState& ToBase::getInstance()
{
    static ToBase instance;
    return instance;
}

void ToBase::enter(Drone* drone)
{
    // spdlog::info("TICK {} Drone {} is moving to the base", drone->tick_drone, drone->id);
}

void ToBase::run(Drone* drone)
{
    // Check if the drone has reached the starting line
    if (drone->position.x == 0)
    {
        // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
        // Change the state to READY
        drone->setState(Charging::getInstance());
    }
    else
    {
        if (drone->dir.x * DRONE_STEP_SIZE + drone->position.x <= 0)
        {
            drone->position.x = 0;
            drone->position.y = 0; // TODO check if this is correct
            drone->charge -= DRONE_CONSUMPTION_RATE;
        } else
        {
            drone->position.x += drone->dir.x * DRONE_STEP_SIZE;
            drone->position.y += -drone->dir.y * DRONE_STEP_SIZE;
            drone->charge -= DRONE_CONSUMPTION_RATE;
            // spdlog::info("TICK {} Drone {} ({}, {})", drone->tick_drone, drone->id, drone->position.x, drone->position.y);
        }
    }
}

void ToBase::exit(Drone* drone)
{
    spdlog::info("TICK {} Drone {} has reached the base", drone->tick_drone, drone->id);
}

DroneState& Charging::getInstance()
{
    static Charging instance;
    return instance;
}

void Charging::enter(Drone* drone)
{
    // spdlog::info("TICK {} Drone {} is charging", drone->tick_drone, drone->id);
}

// TODO: Correctly implement the Charging state
void Charging::run(Drone* drone)
{
    // Check if the drone is fully charged
    if (drone->charge >= 100.0f)
    {
        // Change the state to IDLE_IN_BASE
        drone->setState(Idle::getInstance());
    }
    // TODO: In Charging state the drone should be destroyed and its data uploaded onto Redis
}

void Dead::run(Drone* drone)
{
    spdlog::info("[TestGenerator] TICK {} Drone {} is dead", drone->tick_drone, drone->id);
}