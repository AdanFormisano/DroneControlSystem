#include "DroneZone.h"

namespace drones {
/* The 6x6 km area is divided into zones of 62x2 squares, each square is 20x20 meters. The subdivision is not
perfect: there is enough space for 4 zones in the x-axis and 150 zone in the y-axis. This creates a right "column"
that is 52 squares wide and 2 squares tall. The drones in this area will have to move slower because they have less
space to cover.
The right column will have to be managed differently than the rest of the zones,

For testing purposes, the right column will be ignored for now.*/

    // The zone is created with the global coordinates.
    DroneZone::DroneZone(int x, int y) : global_coordinates(x, y) {

    }
}

