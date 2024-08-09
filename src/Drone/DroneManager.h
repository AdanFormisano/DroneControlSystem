#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H

#include "../../utils/utils.h"
#include "../globals.h"
#include "sw/redis++/redis++.h"
#include <algorithm>
#include <array>
#include <boost/thread.hpp>
#include <chrono>
#include <cmath>
#include <queue>
#include <vector>

using namespace sw::redis;

namespace drones {
    class DroneZone;
    class Drone;
}  // namespace drones

namespace drones {
class DroneManager {
public:
    Redis &shared_redis;
    std::array<std::array<std::pair<float, float>, 4>, ZONE_NUMBER> zones_vertex;  // Array of all the zones_vertex' vertex_coords_sqr, NOT global coords
    std::unordered_map<int, std::shared_ptr<DroneZone>> zones;                     // Vector of all the DroneZones
    std::vector<boost::thread> zone_threads;                                       // Vector of all the DroneZones threads

    explicit DroneManager(Redis &);

    void Run();                         // Main execution loop
    void CalculateGlobalZoneCoords();   // Calculates the global coords of the zones_vertex

private:
    int tick_n = 0;
    int column_n = 0;

    void CreateZones();        // Create the zones_vertex and drones objects
    void CreateZoneThreads();  // Create the threads for the zones_vertex
    void CheckNewDrones();     // Check if there are new drones to be created
};

class DroneZone {
public:
    int tick_n = 0;
    Redis &zone_redis;
    std::array<std::pair<float, float>, 4> vertex_coords;   // Global coords that define the zone
    std::pair<float, float> path_furthest_point;            // Furthest point of the drone path from the base
    std::vector<std::shared_ptr<Drone>> drones;             // Vector of drones owned by the zone
    std::vector<std::pair<float, float>> drone_path;        // Drone path in global coords
    int drone_path_index = 0;                               // Index of the current position in the drone_path
    std::array<float, 124> drone_path_charge{};             // Charge needed to go back to the base for every point in the drone_path
    std::vector<drone_state_enum> last_drones_state;        // Vector of the last state of the drones

    DroneZone(int zone_id, std::array<std::pair<float, float>, 4> &zone_coords, Redis &redis);

    [[nodiscard]] int getZoneId() const { return zone_id; }
    [[nodiscard]] int getNewDroneId() const { return new_drone_id; }
    [[nodiscard]] bool getIsDroneWorking() const { return drone_is_working; }

    void setIsDroneWorking(bool value) { drone_is_working = value; }

    void Run();                                             // Main execution loop
    void CreateDrone(int drone_id);                         // Create a drone with a given id (respawn a drone)
    void CreateNewDrone();                                  // Create a new drone for the zone
    void SwapFaultyDrone(std::pair<float, float> coords);   // Swaps the faulty drone with a working one
    void SpawnThread(int tick_n_drone_manager);             // Spawn the zone thread
    void CreateDroneFault(int drone_id,
        drone_state_enum fault_state,
        std::pair<float, float> fault_coords,
        int tick_start, int tick_end, int reconnect_tick);  // Create a drone fault

private:
    const int zone_id;
    int new_drone_id = 1;
    bool drone_is_working = false;
    int drone_working_id = 0;
    boost::thread zone_thread;

    // TODO: Make global
    struct drone_fault {
        int drone_id;
        drone_state_enum fault_state;
        std::pair<float, float> fault_coords;
        int zone_id;
        int tick_start;
        int tick_end;
        int reconnect_tick;
    };

    std::vector<drone_fault> drone_faults;
    bool drone_fault_ack = false;

    void CreateDronePath();                                              // Create the drone path for the zone using global coords
    static float CalculateChargeNeeded(std::pair<float, float> coords);  // Calculate the charge needed to go back to the base
    void UploadPathToRedis();                                            // Upload path to the Redis server
    std::pair<float, float> CalculateFurthestPoint();                    // Calculate the furthest point of the drone path
    void CheckDroneWorking();                                            // Check if the drone is working
    void ManageFaults();                                                 // Manage drone faults
};

class Drone {
public:
    float drone_charge_to_base = 0.0f;

    Drone(int drone_id, DroneZone &droneZone);
    Drone(int drone_id, DroneZone& droneZone, drone_state_enum drone_state);

    [[nodiscard]] int getDroneId() const { return drone_id; }
    [[nodiscard]] std::pair<float, float> getDronePosition() const { return drone_position; }
    [[nodiscard]] int getDronePathIndex() const { return path_index; }
    [[nodiscard]] bool getDestroy() const { return destroy; }
    [[nodiscard]] bool getSwap() const { return swap; }
    [[nodiscard]] drone_state_enum getDroneState() const { return drone_state; }
    [[nodiscard]] bool getConnectedToSys() const { return connected_to_sys; }

    void setDronePathIndex(int index) { path_index = index; }
    void setDroneState(drone_state_enum state);
    void setDestroy(bool value) { destroy = value; }
    void setSwap(bool value) { swap = value; }
    void setConnectedToSys(bool value) { connected_to_sys = value; }
    void setFinalCoords(std::pair<float, float> coords) { swap_final_coords = coords; }

    void Run();                         // Main execution loop
    void CalculateSwapFinalCoords();    // Calculate the final coords of the swap

private:
    int tick_n;
    Redis &drone_redis;
    std::string redis_id;
    std::pair<float, float> swap_final_coords;  // Final coords of the swap
    DroneZone &dz;                              // Reference to the "parent" DroneZone object
    int path_index = 0;                         // Index of the current position in the drone_path
    bool swap = false;                          // Flag to swap the drone
    bool destroy = false;                       // Flag to destroy the drone
    int reconnect_tick = -1;                    // Tick when the drone will reconnect
    bool fault_managed = false;                 // Flags whether a new fault has been created
    bool connected_to_sys = true;               // Flags whether the drone is in a fault state

    // Drone data
    const int drone_id;
    drone_state_enum drone_state = drone_state_enum::IDLE_IN_BASE;  // Drone state
    float drone_charge = 100.0f;                                    // Drone charge in percentage
    std::pair<float, float> drone_position = {0, 0};            // Drone position in global coords
    std::vector<std::pair<std::string, std::string>> drone_data;    // Drone data container

    void SetChargeNeededToBase();               // Set the charge needed to go back to the base
    void Work();                                // Drone work
    void Move(float, float);                    // Move the drone
    void UseCharge(float distance_in_meters);   // Use the charge of the drone
    void FollowPath();                          // Move the drone following the drone_path
    void FollowDrone();                         // Move the drone following another drone's path
    void UploadStatusOnStream();                // FIXME: This is a placeholder for the status update function
    void UploadStatus();                        // Upload the drone status to the Redis server
    void SendChargeRequest();                   // Sends a charge request to the base
    drone_state_enum CheckDroneStateOnRedis();  // Check the drone state from the Redis server
    bool Exists();                              // Check if the drone exists on the Redis server
};
}  // namespace drones

#endif  // DRONECONTROLSYSTEM_DRONEMANAGER_H
