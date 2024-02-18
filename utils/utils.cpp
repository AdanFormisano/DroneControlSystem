#include "utils.h"
#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace utils {
    int RedisConnectionCheck(Redis& redis) {
        try {
            spdlog::info("Waiting Redis-server response");
            redis.ping();
            spdlog::info("Connection to Redis-server successful");
        }
        catch (const sw::redis::IoError& e) {
            spdlog::error("Connection failed: {}", e.what());
            return 1;
        }

        return 0;
    }
}
