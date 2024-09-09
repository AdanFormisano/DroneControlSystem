#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <array>
#include <random>
#include <unordered_map>
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <mutex>
#include <queue>

#include "../src/globals.h"

extern std::default_random_engine generator;
extern std::uniform_real_distribution<float> float_distribution_mten_to_ten;

float generateRandomFloat();

// Message structure for IPC TestGenerator <=> DroneManager
struct DroneStatusMessage
{
    int target_zone;
    drone_state_enum status;
};

namespace utils
{
    const char* droneStateToString(drone_state_enum state);
    drone_state_enum stringToDroneStateEnum(const std::string& stateStr);
    void signalHandler(int signal);

    template <typename T>
    struct synced_queue
    {
        std::queue<T> queue;
        std::mutex mtx;

        void push(const T& value)
        {
            std::lock_guard lock(mtx);
            queue.push(value);
        }

        std::optional<T> pop()
        {
            std::lock_guard lock(mtx);
            if (queue.epmty())
            {
                return std::nullopt;
            }
            T value = queue.front();
            queue.pop();
            return value;
        }

        [[nodiscard]] bool empty() const
        {
            std::lock_guard lock(mtx);
            return queue.empty();
        }

        [[nodiscard]] size_t size() const
        {
            std::lock_guard lock(mtx);
            return queue.size();
        }
    };
}
#endif // DRONECONTROLSYSTEM_UTILS_H
