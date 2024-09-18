#ifndef WAVE_H
#define WAVE_H
#include <vector>
#include <array>
#include "Drone.h"
#include "ThreadUtils.h"

class Drone;

class Wave {
public:
    Wave(int tick_n, int wave_id, Redis &shared_redis, TickSynchronizer &synchronizer);

    void Run(); // Function executed by the thread

    int X = 0;             // The position of the wave
    int starting_tick = 0; // The tick when the wave was created
    std::array<DroneData, 300> drones_data;
    synced_queue<TG_data> tg_data;
    int tick = 0;

    [[nodiscard]] int getId() const { return id; }
    [[nodiscard]] int getReadyDrones() const { return ready_drones; }
    void incrReadyDrones() { ready_drones++; }

private:
    Redis &redis;
    int id = 0;
    std::vector<Drone> drones;
    TickSynchronizer &tick_sync;
    int ready_drones = 0;

    void Move();
    void UploadData();
};




#endif //WAVE_H
