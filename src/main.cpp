#include <spdlog/spdlog.h>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

int SystemStart() {
    spdlog::info("Drone Control System starting");

    spdlog::info("Connecting to Redis");
    auto redis = Redis("tcp://127.0.0.1:7777");

    spdlog::info("Waiting Redis-server response");
    //spdlog::info("PING");
    if (redis.ping() == "PONG") {
        // spdlog::info("PONG. Redis is working correctly");
        spdlog::info("Redis is working correctly");
    } else {
        spdlog::error("Redis is not working correctly");
    }

    return 0;
}

int main() {
    SystemStart();
    return 0;
}

