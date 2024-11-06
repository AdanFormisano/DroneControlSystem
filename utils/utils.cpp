#include "utils.h"

#include <boost/thread/detail/move.hpp>

#include "spdlog/spdlog.h"

std::default_random_engine generator;
std::uniform_real_distribution<float> float_distribution_mten_to_ten(-10.0f, 10.0f);

float generateRandomFloat() {
    float randomFloat = float_distribution_mten_to_ten(generator);
    randomFloat = std::round(randomFloat * 100) / 100;
    return randomFloat;
}

namespace utils {
    const char *droneStateToString(drone_state_enum state) {
        return drone_state_str[static_cast<std::size_t>(state)];
    };

    drone_state_enum stringToDroneStateEnum(const std::string& stateStr) {
        if (stateStr == "IDLE") return drone_state_enum::IDLE;
        if (stateStr == "TO_STARTING_LINE") return drone_state_enum::TO_STARTING_LINE;
        if (stateStr == "READY") return drone_state_enum::READY;
        if (stateStr == "WORKING") return drone_state_enum::WORKING;
        if (stateStr == "TO_BASE") return drone_state_enum::TO_BASE;
        if (stateStr == "DISCONNECTED") return drone_state_enum::DISCONNECTED;
        if (stateStr == "RECONNECTED") return drone_state_enum::RECONNECTED;
        if (stateStr == "DEAD") return drone_state_enum::DEAD;
        if (stateStr == "CHARGING") return drone_state_enum::CHARGING;
        return drone_state_enum::NONE;
    }

// Signal handler function
    void signalHandler(int signal, siginfo_t *si, void *unused) {
        std::cout << "Caught signal: " << signal << std::endl;
        // Optionally, you can perform cleanup here
        // and then exit the program gracefully
        std::exit(signal);
    }
}

namespace utils_sync
{
    sem_t* create_or_open_semaphore(const char* name, unsigned int initial_value)
    {
        sem_t* sem = sem_open(name, O_CREAT, 0644, initial_value);
        if (sem == SEM_FAILED)
        {
            std::cerr << "Failed to create semaphore: " << strerror(errno) << std::endl;
            return nullptr;
        }
        return sem;
    }
}