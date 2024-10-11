#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <fstream>
#include <string>

extern std::ofstream dcsa_log; // Tutto
extern std::ofstream dcs_log;  // Tutto dcs
extern std::ofstream mon_log;  // Tutti i monitor

extern std::ofstream coverage_log;
extern std::ofstream recharge_log;
extern std::ofstream system_log;
extern std::ofstream charge_log;

void log_all(const std::string &message);  // Log cumulativo
void log_main(const std::string &message); // Su dcs_log + [Main]
void log_dc(const std::string &message);   // Su dcs_log + [DroneControl]
void log_d(const std::string &message);    // Su dcs_log + [Drone]
void log_ds(const std::string &message);   // Su dcs_log + [DroneState]
void log_cb(const std::string &message);   // Su dcs_log + [ChargeBase]
void log_tg(const std::string &message);   // Su dcs_log + [TestGenerator]
void log_sm(const std::string &message);   // Su dcs_log + [ScannerManager]
void log_wv(const std::string &message);   // Su dcs_log + [Wave]
void log(const std::string &message);      // Su dcs_log

void log_db(const std::string &message);

void log_monitor(const std::string &message);
void log_coverage(const std::string &message);
void log_recharge(const std::string &message);
void log_system(const std::string &message);
void log_charge(const std::string &message);

void log_error(const std::string &message, const std::string &process = "");

void close_logs(); // Funzione per chiudere tutti i file di log

#endif // LOG_UTILS_H
