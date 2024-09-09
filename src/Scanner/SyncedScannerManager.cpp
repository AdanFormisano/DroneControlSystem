#include "SyncedScannerManager.h"

Wave::Wave(const int wave_id, Redis& shared_redis) : redis(shared_redis)
{
    id = wave_id;

    int i = 0;
    float y = -2990; // Drone has a coverage radius of 10.0f
    for (auto drone : drones)
    {
        drone.id = id * 1000 + i;
        drone.position.x = static_cast<float>(X);
        drone.position.y = y;
        drone.state = drone_state_enum::IDLE_IN_BASE;
        drone.wave_id = id;

        y += 20.0f;
        i++;
    }
}

void Wave::Move()
{
    X += 20;
    for (auto& drone : drones)
    {
        drone.position.x = static_cast<float>(X);
        drone.charge -= DRONE_CONSUMPTION_RATE;
    }
}

void Wave::UploadData()
{
    // A redis pipeline is used to upload the data to the redis server
    // Create a pipeline from the group of redis connections
    auto pipe = redis.pipeline(false);

    // For each drone a redis function will be added to the pipeline
    // When every command has been added, the pipeline will be executed

    for (auto& [id, position, state, wave_id, charge] : drones)
    {
        // Create a DroneData object
        DroneData data(tick, id, utils::droneStateToString(state), charge, position, wave_id);
        auto _ = data.toVector();

        // Add the command to the pipeline
        pipe.xadd("scanner_stream", "*", _.begin(), _.end());
    }

    // Redis connection is returned to the pool after the pipeline is executed
    pipe.exec();

    // TODO: add exception handling
}

void Wave::Run()
{
    // TODO: fix the condition of the while loop
    while (true)
    {
        // Before the "normal" execution, check if SM did put any "input" inside the "queue" (aka TestGenerator scenarios' input)

        Move();
        UploadData();

        // Use atomic variable to sync with other threads
        // Each tick of the execution will be synced with the other threads. This will make writing to the DB much easier
        // because the data will be consistent/historical
    }
}

SyncedScannerManager::SyncedScannerManager(Redis& shared_redis) : shared_redis(shared_redis)
{

}

// TODO: Check if it's better to use a thread-pool or a thread for each wave

