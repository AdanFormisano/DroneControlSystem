/* Each monitor will run on a different thread, all will be started by the main thread

All monitors are going to be executed in parallel during the simulation. In particular there are 3 functional monitors
and 2 non-functional monitors.

The functional monitors are:
    - checkDroneRechargeTime(): This monitor checks that the number of ticks that a drone has been charging is between
        [2,3] hours.
    - checkAreaCoverage(): This monitor checks that all the points of the 6x6 Km area are verified (covered by the drones)
        every 5 minutes.
    - checkZoneVerification(): This monitor checks that each zone is verified every 5 minutes (this has some internal
        implications).

The non-functional monitors are:
    - checkTimeToReadDroneData(): This monitor checks that the amount of time that the system takes to read one set of
        data entries from the drones is less than 2.24 seconds (this is the amount of time that a tick simulates).
    - checkDroneCharge(): This monitor checks that the drones' charge is correctly managed by the system. This means that
        the drones are swapped when their charge is just enough to return to the base.
*/

#include "Monitor.h"
#include "../../utils/LogUtils.h"
#include <iostream>

Monitor::Monitor(Redis &redis) : shared_redis(redis) {

    // Create db connection
    db.ConnectToDB();
}

int Monitor::JoinThread() {
    if (t.joinable()) {
        t.join();
        return EXIT_SUCCESS;
    }
    return EXIT_FAILURE;
}

void Monitor::WriteToDB(const std::string &query) {
    try {
        log_monitor("Writing to DB");
        pqxx::work W(db.getConnection());
        W.exec(query);
        W.commit();
    } catch (const std::exception &e) {
        log_error("Monitor", e.what());
        // std::cerr << e.what() << std::endl;
    }
}

void Monitor::CheckSimulationEnd() {
    // Check if the timestamp is the same as the last processed time
    if (old_processed_time == latest_processed_time) {
        log_monitor("Simulation ended");
        sim_running = false;
    }
}

void RechargeTimeMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = std::thread(&RechargeTimeMonitor::checkDroneRechargeTime, this);
}

// Currently implemented by checking every log entry if the checked field is false
void WaveCoverageMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = std::thread(&WaveCoverageMonitor::checkWaveCoverage, this);
}

void AreaCoverageMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = std::thread(&AreaCoverageMonitor::checkAreaCoverage, this);
}

void SystemPerformanceMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = std::thread(&SystemPerformanceMonitor::checkPerformance, this);
}

void DroneChargeMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = std::thread(&DroneChargeMonitor::checkDroneCharge, this);
}
