#ifndef SYNCEDDRONECONTROL_H
#define SYNCEDDRONECONTROL_H
#include "../globals.h"
#include "../../utils/RedisUtils.h"
#include "../Database/Buffer.h"
#include "../Database/Database.h"

using namespace sw::redis;

class SyncedDroneControl {
public:
    explicit SyncedDroneControl(Redis &shared_redis) : redis(shared_redis) { db.get_DB(); };

    Redis &redis;

    void Run();
    void ReadDataStream();
    void ParseStreamData(const std::vector<std::pair<std::string, std::string>>& data, std::vector<DroneData>& drones_data);
    void WriteDroneDataToDB();

private:
    int tick_n = 0;
    const int num_consumers = 5;
    std::string current_stream_id = "*";
    std::string stream = "scanner_stream";
    std::string group = "scanner_group";
    std::unordered_map<int, std::vector<coords>> drones_paths;
    Database db;
    Buffer buffer;

    void GetDronesPaths();
    void SendWaveCreationCommand();
    void Consume(Redis& redis, const std::string& stream, const std::string& group, const std::string& consumer);
};
#endif //SYNCEDDRONECONTROL_H
