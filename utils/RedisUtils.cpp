#include "RedisUtils.h"
#include "../src/globals.h"
#include <filesystem>
#include <iostream>

namespace utils {
// TODO: Check if it's used
int RedisConnectionCheck(Redis &redis, std::string clientName) {
    try {
        std::cout << "Waiting Redis-server responce to " << clientName << std::endl;
        redis.ping();
        auto clientID = redis.command<long long>("CLIENT", "ID");
        std::cout << clientName << "'s connection successful (Client ID: " << clientID << ")" << std::endl;
    } catch (const sw::redis::IoError &e) {
        std::cout << R"({}'s connection failed: {}), clientName, e.what()";
        return 1;
    }
    return 0;
}
// TODO: Check if it's used
long long RedisGetClientID(Redis &redis) {
    try {
        auto clientID = redis.command<long long>("CLIENT", "ID");
        return clientID;
    } catch (const sw::redis::IoError &e) {
        //spdlog::error("Couldn't get ID: {}", e.what());
        return -1;
    }
}

// Utility function to get the simulation status from Redis
bool getSimStatus(Redis &redis) {
    //spdlog::info("Getting sim status from Redis");
    auto r = redis.get("sim_running");

    if (r.has_value()) {
        //spdlog::info("Sim status: {}", r.value());
        return r.value() == "true";
    } else {
        //spdlog::error("Couldn't get sim status");
        return false;
    }
}

// Add the caller process to the set of waiting processes on Redis
void AddThisProcessToSyncCounter(Redis &redis, const std::string &process_name) {
    redis.sadd(sync_counter_key, process_name);
}

// Once the process has initialized, if it isn't the last process self-remove from set on Redis and subscribes to
// the sync_channel waiting for sync done.
// The last process execute the function checks if set is empty, then ends and confirms sync of processes.
int NamedSyncWait(Redis &redis, const std::string &process_name) {
    try {
        bool sync_done = false;

        // Check if the process is in the set
        auto cmd = redis.command<OptionalLongLong>("sismember", sync_counter_key, process_name);
        int is_member = static_cast<int>(cmd.value_or(0));
        if (!is_member) {
           // spdlog::error("Process {} not in the set", process_name);
            return 1;
        }
        redis.srem(sync_counter_key, process_name);

        // Get the current count
        auto count = redis.scard(sync_counter_key);

        // If the counter is 0 all processes are ready
        if (count == 0) {
            // Start the simulation
            redis.set("sim_running", "true");
            // spdlog::info("----SIMULATION STARTED----");

            // Notify all processes that they can continue and sync is done
            redis.publish(sync_channel, "SYNC_DONE");
            // spdlog::info("SYNC IS DONE!");

        } else {
            // If the counter is not 0, there are still processes working, wait for the SYNC_DONE message

            // Create a Redis subscriber to listen at the sync_channel
            auto sub = redis.subscriber();
            sub.subscribe(sync_channel);

            // Wait for the SYNC_DONE message
            sub.on_message([&sub, &sync_done](const std::string &channel, const std::string &msg) {
                if (msg == "SYNC_DONE") {
                    sync_done = true;
                    sub.unsubscribe(sync_channel);
                } else {
                    // spdlog::error("Unknown message: {}", msg);
                }
            });

            while (!sync_done) {
                try {
                    // This will block the process until a message is received
                    sub.consume();
                } catch (const Error &err) {
                    // spdlog::error("Error consuming messages");
                }
            }
        }
        return 0;
    } catch (const std::invalid_argument &e) {
        // spdlog::error("Invalid argument: {}", e.what());
        return 1;
    } catch (const std::out_of_range &e) {
        // spdlog::error("Out of range: {}", e.what());
        return 1;
    } catch (const Error &e) {
        // spdlog::error("Error: {}", e.what());
        return 1;
    }
}

// TODO Clean up unused functions

void UpdateSyncTick(Redis &redis, int tick_n) {
    // spdlog::info("Updating scanner_tick to {}", tick_n);
    redis.set("scanner_tick", std::to_string(tick_n));

    // std::pair tick_ack = {tick_n, 0};
    // redis.rpush("scanner_tick_ack", tick_ack.first, tick_ack.second);
}

void AckSyncTick(Redis &redis, int tick_n) {
    // redis.set("scanner_tick_ack", std::to_string(tick_n));
    redis.incr("scanner_tick_ack");
}

bool CheckSyncTickAck(Redis &redis, int tick_n) {
    auto _ = redis.get("scanner_tick_ack");
    int current_tick = std::stoi(_.value_or("-1"));
    return current_tick == tick_n;
}

int GetSyncTick(Redis &redis) {
    auto _ = redis.get("scanner_tick");
    return std::stoi(_.value_or("-1"));
}

void clearRedis() {
    try {
        auto redis = sw::redis::Redis("tcp://127.0.0.1:6379");
        redis.flushall();
        std::cout << "Redis cache cleared." << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Errore nella connessione a Redis: " << e.what() << std::endl;
    }
}

void clearCache(const std::string &cachePath) {
    try {
        std::filesystem::remove_all(cachePath);
        std::filesystem::create_directory(cachePath);
        std::cout << "Cache cleared at " << cachePath << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "Errore nella cancellazione della cache: " << e.what() << std::endl;
    }
}

void clearRedisCache() {
    // Connessione a Redis
    redisContext *context = redisConnect("127.0.0.1", 7777);
    if (context == nullptr || context->err) {
        if (context) {
            std::cerr << "Errore nella connessione a Redis: " << context->errstr << std::endl;
        } else {
            std::cerr << "Impossibile allocare il contesto Redis" << std::endl;
        }
        return;
    }

    // Comando per svuotare la cache
    redisReply *reply = (redisReply *)redisCommand(context, "FLUSHALL");
    if (reply == nullptr) {
        std::cerr << "Errore nell'esecuzione del comando FLUSHALL" << std::endl;
    } else {
        std::cout << "Cache di Redis svuotata" << std::endl;
    }

    // Libera la memoria
    freeReplyObject(reply);
    redisFree(context);
}

} // namespace utils
