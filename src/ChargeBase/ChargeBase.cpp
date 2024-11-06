#include "ChargeBase.h"
#include "../../utils/RedisUtils.h"
#include <algorithm>

#include "../../utils/LogUtils.h"
#include "../../utils/utils.h"

ChargeBase* instance = nullptr;

ChargeBase* ChargeBase::getInstance(Redis& redis)
{
    if (!instance)
    {
        instance = new ChargeBase(redis); // Constructor
    }
    return instance;
}

ChargeBase::ChargeBase(Redis& redis) : mq_charge(open_or_create, "charge_fault_queue", 100, sizeof(TG_charge_data)), redis(redis)
{
    // spdlog::set_pattern("[%T.%e][%^%l%$][ChargeBase] %v");
    // spdlog::info("ChargeBase process starting");
    log_cb("ChargeBase created");
    // std::cout << "[ChargeBase] ChargeBase created" << std::endl;
}

void ChargeBase::Run()
{
    // std::cout << "[ChargeBase] Running" << std::endl;
    log_cb("Running");

    // Create or open the semaphore for synchronization
    sem_t* sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_cb", 0);
    sem_t* sem_cb = utils_sync::create_or_open_semaphore("/sem_cb", 0);

    while (tick_n < sim_duration_ticks)
    {
        // Wait for the semaphore to be released
        sem_wait(sem_sync);
        // std::cout << "[Chargebase] TICK: " << tick_n << std::endl;
        log_cb("TICK: " + std::to_string(tick_n));

        // Work

        // Check if TestGenerator sent a message to change the charge rate
        if (auto size = mq_charge.get_num_msg(); size > 0)
        {
            // Iterate over the message queue and receive all the messages
            for (int i = 0; i < size; i++)
            {
                message_queue::size_type recvd_size;
                unsigned int priority;
                TG_charge_data msg{};
                mq_charge.receive(&msg, sizeof(msg), recvd_size, priority);

                charging_drones[msg.drone_id].charge_rate_factor = msg.charge_rate_factor;
                std::cout << "[ChargeBase] Drone " << msg.drone_id << " has charge rate factor of " << msg.charge_rate_factor << std::endl;
            }
        }

        ChargeDrone(); // Normal charge rate
        // ChargeDroneMegaSpeed(); // TESTING: Charge drones at 5x speed
        ReadChargeStream();

        // Release the semaphore to signal the end of the tick
        sem_post(sem_cb);
        tick_n++;
    }


    // std::cout << "[ChargeBase] finished" << std::endl;
    log_cb("Finished");
}

// Read the charge_stream and adds the drones to the vector of drones to charge
void ChargeBase::ReadChargeStream()
{
    using new_Attrs = std::vector<std::pair<std::string, std::string>>;
    // This NEEDS to be a vector for xread to work
    using new_Item = std::pair<std::string, new_Attrs>;
    using new_ItemStream = std::vector<new_Item>;

    // Parse the stream and save/update the drones' data, trims the stream after reading
    try
    {
        std::unordered_map<std::string, new_ItemStream> new_result;
        // TODO: Check of number_items_stream is necessary
        // Gets the number of items in the stream
        auto number_items_stream = redis.command<long long>("XLEN", "charge_stream");

        // Reads the stream
        redis.xread("charge_stream", current_stream_id,
            number_items_stream, std::inserter(new_result, new_result.end()));

        // Parses the stream
        for (const auto& item : new_result)
        {
            // There is only one pair: stream_key and stream_data
            // item.first is the key of the stream. In this case it is "drone_stream"

            // spdlog::info("-----------------Tick {}-----------------", tick_n);
            for (const auto& stream_drone : item.second)
            {
                // stream_drone.first is the id of the message in the stream
                // stream_drone.second is the unordered_map with the data of the drone

                // Update the local drone data
                SetChargeData(stream_drone.second);
            }
            // FollowPath the start_id to the last item read
            current_stream_id = item.second.back().first;
        }
        // Trim the stream
        // spdlog::info("{} entries read from charge_stream. Trimming the stream.", number_items_stream);
        redis.command<long long>("XTRIM", "charge_stream", "MINID", current_stream_id);
    }
    catch (const Error &e)
    {
        // std::cerr << "[ChargeBase] Error reading the stream: " << e.what() << std::endl;
        log_error("ChargeBase", "Error reading the stream: " + std::string(e.what()));
    }
}

void ChargeBase::SetChargeData(const std::vector<std::pair<std::string, std::string>>& data)
{
    // The data is structured as a known array
    ChargingDroneData drone_data{};
    drone_data.id = std::stoi(data[0].second);
    drone_data.wave_id = std::stoi(data[1].second);
    drone_data.charge = std::stof(data[2].second);
    drone_data.state = utils::stringToDroneStateEnum(data[3].second);
    drone_data.charge_rate = getChargeRate();

    // Update the drone unordered map
    charging_drones[drone_data.id] = drone_data;

    // Add drone to set of charging drones on Redis
    redis.sadd("charging_drones", std::to_string(drone_data.id));
}

void ChargeBase::ChargeDrone()
{
    // List of drones to remove from the charging list
    std::vector<int> drones_to_remove;

    for (auto &drone : charging_drones)
    {
        auto &drone_data = drone.second;
        if (drone_data.charge < 100)
        {
            if (drone_data.charge_rate_factor < 1.0f)
            {
                drone_data.charge += drone_data.charge_rate * (1 - drone_data.charge_rate_factor);
            }
            else
            {
                drone_data.charge += drone_data.charge_rate * drone_data.charge_rate_factor;
            }
        } else if (drone_data.charge >= 100)
        {
            drone_data.state = drone_state_enum::CHARGING_COMPLETED;
            ReleaseDrone(drone_data);

            // Add the drone to the list of drones to remove
            drones_to_remove.push_back(drone_data.id);
        }
    }

    // Remove the drones from the charging list
    for (auto& drone_id : drones_to_remove)
    {
        charging_drones.erase(drone_id);
    }
}

void ChargeBase::ChargeDroneMegaSpeed()
{
    // List of drones to remove from the charging list
    std::vector<int> drones_to_remove;

    for (auto &drone : charging_drones)
    {
        auto &drone_data = drone.second;
        if (drone_data.charge < 100)
        {
            drone_data.charge += drone_data.charge_rate * drone_data.charge_rate_factor * 5;
        } else if (drone_data.charge >= 100)
        {
            drone_data.state = drone_state_enum::CHARGING_COMPLETED;
            // std::cout << "[ChargeBase] Drone ID: " << drone_data.id << " State updated to CHARGING_COMPLETED" << std::endl;

            ReleaseDrone(drone_data);

            // Add the drone to the list of drones to remove
            drones_to_remove.push_back(drone_data.id);
        }
    }

    // Remove the drones from the charging list
    for (auto& drone_id : drones_to_remove)
    {
        charging_drones.erase(drone_id);
    }
}

void ChargeBase::ReleaseDrone(const ChargingDroneData& drone_data) const
{
    // Add the drone to the list of charged drones for SM (to recycle drones)
    redis.sadd("charged_drones", std::to_string(drone_data.id));

    // Remove the drone from the list of charging drones
    redis.srem("charging_drones", std::to_string(drone_data.id));

    // Write on DC stream that the drone is charged
    DroneData data(tick_n, drone_data.id, utils::droneStateToString(drone_data.state), 100.0f, {0.0f, 0.0f}, drone_data.wave_id);
    auto v = data.toVector();
    redis.xadd("scanner_stream", "*", v.begin(), v.end());
}

void ChargeBase::SetEngine()
{
    std::random_device rd;
    engine = std::mt19937(rd());
}

float ChargeBase::getChargeRate()
{
    // Generate a random float between 2.0 and 3.0 for the charge rate
    auto generateRandomFloat = [this]() -> float
    {
        std::uniform_real_distribution<float> dist(2.0f, 3.0f);
        return dist(engine);
    };

    auto tick_needed_to_charge = (generateRandomFloat() * (60 * 60)) / TICK_TIME_SIMULATED;

    return 100.0f / tick_needed_to_charge;
}
