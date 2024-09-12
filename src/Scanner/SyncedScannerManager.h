/* For now checkpointing is not being implemented, if threads are actually not being executed
 * then it will be implemented.
 */

#ifndef SYNCEDSCANNERMANAGER_H
#define SYNCEDSCANNERMANAGER_H
#include <thread>
#include <vector>

#include "ThreadUtils.h"
#include "../globals.h"
#include "../../utils/utils.h"
#include "../../utils/RedisUtils.h"

class Wave
{
public:
    Wave(int tick_n, int wave_id, Redis& shared_redis, TickSynchronizer& synchronizer);

    void Run(); // Function executed by the thread

    int X = 0; // The position of the wave
    int starting_tick = 0; // The tick when the wave was created
    std::array<DroneData, 300> drones_data;
    synced_queue<TG_data> tg_data;

    [[nodiscard]] int getId() const { return id; }

private:
    Redis& redis;
    int tick = 0;
    int id = 0;
    std::array<Drone, 300> drones;
    TickSynchronizer& tick_sync;

    void Move();
    void UploadData();
};

class SyncedScannerManager
{
public:
    explicit SyncedScannerManager(Redis& shared_redis);

    TickSynchronizer synchronizer;
    ThreadPool pool;

    void Run();

private:
    std::unordered_map<int, std::shared_ptr<Wave>> waves;
    Redis& shared_redis;
    int tick = 0;
    int timeout_ms = 1000;
    int wave_id = 0;

    bool CheckSyncTickAck();
    bool CheckSpawnWave();
    void SpawnWave();
};
#endif //SYNCEDSCANNERMANAGER_H
