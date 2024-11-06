#include "../utils/RedisUtils.h"
#include "../utils/utils.h"
#include "DroneControl.h"
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

    // Create the Redis object for DroneControl
    auto drone_control_redis = Redis(connection_options);

    // Create the DroneControl object
    DroneControl dc(drone_control_redis);

    // std::cout << "DroneControl process started" << std::endl;

    // Start simulation
    dc.Run();
}