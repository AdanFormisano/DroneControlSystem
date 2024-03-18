/** DroneControl explanation
 * The DroneControl class is the main class of the DroneControl process. It is responsible for reading the stream of data
 * from Redis and updating the drones' data. It also creates a log-entry in PostgreSQL for each update received.
 * For each drone data read from the stream it calls the function CheckPath to check if the drone is following the predefined path
 */

#include "DroneControl.h"
#include "../../utils/RedisUtils.h"
#include "spdlog/spdlog.h"
#include <chrono>
#include <iostream>

#include "../../utils/RedisUtils.h"
#include "spdlog/spdlog.h"
namespace drone_control {
DroneControl::DroneControl(Redis &shared_redis) : redis(shared_redis) {
    db.get_DB();
};

// Run the DroneControl process
void DroneControl::Run() {
    spdlog::set_pattern("[%T.%e][%^%l%$][DroneControl] %v");
    spdlog::info("DroneControl process starting");

    utils::SyncWait(redis);

    // First thing to do is to get all the drone paths from the Redis server
    GetDronePaths();

    // std::this_thread::sleep_for(std::chrono::milliseconds (100));

    // TODO: Implement as a thread
    bool sim_running = (redis.get("sim_running") == "true");
    while (sim_running) {
        // Get the time_point
        auto tick_start = std::chrono::steady_clock::now();

        // Work
        ReadStream();

        //TESTING: Tell drone[tick_n-1] to go to work
        if (tick_n > 0) {
            int drone_id = tick_n - 1;
            redis.set("drone:"+std::to_string(drone_id)+":command", "work");
        }

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
    using new_Attrs = std::vector<std::pair<std::string, std::string>>; // This NEEDS to be a vector for xread to work
    using new_Item = std::pair<std::string, new_Attrs>;
    using new_ItemStream = std::vector<new_Item>;
    std::unordered_map<std::string, new_ItemStream> new_result;

    // Parse the stream and save/update the drones' data, trims the stream after reading
    try {
        // TODO: Check of number_items_stream is necessary
        // Gets the number of items in the stream
        auto number_items_stream = redis.command<long long>("XLEN", "drone_stream");

        // Reads the stream
        redis.xread("drone_stream", current_stream_id, std::chrono::milliseconds(10), number_items_stream, std::inserter(new_result, new_result.end()));

        // Parses the stream
        for (const auto& item : new_result) {
            // There is only one pair: stream_key and stream_data
            // item.first is the key of the stream. In this case it is "drone_stream"

            // spdlog::info("-----------------Tick {}-----------------", tick_n);
            for (const auto& stream_drone : item.second) {
                // stream_drone.first is the id of the message in the stream
                // stream_drone.second is the unordered_map with the data of the drone

                // Update the local drone data
                new_setDroneData(stream_drone.second);
            }
            // FollowPath the start_id to the last item read
            current_stream_id = item.second.back().first;
        }
        // Trim the stream
        spdlog::info("{} entries read from stream. Trimming the stream.", number_items_stream);
        redis.command<long long>("XTRIM", "drone_stream", "MINID", current_stream_id);
    } catch (const Error& e) {
        spdlog::error("Error reading the stream: {}", e.what());
    }
}

// Updates the local drone data and executes the check for the drone's path
void DroneControl::new_setDroneData(const std::vector<std::pair<std::string, std::string>>& data) {
    // The data is structured as a known array
    drone_data temp_drone_struct;
    temp_drone_struct.id = std::stoi(data[0].second);
    // Make the check for the drone's path and add the result to the checklist

    temp_drone_struct.status = data[1].second;
    temp_drone_struct.charge = std::stof(data[2].second);
    temp_drone_struct.position.first = std::stof(data[3].second);
    temp_drone_struct.position.second = std::stof(data[4].second);
    temp_drone_struct.zone_id = std::stoi(data[5].second);
    temp_drone_struct.charge_needed_to_base = std::stof(data[6].second);

    checklist[temp_drone_struct.id] = CheckPath(temp_drone_struct.id, temp_drone_struct.position);
    // Check if the drone's charge is enough to go back to the base
    CheckDroneCharge(temp_drone_struct.id, temp_drone_struct.charge, temp_drone_struct.charge_needed_to_base);

    // Update the drone data array
    drones[temp_drone_struct.id] = temp_drone_struct;

//    spdlog::info("Drone {} updated: {}, {}, {}, {}, {}, {}",
//                 temp_drone_struct.id, temp_drone_struct.status, temp_drone_struct.charge,
//                 temp_drone_struct.position.first, temp_drone_struct.position.second,
//                 temp_drone_struct.zone_id, temp_drone_struct.charge_needed_to_base);

    // Upload the data to the database
    // db.logDroneData(temp_drone_struct, checklist);
}

// Gets the local data of a drone
std::unordered_map<std::string, std::string> DroneControl::getData(int drone_id) {
    std::string drone_key = "drone:" + std::to_string(drone_id);
    std::unordered_map<std::string, std::string> redis_data;
    redis.hgetall(drone_key, std::inserter(redis_data, redis_data.begin()));

    return redis_data;
}

// Get all the drone paths from the Redis server. Each index is a specific drone ID. This is for faster access.
void DroneControl::GetDronePaths() {
    // Loop for every drone path in the Redis server
    for (int i = 0; i < 300; ++i) {
        std::string path_id = "path:" + std::to_string(i);
        auto path_length = redis.llen(path_id);
        // Loop for every entry in the path-loop
        for (int j = 0; j < 124; ++j) {
            auto temp_x = redis.lpop(path_id);
            auto temp_y = redis.lpop(path_id);
            if (temp_x.has_value() && temp_y.has_value()) {
                std::string x = temp_x.value();
                std::string y = temp_y.value();

                drone_paths[i][j].first = std::stof(x);
                drone_paths[i][j].second = std::stof(y);
            } else {
                spdlog::error("Error getting the drone {} path from Redis", i);
            }
        }
    }
}

// Check if the drone's path is correct
bool DroneControl::CheckPath(int drone_id, std::pair<float, float>& drone_position) {
    // Get the next point in the path
    auto next_point = drone_paths[drone_id][drone_path_next_index[drone_id]];

    // Check if the drone is in the next point
    if (drone_position == next_point) {
        // Update the next point
        ++drone_path_next_index[drone_id];
        return true;
    } else {
        return false;
    }
}

// Check if the drone has enough charge to go back to the base
void DroneControl::CheckDroneCharge(int drone_id, float current_charge, float charge_needed) {
    if (current_charge - (DRONE_CONSUMPTION * 80.0f) <= charge_needed) {
        redis.set("drone:"+std::to_string(drone_id)+":command", "charge");
        spdlog::info("TICK {}: Drone {} needs to charge: current chg: {}%, chg needed: {}%", tick_n, drone_id, current_charge, charge_needed);
    }
}
}  // namespace drone_control
