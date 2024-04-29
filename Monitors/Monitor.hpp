#ifndef MONITOR_H
#define MONITOR_H

#include "../Database/Database.h"

class Monitor {
public:
    explicit Monitor(Database* db);
    
    // Functional monitors
    void checkAreaCoverage();              // 1.1
    void checkZoneVerification();          // 1.2
    void checkDroneRechargeTime();         // 3.1

    // Non-functional monitors
    void checkTimeToReadDroneData();       
    void checkDroneCharge();

    // void checkSystemPerformance();       // 2
    // void checkSystemReliability();

    
private:
    Database* db_;
};

#endif // MONITOR_H





#include "Monitor.h"
#include <chrono>

Monitor::Monitor(Database* db) : db_(db) {}

void Monitor::checkAreaCoverage() {
    // Control of area coverage control
    }

void Monitor::checkPointVerification() {
    // Control of point verification
}

void Monitor::checkDroneAutonomyManagement() {
    // Control of drone autonomy management
}

void Monitor::checkSystemPerformance() {
    // Control of the DCS performance
}

void Monitor::checkSystemReliability() {
    // Control of system reliability
}

