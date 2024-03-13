#include "ChargeBase.h"
#include <algorithm>
#ifndef SPDLOG_GUARD_H
#define SPDLOG_GUARD_H
#include "spdlog/spdlog.h"
#endif

namespace charge_base {

    ChargeBase* ChargeBase::instance = nullptr;

    bool ChargeBase::takeDrone(Drone &drone) {

        auto generateRandomFloat = []() -> float {
            thread_local std::random_device rd;
            thread_local std::mt19937 engine(rd());
            std::uniform_real_distribution<float> dist(20.0f, 30.0f);
            return dist(engine);
        }; //GBT FTW

        for (int i = 0; i < chargingSlots.size(); ++i) {
            ChargeBase::ChargingSlot &slot = chargingSlots[i];
            if(!slot.isOccupied) {
                slot.isOccupied = true;
                slot.chargeRate= generateRandomFloat();
                slot.drone = &drone;
                drone_data = {
                        {"id", std::to_string(drone.get_id())},
                        {"slot_id", std::to_string(i)}
                };

                // Updating the drone's status in Redis using streams
                //auto redis_stream_id = drone.drone_redis.xadd("drones_at_base", "*", drone_data.begin(), drone_data.end()); // Returns the ID of the message

                return true;
            }
        }
        return false;
    }

    void ChargeBase::chargeDrones() {
        for (auto &slot: chargingSlots) {
            if (slot.isOccupied && slot.drone) {
                float newCharge = std::min(100.f, slot.drone->getCharge() + slot.chargeRate);
                slot.drone->setCharge(newCharge);
                if (newCharge == 100.f) {
                    slot.drone->onChargingComplete();
                }else{
                    slot.drone->onCharging();
                }
            }
        }
    }

    //void ChargeBase::releaseDrone(int drone_id) {
    //    for (auto &slot: chargingSlots) {
    //        if (slot.isOccupied && slot.drone && slot.drone->getCharge() == 100.f) {
    //            Drone *chargedDrone = slot.drone;
    //            slot.isOccupied = false;
    //            slot.drone = nullptr;
    //            return chargedDrone;
    //        }
    //    }
    //    return std::nullopt;
    //}
    void ChargeBase::releaseDrone(int drone_id) {
        for (auto &slot: chargingSlots) {
            if (slot.drone->get_id()==drone_id) {
                slot.isOccupied = false;
                slot.drone = nullptr;

            }}}



}

