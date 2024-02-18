#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>
#include "../utils/utils.h"

using namespace sw::redis;

int SystemStart() {
    spdlog::info("System starting");

    spdlog::info("Connecting to Redis");
    auto redis = Redis("tcp://127.0.0.1:7777");

    utils::RedisConnectionCheck(redis);

    return 0;
}

int main() {
    SystemStart();

    return 0;
}

