#ifndef LOG_UTILS_H
#define LOG_UTILS_H

#include <fstream>
#include <map>
#include <string>
#include <vector>

// Flussi di output per i log principali
extern std::ofstream dcsa_log;      // Log cumulativo di tutto
extern std::ofstream dcs_log;       // Log principale per le operazioni DCS
extern std::ofstream dcs_slow_log;  // Log lento per DCS (con intervallo)
extern std::ofstream dcsa_slow_log; // Log lento per DCSA (con intervallo)
extern std::ofstream mon_log;       // Log per tutti i monitor

// Log specifici per vari monitor
extern std::ofstream coverage_log;
extern std::ofstream recharge_log;
extern std::ofstream system_log;
extern std::ofstream charge_log;

// Funzioni di logging generale
void log_all(const std::string &message);  // Log cumulativo
void log_dcsa(const std::string &message); // Log su dcsa_log
void log(const std::string &message);      // Log generico su dcs_log

// Funzioni di logging per DCS
void log_dcs(const std::string &process, const std::string &message);      // Log su dcs_log con prefisso processo
void log_dcs_slow(const std::string &process, const std::string &message); // Log lento su dcs_slow_log (con intervallo)

// Funzioni di logging per DCSA con intervallo
void log_dcsa_slow(const std::string &process, const std::string &message);

// Funzioni di logging specifico per processi
void log_main(const std::string &message); // Log per Main
void log_dc(const std::string &message);   // Log per DroneControl
void log_d(const std::string &message);    // Log per Drone
void log_ds(const std::string &message);   // Log per DroneState
void log_cb(const std::string &message);   // Log per ChargeBase
void log_tg(const std::string &message);   // Log per TestGenerator
void log_sm(const std::string &message);   // Log per ScannerManager
void log_wv(const std::string &message);   // Log per Wave
void log_db(const std::string &message);   // Log per Database

// Funzioni di logging specifiche per monitor
void log_monitor(const std::string &message);
void log_coverage(const std::string &message);
void log_recharge(const std::string &message);
void log_system(const std::string &message);
void log_charge(const std::string &message);

// Funzioni di gestione errori e buffer
void log_error(const std::string &process, const std::string &message);
void flush_log_buffer(int tick, std::map<int, std::vector<std::string>> &log_buffer, std::ofstream &log_stream);
void flush_dcs_log();

// Funzione per chiusura dei log
void close_logs();

#endif // LOG_UTILS_H
