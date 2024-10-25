#include "LogUtils.h"
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

// Flussi di log
std::ofstream dcsa_log("../log/dcsa.log");           // Log tutto
std::ofstream dcs_log("../log/dcs.log");             // Log DCS
std::ofstream dcs_slow_log("../log/dcs_slow.log");   // Log DCS lento
std::ofstream dcsa_slow_log("../log/dcsa_slow.log"); // Log DCS lento
std::ofstream mon_log("../log/monitor.log");         // Log monitor

std::ofstream coverage_log("../log/mon/coverage.log");
std::ofstream recharge_log("../log/mon/recharge.log");
std::ofstream system_log("../log/mon/system.log");
std::ofstream charge_log("../log/mon/charge.log");

// Contatore per il log rallentato
int slow_log_counter = 0;
const int SLOW_LOG_INTERVAL = 100;

std::string generate_dashes(int length) {
    return std::string(length, '-');
}

std::map<int, std::vector<std::string>> log_buffer;

void log_dcsa(const std::string &message) {
    dcsa_log << message << std::endl;
    std::cout << message << std::endl;
}

void log_dcs(const std::string &process, const std::string &message) {
    std::string full_message = "[" + process + "] " + message;
    dcs_log << full_message << std::endl;
    log_dcsa(full_message);
}

// void log_dcs_slow(const std::string &process, const std::string &message) {
//     if (slow_log_counter % SLOW_LOG_INTERVAL == 0) {
//         std::string full_message = "[" + process + "] " + message;
//         dcsa_slow_log << full_message << std::endl;
//         std::cout << full_message << std::endl;
//     }
//     slow_log_counter++;
// }

void flush_log_buffer(int tick, std::ofstream &log_stream) {
    if (log_buffer.find(tick) != log_buffer.end()) {
        std::ostringstream oss;
        for (const auto &msg : log_buffer[tick]) {
            oss << msg << std::endl;
        }
        std::string full_message = oss.str();
        std::string dashes = generate_dashes(30); // Lunghezza fissa per le linee

        // Stampa la linea prima del primo processo
        log_stream << dashes << std::endl;
        // std::cout << dashes << std::endl;

        // Stampa i messaggi dei processi
        log_stream << full_message;
        // std::cout << full_message;

        // Stampa la linea dopo l'ultimo processo
        log_stream << dashes << std::endl;
        // std::cout << dashes << std::endl;

        log_buffer.erase(tick);
    }
}

void log_dcs_slow(const std::string &process, const std::string &message) {
    static int last_synced_tick = 0;

    // Verifica se il messaggio rappresenta un tick
    std::regex tick_regex(R"(^TICK: (\d+)$)");
    std::smatch match;
    if (std::regex_match(message, match, tick_regex)) {
        int tick = std::stoi(match[1].str());
        if (tick % SLOW_LOG_INTERVAL == 0) {
            if (tick != last_synced_tick) {
                flush_log_buffer(last_synced_tick, dcs_slow_log);
                last_synced_tick = tick;
            }

            // Aggiungi il messaggio al buffer solo se non è già presente
            std::ostringstream oss;
            oss << std::left << std::setw(20) << "[" + process + "]"
                << message;

            if (std::find(log_buffer[tick].begin(), log_buffer[tick].end(), oss.str()) == log_buffer[tick].end()) {
                log_buffer[tick].push_back(oss.str());
            }
        }
    } else {
        std::string full_message = "[" + process + "] " + message;
        dcs_slow_log << full_message << std::endl;
    }
}

void log_dcsa_slow(const std::string &process, const std::string &message) {
    static int last_synced_tick = 0;

    // Verifica se il messaggio rappresenta un tick
    std::regex tick_regex(R"(^TICK: (\d+)$)");
    std::smatch match;
    if (std::regex_match(message, match, tick_regex)) {
        int tick = std::stoi(match[1].str());
        std::cout << "Tick recognized: " << tick << std::endl; // Debug

        if (tick % SLOW_LOG_INTERVAL == 0) {
            std::cout << "Tick is multiple of SLOW_LOG_INTERVAL: " << tick << std::endl; // Debug

            if (tick != last_synced_tick) {
                std::cout << "Tick is different from the last_synced_tick: " << tick << std::endl; // Debug
                flush_log_buffer(last_synced_tick, dcsa_slow_log);
                last_synced_tick = tick;
            }

            // Aggiungi il messaggio al buffer solo se non è già presente
            std::ostringstream oss;
            oss << std::left << std::setw(20) << "[" + process + "]"
                << message;

            if (std::find(log_buffer[tick].begin(), log_buffer[tick].end(), oss.str()) == log_buffer[tick].end()) {
                std::cout << "Adding message to buffer: " << oss.str() << std::endl; // Debug
                log_buffer[tick].push_back(oss.str());
            }

            // Scrivi immediatamente il messaggio di "tick" nel file dcsa_slow_log con separatori
            std::string dashes = generate_dashes(30); // Lunghezza fissa per le linee
            dcsa_slow_log << dashes << std::endl;
            dcsa_slow_log << oss.str() << std::endl;
            dcsa_slow_log << dashes << std::endl;
        }
    } else {
        std::string full_message = "[" + process + "] " + message;
        dcsa_slow_log << full_message << std::endl;
    }
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

void log(const std::string &message) {
    dcs_log << message << std::endl;
    std::cout << message << std::endl;
}

void log_cb(const std::string &message) {
    log_dcs("ChargeBase", message);
    log_dcs_slow("ChargeBase", message);
    log_dcsa_slow("ChargeBase", message);
}

void log_charge(const std::string &message) {
    std::string full_message = "[ChargeMonitor] " + message;
    charge_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_coverage(const std::string &message) {
    std::string full_message = "[CoverageMonitor] " + message;
    coverage_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_d(const std::string &message) {
    log_dcs("Drone", message);
    // log_dcs_slow("Drone", message);
    log_dcsa_slow("Drone", message);
}

void log_db(const std::string &message) {
    std::string full_message = "[Database] " + message;
    dcs_log << full_message << std::endl;
    log_dcsa(full_message);
}

void log_dc(const std::string &message) {
    log_dcs("DroneControl", message);
    log_dcs_slow("DroneControl", message);
    log_dcsa_slow("DroneControl", message);
}

void log_ds(const std::string &message) {
    log_dcs("DroneState", message);
    // log_dcs_slow("DroneState", message);
    log_dcsa_slow("DroneState", message);
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
    log_dcsa(full_message);
}

void log_main(const std::string &message) {
    log_dcs("Main", message);
    log_dcs_slow("Main", message);
    log_dcsa_slow("Main", message);
}

void log_monitor(const std::string &message) {
    std::string full_message = "[Monitors] " + message;
    mon_log << full_message << std::endl;
    log_dcsa(full_message);
}

void log_recharge(const std::string &message) {
    std::string full_message = "[RechargeMonitor] " + message;
    recharge_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_sm(const std::string &message) {
    log_dcs("ScannerManager", message);
    log_dcs_slow("ScannerManager", message);
    log_dcsa_slow("ScannerManager", message);
}

void log_system(const std::string &message) {
    std::string full_message = "[SystemMonitor] " + message;
    system_log << full_message << std::endl;
    log_monitor(full_message);
}

void log_tg(const std::string &message) {
    log_dcs("TestGenerator", message);
    log_dcs_slow("TestGenerator", message);
    log_dcsa_slow("TestGenerator", message);
}

void log_wv(const std::string &message) {
    log_dcs("Wave", message);
    // log_dcs_slow("Wave", message);
    log_dcsa_slow("Wave", message);
}