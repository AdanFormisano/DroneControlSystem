#include "DroneControl.h"

#include <iostream>

void DroneControl::Consume(Redis& redis, const std::string& stream, const std::string& group, const std::string& consumer,
                           const std::array<std::unordered_set<coords>, 300> *drones_paths)
{
    while (true)
    {
        // Read from the stream using the consumer group
        try
        {
            // Attempt to read one message per thread per loop
            using Attrs = std::vector<std::pair<std::string, std::string>>;
            using Item = std::pair<std::string, Attrs>;
            using ItemStream = std::vector<Item>;
            std::unordered_map<std::string, ItemStream> result;

            std::vector<DroneData> drones_data;

            // 'count' is the number of elements read from the stream at once
            redis.xreadgroup(group, consumer, stream, ">", 300, std::inserter(result, result.end()));

            for (const auto& message : result)
            {
                for (const auto& item : message.second)
                {
                    std::string status = item.second[2].second;
                    int drone_id = std::stoi(item.second[1].second);
                    const int drone_index = drone_id % 1000;
                    const float x = std::stof(item.second[4].second);
                    const float y = std::stof(item.second[5].second);
                    std::string checked = "FALSE";

                    // Check if a working drone is the right path position
                    if (status == "WORKING")
                    {
                        // Check if the current position is inside the path array
                        if (coords current_position = {x, y}; drones_paths->at(drone_index).contains(current_position))
                        {
                            checked = "TRUE";
                        }
                    }

                    drones_data.emplace_back(
                        item.second[0].second, // tick_n
                        item.second[1].second, // id
                        item.second[2].second, // status
                        item.second[3].second, // charge
                        item.second[4].second, // x
                        item.second[5].second, // y
                        item.second[6].second, // wave_id
                        checked // checked
                    );

                    // Acknowledge the message (after processing)
                    redis.xack(stream, group, {item.first});
                }
                buffer.WriteBatchToBuffer(drones_data);
            }
        }
        catch (const TimeoutError& e)
        {
            std::cerr << "Timeout while waiting for message: " << e.what() << std::endl;
        } catch (const std::exception& e)
        {
            std::cerr << "Error: " << e.what() << std::endl;
        } catch (...)
        {
            std::cerr << "Unknown error" << std::endl;
        }
    }
}

void DroneControl::WriteDroneDataToDB()
{
    // spdlog::info("DB writer thread started");
    std::cout << "DB writer thread started" << std::endl;

    int max_tick = 0;   // Maximum tick number read from the buffer

    while (max_tick < sim_duration_ticks - 1)
    {
        // Take data from buffer
        // spdlog::info("Reading data from buffer ({} elements)", buffer.getSize());
        std::vector<DroneData> drones_data = buffer.GetAllData();

        if (!drones_data.empty())
        {
            // spdlog::info("Writing TICK {} to DB", drones_data.front().tick_n);
            // std::cout << "Writing TICK " << drones_data.front().tick_n << " to DB" << std::endl;
            // Write to DB
            std::string query =
                "INSERT INTO drone_logs (tick_n, drone_id, status, charge, wave, x, y, checked) VALUES ";
            for (const auto& data : drones_data)
            {
                // Set the maximum tick number
                max_tick = std::max(max_tick, std::stoi(data.tick_n));

                query += "(" + data.tick_n + ", " + data.id + ", '" + data.status + "', " + data.charge + ", " + data.
                    wave_id + ", " + data.x + ", " + data.y + ", " + data.checked + "),";
            }
            query.pop_back();
            query += ";";
            db.ExecuteQuery(query);
        }
    }
    // spdlog::info("DB writer thread finished");
    std::cout << "DB writer thread finished" << std::endl;
}

void DroneControl::SendWaveSpawnCommand()
{
    // spdlog::warn("Sending wave spawn command");
    std::cout << "Sending wave spawn command" << std::endl;
    redis.incr("spawn_wave");

    // Wait for the wave to spawn
    while (std::stoi(redis.get("spawn_wave").value_or("-1")) != 0)
    {
        // spdlog::warn("Waiting for wave to spawn...");
        std::cout << "Waiting for wave to spawn..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    // spdlog::warn("Wave spawned");
    std::cout << "Wave spawned" << std::endl;
}

void DroneControl::GetDronePaths()
{
    // Calculate the paths for the working drones
    int Y = -2990;

    // Iterate over the drones
    for (int i = 0; i < 300; i++)
    {
        coords step = {-3010.0f, static_cast<float>(Y)};

        // Iterate over the steps
        for (int j = 0; j < 299; j++)
        {
            step.x += 20.0f;
            drones_paths[i].insert(step);
        }
        Y += 20;
    }
}


void DroneControl::Run()
{
    // Get the drone paths from the database
    // GetDronesPaths();

    // Create or open the semaphore for synchronization
    sem_t* sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_dc", 0);
    sem_t* sem_dc = utils_sync::create_or_open_semaphore("/sem_dc", 0);

    // Calculate the paths for the working drones
    GetDronePaths();

    std::vector<std::thread> consumers;

    try
    {
        redis.xgroup_create(stream, group, "$", true);
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error creating consumer group: {}", e.what());
        std::cerr << "Error creating consumer group: " << e.what() << std::endl;
    }
    for (int i = 0; i < num_consumers; i++)
    {
        consumers.emplace_back(&DroneControl::Consume, this, std::ref(redis), stream, group, std::to_string(i),
                               &drones_paths);
    }

    // Create thread for writing to DB
    std::thread db_thread(&DroneControl::WriteDroneDataToDB, this);
    // db_thread.detach();

    while (tick_n < sim_duration_ticks)
    {
        // Wait for the semaphore to be released
        sem_wait(sem_sync);
        // spdlog::info("TICK: {}", tick_n);
        std::cout << "[DroneControl] TICK: " << tick_n << std::endl;

        // Spawn a Wave every 150 ticks
        if (tick_n % WAVE_DISTANCE_TICKS == 0)
        {
            SendWaveSpawnCommand();
        }

        // End of tick
        // TickCompleted();

        // Release the semaphore to signal the end of tick
        sem_post(sem_dc);
        tick_n++;
    }

    // Wait for the consumers to finish
    for (auto& consumer : consumers)
    {
        consumer.join();
    }

    // Wait for the DB writer thread to finish
    db_thread.join();

    // spdlog::info("DroneControl finished");
    std::cout << "DroneControl finished" << std::endl;
}
