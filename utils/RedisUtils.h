#ifndef DRONECONTROLSYSTEM_REDISUTILS_H
#define DRONECONTROLSYSTEM_REDISUTILS_H

#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace utils {
    // Check if the connection to Redis is successful
    int RedisConnectionCheck(sw::redis::Redis& redis, std::string clientName);
    long long RedisGetClientID(sw::redis::Redis& redis);

    // Used for synchronization of processes
    void SyncWait(Redis& redis);
}

#endif //DRONECONTROLSYSTEM_REDISUTILS_H
