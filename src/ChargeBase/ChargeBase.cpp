#include "ChargeBase.h"
#include "../../utils/RedisUtils.h"
#include <algorithm>

#include "../../utils/utils.h"

ChargeBase *instance = nullptr;

ChargeBase *ChargeBase::getInstance(Redis &redis) {
    if (!instance) {
        instance = new ChargeBase(redis); // Constructor
    }
    return instance;
}

ChargeBase::ChargeBase(Redis &redis) : redis(redis) {
    // spdlog::set_pattern("[%T.%e][%^%l%$][ChargeBase] %v");
    // spdlog::info("ChargeBase process starting");
    // std::cout << "[ChargeBase] ChargeBase process starting" << std::endl;
}

void ChargeBase::Run() {
    // Create or open the semaphore for synchronization
    sem_t *sem_sync = utils_sync::create_or_open_semaphore("/sem_sync_cb", 0);
    sem_t *sem_cb = utils_sync::create_or_open_semaphore("/sem_cb", 0);

    while (tick_n < sim_duration_ticks) {

        // Wait for the semaphore to be released
        // std::cout << "[ChargeBase] Before waiting for semaphore. Tick: " << tick_n << std::endl;
        sem_wait(sem_sync);
        // spdlog::info("TICK: {}", tick_n);
        // std::cout << "[ChargeBase] After semaphore released. Tick: " << tick_n << std::endl;
        std::cout << "[Chargebase] TICK: " << tick_n << std::endl;

        // Work

        // ChargeDrone(); // Normal charge rate

        // std::cout << "[ChargeBase] Calling ChargeDroneMegaSpeed." << std::endl;
        ChargeDroneMegaSpeed(); // TESTING: Charge drones at 5x speed

        // std::cout << "[ChargeBase] Calling ReadChargeStream." << std::endl;
        ReadChargeStream();

        // Tick completed
        // sync with other processes
        // std::cout << "[ChargeBase] TickCompleted." << std::endl;

        // TickCompleted();

        // Release the semaphore to signal the end of the tick
        sem_post(sem_cb);
        tick_n++;
        // std::cout << "[ChargeBase] Tick incremented to: " << tick_n << std::endl;
    }

    // spdlog::info("ChargeBase process finished");
    std::cout << "[ChargeBase] finished" << std::endl;
}

// Read the charge_stream and adds the drones to the vector of drones to charge
void ChargeBase::ReadChargeStream() {
    using new_Attrs = std::vector<std::pair<std::string, std::string>>;
    // This NEEDS to be a vector for xread to work
    using new_Item = std::pair<std::string, new_Attrs>;
    using new_ItemStream = std::vector<new_Item>;

    // std::cout << "[ChargeBase] ReadChargeStream called." << std::endl;

    // Parse the stream and save/update the drones' data, trims the stream after reading
    try {
        std::unordered_map<std::string, new_ItemStream> new_result;
        // TODO: Check of number_items_stream is necessary
        // Gets the number of items in the stream
        auto number_items_stream = redis.command<long long>("XLEN", "charge_stream");
        // std::cout << "[ChargeBase] Number of items in charge_stream: " << number_items_stream << std::endl;

        // if (number_items_stream == 0) {
        //     // std::cout << "[ChargeBase] charge_stream is empty." << std::endl;
        //     return;
        // }

        // Comment to build differentl

        // Reads the stream
        redis.xread("charge_stream", current_stream_id,
                    number_items_stream, std::inserter(new_result, new_result.end()));

        // Parses the stream
        for (const auto &item : new_result) {
            // std::cout << "[ChargeBase] Reading stream: " << item.first << std::endl;

            // There is only one pair: stream_key and stream_data
            // item.first is the key of the stream. In this case it is "drone_stream"

            // spdlog::info("-----------------Tick {}-----------------", tick_n);
            for (const auto &stream_drone : item.second) {
                // std::cout << "[ChargeBase] Drone ID read from stream: " << stream_drone.second[0].second << std::endl;

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
    } catch (const Error &e) {
        // splog::error("Error reading the stream: {}", e.what());
        std::cerr << "[ChargeBase] Error reading the stream: " << e.what() << std::endl;
    }
}

void ChargeBase::SetChargeData(const std::vector<std::pair<std::string, std::string>> &data) {
    // The data is structured as a known array
    ChargingDroneData drone_data{};
    drone_data.id = std::stoi(data[0].second);
    drone_data.wave_id = std::stoi(data[1].second);
    drone_data.charge = std::stof(data[2].second);
    drone_data.state = utils::stringToDroneStateEnum(data[3].second);
    drone_data.charge_rate = getChargeRate();

    // Update the drone unordered map
    charging_drones[drone_data.id] = drone_data;
}

void ChargeBase::ChargeDrone() {
    // List of drones to remove from the charging list
    std::vector<int> drones_to_remove;

    for (auto &drone : charging_drones) {
        auto &drone_data = drone.second;
        if (drone_data.charge < 100) {
            drone_data.charge += drone_data.charge_rate;
        } else if (drone_data.charge >= 100) {
            drone_data.state = drone_state_enum::CHARGING_COMPLETED;
            ReleaseDrone(drone_data);

            // Add the drone to the list of drones to remove
            drones_to_remove.push_back(drone_data.id);
        }
    }

    // Remove the drones from the charging list
    for (auto &drone_id : drones_to_remove) {
        charging_drones.erase(drone_id);
    }
}

void ChargeBase::ChargeDroneMegaSpeed() {
    // List of drones to remove from the charging list

    // std::cout << "[ChargeBase] ChargeDroneMegaSpeed called. Number of drones in charging_drones: " << charging_drones.size() << std::endl;

    // if (charging_drones.empty()) {
    //     std::cout << "[ChargeBase] No drones to charge." << std::endl;
    //     return;
    // }

    std::vector<int> drones_to_remove;

    for (auto &drone : charging_drones) {
        auto &drone_data = drone.second;

        // std::cout << "[ChargeBase] Drone ID: " << drone_data.id << " Charge: " << drone_data.charge << std::endl;

        if (drone_data.charge < 100) {
            drone_data.charge += drone_data.charge_rate * 5;
        } else if (drone_data.charge >= 100) {
            drone_data.state = drone_state_enum::CHARGING_COMPLETED;
            // std::cout << "[ChargeBase] Drone ID: " << drone_data.id << " State updated to CHARGING_COMPLETED" << std::endl;

            ReleaseDrone(drone_data);

            // Add the drone to the list of drones to remove
            drones_to_remove.push_back(drone_data.id);
        }
    }

    // Remove the drones from the charging list
    for (auto &drone_id : drones_to_remove) {
        // std::cout << "[ChargeBase] Removing Drone ID: " << drone_id << " from charging list." << std::endl;

        charging_drones.erase(drone_id);
    }
}

void ChargeBase::ReleaseDrone(const ChargingDroneData &drone_data) const {
    // Add the drone to the list of charged drones for SM (to recycle drones)
    // std::cout << "[ChargeBase] Releasing Drone ID: " << drone_data.id << " with state: " << utils::droneStateToString(drone_data.state) << std::endl;

    redis.sadd("charged_drones", std::to_string(drone_data.id));

    // Write on DC stream that the drone is charged
    DroneData data(tick_n, drone_data.id, utils::droneStateToString(drone_data.state), 100.0f, {0.0f, 0.0f}, drone_data.wave_id);
    auto v = data.toVector();
    redis.xadd("scanner_stream", "*", v.begin(), v.end());
    // std::cout << "[ChargeBase] Drone ID: " << drone_data.id << " added to scanner_stream with state: " << utils::droneStateToString(drone_data.state) << std::endl;
}

void ChargeBase::SetEngine() {
    std::random_device rd;
    engine = std::mt19937(rd());
}

float ChargeBase::getChargeRate() {
    // Generate a random float between 2.0 and 3.0 for the charge rate
    auto generateRandomFloat = [this]() -> float {
        std::uniform_real_distribution<float> dist(2.0f, 3.0f);
        return dist(engine);
    }; // GBT FTW

    auto tick_needed_to_charge = (generateRandomFloat() * (60 * 60)) / TICK_TIME_SIMULATED;

    return 100.0f / tick_needed_to_charge;
}
