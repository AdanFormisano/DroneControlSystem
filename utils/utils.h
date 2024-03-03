#ifndef DRONECONTROLSYSTEM_UTILS_H
#define DRONECONTROLSYSTEM_UTILS_H

#include <random>
#include <unordered_map>

extern std::default_random_engine generator;
extern std::uniform_real_distribution<float> float_distribution_mten_to_ten;

float generateRandomFloat();

extern std::unordered_map<std::string, int> StatusMap;

#endif //DRONECONTROLSYSTEM_UTILS_H
