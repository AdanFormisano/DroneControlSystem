#include "DroneManager.h"
#include <spdlog/spdlog.h>
#include <utility>

/* The 6x6 km area is divided into zones_vertex of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones_vertex in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones_vertex,

For testing purposes, the right column will be ignored for now.*/

namespace drones {
    // The zone is created with vertex_coords_sqr set.
    DroneZone::DroneZone(int zone_id, std::array<std::pair<float, float>, 4> &coords, Redis &redis)
            : zone_id(zone_id), vertex_coords(coords), zone_redis(redis) {

        // Create drone path
        CreateDronePath();
        UploadPathToRedis();

        // Calculate the furthest point of the drone path
        path_furthest_point = CalculateFurthestPoint();
#ifdef DEBUG
        spdlog::info("Furthest point of zone {}: ({}, {})", zone_id, path_furthest_point.first,
                     path_furthest_point.second);
#endif

        // Upload the zone to the Redis server
        std::string redis_zone_id = "zone:" + std::to_string(zone_id);
        zone_redis.set(redis_zone_id + ":id", std::to_string(zone_id));
        zone_redis.set("zone:" + std::to_string(zone_id) + ":swap", "none");
    }

    // Thread function for the zone-thread
    void DroneZone::Run() {
        // Set initial drone to working
        zone_redis.set("drone:" + std::to_string(drones[0]->getDroneId()) + ":command", "work");

        // Get sim_running from Redis
        bool sim_running = (zone_redis.get("sim_running") == "true");
        spdlog::info("DroneZone {} sim value: {}", zone_id, sim_running);

        // Simulate until sim_running is false
        while (sim_running) {
            try {
                spdlog::info("DroneZone {} tick {}", zone_id, tick_n);
                auto tick_start = std::chrono::steady_clock::now();

                // Execute each drone in the zone
                for (auto &drone: drones) {
                    drone->Run();
                    spdlog::info("Drone {} executed", drone->getDroneId());
                    if (drone->isDestroyed()) {
                        spdlog::info("Drone {} destroyed", drone->getDroneId());
                        // Remove drone from the zone's vector
                        drone.reset();
                    }
                }

                // Check if there is time left in the tick
                auto tick_now = std::chrono::steady_clock::now();
                if (tick_now < tick_start + tick_duration_ms) {
                    spdlog::info("DroneZone {} going to sleep", zone_id);
                    // Sleep for the remaining time
                    std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
                } else if (tick_now > tick_start + tick_duration_ms) {
                    // Log if the tick took too long
                    spdlog::warn("DroneZone {} tick took too long", zone_id);
                    break;
                }
                // Get sim_running from Redis
                sim_running = (zone_redis.get("sim_running") == "true");
                ++tick_n;
            } catch (const std::exception &e) {
                spdlog::error("DroneZone {} tick {} error: {}", zone_id, tick_n, e.what());
            }
        }
        spdlog::info("DroneZone {} finished", zone_id);
    }

    void DroneZone::CreateDrone(int drone_id) {
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
    }

    void DroneZone::CreateNewDrone() {
        auto drone_id = new_drone_id + zone_id * 10;
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
        ++new_drone_id;
    }

    // Creates the drone path for the zone using global coords
    void DroneZone::CreateDronePath() {
        std::array<std::pair<float, float>, 4> drone_boundaries;
        drone_boundaries[0] = {vertex_coords[0].first + 10, vertex_coords[0].second - 10};
        drone_boundaries[1] = {vertex_coords[1].first - 10, vertex_coords[1].second - 10};
        drone_boundaries[2] = {vertex_coords[2].first - 10, vertex_coords[2].second + 10};
        drone_boundaries[3] = {vertex_coords[3].first + 10, vertex_coords[3].second + 10};

        GenerateLoopPath(drone_boundaries, 20.0f);
    }

    // Generates a loop path for the drone
    void DroneZone::GenerateLoopPath(const std::array<std::pair<float, float>, 4> &drone_boundaries, float step_size) {
        using namespace std;

        // From the top left corner to the top right corner
        for (float x = drone_boundaries[0].first; x <= drone_boundaries[1].first; x += step_size) {
            drone_path.emplace_back(x, drone_boundaries[0].second);
        }
        // From the bottom right corner to the bottom left corner
        for (float x = drone_boundaries[2].first; x >= drone_boundaries[3].first; x -= step_size) {
            drone_path.emplace_back(x, drone_boundaries[2].second);
        }
    }

    // Calculates the furthest point of the drone path from the base
    std::pair<float, float> DroneZone::CalculateFurthestPoint() {
        std::pair<float, float> furthest_point = {0, 0};

        for (const auto &point: vertex_coords) {
            if (abs(point.first) > abs(furthest_point.first)) {
                furthest_point.first = point.first;
            }
            if (abs(point.second) > abs(furthest_point.second)) {
                furthest_point.second = point.second;
            }
        }

        return furthest_point;
    }

    // Uploads the path to the Redis server
    void DroneZone::UploadPathToRedis() {
        std::string redis_path_id = "path:" + std::to_string(zone_id);
        zone_redis.rpush(redis_path_id, drone_path.begin(), drone_path.end());
    }
} // namespace drones
