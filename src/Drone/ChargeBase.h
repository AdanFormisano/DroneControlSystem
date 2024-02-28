//
// Created by roryu on 23/02/2024.
//

#ifndef DRONECONTROLSYSTEM_CHARGEBASE_H
#define DRONECONTROLSYSTEM_CHARGEBASE_H
#include <vector>
#include <optional>
#include "Drone.h"

namespace drones {

// Forward declaration of Drone to resolve circular dependency
//  Singleton pattern

    class Drone;

    class ChargeBase {
    private:
        struct ChargingSlot {
            bool isOccupied = false;
            Drone* drone; // Pointer to the drone currently being charged (nullptr if empty)
            float chargeRate = 10.0f; // can be changed according to how we deal with time
        };

        std::vector<ChargingSlot> chargingSlots;
        static ChargeBase* instance;

        // Private constructor to enforce singleton pattern
        ChargeBase(int numSlots) {
            chargingSlots.resize(numSlots); // Initialize charging slots
        }

        // Prevent copy construction and assignment
        ChargeBase(const ChargeBase&) = delete;
        ChargeBase& operator=(const ChargeBase&) = delete;

    public:
        static ChargeBase* getInstance(int numSlots = 1) {
            if (!instance) {
                instance = new ChargeBase(numSlots); // Constructor
            }
            return instance;
        }

        bool takeDrone(Drone& drone);
        void chargeDrones();
        std::optional<Drone*> releaseDrone();

        // Destructor
        ~ChargeBase() = default;
    };

// Initialize the static instance pointer

}
#endif //DRONECONTROLSYSTEM_CHARGEBASE_H