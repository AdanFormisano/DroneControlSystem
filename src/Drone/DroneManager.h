#ifndef DRONECONTROLSYSTEM_DRONEMANAGER_H
#define DRONECONTROLSYSTEM_DRONEMANAGER_H
#include "sw/redis++/redis++.h"
#include "../ChargeBase/ChargeBase.h"
#include <array>
#include <thread>

using namespace sw::redis;

namespace drones {
class DroneZone;
    class Drone;
}

namespace drones {


class DroneManager {
public:
    Redis& shared_redis;
    std::array<std::array<std::pair<int, int>, 4>, 300> zones;// Array of all the zones' vertex_coords_sqr, NOT global coords
    std::vector<DroneZone> drone_zones;// Vector of all the zones objects
    std::vector<std::shared_ptr<Drone>> drone_vector;// Vector of all the drones objects
    std::vector<std::thread> drone_threads;// Vector of all the drones threads
    int n_data_sent = 0;

    explicit DroneManager(Redis &);
    ~DroneManager();

     void Run();
    void CalculateGlobalZoneCoords();

    // Returns a reference to the vector of drones
    std::vector<std::shared_ptr<Drone>> &getDroneVector() { return drone_vector; }
    // Returns a reference to the vector of threads
    std::vector<std::thread> &getDroneThreads() { return drone_threads; }

    private:
int tick_n = 0;
        // For a set of vertex_coords_sqr creates a DroneZone object
        void CreateDroneZone(std::array<std::pair<int, int>, 4>&, int);
        void CreateThreadBlocks();
};

class DroneZone {
public:
    int zone_width = 62;                                    // In #squares
        int zone_height = 2;                                    // In #squares
        // TODO: Use list of pairs instead of vector
        std::array<std::pair<int, int>, 4> vertex_coords_sqr;   // Coords of the "squares" that define the zone
        // TODO: Use list of pairs instead of vector
        std::array<std::pair<int, int>, 4> vertex_coords_glb;   // Global coords that define the zone
        // TODO: Use list of pairs instead of vector
        std::vector<std::pair<int, int>> drone_path;            // Path that the drone will follow
        std::string redis_path_id;
        DroneManager* dm;DroneZone(int, std::array<std::pair<int, int>, 4> &,  DroneManager *);
    ~DroneZone() = default;


    private:
    const int zone_id;
    std::shared_ptr<Drone> drone_ptr;   // Pointer to its drone

    void CreateDrone();
        std::array<std::pair<int, int>, 4> SqrToGlbCoords();                    // Converts the sqr verteces to global coords
        void CreateDronePath();                                                 // Creates the drone path for the zone using global coords
        void GenerateLoopPath(const std::array<std::pair<int, int>, 4>&, int);  // Generates a loop path for the drone
        void UploadPathToRedis();                                               // Uploads the path to the Redis server
    };

    class Drone {
    public:
        Drone(int, DroneZone*);
        void Run();

        // Utilize for charging simulation
        void setCharge(float newCharge);
        [[nodiscard]] float getCharge() const;
        void onChargingComplete();

private:
    std::string redis_id;
        drones::DroneZone* dz;
        Redis& drone_redis;
        int path_index;         // Index of the current position in the drone_path

        // Drone's data
        std::vector<std::pair<std::string, std::string>> drone_data;
    const int drone_id;               // TODO: It should be generated by the system
        std::string drone_status;         // TODO: Change to enum
        float drone_charge;
        std::pair<int, int> drone_position;
        // TODO: Add the last time the drone was updated

        int tick_n;

        void Move();                // FIXME: This is a placeholder for the movement function
        void UpdateStatus();        // FIXME: This is a placeholder for the status update function
        void InitThread();          // FIXME: This is a placeholder for the thread initialization function

        // For charging interaction
        void requestCharging();
};
} // namespace drones

#endif // DRONECONTROLSYSTEM_DRONEMANAGER_H
