#ifndef MONITOR_H
#define MONITOR_H

#include "../Database/Database.h"

class Monitor {
public:
    explicit Monitor(Database* db);
    
    // Functional monitors
    void checkAreaCoverage();
    void checkPointVerification();
    void checkDroneAutonomyManagement();

    // Non-functional monitors
    void checkSystemPerformance();
    void checkSystemReliability();

    
private:
    Database* db_;
};

#endif // MONITOR_H
