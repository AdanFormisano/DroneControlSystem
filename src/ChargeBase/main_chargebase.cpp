#include "../utils/RedisUtils.h"
#include "../utils/utils.h"
#include "ChargeBase.h"
#include "../globals.h"
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>

using namespace sw::redis;

int main()
{
    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 7777;
    connection_options.socket_timeout = std::chrono::milliseconds(3000);

    // Create the Redis object for ChargeBase
    auto charge_base_redis = Redis(connection_options);

    // Create the ChargeBase object
    auto cb = ChargeBase::getInstance(charge_base_redis);

    // Set the charge rate for the charging slots
    thread_local std::random_device rd;
    cb->SetEngine(); // TODO: Check if every slot has the same charge rate

    // Start simulation
    cb->Run();

    std::cout << "ChargeBase simulation finished" << std::endl;
}