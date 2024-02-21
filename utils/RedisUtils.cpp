#include <spdlog/spdlog.h>
#include <iostream>
#include "RedisUtils.h"
#include "../src/globals.h"

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

    long long RedisGetClientID(Redis& redis) {
        try {
            auto clientID = redis.command<long long>("CLIENT", "ID");
            return clientID;
        }
        catch (const sw::redis::IoError& e) {
            spdlog::error("Couldn't get ID: {}", e.what());
            return -1;
        }
    }

    void SyncWait(Redis& redis) {
        bool sync_done = false;
        redis.decr(sync_counter_key);
        auto value = redis.get(sync_counter_key);

        try {
            int current_count = std::stoi(value.value());

            if (current_count == 0) {
                redis.publish(sync_channel, "SYNC_DONE");
                spdlog::info("SYNC_DONE sent");
            } else {
                auto sub = redis.subscriber();
                sub.subscribe(sync_channel);

                // Wait for the SYNC_DONE message
                sub.on_message([&sub, &sync_done](const std::string &channel, const std::string &msg) {
                    if (msg == "SYNC_DONE") {
                        spdlog::info("SYNC_DONE received");
                        sync_done = true;
                        sub.unsubscribe(sync_channel);
                    } else {
                        spdlog::error("Unknown message: {}", msg);
                    }
                });

                while (!sync_done) {
                    try {
                        // This will block the process until a message is received
                        sub.consume();
                    } catch (const Error &err) {
                        spdlog::error("Error consuming messages");
                    }
                }
            }
        } catch (const std::invalid_argument& e) {
            spdlog::error("Invalid argument: {}", e.what());
        } catch (const std::out_of_range& e) {
            spdlog::error("Out of range: {}", e.what());
        }
    }
}
