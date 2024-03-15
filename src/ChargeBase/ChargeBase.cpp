#include "ChargeBase.h"
#include <algorithm>

namespace charge_base {
    ChargeBase* instance = nullptr;

    ChargeBase* ChargeBase::getInstance(Redis& redis) {
        if (!instance) {
            instance = new ChargeBase( redis); // Constructor
        }
        return instance;
    }

    void ChargeBase::Run() {
        spdlog::set_pattern("[%T.%e][%^%l%$][ChargeBase] %v");

        bool sim_running = (redis.get("sim_running") == "true");
        while (sim_running) {
            // Get the time_point
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            ChargeDrone();
            ReadChargeStream();

            // Check if there is time left in the tick
            auto tick_now = std::chrono::steady_clock::now();
            if (tick_now < tick_start + tick_duration_ms) {
                // Sleep for the remaining time
                std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
            } else if (tick_now > tick_start + tick_duration_ms) {
                // Log if the tick took too long
                spdlog::warn("DroneControl tick {} took too long", tick_n);
                break;
            }
            // Get sim_running from Redis
            sim_running = (redis.get("sim_running") == "true");
            ++tick_n;
        }
    }

    // Read the charge_stream and adds the drones to the vector of drones to charge
    void ChargeBase::ReadChargeStream() {
        using new_Attrs = std::vector<std::pair<std::string, std::string>>; // This NEEDS to be a vector for xread to work
        using new_Item = std::pair<std::string, new_Attrs>;
        using new_ItemStream = std::vector<new_Item>;
        std::unordered_map<std::string, new_ItemStream> new_result;

        // Parse the stream and save/update the drones' data, trims the stream after reading
        try {
            // TODO: Check of number_items_stream is necessary
            // Gets the number of items in the stream
            auto number_items_stream = redis.command<long long>("XLEN", "charge_stream");

            // Reads the stream
            redis.xread("charge_stream", current_stream_id, std::chrono::milliseconds(10), number_items_stream, std::inserter(new_result, new_result.end()));

            // Parses the stream
            for (const auto& item : new_result) {
                // There is only one pair: stream_key and stream_data
                // item.first is the key of the stream. In this case it is "drone_stream"

                // spdlog::info("-----------------Tick {}-----------------", tick_n);
                for (const auto& stream_drone : item.second) {
                    // stream_drone.first is the id of the message in the stream
                    // stream_drone.second is the unordered_map with the data of the drone

                    // Update the local drone data
                    SetChargeData(stream_drone.second);
                }
                // FollowPath the start_id to the last item read
                current_stream_id = item.second.back().first;
            }
            // Trim the stream
            spdlog::info("{} entries read from charge_stream. Trimming the stream.", number_items_stream);
            redis.command<long long>("XTRIM", "charge_stream", "MINID", current_stream_id);
        } catch (const Error& e) {
            spdlog::error("Error reading the stream: {}", e.what());
        }
    }

    void ChargeBase::SetChargeData(const std::vector<std::pair<std::string, std::string>>& data) {
        // The data is structured as a known array
        ext_drone_data temp_drone_struct;
        temp_drone_struct.base_data.id = std::stoi(data[0].second);
        temp_drone_struct.base_data.status = data[1].second;
        temp_drone_struct.base_data.charge = std::stof(data[2].second);
        temp_drone_struct.base_data.position.first = std::stof(data[3].second);
        temp_drone_struct.base_data.position.second = std::stof(data[4].second);
        temp_drone_struct.base_data.zone_id = std::stoi(data[5].second);
        temp_drone_struct.charge_rate = getChargeRate();

        // Update the drone unordered set
        charging_drones.push_back(temp_drone_struct);
    }

    void ChargeBase::ChargeDrone() {
        for (auto &drone: charging_drones) {
            if (drone.base_data.charge < 100) {
                drone.base_data.charge += drone.charge_rate;
            } else if (drone.base_data.charge >= 100) {
                drone.base_data.charge = 100;
                releaseDrone(drone);
            }
        }
    }

    void ChargeBase::releaseDrone(ext_drone_data &drone) {
        // Update the drone's status in Redis
        redis.set("drone:" + std::to_string(drone.base_data.id) + ":status", "IDLE_IN_BASE");
        redis.set("drone:" + std::to_string(drone.base_data.id) + ":charge", "100");
    }

    void ChargeBase::SetEngine(std::random_device& rd) {
        engine = std::mt19937(rd());
    }

    float ChargeBase::getChargeRate() {
        // Generate a random float between 2.0 and 3.0 for the charge rate
        auto generateRandomFloat = [this]() -> float {
            std::uniform_real_distribution<float> dist(2.0f, 3.0f);
            return dist(engine);
        }; //GBT FTW

        return generateRandomFloat();
    }
}

