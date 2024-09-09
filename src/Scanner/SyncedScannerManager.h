/* For now checkpointing is not being implemented, if threads are actually not being executed
 * then it will be implemented.
 */

#ifndef SYNCEDSCANNERMANAGER_H
#define SYNCEDSCANNERMANAGER_H
#include <thread>
#include <vector>

#include "../globals.h"
#include "../../utils/utils.h"
#include "../../utils/RedisUtils.h"

class Wave
{
public:
    Wave(int wave_id, Redis& shared_redis);

    void Run(); // Function executed by the thread

    int tick = 0;
    int X = 0; // The position of the wave
    std::array<DroneData, 300> drones_data;
    utils::synced_queue<TG_data> tg_data;

    [[nodiscard]] int getId() const { return id; }

private:
    Redis& redis;
    int id = 0;
    std::array<Drone, 300> drones;

    void Move();
    void UploadData();
};

class SyncedScannerManager
{
public:
    explicit SyncedScannerManager(Redis& shared_redis);

    void Run();

private:
    std::vector<Wave> waves;
    std::vector<std::thread> waves_threads;
    std::atomic<int> number_of_waves = 0;
    std::atomic<int> waiting_waves = 0;
    Redis& shared_redis;
};



#endif //SYNCEDSCANNERMANAGER_H
