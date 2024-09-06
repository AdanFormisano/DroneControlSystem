/* The ScannerManager is responsible for managing the "wave" of drones that are sent out to scan the area.
 * Itś responsible for creating the drones.
 */

#ifndef SCANNERMANAGER_H
#define SCANNERMANAGER_H
#include <boost/thread.hpp>
#include <vector>

#include "../../utils/RedisUtils.h"
#include "../globals.h"

using namespace sw::redis;

class Wave;

class ScannerManager
{
public:
    explicit ScannerManager(Redis& shared_redis);

    std::vector<Wave> waves;
    Redis& redis;

    void Run();

private:
    std::vector<boost::thread> waves_threads;
};

class Wave
{
public:
    explicit Wave(int wave_id, int tick, Redis& redis);

    Redis& shared_redis;
    int tick_n = 0;
    const int id; // The id of the wave
    std::array<Drone, 300> drones{}; // The drones in the wave
    // coords position {0.0f, 0.0f}; // The position of the wave

    void Run();

private:
    void createDrones(); // Creates the drones in the wave
    void moveDrones(); // Moves the drones in the wave
    void dumpDroneData(); // Dumps the drone data to the console
};

#endif  // SCANNERMANAGER_H
