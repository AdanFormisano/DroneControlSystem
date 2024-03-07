#include "DroneManager.h"
#include <spdlog/spdlog.h>
#include <utility>

/* The 6x6 km area is divided into zones of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones,

For testing purposes, the right column will be ignored for now.*/

namespace drones {
    // The zone is created with vertex_coords_sqr set.
DroneZone::DroneZone(int zone_id, std::array<std::pair<int, int>, 4> &coords, DroneManager *drone_manager)
    : zone_id(zone_id), vertex_coords_sqr(coords), dm(drone_manager) {
    // Calculate the real global coord needed for the drone path
    vertex_coords_glb = SqrToGlbCoords();
    // Create drone path
    CreateDronePath();

    // Uploads the path to the Redis server
        UploadPathToRedis();

        // Create the drone
    CreateDrone();
}

void DroneZone::CreateDrone() {
    // Create the zone's drone
    int drone_id = zone_id; // TODO: This is a placeholder, use better drone_id
    auto drone = std::make_shared<Drone>(drone_id, this);

    // Gets the vectors of drones and threads from the DroneManager
    std::vector<std::shared_ptr<Drone>> &drone_vector = dm->getDroneVector();
    // std::vector<std::thread>& thread_vector = dm->getDroneThreads();

    // Create the thread for the drone
    // thread_vector.emplace_back(&Drone::Run, drone.get());
    // Adds the drone to the vector
    drone_vector.push_back(std::move(drone));
}

// Converts the square coordinates to global coordinates used for the drone path
    std::array<std::pair<int, int>, 4> DroneZone::SqrToGlbCoords() {
        std::array<std::pair<int, int>, 4> global_coords;
        for (int i = 0; i < 4; ++i) {
            global_coords[i].first = vertex_coords_sqr[i].first * 20;
            global_coords[i].second = vertex_coords_sqr[i].second * 20;
        }
        return global_coords;
    }

    // Creates the drone path for the zone using global coordsvoid DroneZone::CreateDronePath() {
    std::array<std::pair<int, int>, 4> drone_boundaries;
    drone_boundaries[0] = {vertex_coords_glb[3].first + 10, vertex_coords_glb[3].second - 10};
    drone_boundaries[1] = {vertex_coords_glb[2].first - 10, vertex_coords_glb[2].second - 10};
    drone_boundaries[2] = {vertex_coords_glb[1].first - 10, vertex_coords_glb[1].second + 10};
    drone_boundaries[3] = {vertex_coords_glb[0].first + 10, vertex_coords_glb[0].second + 10};

    GenerateLoopPath(drone_boundaries, 20);
}

    // Generates a loop path for the drone
    void DroneZone::GenerateLoopPath(const std::array<std::pair<int, int>, 4>& drone_boundaries, int step_size) {
        using namespace std;
        // From the top left corner to the top right corner
        for (int x = drone_boundaries[0].first; x <= drone_boundaries[1].first; x += step_size) {
            drone_path.emplace_back(x, drone_boundaries[0].second);
        }
        // From the bottom right corner to the bottom left corner
        for (int x = drone_boundaries[2].first; x >= drone_boundaries[3].first; x -= step_size) {
            drone_path.emplace_back(x, drone_boundaries[2].second);
        }
    }

    // Uploads the path to the Redis server
    void DroneZone::UploadPathToRedis() {
        redis_path_id = "path:" + std::to_string(zone_id);
        dm->shared_redis.rpush(redis_path_id, drone_path.begin(), drone_path.end());
    }
}
} // namespace drones
