//
// Created by roryu on 23/02/2024.
//

#ifndef DRONECONTROLSYSTEM_CHARGEBASE_H
#define DRONECONTROLSYSTEM_CHARGEBASE_H
#include "../Drone/Drone.h"
#include <vector>
#include <optional>
#include "../Drone/DroneManager.h"

namespace charge_base {

// Forward declaration of Drone to resolve circular dependency
//  Singleton pattern
    using namespace drones;

    class ChargeBase {
    private:
        std::unordered_map<std::string, std::string> drone_data;
        struct ChargingSlot {
            bool isOccupied = false;
            Drone* drone = nullptr; // Pointer to the drone currently being charged (nullptr if empty)
            float chargeRate = 30.0f; // value between 20 and 30
        };

        std::vector<ChargingSlot> chargingSlots;
        static ChargeBase* instance;

        // Private constructor to enforce singleton pattern
        ChargeBase(int numSlots) {
            chargingSlots.resize(numSlots); // Initialize charging slots
        }

        // Prevent copy construction and assignment
        ChargeBase(const ChargeBase&) = delete;
        //ChargeBase& operator=(const ChargeBase&) = delete;

    public:
        //not thread safe thought does it really need to be?
        static ChargeBase* getInstance(int numSlots = 1) {
            if (!instance) {
                instance = new ChargeBase(numSlots); // Constructor
            }
            return instance;
        }
        //a thread safe option could be

        //static ChargeBase& getInstance(int numSlots = 1) {
        //    static ChargeBase instance(numSlots);
        //    return instance;
        //}

        bool takeDrone(Drone& drone);
        void chargeDrones();
        void releaseDrone(int drone_id);
        void requestCharging(Drone& drone);


        // Destructor
        ~ChargeBase() = default;
    };

// Initialize the static instance pointer

}
#endif //DRONECONTROLSYSTEM_CHARGEBASE_H
