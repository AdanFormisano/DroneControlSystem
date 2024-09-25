#include "Wave.h"

Wave::Wave(int tick_n, const int wave_id, Redis& shared_redis, TickSynchronizer& synchronizer) :
    redis(shared_redis), tick(tick_n), tick_sync(synchronizer)
{
    // To recycle a drone means to create a new wave with the fully charged drones used in the same "position"
    // of the previous wave. But because each drone's id has its "old" wave id, we need to change it to the new wave id
    // Basically, we need to remove from Redis (charged_drones) the drones that are going to be recycled; but the actual
    // spawning of the drones doesn't change. If there are less then 300 drones to recycle, we need to create new drones
    // but this doesn't meter because we are creating every new drone, hiding the fact that we are recycling some of them
    auto n_recycled_drones = RecycleDrones();

    // spdlog::info("Wave {} recycled {} drones", wave_id, n_recycled_drones);

    id = wave_id;
    starting_tick = tick_n;

    float y = -2990; // Drone has a coverage radius of 10.0f

    for (int i = 0; i < 300; i++)
    {
        int drone_id = id * 1000 + i;

        drones.push_back(new Drone(drone_id, id, *this));
        drones[i]->position.x = 0;
        drones[i]->position.y = 0;
        drones[i]->starting_line.x = -2990.0f;
        drones[i]->starting_line.y = y;
        drones[i]->tick_drone = tick;

        // Calculate drones' direction
        float dx = -2990.0f;
        float dy = y;
        float distance = std::sqrt(dx * dx + dy * dy);

        drones[i]->dir.x = dx / distance;
        drones[i]->dir.y = dy / distance;
        drones[i]->setState(ToStartingLine::getInstance());

        y += 20.0f;
    }

    // Self add alive wave on Redis
    redis.sadd("waves_alive", std::to_string(id));

    // spdlog::info("Wave {} created all drones", id);
    std::cout << "Wave " << id << " created all drones" << std::endl;
}

void Wave::UploadData()
{
    try
    {
        // A redis pipeline is used to upload the data to the redis server
        // Create a pipeline from the group of redis connections
        auto pipe = redis.pipeline(false);

        // For each drone a redis function will be added to the pipeline
        // When every command has been added, the pipeline will be executed

        for (auto& drone : drones)
        {
            // Create a DroneData object
            DroneData data(tick, drone->id, utils::droneStateToString(drone->getCurrentState()->getState()),
                           drone->charge,
                           drone->position, drone->wave_id);
            auto v = data.toVector();

            // Add the command to the pipeline
            pipe.xadd("scanner_stream", "*", v.begin(), v.end());
            // spdlog::info("Drone {} data uploaded to Redis", drone.id);
        }

        // Redis connection is returned to the pool after the pipeline is executed
        pipe.exec();
    }
    catch (const ReplyError& e)
    {
        // spdlog::error("Redis pipeline error: {}", e.what());
        std::cerr << "Redis pipeline error: " << e.what() << std::endl;
    } catch (const IoError& e)
    {
        // spdlog::error("Redis pipeline error: {}", e.what());
        std::cerr << "Redis pipeline error: " << e.what() << std::endl;
    }
}

void Wave::setDroneFault(int wave_drone_id, drone_state_enum state, int reconnect_tick)
{
    // Set the drone state to the new state parameter
    int drone_id = wave_drone_id % 1000; // Get the drone id from the wave_drone_id
    drones[drone_id]->previous = drones[drone_id]->getCurrentState()->getState();
    drones[drone_id]->setState(getDroneState(state));
    drones[drone_id]->reconnect_tick = reconnect_tick;

    // spdlog::info("[TestGenerator] TICK {} Drone {} state set to {}", tick, wave_drone_id, utils::droneStateToString(state));
    std::cout << "[TestGenerator] TICK " << tick << " Drone " << wave_drone_id << " state set to " <<
        utils::droneStateToString(state) << std::endl;
}

int Wave::RecycleDrones()
{
    // Get the drones that are fully charged
    std::vector<std::string> charged_drones;
    redis.spop("charged_drones", 300, std::back_inserter(charged_drones));

    // Convert charged_drones from std::vector<std::string> to std::vector<int>
    std::vector<int> charged_drones_int;
    charged_drones_int.reserve(charged_drones.size());
    std::ranges::transform(charged_drones, std::back_inserter(charged_drones_int),
                           [](const std::string& str) { return std::stoi(str); });

    return static_cast<int>(charged_drones_int.size());
}

void Wave::DeleteDrones()
{
    try
    {
        // Delete drones that are dead
        for (auto& drone_id : drones_to_delete)
        {
            int index = drone_id % 1000;
            // spdlog::info("Drone {} is dead", drone_id);
            delete drones[index];
            drones[index] = nullptr;
        }
    }
    catch (const std::exception& e)
    {
        // spdlog::error("Error deleting drones: {}", e.what());
        std::cerr << "Error deleting drones: " << e.what() << std::endl;
    }
}

bool Wave::AllDronesAreDead()
{
    // Check if all drones are dead
    return std::ranges::all_of(drones, [](Drone* d) { return d == nullptr; });
}

void Wave::Run()
{
    // spdlog::info("Wave {} started", id);
    std::cout << "Wave " << id << " started" << std::endl;
    tick_sync.thread_started();
    auto start_time = std::chrono::high_resolution_clock::now();

    // A redis pipeline is used to upload the data to the redis server
    // Create a pipeline from the group of redis co nnections
    auto pipe = redis.pipeline(false);

    bool AreDroneDead = false;

    while (!AreDroneDead)
    {
        AreDroneDead = AllDronesAreDead();
        std::cout << "Wave " << id << " tick " << tick << " drones are all dead: "<< AreDroneDead << std::endl;
        // spdlog::info("Wave {} tick {}", id, tick);
        // std::cout << "Wave " << id << " tick " << tick << std::endl;

        // Before the "normal" execution, check if SM did put any "input" inside the "queue" (aka TestGenerator scenarios' input)
        // Check if there is any message in the queue
        if (!tg_data.empty())
        {
            // TODO: In theory there will only ever be a single message in the queue, maybe we can optimize this
            // Get the message from the queue
            auto msg = tg_data.pop().value();
            // spdlog::info("[TestGenerator] Drone has new state {}", utils::droneStateToString(msg.new_state));
            std::cout << "[TestGenerator] Drone has new state " << utils::droneStateToString(msg.new_state) <<
                std::endl;

            setDroneFault(msg.drone_id, msg.new_state, msg.reconnect_tick);
        }

        try
        {
            // Execute the wave
            for (auto& drone : drones)
            {
                if (drone != nullptr)
                {
                    // Execute the current state
                    drone->run();

                    // Create a DroneData object
                    DroneData data(tick, drone->id, utils::droneStateToString(drone->getCurrentState()->getState()),
                                   drone->charge,
                                   drone->position, drone->wave_id);
                    auto v = data.toVector();

                    // Add the command to the pipeline
                    pipe.xadd("scanner_stream", "*", v.begin(), v.end());
                    // spdlog::info("Drone {} data uploaded to Redis", drone.id);
                }
            }

            pipe.exec();

            // Delete drones that are dead
            DeleteDrones();

            // Each tick of the execution will be synced with the other threads. This will make writing to the DB much easier
            // because the data will be consistent/historical
            tick_sync.tick_completed();
            tick++;
        }
        catch (const TimeoutError& e)
        {
            // /spdlog::error("Timeout running wave {}: {}", id, e.what());
            std::cerr << "Timeout running wave " << id << ": " << e.what() << std::endl;
        } catch (const IoError& e)
        {
            // spdlog::error("IoError running wave {}: {}", id, e.what());
            std::cerr << "IoError running wave " << id << ": " << e.what() << std::endl;
        } catch (...)
        {
            std::cerr << "Unknown error running wave " << id << std::endl;
        }
    }

    // Remove self from alive waves on Redis
    redis.srem("waves_alive", std::to_string(id));

    tick_sync.thread_finished();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    // spdlog::info("Wave {} duration: {}ms", id, duration.count());
    // spdlog::info("Wave {} finished", id);
    std::cout << "Wave " << id << " duration: " << duration.count() << "ms" << std::endl;
    std::cout << "Wave " << id << " finished" << std::endl;
}
