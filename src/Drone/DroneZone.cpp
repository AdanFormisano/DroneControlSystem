#include "DroneManager.h"
#include <spdlog/spdlog.h>

namespace drones {
/* The 6x6 km area is divided into zones of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones,

For testing purposes, the right column will be ignored for now.*/

    // The zone is created with the global coordinates.
    DroneZone::DroneZone(int zone_id, std::array<std::pair<int, int>, 4> &coords, Redis& shared_redis, DroneManager* dm)
        : id(zone_id), coordinates(coords), drone_redis(shared_redis), drone_manager(dm) {
            // spdlog::info("Zone {} created 1:({},{}) 2:({},{}) 3:({},{}) 4:({},{})", zone_id,
            //              coordinates[0].first, coordinates[0].second,
            //              coordinates[1].first, coordinates[1].second,
            //              coordinates[2].first, coordinates[2].second,
            //              coordinates[3].first, coordinates[3].second);

        // Create the zone's drone
        int drone_id = zone_id;
        auto drone = std::make_shared<Drone>(drone_id, drone_redis);

        // Add the drone to the vector
        // TODO: What is going on here?? Do I need & or not?!?!?1          Check here-->
        std::vector<std::shared_ptr<Drone>>& drone_vector = dm->getDroneVector(); // <-- CHECK HERE
        // Add the drone's thread to the vector
        std::vector<std::thread>& thread_vector = dm->getDroneThreads();
        thread_vector.emplace_back(&Drone::Run, drone.get());
        drone_vector.push_back(std::move(drone));
    }
}
