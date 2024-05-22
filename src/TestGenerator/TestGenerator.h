#ifndef DRONECONTROLSYSTEM_TESTGENERATOR_H
#define DRONECONTROLSYSTEM_TESTGENERATOR_H
#include <map>
#include <functional>
#include <random>
#include "../globals.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;

class TestGenerator {
public:
    TestGenerator(Redis &redis);

    Redis &test_redis;

    void Run();

private:
    std::map<float, std::function<void()>> scenarios;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_int_distribution<> dis_zone;
    std::uniform_int_distribution<> dis_tick;

    float generateRandomFloat() {
        return static_cast<float>(dis(gen));
    }

    int ChooseRandomDrone();
    int ChooseRandomTick();
};

#endif //DRONECONTROLSYSTEM_TESTGENERATOR_H
