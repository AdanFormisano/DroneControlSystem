// Drone.cpp represents the drone. It receives commands from the base station.
// It sends status updates

#include "spdlog/spdlog.h"

class Drone {
public:
    void start() {
        spdlog::info("Drone starting...");
    }

};