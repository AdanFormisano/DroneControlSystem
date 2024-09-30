/* For now checkpointing is not being implemented, if threads are actually not being executed
 * then it will be implemented.
 */

#ifndef SYNCEDSCANNERMANAGER_H
#define SYNCEDSCANNERMANAGER_H
#include "../../libs/boost/interprocess/ipc/message_queue.hpp"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include "ThreadUtils.h"
#include "Wave.h"

using namespace boost::interprocess;

class Drone;
class Wave;

class ScannerManager {
public:
    explicit ScannerManager(Redis &shared_redis);

    // TickSynchronizer synchronizer;
    ThreadSemaphore synchronizer;
    ThreadPool pool;

    void Run();

private:
    std::unordered_map<int, std::shared_ptr<Wave>> waves;
    Redis &shared_redis;
    int tick = 0;
    int timeout_ms = 5000;
    int wave_id = 0;

    bool CheckSpawnWave() const;
    void SpawnWave();
};
#endif // SYNCEDSCANNERMANAGER_H
