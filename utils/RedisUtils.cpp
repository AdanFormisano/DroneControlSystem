#include "RedisUtils.h"
#include <spdlog/spdlog.h>
#include <iostream>

using namespace sw::redis;

namespace utils {
    int RedisConnectionCheck(Redis& redis, std::string clientName) {
        try {
            spdlog::info("Waiting Redis-server response to {}", clientName);
            redis.ping();
            auto clientID = redis.command<long long>("CLIENT", "ID");
            spdlog::info("{}'s connection successful (Client ID: {})", clientName, clientID);
        }
        catch (const sw::redis::IoError& e) {
            spdlog::error("{}'s connection failed: {}", clientName, e.what());
            return 1;
        }

        return 0;
    }
}
