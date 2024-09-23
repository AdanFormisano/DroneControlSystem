#include "Wave.h"

Wave::Wave(int tick_n, const int wave_id, Redis &shared_redis, TickSynchronizer &synchronizer) : tick(tick_n),
                                                                                                 redis(shared_redis), tick_sync(synchronizer) {
    id = wave_id;
    starting_tick = tick_n;

    float y = -2990; // Drone has a coverage radius of 10.0f

    for (int i = 0; i < 300; i++) {
        int drone_id = id * 1000 + i;
        drones.emplace_back(drone_id, id, *this);
        drones[i].position.x = 0;
        drones[i].position.y = 0;
        drones[i].starting_line.x = -2990.0f;
        drones[i].starting_line.y = y;
        drones[i].setState(ToStartingLine::getInstance());
        drones[i].tick_drone = tick;

        // Calculate drones' direction
        float dx = -2990.0f;
        float dy = y;
        float distance = std::sqrt(dx * dx + dy * dy);

        drones[i].dir.x = dx / distance;
        drones[i].dir.y = dy / distance;

        y += 20.0f;
    }

    // Self add alive wave on Redis
    redis.sadd("waves_alive", std::to_string(id));

    spdlog::info("Wave {} created all drones", id);
}

void Wave::Move() {
    X += 20;
    for (auto &drone : drones) {
        drone.position.x = static_cast<float>(X);
        drone.charge -= DRONE_CONSUMPTION_RATE;
    }
}

void Wave::UploadData() {
    try {
        // A redis pipeline is used to upload the data to the redis server
        // Create a pipeline from the group of redis connections
        auto pipe = redis.pipeline(false);

        // For each drone a redis function will be added to the pipeline
        // When every command has been added, the pipeline will be executed

        for (auto &drone : drones) {
            // Create a DroneData object
            DroneData data(tick, drone.id, utils::droneStateToString(drone.getCurrentState()->getState()), drone.charge, drone.position, drone.wave_id);
            auto v = data.toVector();

            // Add the command to the pipeline
            pipe.xadd("scanner_stream", "*", v.begin(), v.end());
            // spdlog::info("Drone {} data uploaded to Redis", drone.id);
        }

        // Redis connection is returned to the pool after the pipeline is executed
        pipe.exec();
    } catch (const ReplyError &e) {
        spdlog::error("Redis pipeline error: {}", e.what());
    } catch (const IoError &e) {
        spdlog::error("Redis pipeline error: {}", e.what());
    }
}

void Wave::setDroneFault(int wave_drone_id, drone_state_enum state, int reconnect_tick) {
    // Set the drone state to the new state parameter
    int drone_id = wave_drone_id % 1000; // Get the drone id from the wave_drone_id
    drones[drone_id].previous = drones[drone_id].getCurrentState()->getState();
    drones[drone_id].setState(getDroneState(state));
}

void Wave::Run() {
    spdlog::info("Wave {} started", id);
    tick_sync.thread_started();
    auto start_time = std::chrono::high_resolution_clock::now();

    // A redis pipeline is used to upload the data to the redis server
    // Create a pipeline from the group of redis connections
    auto pipe = redis.pipeline(false);

    // TODO: Implement states for the waves
    while (tick < starting_tick + 1000) {
        spdlog::info("Wave {} tick {}", id, tick);
        // Before the "normal" execution, check if SM did put any "input" inside the "queue" (aka TestGenerator scenarios' input)
        // Check if there is any message in the queue
        if (!tg_data.empty()) {
            // TODO: In theory there will only ever be a single message in the queue, maybe we can optimize this
            // Get the message from the queue
            auto msg = tg_data.pop().value();
            spdlog::info("[TestGenerator] Drone has new state {}", msg.drone_id, utils::droneStateToString(msg.new_state));

            setDroneFault(msg.drone_id, msg.new_state, msg.reconnect_tick);
        }

        // Execute the wave
        for (auto &drone : drones) {
            // Execute the current state
            drone.run();

            // Create a DroneData object
            DroneData data(tick, drone.id, utils::droneStateToString(drone.getCurrentState()->getState()), drone.charge, drone.position, drone.wave_id);
            auto v = data.toVector();

            // Add the command to the pipeline
            pipe.xadd("scanner_stream", "*", v.begin(), v.end());
            // spdlog::info("Drone {} data uploaded to Redis", drone.id);
        }
        pipe.exec();

        // Manage the drones' faults
        // SERVE LA FUNC CHE ELIMINI I DRONI DA ELIMINARE
        // (FAR PUNTARE A NULL I DRONI NEL VETTORE DA ELIMINARE, SENZA ELIMINARLI REALMENTE?)

        // Each tick of the execution will be synced with the other threads. This will make writing to the DB much easier
        // because the data will be consistent/historical
        tick_sync.tick_completed();
        tick++;
    }

    // Remove self from alive waves on Redis
    redis.srem("waves_alive", std::to_string(id));

    tick_sync.thread_finished();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    spdlog::info("Wave {} duration: {}ms", id, duration.count());
    spdlog::info("Wave {} finished", id);
}