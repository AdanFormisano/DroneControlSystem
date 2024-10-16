#include "LogUtils.h"
#include <iostream>

// Flussi di log
std::ofstream dcsa_log("dcsa.log");   // Log tutto
std::ofstream dcs_log("dcs.log");     // Log DCS
std::ofstream mon_log("monitor.log"); // Log monitor

std::ofstream coverage_log("coverage.log");
std::ofstream recharge_log("recharge.log");
std::ofstream system_log("system.log");
std::ofstream charge_log("charge.log");

void log(const std::string &message) {
    dcs_log << message << std::endl;
    std::cout << message << std::endl;
}

void log_all(const std::string &message) {
    dcsa_log << message << std::endl;
    std::cout << message << std::endl;
}

void log_dcs(const std::string &process, const std::string &message) {
    std::string full_message = "[" + process + "] " + message;
    dcs_log << full_message << std::endl;
    log_all(full_message);
}

void log_main(const std::string &message) {
    log_dcs("Main", message);
}

void log_dc(const std::string &message) {
    log_dcs("DroneControl", message);
}

void log_d(const std::string &message) {
    log_dcs("Drone", message);
}

void log_ds(const std::string &message) {
    log_dcs("DroneState", message);
}

void log_cb(const std::string &message) {
    log_dcs("ChargeBase", message);
}

void log_tg(const std::string &message) {
    log_dcs("TestGenerator", message);
}

void log_sm(const std::string &message) {
    log_dcs("ScannerManager", message);
}

void log_wv(const std::string &message) {
    log_dcs("Wave", message);
}

void log_db(const std::string &message) {
    std::string full_message = "[Database] " + message;
    dcs_log << full_message << std::endl;
    log_all(full_message);
}

void log_monitor(const std::string &message) {
    std::string full_message = "[Monitors] " + message;
    mon_log << full_message << std::endl;
    log_all(full_message);
}

void log_coverage(const std::string &message) {
    std::string full_message = "[CoverageMonitor] " + message;
    coverage_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_recharge(const std::string &message) {
    std::string full_message = "[RechargeMonitor] " + message;
    recharge_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_system(const std::string &message) {
    std::string full_message = "[SystemMonitor] " + message;
    system_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_charge(const std::string &message) {
    std::string full_message = "[ChargeMonitor] " + message;
    charge_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_error(const std::string &process, const std::string &message) {
    std::string full_message;

    if (process.empty()) {
        full_message = "[ERROR] " + message;
    } else {
        full_message = "[ERROR][" + process + "] " + message;
    }

    dcs_log << full_message << std::endl;
    std::cerr << full_message << std::endl;
    log_all(full_message);
}

void close_logs() {
    dcs_log.close();
    dcsa_log.close();
    mon_log.close();
    coverage_log.close();
    recharge_log.close();
    system_log.close();
    charge_log.close();
}