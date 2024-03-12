#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H
#include <sw/redis++/redis++.h>

#include <vector>

#include "../globals.h"

using namespace sw::redis;

namespace drone_control {
// This is the data that represents a drone's status
struct drone_data {
    int id;
    std::string status;
    ;
    std::string charge;
    std::pair<int, int> position;
    // std::string latest_update;  // TODO: Add the last time the drone was updated
    // TODO: Add zoneId
};

class DroneControl {
   public:
    void Init();
    explicit DroneControl(Redis& shared_redis);

    Redis& redis;
    drone_data drones[300];  // Array with data of all the drones

    void Run();                                                                      // Run the DroneControl process
    void ReadStream();                                                               // Read the stream of data from Redis
    void setDroneData(const std::unordered_map<std::string, std::string>&);          // Update the drones' local data
    void new_setDroneData(const std::vector<std::pair<std::string, std::string>>&);  // Update the drones' local data
    std::unordered_map<std::string, std::string> getData(int drone_id);              // Get the local data of a drone

   private:
    std::string current_stream_id = "0";  // The id of the last message read from the stream
    int tick_n = 0;
    std::array<std::array<std::pair<int, int>, 124>, 300> drone_paths;  // Array with the paths of all the drones
    std::array<int, 300> drone_path_next_index{};                       // Array with the index of the last point of the path of all the drones
    std::array<bool, 300> checklist;                                    // Array with bool values to check if the drone is following the path

    void GetDronePaths();
    bool CheckPath(int drone_id, std::pair<int, int>&);
};
}  // namespace drone_control

#endif  // DRONECONTROLSYSTEM_DRONECONTROL_H