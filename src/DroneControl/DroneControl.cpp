/* DroneControl is responsible for:
 * - Reading the stream of data that the drones upload, parse it and transfer it to the Buffer for logging.
 * - Checking if every drone is following the designated path.
 * - Checking every drone charge value and when needed start the swap process.
 * - Acknowladge faults that the DroneZones send to start the process of fault managment.
 */

#include "DroneControl.h"
#include "../../utils/RedisUtils.h"
#include <chrono>

namespace drone_control {
    DroneControl::DroneControl(Redis &shared_redis) : redis(shared_redis) {
        // Connect to db
        db.get_DB();
    };

// Main DroneConrol run function
    void DroneControl::Run() {
        spdlog::info("DroneControl process starting");

        // Wait for other processes
        utils::NamedSyncWait(redis, "DroneControl");

        // Getting drone paths needs to be done after sync is done because this ensures that Drone process has actually
        // uploaded on Redis the path-data
        GetDronePaths();
        spdlog::info("Drone paths loaded");

        // Launch thread to dispatch the drone data to the mini buffers
        boost::thread dispatch_thread(DispatchDroneData, std::ref(buffer), std::ref(mini_buffer_container), std::ref(redis));
        // Lauch thread to write log-entries from mini-buffers to db
        boost::thread write_thread(WriteToDB, std::ref(mini_buffer_container), std::ref(db), std::ref(redis));

        // TODO: Check if the stream-data parsing needs to be implemented as a thread
        auto sim_running = utils::getSimStatus(redis);
        while (sim_running) {
            // Get the real-time start for the tick
            auto tick_start = std::chrono::steady_clock::now();

            // Work
            ReadDataStream();
            FaultAck();

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


    // Read the stream from Redis and update the local drone-data
    void DroneControl::ReadDataStream() {
        using new_Attrs = std::vector<std::pair<std::string, std::string>>; // This NEEDS to be a vector for xread to work
        using new_Item = std::pair<std::string, new_Attrs>;
        using new_ItemStream = std::vector<new_Item>;
        std::unordered_map<std::string, new_ItemStream> new_result;

        try {
            // TODO: Check of number_items_stream is necessary
            // Get number of items in the stream
            auto number_items_stream = redis.command<long long>("XLEN", "drone_stream");

            // Read stream
            redis.xread("drone_stream", current_stream_id, std::chrono::milliseconds(10), number_items_stream,
                        std::inserter(new_result, new_result.end()));

            // Parses the stream
            for (const auto &item: new_result) {
                // There is only one pair: stream_key and stream_data
                // item.first is the key of the stream. In this case it is "drone_stream"
                for (const auto &stream_drone: item.second) {
                    // stream_drone.first is the id of the message in the stream
                    // stream_drone.second is the unordered_map with the data of the drone

                    // Execute work on stream
                    WorkOnStream(stream_drone.second);
                }
                current_stream_id = item.second.back().first;
            }
            // Trim the stream
            spdlog::info("{} entries read from stream. Trimming the stream.", number_items_stream);
            redis.command<long long>("XTRIM", "drone_stream", "MINID", current_stream_id);
        } catch (const Error &e) {
            spdlog::error("Error reading the stream: {}", e.what());
        }
    }

    // Updates the local drone-data, executes the checks and uploads data to Buffer
    void DroneControl::WorkOnStream(const std::vector<std::pair<std::string, std::string>> &data) {
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

        // Check if the drone is following the designated path
        auto check = CheckPath(temp_drone_struct.data.zone_id, temp_drone_struct.data.position);
        checklist[temp_drone_struct.data.zone_id] = check;
        temp_drone_struct.check = check;

        // Check if the drone's charge is enough to go back to the base and check when to swap drones
        CheckDroneCharge(temp_drone_struct.data.id, temp_drone_struct.data.charge, temp_drone_struct.data.charge_needed_to_base);
        CheckForSwap(temp_drone_struct.data.zone_id, temp_drone_struct.data.id, temp_drone_struct.data.charge, temp_drone_struct.data.charge_needed_to_base);

        // Update the local drone-data
        // drones[std::to_string(temp_drone_struct.id)] = temp_drone_struct;

        // Upload the data to the Buffer
        buffer.WriteToBuffer(temp_drone_struct);
    }

    // Get all the DroneZone paths from the Redis. This is for faster access.
    void DroneControl::GetDronePaths() {
        // Loop for every zone in the Redis server
        for (int i = 0; i < ZONE_NUMBER; ++i) {
            std::string path_id = "path:" + std::to_string(i);
            auto path_length = redis.llen(path_id);
            std::vector<std::string> out;
            redis.lrange(path_id, 0, -1, std::back_inserter(out));

#ifdef DEBUG
            spdlog::info("Path on redis {} has {} entries", i, path_length);
            spdlog::info("Path local {} has {} entries", i, out.size());
#endif

            // Save path coords in the drone_paths vector
            for (size_t j = 0; j < path_length; j += 2) {
                float x = std::stof(out[j]);
                float y = std::stof(out[j + 1]);
                drone_paths[i].emplace_back(std::make_pair(x, y));
            }
        }
    }

    // Check if the drone's path is correct
    bool DroneControl::CheckPath(const int zone_id, const std::pair<float, float> &drone_position) {
        // Get the next point in the path
        auto desired_position = drone_paths[zone_id][drone_path_next_index[zone_id]];

        if (drone_position == desired_position) {
            // Update the next point
            ++drone_path_next_index[zone_id];

            // Check if the drone has reached the end of the path
            if (drone_path_next_index[zone_id] == drone_paths[zone_id].size()) {
                drone_path_next_index[zone_id] = 0;
            }
            return true;
        }
        return false;
    }

    // TODO: CheckDroneCharge and CheckForSwap do almost the same thing. Check if they can be merged/changed

    // Check if the drone has enough charge to go back to base
    void DroneControl::CheckDroneCharge(int drone_id, float current_charge, float charge_needed) {
        // Beacuse the function is called for every drone, even if not needed, the function must check if the drone is working
        // The check of 'drone:id:command' is needed because processes are not synced, therfore there might be overlap and command sent more then once
        if ((redis.hget("drone:" + std::to_string(drone_id), "status") == "WORKING") &&
            (current_charge - (DRONE_CONSUMPTION * 80.0f) <= charge_needed) &&
            (redis.get("drone:" + std::to_string(drone_id) + ":command") != "charge")) {
            redis.set("drone:" + std::to_string(drone_id) + ":command", "charge");

#ifdef DEBUG
            spdlog::info("TICK {}: Drone {} needs to charge: current chg: {}%, chg needed: {}%", tick_n, drone_id,
                         current_charge, charge_needed);
#endif
        }
    }

    // Check if the zone needs a drone swap
    void DroneControl::CheckForSwap(int zone_id, int drone_id, float current_charge, float charge_needed) {
        auto cmd = redis.command<OptionalLongLong>("sismember", "zone:" + std::to_string(zone_id) + ":drones_active", std::to_string(drone_id));
        int is_member = static_cast<int>(cmd.value_or(0));
        if (is_member &&
            (redis.get("zone:" + std::to_string(zone_id) + ":swap") != "started") &&
            (current_charge <= ((charge_needed * 2) + (40.0f * DRONE_CONSUMPTION)))) {
            // Add the DroneZone to the set of zones that need a drone swap
            redis.sadd("zones_to_swap", std::to_string(zone_id));
        }
    }

    // When a fault happens, the DroneControl system must acknowledge it to the DroneZone enabling it to start the fault management
    void DroneControl::FaultAck() {
        // Check if there are new faults
        auto faults_on_redis = redis.scard("drones_faults");
        if (faults_on_redis > 0) {
            // Acknowledge the fault
            redis.publish("fault_ack", "ack");

            // Get drones' ids with faults
            std::unordered_set<std::string> faulting_drones_ids;
            redis.smembers("drones_faults", std::inserter(faulting_drones_ids, faulting_drones_ids.end()));

            // Get the fault data and store it in the faults_data vector
            for (const auto &drone_id : faulting_drones_ids) {
                std::unordered_map<std::string, std::string> fault_data;
                redis.hgetall("drones_fault:" + drone_id, std::inserter(fault_data, fault_data.begin()));

                drone_fault temp_fault;
                temp_fault.fault_state = fault_data.at("fault_state");
                temp_fault.drone_id = std::stoi(drone_id);
                temp_fault.zone_id = std::stoi(fault_data.at("zone_id"));
                temp_fault.position.first = stof(fault_data.at("fault_coords_X"));
                temp_fault.position.second = stof(fault_data.at("fault_coords_Y"));
                temp_fault.fault_tick = std::stoi(fault_data.at("tick_start"));
                temp_fault.reconnect_tick = std::stoi(fault_data.at("reconnect_tick"));

                // Write the fault to the Buffer
                buffer.WriteFault(temp_fault);
            }
            // Clear the faults from the Redis server
            redis.del("drones_faults");
        }
    }
} // namespace drone_control
