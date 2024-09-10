#include "SyncedDroneControl.h"

#include <iostream>

void SyncedDroneControl::Consume(Redis& redis, const std::string& stream, const std::string& group, const std::string& consumer) {
    spdlog::info("Consumer {} started", consumer);
    while (true) {
        // Read from the stream using the consumer group
        try {
            // Attempt to read one message per thread per loop
            using Attrs = std::vector<std::pair<std::string, std::string>>;
            using Item = std::pair<std::string, Attrs>;
            using ItemStream = std::vector<Item>;
            std::unordered_map<std::string, ItemStream> result;

            std::vector<DroneData> drones_data;

            // 'count' is the number of elements read from the stream at once
            redis.xreadgroup(group, consumer, stream, ">", 300, std::inserter(result, result.end()));

            for (const auto& message : result) {
                for (const auto& item : message.second) {
                    // std::cout << "Consumer: " << consumer << ", Message: " << item.first << " - " << item.second << std::endl;
                    ParseStreamData(item.second, drones_data);

                    // Acknowledge the message (after processing)
                    redis.xack(stream, group, {item.first});
                }
                buffer.WriteBatchToBuffer(drones_data);
            }
        } catch (const TimeoutError& e) {
            std::cerr << "Timeout while waiting for message: " << e.what() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        } catch (...)
        {
            std::cerr << "Unknown error" << std::endl;
        }
    }
}

void SyncedDroneControl::ParseStreamData(const std::vector<std::pair<std::string, std::string>>& data, std::vector<DroneData>& drones_data)
{
    DroneData temp_data;
    temp_data.tick_n = data[0].second;
    temp_data.id = data[1].second;
    temp_data.status = data[2].second;
    temp_data.charge = data[3].second;
    temp_data.x = data[4].second;
    temp_data.y = data[5].second;
    temp_data.wave_id = data[6].second;
    temp_data.checked = data[7].second;

    // spdlog::info("Tick {} - Drone {} ({}, {})", temp_data.tick_n, temp_data.id, temp_data.x, temp_data.y);
    drones_data.push_back(temp_data);
}

void SyncedDroneControl::WriteDroneDataToDB()
{
    spdlog::info("DB writer thread started");
    while (true)
    {
        // Take data from buffer
        // spdlog::info("Reading data from buffer ({} elements)", buffer.getSize());
        std::vector<DroneData> drones_data = buffer.GetAllData();

        if (!drones_data.empty())
        {
            spdlog::info("Writing TICK {} to DB", drones_data.front().tick_n);
            // Write to DB
            std::string query = "INSERT INTO drone_logs (tick_n, drone_id, status, charge, wave, x, y, checked) VALUES ";
            for (const auto& data : drones_data)
            {
                                    query += "(" + data.tick_n + ", "
                                    + data.id + ", '"
                                    + data.status + "', "
                                    + data.charge + ", "
                                    + data.wave_id + ", "
                                    + data.x + ", "
                                    + data.y + ", "
                                    + data.checked + "),";
            }
            query.pop_back();
            query += ";";
            db.ExecuteQuery(query);
        }
    }
}

void SyncedDroneControl::Run()
{
    // Get the drone paths from the database
    // GetDronesPaths();

    utils::NamedSyncWait(redis, "DroneControl");

    std::vector<std::thread> consumers;

    try
    {
        redis.xgroup_create(stream, group, "$", true);
    }
    catch (const std::exception& e)
    {
        spdlog::error("Error creating consumer group: {}", e.what());
    }
    for (int i = 0; i < num_consumers; i++) {
        consumers.emplace_back(&SyncedDroneControl::Consume, this, std::ref(redis), stream, group, std::to_string(i));
    }

    // Create thread for writing to DB
    std::thread db_thread(&SyncedDroneControl::WriteDroneDataToDB, this);

    while (true)
    {
        //
    }
}

// TODO: Implement sending commands to ScannerManager
