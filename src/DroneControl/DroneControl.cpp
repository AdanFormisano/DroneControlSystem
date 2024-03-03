#include "spdlog/spdlog.h"
#include "../../utils/RedisUtils.h"
#include "../../utils/utils.h"
#include "DroneControl.h"
#include <chrono>


/*
DroneControl could access the drones' status on Redis twice every tick:
    - First as soon as a new tick starts
    - As the last thing before the tick ends
By doing this it should be safe to assume that every status update is read.
*/

// FIXME: On the last tick the stream is not trimmed
namespace drone_control {
    DroneControl::DroneControl(Redis& shared_redis) : redis(shared_redis) {};

    void Init(Redis& redis) {
        spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");
        spdlog::info("DroneControl process starting");

        // Create DroneControl
        DroneControl dc(redis);

        // Initialization finished
        utils::SyncWait(redis);

        // FIXME: The simulation loop should be inside the run function
        // Get sim_running from Redis
        dc.tick_n = 0;
        bool sim_running = (redis.get("sim_running") == "true");
        while (sim_running) {
            // Get the time_point
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            dc.Run();

            // Check if there is time left in the tick
            auto tick_now = std::chrono::steady_clock::now();
            if (tick_now < tick_start + tick_duration_ms) {
                // Sleep for the remaining time
                std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
            } else if (tick_now > tick_start + tick_duration_ms) {
                // Log if the tick took too long
                spdlog::warn("DroneControl tick {} took too long", dc.tick_n);
                break;
            }
            // Get sim_running from Redis
            sim_running = (redis.get("sim_running") == "true");
        }
    }

    // Run the DroneControl process
    void DroneControl::Run() {
        // TODO: This should be a while loop that runs until the program is stopped or implemented as a thread
        ReadStream();
        ++tick_n;
    }

    // Reads the stream of data from Redis and updates the drones' data
    void DroneControl::ReadStream() {
        using Attrs = std::unordered_map<std::string, std::string>; // This is the data of the drone
        using Item = std::pair<std::string, Attrs>;                 // This is the id (will be the msg id once the stream is read) and the data of the drone
        using ItemStream = std::vector<Item>;                       // This is the stream of data
        std::unordered_map<std::string, ItemStream> result;         // This is the key of the stream and the stream of data

        // Parse the stream and save/update the drones' data, trims the stream after reading
        try {
            // TODO: Check of number_items_stream is necessary
            // Gets the number of items in the stream
            auto number_items_stream = redis.command<long long>("XLEN", "drone_stream");

            // Reads the stream
            redis.xread("drone_stream", current_stream_id, std::chrono::milliseconds(1000), number_items_stream,std::inserter(result, result.end()));

            // Parses the stream
            for (const auto& item: result) {
                // There is only one pair: stream_key and stream_data
                // item.first is the key of the stream. In this case it is "drone_stream"

                spdlog::info("-----------------Tick {}-----------------", tick_n);
                for (const auto& stream_drone: item.second) {
                    // stream_drone.first is the id of the message in the stream
                    // stream_drone.second is the unordered_map with the data of the drone

                    // Update the local drone data
                    setDroneData(stream_drone.second);
                }
            // Move the start_id to the last item read
            current_stream_id = item.second.back().first;
            }
            // Trim the stream
            redis.command<long long>("XTRIM", "drone_stream", "MINID", current_stream_id);
        } catch (const Error &e) {
            spdlog::error("Error reading the stream: {}", e.what());
        }
    }

    // Updates the local drone data
    void DroneControl::setDroneData(const std::unordered_map<std::string, std::string>& data) {
        drone_data temp_drone_struct;

        // Uses a temp struct to get the data from the unordered_map
        for (const auto& item : data) {
            const auto& key = item.first;
            const auto& value = item.second;

            if (key == "id") {
                // Update the status of the drone
                temp_drone_struct.id = std::stoi(value);
                spdlog::info("Drone ID: {}", temp_drone_struct.id);
            } else if (key == "status") {
                // Update the status of the drone
                temp_drone_struct.status = value;
                spdlog::info("Drone status: {}", temp_drone_struct.status);
            } else if (key == "charge") {
                // Update the charge of the drone
                temp_drone_struct.charge = value;
                spdlog::info("Drone charge: {}", temp_drone_struct.charge);
            } else if (key == "X") {
                // Update the X position of the drone
                temp_drone_struct.position.first = std::stof(value);
                spdlog::info("Drone X position: {}", temp_drone_struct.position.first);
            } else if (key == "Y") {
                // Update the Y position of the drone
                temp_drone_struct.position.second = std::stof(value);
                spdlog::info("Drone Y position: {}", temp_drone_struct.position.second);
            }
            StatusHandler(temp_drone_struct);
        }
        // Update the drone data array
        drones[temp_drone_struct.id] = temp_drone_struct;
    }

    // Gets the local data of a drone
    std::unordered_map<std::string, std::string> DroneControl::getData(int drone_id) {
        std::string drone_key = "drone:" + std::to_string(drone_id);
        std::unordered_map<std::string, std::string> redis_data;
        redis.hgetall(drone_key, std::inserter(redis_data, redis_data.begin()));

        return redis_data;
    }

    int DroneControl::check_at_base(int id){
        for(int i = 0; i < atBase.size(); ++i) {
            if (id == atBase[i]) {
                return i;
            };
        }
        return -1;
    }

    void DroneControl::StatusHandler(drone_data& dd){
         int status_id = StatusMap[dd.status];
         switch (status_id) {
            case 1:
                 // TODO : Funtion that calculates the charge needed to get back to the base
                 // TODO : IF the charge is less than charge needed + margin send the d to the charge base
            case 2:
                if(check_at_base(dd.id) == -1){
                    atBase.push_back(dd.id);
                };
               // TODO : DO NOTHING
               // TODO : If the charge is less than the charge needed for a roundtrip don't let the drone out
             case 3:
                 atBase.erase(atBase.begin() + check_at_base(dd.id));
               // TODO: Send the drone back to its zone

             default:
                 //yarragimi ye
         };
    }
} // drone_control
