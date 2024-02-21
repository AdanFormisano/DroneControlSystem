#ifndef DRONECONTROLSYSTEM_REDISUTILS_H
#define DRONECONTROLSYSTEM_REDISUTILS_H

#include <sw/redis++/redis++.h>

namespace utils {
    // Check if the connection to Redis is successful
    int RedisConnectionCheck(sw::redis::Redis& redis, std::string clientName);
    long long RedisGetClientID(sw::redis::Redis& redis);
}

#endif //DRONECONTROLSYSTEM_REDISUTILS_H
