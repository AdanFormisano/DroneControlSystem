#ifndef DRONECONTROLSYSTEM_REDISUTILS_H
#define DRONECONTROLSYSTEM_REDISUTILS_H

#include <sw/redis++/redis++.h>
#include <chrono>

using namespace sw::redis;

namespace utils {
    int RedisConnectionCheck(sw::redis::Redis& redis, std::string clientName);
    long long RedisGetClientID(sw::redis::Redis& redis);
    bool getSimStatus(sw::redis::Redis& redis);

    // Used for synchronization of processes
    void AddThisProcessToSyncCounter(Redis& redis, const std::string& process_name);
    int NamedSyncWait(Redis& redis, const std::string& process_name);
}

#endif //DRONECONTROLSYSTEM_REDISUTILS_H
