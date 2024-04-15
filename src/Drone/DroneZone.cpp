#include "DroneManager.h"
#include <spdlog/spdlog.h>
#include <utility>
#include <iostream>

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
        zone_redis.set(redis_zone_id + ":id", std::to_string(zone_id)); // TODO: Maybe remove this
        zone_redis.set("zone:" + std::to_string(zone_id) + ":swap", "none");
    }

    // Thread function for the zone-thread
    void DroneZone::Run() {
        // Set initial drone to working
        zone_redis.set("drone:" + std::to_string(drones[0]->getDroneId()) + ":command", "work");
        // Get sim_running from Redis
        bool sim_running = (zone_redis.get("sim_running") == "true");

        // Simulate until sim_running is false
        while (sim_running) {
            try {
                auto tick_start = std::chrono::steady_clock::now();

                // Execute each drone in the zone
                auto number_of_drones = drones.size();
                for (auto &drone: drones) {
                    drone->Run();
                }

                if (drones[0]->getSwap()) {
                    // Set the new drone to working
                    // Set the drone to working
                    drones[1]->SetDronePathIndex(drone_path_index);
#ifdef DEBUG
                    spdlog::info("Drone {} is now working: path index {}", drones[1]->getDroneId(), drones[1]->GetDronePathIndex());
#endif
                    drones[1]->SetDroneState(drone_state_enum::WORKING);
                    zone_redis.hset("drone:" + std::to_string(drones[1]->getDroneId()), "status", "WORKING");
                    zone_redis.set("zone:" + std::to_string(zone_id) + ":swap", "none");

                    drones[0]->setSwap(false);
                }

                if (drones[0]->getDestroy()) {
                    spdlog::info("Drone {} getting destroyed", drones[0]->getDroneId());
                    // Remove the drone from the vector
                    drones.erase(drones.begin());

                    spdlog::info("DroneZone {} now has {} drones", zone_id, drones.size());
                }

                // Check if there is time left in the tick
                auto tick_now = std::chrono::steady_clock::now();
                if (tick_now < tick_start + (tick_duration_ms * number_of_drones)) {
                    // Sleep for the remaining time
                    std::this_thread::sleep_for(tick_start + tick_duration_ms - tick_now);
                } else if (tick_now > tick_start + (tick_duration_ms * number_of_drones)) {
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

    void DroneZone::SpawnThread() {
        zone_thread = boost::thread(&DroneZone::Run, this);
    }

    void DroneZone::CreateDrone(int drone_id) {
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
        spdlog::info("Drone {} created in zone {}", drone_id, zone_id);
    }

    void DroneZone::CreateNewDrone() {
        auto drone_id = new_drone_id + zone_id * 10;
        drones.emplace_back(std::make_shared<Drone>(drone_id, *this));
        zone_redis.sadd("zone:" + std::to_string(zone_id) + ":drones_active", std::to_string(drone_id));
        ++new_drone_id;
    }

    // Creates the drone path for the zone using global coords
    void DroneZone::CreateDronePath() {
        // Determine the boundaries of the drone path
        std::array<std::pair<float, float>, 4> drone_boundaries;
        drone_boundaries[0] = {vertex_coords[0].first + 10, vertex_coords[0].second - 10};
        drone_boundaries[1] = {vertex_coords[1].first - 10, vertex_coords[1].second - 10};
        drone_boundaries[2] = {vertex_coords[2].first - 10, vertex_coords[2].second + 10};
        drone_boundaries[3] = {vertex_coords[3].first + 10, vertex_coords[3].second + 10};

        auto step_size = 20.0f;
        int i = 0;

        using namespace std;

        // From the top left corner to the top right corner
        for (float x = drone_boundaries[0].first; x <= drone_boundaries[1].first; x += step_size) {
            drone_path[i] = {x, drone_boundaries[0].second};
            drone_path_charge[i] = CalculateChargeNeeded(drone_path[i]);
            ++i;
        }
        // From the bottom right corner to the bottom left corner
        for (float x = drone_boundaries[2].first; x >= drone_boundaries[3].first; x -= step_size) {
            drone_path[i] = {x, drone_boundaries[2].second};
            drone_path_charge[i] = CalculateChargeNeeded(drone_path[i]);
            ++i;
        }
    }

    float DroneZone::CalculateChargeNeeded(std::pair<float, float> path_position) {
        // Get the distance from the drone to the base
        float distance = std::sqrt(path_position.first * path_position.first +
                                      path_position.second * path_position.second);
        return distance * DRONE_CONSUMPTION;
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
        // Convert the path array to a vector
        std::vector<std::pair<float, float>> drone_path_v(drone_path.begin(), drone_path.end());

        zone_redis.rpush(redis_path_id, drone_path_v.begin(), drone_path_v.end());
    }
} // namespace drones
