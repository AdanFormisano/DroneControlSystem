//
// Created by roryu on 23/02/2024.
//
#include "ChargeBase.h"
#include <algorithm>

namespace drones {

    ChargeBase* ChargeBase::instance = nullptr;

    bool ChargeBase::takeDrone(Drone &drone) {
        for (auto &slot: chargingSlots) {
            if (!slot.isOccupied) {
                slot.isOccupied = true;
                slot.drone = &drone;
                return true;
            }
        }
        return false;
    }

    void ChargeBase::chargeDrones() {
        for (auto &slot: chargingSlots) {
            if (slot.isOccupied && slot.drone) {
                float newCharge = std::min(100.0f, slot.drone->getCharge() + slot.chargeRate);
                slot.drone->setCharge(newCharge);
                if (newCharge == 100) {
                    slot.drone->onChargingComplete();
                }
            }
        }
    }

    std::optional<Drone *> ChargeBase::releaseDrone() {
        for (auto &slot: chargingSlots) {
            if (slot.isOccupied && slot.drone && slot.drone->getCharge() == 100) {
                Drone *chargedDrone = slot.drone;
                slot.isOccupied = false;
                slot.drone = nullptr;
                return chargedDrone;
            }
        }
        return std::nullopt;
    }
}

