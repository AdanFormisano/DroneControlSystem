#ifndef WAVE_H
#define WAVE_H
#include <vector>
#include <array>
#include "Drone.h"
#include "ThreadUtils.h"
#include <sw/redis++/redis++.h>

using namespace sw::redis;

class Drone;

class Wave {
public:
    Wave(int tick_n, int wave_id, Redis &shared_redis, ThreadSemaphore *tick_sync);

    void Run(); // Function executed by the thread

    Redis &redis;
    int X = 0;             // The position of the wave
    int starting_tick = 0; // The tick when the wave was created
    std::array<DroneData, 300> drones_data;
    synced_queue<TG_data> tg_data;
    std::vector<int> drones_to_delete;
    int tick = 0;

    [[nodiscard]] int getId() const { return id; }
    [[nodiscard]] int getReadyDrones() const { return ready_drones; }
    void incrReadyDrones() { ready_drones++; }

private:
    int id = 0;
    std::vector<Drone*> drones;
    // TickSynchronizer &tick_sync;
    ThreadSemaphore *sem_sync;
    int ready_drones = 0;

    void setDroneFault(int wave_drone_id, drone_state_enum state, int reconnect_tick, float high_consumption_factor) const;
    void UploadData() const;
    void RecycleDrones() const;
    void DeleteDrones();
    bool AllDronesAreDead();
};
#endif //WAVE_H
