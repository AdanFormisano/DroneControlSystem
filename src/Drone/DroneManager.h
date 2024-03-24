#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H
#include "sw/redis++/redis++.h"
#include "../globals.h"
#include "../../utils/utils.h"
#include <array>
#include <thread>
#include <queue>
#include <boost/thread.hpp>
#include <chrono>

using namespace sw::redis;

namespace drones {
class DroneZone;
class Drone;
} // namespace drones

namespace drones {
class DroneManager {
public:
    Redis &shared_redis;
    std::array<std::array<std::pair<int, int>, 4>, 300> zones; // Array of all the zones' vertex_coords_sqr, NOT global coords
    std::vector<DroneZone> drone_zones;                        // Vector of all the zones objects
    std::vector<std::shared_ptr<Drone>> drone_vector;          // Vector of all the drones objects
    std::vector<boost::thread> drone_threads;                    // Vector of all the drones threads

    explicit DroneManager(Redis &);
     ~DroneManager();

    void Run();
    void CalculateGlobalZoneCoords();

    // Returns a reference to the vector of drones
    //std::vector<std::shared_ptr<Drone>> &getDroneVector() { return drone_vector; }
    // Returns a reference to the vector of threads
    //std::vector<std::thread> &getDroneThreads() { return drone_threads; }

private:
    int tick_n = 0;
    std::array<int, 300> new_drones_id{};
    // For a set of vertex_coords_sqr creates a DroneZone object
    DroneZone* CreateDroneZone(std::array<std::pair<int, int>, 4> &, int);

    void CreateDrone(int, DroneZone*);
    void CreateThreadBlocks();
    // void CheckNewDrones();
};

class DroneZone {
public:
    int zone_width = 62;                                    // In #squares
    int zone_height = 2;                                    // In #squares
    std::shared_ptr<DroneManager> dm;
    std::array<std::pair<int, int>, 4> vertex_coords_sqr;   // Coords of the "squares" that define the zone
    std::array<std::pair<int, int>, 4> vertex_coords_glb;   // Global coords that define the zone
    // TODO: Use list of pairs instead of vector
    std::vector<std::pair<int, int>> drone_path;            // Path that the drone will follow
    int drone_path_index = 0;

    DroneZone(int, std::array<std::pair<int, int>, 4> &, DroneManager *);
    ~DroneZone() = default;
    int getZoneId() const { return zone_id; }

private:
    const int zone_id;
    std::shared_ptr<Drone> drone_ptr; // Pointer to its drone

    std::array<std::pair<int, int>, 4> SqrToGlbCoords();                    // Converts the sqr verteces to global coords
    void CreateDronePath();                                                 // Creates the drone path for the zone using global coords
    void GenerateLoopPath(const std::array<std::pair<int, int>, 4> &, int);// Generates a loop path for the drone
    void UploadPathToRedis();                                               // Uploads the path to the Redis server
};

class Drone {
public:
    float drone_charge_to_base;         // Charge needed to go back to the base

    Drone(int, DroneZone *, const DroneManager *);
    int Run();

private:
    Redis &drone_redis;
    std::string redis_id;
    DroneZone *dz;
    const DroneManager *dm;
    int path_index;         // Index of the current position in the drone_path

    // Drone data
    const int drone_id;       // TODO: It should be generated by the system
    drone_state_enum drone_state;
    float drone_charge;
    std::pair<float, float> drone_position;
    int tick_n;

    void SetChargeNeededToBase();   // Sets the charge needed to go back to the base
    void Work();
    void Move(float, float);
    void UseCharge(float distance_in_meters);  // Uses the charge of the drone
    void FollowPath();              // Moves the drone following the drone_path
    void UploadStatusOnStream();    // FIXME: This is a placeholder for the status update function
    void UploadStatus();
    void SendChargeRequest();       // Sends a charge request to the base
    float CalculateChargeNeeded();
    bool Exists();
    // TODO: Add the last time the drone was updated
};
} // namespace drones

#endif // DRONECONTROLSYSTEM_DRONEMANAGER_H
