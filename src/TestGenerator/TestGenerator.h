#ifndef DRONECONTROLSYSTEM_TESTGENERATOR_H
#define DRONECONTROLSYSTEM_TESTGENERATOR_H
#include "../../libs/boost/interprocess/ipc/message_queue.hpp"
#include "sw/redis++/redis++.h"
#include <functional>
#include <map>
#include <random>

using namespace sw::redis;
using namespace boost::interprocess;

struct DroneInfo {
    int wave_id = -1;
    int drone_id = -1;
};

class TestGenerator {
public:
    explicit TestGenerator(Redis &redis);

    Redis &test_redis;

    void Run();

private:
    message_queue mq;
    message_queue mq_charge;
    std::map<float, std::function<void()>> scenarios;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_real_distribution<> dis_consumption;
    std::uniform_int_distribution<> dis_drone;
    std::uniform_int_distribution<> dis_tick;
    std::uniform_real_distribution<> dis_charge_rate;

    float generateRandomFloat() {
        return static_cast<float>(dis(gen));
    }

    DroneInfo ChooseRandomDrone();
    int ChooseRandomTick();
};

#endif // DRONECONTROLSYSTEM_TESTGENERATOR_H
