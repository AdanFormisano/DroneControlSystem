#include "ChargeBase.h"
#include <algorithm>
#ifndef SPDLOG_GUARD_H
#define SPDLOG_GUARD_H
#include "spdlog/spdlog.h"
#endif

namespace charge_base {

    ChargeBase* ChargeBase::instance = nullptr;

    bool ChargeBase::takeDrone(Drone &drone) {
        for (int i = 0; i < chargingSlots.size(); ++i) {
            ChargeBase::ChargingSlot slot = chargingSlots[i];
            if(!slot.isOccupied) {
                slot.isOccupied = true;
                slot.drone = &drone;
                drone_data = {
                        {"id", std::to_string(drone.get_id())},
                        {"slot_id", std::to_string(i)}
                };

                // Updating the drone's status in Redis using streams
                auto redis_stream_id = drone.drone_redis.xadd("drones_at_base", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message
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
                }else{
                    slot.drone->onCharging();
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

    void requestCharging(Drone& drone) {
        // Example logging, adjust as needed
        spdlog::info("Drone {} requesting charging", drone.get_id());
        charge_base::ChargeBase* chargeBase = charge_base::ChargeBase::getInstance();
        if (chargeBase && chargeBase->takeDrone(drone)){
            drone.status = "Charging Requested";
            drone.drone_redis.hset(drone.getRedisId(), "status", drone.status);
        }
    }
}

