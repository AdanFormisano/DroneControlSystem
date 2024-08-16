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

Monitor::Monitor() {
    // spdlog::info("Monitor object created");

    // Create db connection
    db.ConnectToDB("dcs", "postgres", "admin@123", "127.0.0.1", "5432");
}

void RechargeTimeMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = boost::thread(&RechargeTimeMonitor::checkDroneRechargeTime, this);
    // boost::thread t(&RechargeTimeMonitor::checkDroneRechargeTime, this);
}

// Currently implemented by checking every log entry if the checked field is false
void CoverageMonitor::RunMonitor() {
    // Create a thread to run the monitor
    t = boost::thread(&CoverageMonitor::checkCoverage, this);
    // boost::thread t(&ZoneCoverageMonitor::checkAreaCoverage, this);
}

void DroneChargeMonitor::RunMonitor()
{
    // Create a thread to run the monitor
    t = boost::thread(&DroneChargeMonitor::checkDroneCharge, this);
}

void TimeToReadDataMonitor::RunMonitor()
{
    // Create a thread to run the monitor
    t = boost::thread(&TimeToReadDataMonitor::checktimeToReadData, this);
}
