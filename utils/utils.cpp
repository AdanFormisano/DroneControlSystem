#include "utils.h"

std::default_random_engine generator;
std::uniform_real_distribution<float> float_distribution_mten_to_ten(-10.0f, 10.0f);

float generateRandomFloat() {
    float randomFloat = float_distribution_mten_to_ten(generator);
    randomFloat = std::round(randomFloat * 100) / 100;
    return randomFloat;
}

namespace utils {
    const char* CaccaPupu(drone_state_enum state) {
        return drone_state_str[static_cast<std::size_t>(state)];
    };
}