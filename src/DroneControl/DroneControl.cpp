#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "DroneControl.h"
#include <chrono>
#include <iostream>


/*
DroneControl could access the drones' status on Redis twice every tick:
    - First as soon as a new tick starts
    - As the last thing before the tick ends
By doing this it should be safe to assume that every status update is read.
*/

// FIXME: On the last tick the stream is not trimmed
namespace drone_control {
    DroneControl::DroneControl(Redis& shared_redis) : redis(shared_redis) {};

    void DroneControl::Init() {
        spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");
        spdlog::info("DroneControl process starting");
    }

    // Run the DroneControl process
    void DroneControl::Run() {
        spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");
        spdlog::info("DroneControl process starting");

        utils::SyncWait(redis);

        // std::this_thread::sleep_for(std::chrono::milliseconds (100));

        // TODO: Implement as a thread
        bool sim_running = (redis.get("sim_running") == "true");
        std::cout << "DroneControl sim_running: " << sim_running << std::endl;
        while (sim_running) {
            // Get the time_point
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            ReadStream();

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

    // Reads the stream of data from Redis and updates the drones' data
    void DroneControl::ReadStream() {
        using new_Attrs = std::vector<std::pair<std::string, std::string>>; //This NEEDS to be a vector for xread to work
        using new_Item = std::pair<std::string, new_Attrs>;
        using new_ItemStream = std::vector<new_Item>;
        std::unordered_map<std::string, new_ItemStream> new_result;


        // Parse the stream and save/update the drones' data, trims the stream after reading
        try {
            // TODO: Check of number_items_stream is necessary
            // Gets the number of items in the stream
            auto number_items_stream = redis.command<long long>("XLEN", "drone_stream");
            std::cout << "Tick " << tick_n << "Number of items in the stream: " << number_items_stream << std::endl;

            // Reads the stream
            redis.xread("drone_stream", current_stream_id, std::chrono::milliseconds (10),number_items_stream,std::inserter(new_result, new_result.end()));

            // Parses the stream
            for (const auto& item: new_result) {
                // There is only one pair: stream_key and stream_data
                // item.first is the key of the stream. In this case it is "drone_stream"

                // spdlog::info("-----------------Tick {}-----------------", tick_n);
                for (const auto& stream_drone: item.second) {
                    // stream_drone.first is the id of the message in the stream
                    // stream_drone.second is the unordered_map with the data of the drone

                    // Update the local drone data
                    new_setDroneData(stream_drone.second);
                }
            // Move the start_id to the last item read
            current_stream_id = item.second.back().first;
            }
            // Trim the stream
            std::cout << "Trimming #"<< number_items_stream <<" elements in tick " << tick_n << std::endl;
            redis.command<long long>("XTRIM", "drone_stream", "MINID", current_stream_id);
        } catch (const Error &e) {
            spdlog::error("Error reading the stream: {}", e.what());
        }
    }

    // Updates the local drone data
    void DroneControl::new_setDroneData(const std::vector<std::pair<std::string, std::string>> &data) {
        // The data is structured as a known array
        drone_data temp_drone_struct;
        temp_drone_struct.id = std::stoi(data[0].second);
        temp_drone_struct.status = data[1].second;
        temp_drone_struct.charge = data[2].second;
        temp_drone_struct.position.first = std::stof(data[3].second);
        temp_drone_struct.position.second = std::stof(data[4].second);

        // Update the drone data array
        drones[temp_drone_struct.id] = temp_drone_struct;
        spdlog::info("Drone {} updated: {}, {}, {}, {}",
                    temp_drone_struct.id, temp_drone_struct.status, temp_drone_struct.charge,
                    temp_drone_struct.position.first, temp_drone_struct.position.second);
    }

    // Gets the local data of a drone
    std::unordered_map<std::string, std::string> DroneControl::getData(int drone_id) {
        std::string drone_key = "drone:" + std::to_string(drone_id);
        std::unordered_map<std::string, std::string> redis_data;
        redis.hgetall(drone_key, std::inserter(redis_data, redis_data.begin()));

        return redis_data;
    }
} // drone_control
