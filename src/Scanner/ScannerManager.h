/* For now checkpointing is not being implemented, if threads are actually not being executed
 * then it will be implemented.
 */

#ifndef SYNCEDSCANNERMANAGER_H
#define SYNCEDSCANNERMANAGER_H
#include <thread>
#include <vector>

#include "ThreadUtils.h"
#include "../globals.h"
#include "DroneState.h"
#include "../../utils/utils.h"
#include "../../utils/RedisUtils.h"
#include "../../libs/interprocess/ipc/message_queue.hpp"

using namespace boost::interprocess;

class Drone;

class Wave
{
public:
    Wave(int tick_n, int wave_id, Redis& shared_redis, TickSynchronizer& synchronizer);

    void Run(); // Function executed by the thread

    int X = 0; // The position of the wave
    int starting_tick = 0; // The tick when the wave was created
    std::array<DroneData, 300> drones_data;
    synced_queue<TG_data> tg_data;
    int tick = 0;

    [[nodiscard]] int getId() const { return id; }
    [[nodiscard]] int getReadyDrones() const { return ready_drones; }
    void incrReadyDrones() { ready_drones++; }

private:
    Redis& redis;
    int id = 0;
    std::vector<Drone> drones;
    TickSynchronizer& tick_sync;
    int ready_drones = 0;

    void Move();
    void UploadData();
};

class ScannerManager
{
public:
    explicit ScannerManager(Redis& shared_redis);

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
