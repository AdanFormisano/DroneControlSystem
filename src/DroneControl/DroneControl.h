#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H

#include "../globals.h"
#include "../db/Database.h"
#include <sw/redis++/redis++.h>
#include <vector>

using namespace sw::redis;

namespace drone_control {
// This is the data that represents a drone's status
    class DroneControl {
    public:
        explicit DroneControl(Redis &shared_redis);

        Redis &redis;
        std::unordered_map<std::string, drone_data> drones;

        void Run();                                                                      // Run the DroneControl process
        void
        ReadStream();                                                               // Read the stream of data from Redis
        void
        new_setDroneData(const std::vector<std::pair<std::string, std::string>> &); // Update the drones' local data
        std::unordered_map<std::string, std::string>
        getData(int drone_id);              // Get the local data of a drone

    private:
        std::string current_stream_id = "0";  // The id of the last message read from the stream
        int tick_n = 0;
        std::array<std::array<std::pair<float, float>, 124>, 300> drone_paths;  // Array with the paths of all the drones
        std::array<int, 300> drone_path_next_index{};                       // Array with the index of the last point of the path of all the drones
        std::array<bool, 300> checklist{};                                    // Array with bool values to check if the drone is following the path
        Database db;

        void GetDronePaths();

        bool CheckPath(int zone_id, std::pair<float, float> &position);

        void CheckDroneCharge(int drone_id, float charge, float charge_needed);
    };
} // namespace drone_control

#endif  // DRONECONTROLSYSTEM_DRONECONTROL_H