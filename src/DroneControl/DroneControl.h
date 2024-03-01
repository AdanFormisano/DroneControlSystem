#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H
#include "../globals.h"
#include <vector>
#include <sw/redis++/redis++.h>

using namespace sw::redis;

namespace drone_control {
    void Init(Redis& redis);

    // This is the data that represents a drone's status
    struct drone_data {
        int id;
        std::string status;;
        std::string charge;
        std::pair<float, float> position;
        // TODO: Add zoneId
        // TODO: Add latest status update time
    };

    class DroneControl {
    public:
        explicit DroneControl(Redis& shared_redis);
        
        Redis& redis;
        drone_data drones[300];     // Array with data of all the drones

        void Run();                 // Run the DroneControl process
        void ReadStream();          // Read the stream of data from Redis
        void setDroneData(const std::unordered_map<std::string, std::string>&); // Update the drones' local data
        std::unordered_map<std::string, std::string> getData(int drone_id);     // Get the local data of a drone

        int tick_n;

    private:
        std::string current_stream_id = "0";    // The id of the last message read from the stream
    };
} // drone_control

#endif //DRONECONTROLSYSTEM_DRONECONTROL_H