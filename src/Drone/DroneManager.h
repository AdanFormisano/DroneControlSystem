#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H

#include "sw/redis++/redis++.h"
#include "../globals.h"
#include "../../utils/utils.h"
#include <array>
#include <thread>
#include <queue>
#include <boost/thread.hpp>
#include <boost/thread/lockable_adapter.hpp>
#include <chrono>
#include <vector>
#include <cmath>
#include <algorithm>

using namespace sw::redis;

namespace drones {
    class DroneZone;

    class Drone;
}

namespace drones {
    class DroneManager {
    public:
        Redis &shared_redis;
        std::array<std::array<std::pair<float, float>, 4>, ZONE_NUMBER> zones_vertex;  // Array of all the zones_vertex' vertex_coords_sqr, NOT global coords
        std::unordered_map<int, std::shared_ptr<DroneZone>> zones;                                     // Vector of all the zones_vertex objects
        std::vector<boost::thread> zone_threads;                                // Vector of all the drones threads

        explicit DroneManager(Redis &);

        void Run();
        void CalculateGlobalZoneCoords();

    private:
        int tick_n = 0;

        void CreateZones();                                                   // Creates the zones_vertex and drones objects
        void CreateZoneThreads();                                               // Creates the threads for the zones_vertex
        void CheckNewDrones();    // Checks if there are new drones to be created
    };

    class DroneZone {
    public:
        Redis &zone_redis;
        std::array<std::pair<float, float>, 4> vertex_coords;   // Global coords that define the zone
        std::pair<float, float> path_furthest_point;
        std::vector<std::shared_ptr<Drone>> drones;              // Vector of drones owned by the zone
        std::array<std::pair<float, float>, 124> drone_path;
        int drone_path_index = 0;
        std::array<float, 124> drone_path_charge;
        std::shared_ptr<Drone> drone_working;

        DroneZone(int zone_id, std::array<std::pair<float, float>, 4> &zone_coords, Redis &redis);
        ~DroneZone() = default;

        [[nodiscard]] int getZoneId() const { return zone_id; }
        [[nodiscard]] int getNewDroneId() const { return new_drone_id; }

        void Run();
        void CreateDrone(int drone_id);     // Creates a new drone
        void CreateNewDrone();                                                          // Creates a new drone

    private:
        const int zone_id;
        int tick_n = 0;
        int new_drone_id = 1;

        void CreateDronePath();                                                         // Creates the drone path for the zone using global coords
        static float CalculateChargeNeeded(std::pair<float, float> coords);                                                  // Calculates the charge needed to go back to the base
        void UploadPathToRedis();                                                       // Uploads the path to the Redis server
        std::pair<float, float> CalculateFurthestPoint();                               // Calculates the furthest point of the drone path
    };

    class Drone {
    public:
        float drone_charge_to_base = 0.0f;         // Charge needed to go back to the base

        Drone(int, DroneZone &);

        [[nodiscard]] bool isDestroyed() const { return drone_destroy; }
        [[nodiscard]] int getDroneId() const { return drone_id; }
        [[nodiscard]] std::pair<float, float> getDronePosition() const { return drone_position; }

        void Run();

    private:
        int tick_n = 0;
        Redis &drone_redis;
        std::string redis_id;
        DroneZone &dz;
        int path_index = 0;                         // Index of the current position in the drone_path
        bool drone_destroy = false;                 // Flag to destroy the drone

        // Drone data
        const int drone_id;
        drone_state_enum drone_state = drone_state_enum::IDLE_IN_BASE;
        float drone_charge = 100.0f;                // Drone charge in percentage
        std::pair<float, float> drone_position ={0, 0};   // Drone position in global coords
        std::vector<std::pair<std::string, std::string>> drone_data;

        void SetChargeNeededToBase();               // Sets the charge needed to go back to the base
        void Work();                                // Drone work
        void Move(float, float);                    // Moves the drone
        void UseCharge(float distance_in_meters);   // Uses the charge of the drone
        void FollowPath();                          // Moves the drone following the drone_path
        void FollowDrone();                         // Moves the drone following another drone's path
        void UploadStatusOnStream();                // FIXME: This is a placeholder for the status update function
        void UploadStatus();                        // Uploads the drone status to the Redis server
        void SendChargeRequest();                   // Sends a charge request to the base
        bool Exists();
    };
} // namespace drones

#endif // DRONECONTROLSYSTEM_DRONEMANAGER_H
