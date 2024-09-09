#include "utils.h"

#include <boost/thread/detail/move.hpp>

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
        if (stateStr == "IDLE_IN_BASE") return drone_state_enum::IDLE_IN_BASE;
        if (stateStr == "TO_ZONE") return drone_state_enum::TO_ZONE;
        if (stateStr == "WORKING") return drone_state_enum::WORKING;
        if (stateStr == "TO_BASE") return drone_state_enum::TO_BASE;
        if (stateStr == "WAITING_CHARGE") return drone_state_enum::WAITING_CHARGE;
        if (stateStr == "TO_ZONE_FOLLOWING") return drone_state_enum::TO_ZONE_FOLLOWING;
        if (stateStr == "FOLLOWING") return drone_state_enum::FOLLOWING;
        if (stateStr == "NOT_CONNECTED") return drone_state_enum::NOT_CONNECTED;
        if (stateStr == "DEAD") return drone_state_enum::DEAD;
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