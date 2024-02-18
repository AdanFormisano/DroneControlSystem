#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <sw/redis++/redis++.h>

namespace utils {
    int RedisConnectionCheck(sw::redis::Redis& redis);
}

#endif //DRONECONTROLSYSTEM_UTILS_H
