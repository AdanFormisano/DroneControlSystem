#include "DroneManager.h"
#include <spdlog/spdlog.h>
#include <utility>

namespace drones {
/* The 6x6 km area is divided into zones of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones,

For testing purposes, the right column will be ignored for now.*/

// The zone is created with the global vertex_coords.
DroneZone::DroneZone(int zone_id, std::array<std::pair<int, int>, 4> &coords, DroneManager *drone_manager)
    : zone_id(zone_id), vertex_coords(coords), dm(drone_manager) {
    // Create drone path
    CreateDronePath();

    // Create the zone's drone
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

void DroneZone::CreateDronePath() {
    std::array<std::pair<int, int>, 4> drone_boundaries;
    drone_boundaries[0] = {vertex_coords[3].first + 10, vertex_coords[3].second - 10};
    drone_boundaries[1] = {vertex_coords[2].first - 10, vertex_coords[2].second - 10};
    drone_boundaries[2] = {vertex_coords[1].first - 10, vertex_coords[1].second + 10};
    drone_boundaries[3] = {vertex_coords[0].first + 10, vertex_coords[0].second + 10};

    GenerateLoopPath(drone_boundaries, 1);
}

void DroneZone::GenerateLoopPath(const std::array<std::pair<int, int>, 4> &drone_boundaries, int step_size) {
    using namespace std;
    // from drone_boundaries[0] to drone_boundaries[1]
    for (int i = 1; i <= abs(drone_boundaries[0].first - drone_boundaries[1].first); ++i)
        drone_path.emplace_back(drone_boundaries[0].first - step_size * i, drone_boundaries[0].second);

    // from drone_boundaries[1] to drone_boundaries[2]
    for (int i = 1; i <= abs(drone_boundaries[1].second - drone_boundaries[2].second); ++i)
        drone_path.emplace_back(drone_boundaries[1].first, drone_boundaries[1].second + step_size * i);

    // from drone_boundaries[2] to drone_boundaries[3]
    for (int i = 1; i <= abs(drone_boundaries[2].first - drone_boundaries[3].first); ++i)
        drone_path.emplace_back(drone_boundaries[2].first + step_size * i, drone_boundaries[2].second);

    // from drone_boundaries[3] to drone_boundaries[0]
    for (int i = 1; i <= abs(drone_boundaries[3].second - drone_boundaries[0].second); ++i)
        drone_path.emplace_back(drone_boundaries[3].first, drone_boundaries[3].second - step_size * i);
}
} // namespace drones
