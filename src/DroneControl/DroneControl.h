#ifndef SYNCEDDRONECONTROL_H
#define SYNCEDDRONECONTROL_H
#include "../globals.h"
#include "../../utils/utils.h"
#include "../../utils/RedisUtils.h"
#include "../Database/Database.h"
#include "../Database/NewBuffer.h"

using namespace sw::redis;

class DroneControl {
public:
    explicit DroneControl(Redis &shared_redis) : redis(shared_redis) { db.get_DB(); };

    Redis &redis;

    void Run();
    void WriteDroneDataToDB();

private:
    int tick_n = 0;
    const int num_consumers = 5;
    std::string current_stream_id = "*";
    std::string stream = "scanner_stream";
    std::string group = "scanner_group";
    std::array<std::unordered_set<coords>, 300> drones_paths{}; // Working paths for drones
    std::atomic_bool sim_running{true};

    std::mutex tick_mutex;

    Database db;
    NewBuffer buffer;

    void Consume(Redis& redis, const std::string& stream, const std::string& group, const std::string& consumer, const std::array<std::unordered_set<coords>, 300> *drones_paths);
    void SendWaveSpawnCommand() const;
    void GetDronePaths();
};
#endif //SYNCEDDRONECONTROL_H
