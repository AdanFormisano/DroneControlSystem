#ifndef DRONECONTROLSYSTEM_REDISUTILS_H
#define DRONECONTROLSYSTEM_REDISUTILS_H

#include <chrono>
#include <string>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace utils {
int RedisConnectionCheck(sw::redis::Redis &redis, std::string clientName);
long long RedisGetClientID(sw::redis::Redis &redis);
bool getSimStatus(sw::redis::Redis &redis);

// Clear Redis cache
void clearRedis();
void clearCache(const std::string &cachePath);

// Used for synchronization of processes
void AddThisProcessToSyncCounter(Redis &redis, const std::string &process_name);
int NamedSyncWait(Redis &redis, const std::string &process_name);

// Used for synchronization of Scanner and DroneControl processes
void UpdateSyncTick(Redis &redis, int tick_n);
void AckSyncTick(Redis &redis, int tick_n);
bool CheckSyncTick(Redis &redis, int tick_n);
int GetSyncTick(Redis &redis);
} // namespace utils

#endif // DRONECONTROLSYSTEM_REDISUTILS_H
