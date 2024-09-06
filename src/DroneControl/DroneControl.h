#ifndef DRONECONTROLSYSTEM_DRONECONTROL_H
#define DRONECONTROLSYSTEM_DRONECONTROL_H
#include "../globals.h"
#include "../Database/Buffer.h"
#include <sw/redis++/redis++.h>
#include <boost/interprocess/ipc/message_queue.hpp>
#include <vector>
#include <spdlog/spdlog.h>

using namespace sw::redis;
using namespace boost::interprocess;

namespace drone_control {
    class DroneControl {
    public:
        Redis &redis;
        std::unordered_map<std::string, drone_data> drones;     // Container for drone-data

        explicit DroneControl(Redis &shared_redis);

        void Run();                 // Run the DroneControl process
        void ReadDataStream();      // Read the stream of data sent from drones on Redis
        void WorkOnStream(const std::vector<std::pair<std::string, std::string>> &data); // Parse the stream data
        void WorkOnStreamDump(const std::vector<std::pair<std::string, std::string>> &data); // Parse the stream data
        void FaultAck();            // Acknowladge all drone faults sent from DroneZones

    private:
        int tick_n = 0;
        std::string current_stream_id = "0";  // Id of the last message read from the stream
        std::array<std::vector<std::pair<float, float>>, ZONE_NUMBER> drone_paths;  // Paths for every drone
        std::array<int, ZONE_NUMBER> drone_path_next_index{};                       // Index of next path step for each drone
        std::array<bool, ZONE_NUMBER> checklist{};                                  // Bool values to check if each drone is following the designated path
        Database db;                                // Dabase object
        Buffer buffer;                              // Buffer object
        MiniBufferContainer mini_buffer_container;  // Container for mini_buffers

        void GetDronePaths();                       // Retrieve from Redis every drone path
        bool CheckPath(int zone_id, const std::pair<float, float> &position);         // Compare the current drone position with the designated path
        void CheckDroneCharge(int drone_id, float charge, float charge_needed); // Check current drone charge value
        void CheckForSwap(int zone_id, int drone_id, float current_charge, float charge_needed);    // Check if zone needs a drone swap
    };
} // namespace drone_control
#endif  // DRONECONTROLSYSTEM_DRONECONTROL_H
