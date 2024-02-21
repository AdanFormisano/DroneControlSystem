#ifndef DRONECONTROLSYSTEM_REDISUTILS_H
#define DRONECONTROLSYSTEM_REDISUTILS_H

#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace utils {
    // Check if the connection to Redis is successful
    int RedisConnectionCheck(sw::redis::Redis& redis, std::string clientName);
    long long RedisGetClientID(sw::redis::Redis& redis);

    // Used to sync processes in known parts of the code (or states)
//    void RedisWaitSync(sw::redis::Redis& redis);
//    void RedisSyncAwk(sw::redis::Redis& redis);

    void SyncWait(Redis& redis);

    void SyncProcess(Redis& redis);
    void SendSyncAck(Redis& redis);
    void WaitSyncAck(Redis& redis);

    void SignalReady(Redis& redis, const std::string& counter_key, const std::string& channel, int total_processes);
    void WaitReady(Redis& redis, const std::string& counter_key, const std::string& channel, int total_processes);
}

#endif //DRONECONTROLSYSTEM_REDISUTILS_H
