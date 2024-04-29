/** DroneControl explanation
 * The DroneControl class is the main class of the DroneControl process. It is responsible for reading the stream of data
 * from Redis and updating the drones' data. It also creates a log-entry in PostgreSQL for each update received.
 * For each drone data read from the stream it calls the function CheckPath to check if the drone is following the predefined path
 */

#include "DroneControl.h"
#include <chrono>
#include <iostream>

namespace drone_control {
    DroneControl::DroneControl(Redis &shared_redis) : redis(shared_redis) {
        // Initialize the database

        db.get_DB();
    };

// Run the DroneControl process
    void DroneControl::Run() {
        // Initial setup
        spdlog::info("DroneControl process starting");

        // Init the buffers

//        std::this_thread::sleep_for(std::chrono::milliseconds(200));
//      utils::SyncWait(redis);
        utils::NamedSyncWait(redis, "DroneControl");

        // First thing to do is to get all the drone paths from the Redis server
        GetDronePaths();
        spdlog::info("Drone paths loaded");

        // Launch the thread to dispatch the drone data to the mini buffers
        boost::thread dispatch_thread(DispatchDroneData, std::ref(buffer), std::ref(mini_buffer_container), std::ref(redis));
        boost::thread write_thread(WriteToDB, std::ref(mini_buffer_container), std::ref(db), std::ref(redis));
//        spdlog::info("Threads started");

        // TODO: Implement as a thread
        auto sim_running = utils::getSimStatus(redis);
//        spdlog::info("Sim status: {}", sim_running);
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
                // break;
            }
            // Get sim_running from Redis
            sim_running = (redis.get("sim_running") == "true");
            ++tick_n;
        }
    }

// Reads the stream of data from Redis and updates the drones' data
    void DroneControl::ReadStream() {
//        std::cout << "Creating stream data structure" << std::endl;
        using new_Attrs = std::vector<std::pair<std::string, std::string>>; // This NEEDS to be a vector for xread to work
        using new_Item = std::pair<std::string, new_Attrs>;
        using new_ItemStream = std::vector<new_Item>;
        std::unordered_map<std::string, new_ItemStream> new_result;

        // Parse the stream and save/update the drones' data, trims the stream after reading
        try {
            // TODO: Check of number_items_stream is necessary
            // Gets the number of items in the stream
            auto number_items_stream = redis.command<long long>("XLEN", "drone_stream");
//            std::cout << number_items_stream << " elements in stream" << std::endl;
            // Reads the stream
            redis.xread("drone_stream", current_stream_id, std::chrono::milliseconds(10), number_items_stream,
                        std::inserter(new_result, new_result.end()));
//            std::cout << "Stream read" << std::endl;
            // Parses the stream
            for (const auto &item: new_result) {
                // There is only one pair: stream_key and stream_data
                // item.first is the key of the stream. In this case it is "drone_stream"
                for (const auto &stream_drone: item.second) {
                    // stream_drone.first is the id of the message in the stream
                    // stream_drone.second is the unordered_map with the data of the drone

                    // Update the local drone data
                    ParseStreamData(stream_drone.second);
                }
                // FollowPath the start_id to the last item read
                current_stream_id = item.second.back().first;
            }
            // Trim the stream
            spdlog::info("{} entries read from stream. Trimming the stream.", number_items_stream);
            redis.command<long long>("XTRIM", "drone_stream", "MINID", current_stream_id);
        } catch (const Error &e) {
            spdlog::error("Error reading the stream: {}", e.what());
        }
    }

// Updates the local drone data and executes the check for the drone's path
    void DroneControl::ParseStreamData(const std::vector<std::pair<std::string, std::string>> &data) {
        // The data is structured as a known array
        drone_data_ext temp_drone_struct;
        temp_drone_struct.data.id = std::stoi(data[0].second);
        temp_drone_struct.data.status = data[1].second;
        temp_drone_struct.data.charge = std::stof(data[2].second);
        temp_drone_struct.data.position.first = std::stof(data[3].second);
        temp_drone_struct.data.position.second = std::stof(data[4].second);
        temp_drone_struct.data.zone_id = std::stoi(data[5].second);
        temp_drone_struct.data.charge_needed_to_base = std::stof(data[6].second);
        temp_drone_struct.tick_n = std::stoi(data[7].second);

//        spdlog::info("Drone {} at tick {}", temp_drone_struct.data.id, temp_drone_struct.tick_n);

        auto check = CheckPath(temp_drone_struct.data.zone_id, temp_drone_struct.data.position);
        checklist[temp_drone_struct.data.zone_id] = check;
        temp_drone_struct.check = check;

        // Check if the drone's charge is enough to go back to the base and check when to swap drones
        CheckDroneCharge(temp_drone_struct.data.id, temp_drone_struct.data.charge, temp_drone_struct.data.charge_needed_to_base);
        CheckForSwap(temp_drone_struct.data.zone_id, temp_drone_struct.data.id, temp_drone_struct.data.charge, temp_drone_struct.data.charge_needed_to_base);

        // Update the drone data array
        // drones[std::to_string(temp_drone_struct.id)] = temp_drone_struct;

        // Upload the data to the database
        // db.logDroneData(temp_drone_struct, checklist);
        buffer.WriteToBuffer(temp_drone_struct);
    }

// Get all the drone paths from the Redis server. Each index is a specific drone ID. This is for faster access.
    void DroneControl::GetDronePaths() {
        // Loop for every drone path in the Redis server
        for (int i = 0; i < ZONE_NUMBER; ++i) {
            std::string path_id = "path:" + std::to_string(i);
            auto path_length = redis.llen(path_id);
            std::vector<std::string> out;
            redis.lrange(path_id, 0, -1, std::back_inserter(out));
#ifdef DEBUG
            spdlog::info("Path on redis {} has {} entries", i, path_length);
            spdlog::info("Path local {} has {} entries", i, out.size());
#endif
            for (size_t j = 0; j < path_length; j += 2) {
                float x = std::stof(out[j]);
                float y = std::stof(out[j + 1]);
                drone_paths[i].emplace_back(std::make_pair(x, y));
            }
        }
    }

// Check if the drone's path is correct
    bool DroneControl::CheckPath(int zone_id, std::pair<float, float> &drone_position) {
        // Get the next point in the path
        auto next_point = drone_paths[zone_id][drone_path_next_index[zone_id]];

        // Check if the drone is in the next point
        if (drone_position == next_point) {
            // Update the next point
            ++drone_path_next_index[zone_id];
            return true;
        } else {
            return false;
        }
    }

//       Check if the drone has enough charge to go back to the base and check when to swap drones
//       Create variable instead of converting/getting the value each time
    void DroneControl::CheckDroneCharge(int drone_id, float current_charge, float charge_needed) {
        if ((redis.hget("drone:" + std::to_string(drone_id), "status") == "WORKING") &&
            (current_charge - (DRONE_CONSUMPTION * 80.0f) <= charge_needed) &&
            (redis.get("drone:" + std::to_string(drone_id) + ":command") != "charge")) {
            redis.set("drone:" + std::to_string(drone_id) + ":command", "charge");

            spdlog::info("TICK {}: Drone {} needs to charge: current chg: {}%, chg needed: {}%", tick_n, drone_id,
                         current_charge, charge_needed);
        }
    }

    void DroneControl::CheckForSwap(int zone_id, int drone_id, float current_charge, float charge_needed) {
        auto cmd = redis.command<OptionalLongLong>("sismember", "zone:" + std::to_string(zone_id) + ":drones_active", std::to_string(drone_id));
        int is_member = static_cast<int>(cmd.value_or(0));
        if (is_member &&
            (redis.get("zone:" + std::to_string(zone_id) + ":swap") != "started") &&
            (current_charge <= ((charge_needed * 2) + (40.0f * DRONE_CONSUMPTION)))) {
            // Create a redis list of all the zones_vertex that need to be switched on
            redis.sadd("zones_to_swap", std::to_string(zone_id));
        }
    }
} // namespace drone_control
