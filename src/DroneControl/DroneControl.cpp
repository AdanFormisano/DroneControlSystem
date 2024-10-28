#include "DroneControl.h"
#include "../../utils/LogUtils.h"
#include <iostream>

void ConsumerThreadRun(Redis& redis, std::string stream, std::string group, std::string consumer,
                       const std::array<std::unordered_set<coords>, 300>* drones_paths, NewBuffer* buffer, int* tick_n,
                       std::mutex& tick_mutex, const std::atomic_bool* sim_running)
{
    while (*sim_running)
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
            redis.xreadgroup(group, consumer, stream, ">", 900, std::inserter(result, result.end()));

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
                        checked                // checked
                    );

                    // Acknowledge the message (after processing)
                    // redis.xack(stream, group, {item.first});
                }
                buffer->WriteToBuffer(drones_data);
            }
        } catch (const TimeoutError &e)
        {
            // std::cerr << "Timeout while waiting for message: " << e.what() << std::endl;
            log_error("DroneControl", "Timeout while waiting for message: " + std::string(e.what()));
        } catch (const std::exception &e)
        {
            // std::cerr << "Error: " << e.what() << std::endl;
            log_error("DroneControl", "Error: " + std::string(e.what()));
        } catch (...)
        {
            // std::cerr << "Unknown error" << std::endl;
            log_error("DroneControl", "Unknown error");
        }
    }
}

void DroneControl::Consume(Redis& redis, const std::string& stream, const std::string& group,
                           const std::string& consumer,
                           const std::array<std::unordered_set<coords>, 300>* drones_paths)
{
    // int max_tick = 0;
    while (sim_running)
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
            std::vector<std::string> message_ids;

            // 'count' is the number of elements read from the stream at once
            redis.xreadgroup(group, consumer, stream, ">", 900, std::inserter(result, result.end()));

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
                        checked                // checked
                    );

                    // max_tick = std::max(max_tick, std::stoi(item.second[0].second));

                    // Acknowledge the message (after processing)
                    // redis.xack(stream, group, {item.first});
                    // message_ids.push_back(item.first);
                }
                // std::cout << "Consumer " << consumer << " max tick: " << max_tick << std::endl;
                buffer.WriteToBuffer(drones_data);
            }

            if (!message_ids.empty())
            {
                redis.xack(stream, group, message_ids.begin(), message_ids.end());
            }

            // Get number of pending messages
            // using PendingItem = std::tuple<OptionalString, OptionalString>;
            // std::vector<PendingItem> pending_items;
            // auto r = redis.xpending(stream, group, std::back_inserter(pending_items));
            // n_pending = std::get<0>(r);
            // std::cout << "Consumer " << consumer << " pending: " << n_pending << std::endl;
        } catch (const TimeoutError &e)
        {
            // std::cerr << "Timeout while waiting for message: " << e.what() << std::endl;
            log_error("DroneControl", "Timeout while waiting for message: " + std::string(e.what()));
        } catch (const std::exception &e)
        {
            // std::cerr << "Error: " << e.what() << std::endl;
            log_error("DroneControl", "Error: " + std::string(e.what()));
        } catch (...)
        {
            // std::cerr << "Unknown error" << std::endl;
            log_error("DroneControl", "Unknown error");
        }
    }
}

void DroneControl::WriteDroneDataToDB()
{
    // std::cout << "[DB-W] thread started" << std::endl;
    log_dc("DB writer thread started");

    int max_tick = 0; // Maximum tick number read from the buffer

    while (max_tick < sim_duration_ticks - 1)
    {
        // Take data from buffer
        std::vector<DroneData> drones_data = buffer.ReadFromBuffer();

        if (!drones_data.empty())
        {
            std::string query = "INSERT INTO drone_logs (tick_n, drone_id, status, charge, wave_id, x, y, checked) VALUES ";
            int count = 0;

            for (const auto& data : drones_data)
            {
                // Set the maximum tick number
                max_tick = std::max(max_tick, std::stoi(data.tick_n));

                query += "(" + data.tick_n + ", " + data.id + ", '" + data.status + "', " + data.charge + ", " + data.wave_id + ", " + data.x + ", " + data.y + ", " + data.checked + "),";
                count++;

                // Execute the query in batches
                if (constexpr int batch_size = 15000; count >= batch_size)
                {
                    // std::cout << "DB count: " << count << " max tick: " << max_tick << std::endl;
                    query.pop_back();
                    query += ";";
                    try
                    {
                        db.ExecuteQuery(query);
                    }
                    catch (const std::exception& e)
                    {
                        // std::cerr << "Error executing query: " << e.what() << std::endl;
                        log_error("DroneControl", "Error executing query: " + std::string(e.what()));
                    }
                    query = "INSERT INTO drone_logs (tick_n, drone_id, status, charge, wave_id, x, y, checked) VALUES ";
                    count = 0;
                }
            }

            // std::cout << "DB remaining count: " << count << std::endl;

            // Execute any remaining queries
            if (count > 0)
            {
                // std::cout << "DB remaining count: " << count << " max tick: " << max_tick << std::endl;
                query.pop_back();
                query += ";";
                try
                {
                    db.ExecuteQuery(query);
                }
                catch (const std::exception& e)
                {
                    // std::cerr << "Error executing query: " << e.what() << std::endl;
                    log_error("DroneControl", "Error executing query: " + std::string(e.what()));
                }
            }

            // std::cout << "[DB-W] Max tick: " << max_tick << std::endl;
            log_dc("DB count: " + std::to_string(count) + " max tick: " + std::to_string(max_tick));
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    // std::cout << "[DB-W] finished" << std::endl;
    log_dc("DB writer thread finished");
}

void DroneControl::SendWaveSpawnCommand() const
{
    // std::cout << "[DroneControl] Sending wave spawn command" << std::endl;
    log_dc("Sending wave spawn command");
    redis.incr("spawn_wave");

    int dot_count = 0;
    const int max_dots = 3;
    std::string base_msg = "Waiting for wave to spawn";

    // Stampa il messaggio iniziale
    std::cout << base_msg << std::flush;

    // Wait for the wave to spawn
    while (std::stoi(redis.get("spawn_wave").value_or("-1")) != 0) {
        // Debug: stampa il valore di "spawn_wave" per verificare cosa sta succedendo
        std::string wave_value = redis.get("spawn_wave").value_or("-1");
        std::cout << "\rDEBUG: Current spawn_wave value: " << wave_value << "       " << std::flush;

        // Aggiungi puntini incrementali
        std::cout << "\r" << base_msg; // Torna all'inizio della riga
        for (int i = 0; i < dot_count; i++) {
            std::cout << "." << std::flush;
        }

        // Forza il flush del buffer per aggiornare immediatamente il terminale
        std::cout << std::flush;

        dot_count = (dot_count + 1) % (max_dots + 1);

        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Tempo di attesa più lungo per il debug
    }

    // Sovrascrivi l'ultima linea con il messaggio di successo
    std::cout << "\rWave spawned        " << std::endl; // Gli spazi finali cancellano eventuali residui di puntini
    log_dc("Wave spawned");
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
        for (int j = 0; j < 300; j++)
        {
            step.x += 20.0f;
            drones_paths[i].insert(step);
        }
        Y += 20;
    }
}

void DroneControl::Run()
{
    std::cout << "[DroneControl] Running" << std::endl;
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
        // std::cerr << "Error creating consumer group: " << e.what() << std::endl;
        log_error("DroneControl", "Error creating consumer group: " + std::string(e.what()));
    }
    for (int i = 0; i < num_consumers; i++)
    {
        // consumers.emplace_back(&DroneControl::Consume, this, std::ref(redis), stream, group, std::to_string(i),
        //                        &drones_paths);

        std::thread t(ConsumerThreadRun, std::ref(redis), stream, group, std::to_string(i), &drones_paths, &buffer,
                      &tick_n, std::ref(tick_mutex), &sim_running);
        t.detach();
    }

    // Create thread for writing to DB
    std::thread db_thread(&DroneControl::WriteDroneDataToDB, this);
    // db_thread.detach();

    while (tick_n < sim_duration_ticks)
    {
        // Wait for the semaphore to be released
        sem_wait(sem_sync);
        // spdlog::info("TICK: {}", tick_n);
        // std::cout << "[DroneControl] TICK: " << tick_n << std::endl;
        log_dc("TICK: " + std::to_string(tick_n));

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
    // std::cout << "[DroneControl] Waiting for DB to finish writing..." << std::endl;
    log_dc("Waiting for DB to finish writing...");

    // Wait for the DB writer thread to finish
    if (db_thread.joinable())
    {
        db_thread.join();
        // std::cout << "[DroneControl] DB thread joined" << std::endl;
        log_dc("DB thread joined");
    }

    sim_running = false;

    // Close the semaphore
    sem_close(sem_sync);
    sem_close(sem_dc);

    // spdlog::info("DroneControl finished");
    // std::cout << "[DroneControl] finished" << std::endl;
    log_dc("finished");
}

DroneControl::DroneControl(Redis& shared_redis) : redis(shared_redis)
{
    // std::cout << "[DroneControl] DroneControl created" << std::endl;
    log_dc("DroneControl created");
    db.get_DB();
}
