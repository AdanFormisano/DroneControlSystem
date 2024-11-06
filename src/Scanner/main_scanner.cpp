#include "../utils/RedisUtils.h"
#include "../utils/utils.h"
#include "../globals.h"
#include <sw/redis++/redis++.h>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include "ScannerManager.h"

using namespace sw::redis;

int main() {
    ConnectionOptions connection_options;
    connection_options.host = "127.0.0.1";
    connection_options.port = 7777;
    connection_options.socket_timeout = std::chrono::milliseconds(3000);

    // Create the Redis object for Drone
    ConnectionPoolOptions drone_connection_pool_options;
    drone_connection_pool_options.size = 8;
    drone_connection_pool_options.wait_timeout = std::chrono::milliseconds(1000);

    auto drone_redis = Redis(connection_options, drone_connection_pool_options);

    // Create the DroneManager object
    // drones::DroneManager dm(drone_redis);
    // ScannerManager sm(drone_redis);
    ScannerManager ssm(drone_redis);

    // Start simulation
    // dm.Run();
    ssm.Run();
  }