#ifndef DRONECONTROLSYSTEM_TESTGENERATOR_H
#define DRONECONTROLSYSTEM_TESTGENERATOR_H
#include <map>
#include <functional>
#include <random>
#include "../../libs/interprocess/ipc/message_queue.hpp"
#include "../globals.h"
#include "../../utils/utils.h"
#include "sw/redis++/redis++.h"

using namespace sw::redis;
using namespace boost::interprocess;

struct DroneInfo
{
    int wave_id;
    int drone_id;
};

class TestGenerator {
public:
    explicit TestGenerator(Redis &redis);

    Redis &test_redis;

    void Run();

private:
    message_queue mq;
    std::map<float, std::function<void()>> scenarios;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_int_distribution<> dis_drone;
    std::uniform_int_distribution<> dis_tick;

    float generateRandomFloat() {
        return static_cast<float>(dis(gen));
    }

    DroneInfo ChooseRandomDrone();
    int ChooseRandomTick();
};

#endif //DRONECONTROLSYSTEM_TESTGENERATOR_H
