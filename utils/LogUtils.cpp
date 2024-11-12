#include "LogUtils.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <vector>

std::ofstream dcsa_log("../log/dcsa.log");
std::ofstream dcs_log("../log/dcs.log");
std::ofstream dcs_slow_log("../log/dcs_slow.log");
std::ofstream dcsa_slow_log("../log/dcsa_slow.log");
std::ofstream mon_log("../log/monitor.log");

std::ofstream coverage_log("../log/mon/coverage.log");
std::ofstream recharge_log("../log/mon/recharge.log");
std::ofstream system_log("../log/mon/system.log");
std::ofstream charge_log("../log/mon/charge.log");

// std::map<int, std::vector<std::string>> log_buffer;
std::map<int, std::vector<std::string>> dcs_log_buffer;
std::map<int, std::vector<std::string>> dcsa_log_buffer;
std::vector<std::string> dcs_log_messages;

const int SLOW_LOG_INTERVAL = 100;
int slow_log_counter = 0;
static int last_synced_tick = 0; // Synca tick tra log lenti

std::string generate_dashes(int length) {
    return std::string(length, '-');
}

// Funzione base per log su stream
void log_to_stream(std::ofstream &stream, const std::string &message, bool to_console = true) {
    stream << message << std::endl;
    if (to_console)
        std::cout << message << std::endl;
}

// Formattazione uniforme del messaggio
std::string format_log_msg(const std::string &process, const std::string &message) {
    if (process == "Monitors" && message.find("[") == 0) {
        // Se il messaggio inizia con un'altra etichetta tra parentesi quadre
        return "[" + process + "]" + message;
    } else {
        return "[" + process + "] " + message;
    }
}

// Funzione principale di log DCS
void log_dcs(const std::string &process, const std::string &message) {
    std::ostringstream formatted_msg;

    formatted_msg << std::left << std::setw(18) << ("[" + process + "]")
                  << " " << message;

    log_to_stream(dcs_log, formatted_msg.str());
    log_to_stream(dcsa_log, formatted_msg.str());
}

// Funzione per log lento DCS
void log_dcs_slow(const std::string &process, const std::string &message) {
    std::regex tick_regex(R"(^TICK: (\d+)$)");
    std::smatch match;

    if (std::regex_match(message, match, tick_regex)) {
        int tick = std::stoi(match[1].str());

        if (tick % SLOW_LOG_INTERVAL == 0 && tick != last_synced_tick) {
            flush_log_buffer(last_synced_tick, dcs_log_buffer, dcs_slow_log);
            flush_log_buffer(last_synced_tick, dcsa_log_buffer, dcsa_slow_log);
            last_synced_tick = tick;
        }

        std::ostringstream formatted_msg;
        formatted_msg << std::left << std::setw(20) << "[" + process + "]" << " " << message;

        if (std::find(dcs_log_buffer[tick].begin(), dcs_log_buffer[tick].end(), formatted_msg.str()) == dcs_log_buffer[tick].end()) {
            dcs_log_buffer[tick].push_back(formatted_msg.str());
        }
    } else {
        log_to_stream(dcs_slow_log, format_log_msg(process, message));
    }
}

// Funzione per log lento DCSA
void log_dcsa_slow(const std::string &process, const std::string &message) {
    std::regex tick_regex(R"(^TICK: (\d+)$)");
    std::smatch match;

    if (std::regex_match(message, match, tick_regex)) {
        int tick = std::stoi(match[1].str());

        if (tick % SLOW_LOG_INTERVAL == 0 && tick != last_synced_tick) {
            flush_log_buffer(last_synced_tick, dcs_log_buffer, dcs_slow_log);
            flush_log_buffer(last_synced_tick, dcsa_log_buffer, dcsa_slow_log);
            last_synced_tick = tick;
        }

        std::ostringstream formatted_msg;
        formatted_msg << std::left << std::setw(20) << "[" + process + "]" << " " << message;

        if (std::find(dcsa_log_buffer[tick].begin(), dcsa_log_buffer[tick].end(), formatted_msg.str()) == dcsa_log_buffer[tick].end()) {
            dcsa_log_buffer[tick].push_back(formatted_msg.str());
        }
    } else {
        log_to_stream(dcsa_slow_log, format_log_msg(process, message));
    }
}

// Funzione per svuotare il buffer di log per ciascun tick
void flush_log_buffer(int tick, std::map<int, std::vector<std::string>> &log_buffer, std::ofstream &log_stream) {
    if (log_buffer.find(tick) != log_buffer.end()) {
        size_t max_length = 0;

        // Calcola la lunghezza massima dei messaggi del tick corrente
        for (const auto &msg : log_buffer[tick]) {
            if (msg.length() > max_length) {
                max_length = msg.length();
            }
        }

        // Genera i separatori con la lunghezza calcolata
        std::string separator_line(max_length, '=');
        std::string sub_separator(max_length, '-');

        std::ostringstream oss;
        oss << separator_line << "\n"; // Separatore superiore

        for (size_t i = 0; i < log_buffer[tick].size(); ++i) {
            oss << log_buffer[tick][i] << "\n";
            // Sotto-separatore tra i messaggi
            if (i < log_buffer[tick].size() - 1) {
                oss << sub_separator << "\n";
            }
        }

        oss << separator_line << "\n"; // Separatore inferiore

        // Scrive l'intero blocco formattato nel file di log
        log_to_stream(log_stream, oss.str(), false);

        // Svuota il buffer per questo tick
        log_buffer.erase(tick);
    }
}

// Chiusura di tutti i log
void close_logs() {
    dcs_log.close();
    dcsa_log.close();
    dcs_slow_log.close();
    dcsa_slow_log.close();
    mon_log.close();
    coverage_log.close();
    recharge_log.close();
    system_log.close();
    charge_log.close();
}

// Funzione di log per gli errori
void log_error(const std::string &process, const std::string &message) {
    log_to_stream(dcs_log, format_log_msg("ERROR", format_log_msg(process, message)), true);
}

// Funzioni specifiche di log con messaggi preformattati
void log_dcsa(const std::string &message) {
    log_to_stream(dcsa_log, message);
}

// Funzione per log DCSA
void log_dcsa(const std::string &process, const std::string &message) {
    std::ostringstream formatted_msg;

    formatted_msg << std::left << std::setw(18) << ("[" + process + "]")
                  << " " << message;

    log_to_stream(dcsa_log, formatted_msg.str());
}

void log_monitor(const std::string &message) {
    log_to_stream(mon_log, format_log_msg("Monitors", message));
}

void log_charge(const std::string &message) {
    log_to_stream(charge_log, format_log_msg("ChargeMonitor", message));
    log_monitor("[ChargeMonitor] " + message);
}

void log_wave_coverage(const std::string &message) {
    log_to_stream(coverage_log, format_log_msg("WaveCoverageMonitor", message));
    log_monitor("[WaveCoverageMonitor] " + message);
}

void log_area_coverage(const std::string &message) {
    log_to_stream(coverage_log, format_log_msg("AreaCoverageMonitor", message));
    log_monitor("[AreaCoverageMonitor] " + message);
}

void log_recharge(const std::string &message) {
    log_to_stream(recharge_log, format_log_msg("RechargeMonitor", message));
    log_monitor("[RechargeMonitor] " + message);
}

void log_system(const std::string &message) {
    log_to_stream(system_log, format_log_msg("SystemMonitor", message));
    log_monitor("[SystemMonitor] " + message);
}

// Funzioni di log con processi specifici
void log_cb(const std::string &message) {
    log_dcs("ChargeBase", message);
    log_dcs_slow("ChargeBase", message);
    log_dcsa_slow("ChargeBase", message);
}

void log_d(const std::string &message) {
    log_dcs("Drone", message);
    log_dcsa_slow("Drone", message);
}

void log_db(const std::string &message) {
    log_dcsa("Database", message);
    log_dcsa_slow("Database", message);
}

void log_dc(const std::string &message) {
    log_dcs("DroneControl", message);
    log_dcs_slow("DroneControl", message);
    log_dcsa_slow("DroneControl", message);
}

void log_ds(const std::string &message) {
    log_dcsa("DroneState", message);
    log_dcsa_slow("DroneState", message);
}

void log_main(const std::string &message) {
    log_dcs("Main", message);
    log_dcs_slow("Main", message);
    log_dcsa_slow("Main", message);
}

void log_sm(const std::string &message) {
    log_dcs("ScannerManager", message);
    log_dcs_slow("ScannerManager", message);
    log_dcsa_slow("ScannerManager", message);
}

void log_tg(const std::string &message) {
    log_dcs("TestGenerator", message);
    log_dcs_slow("TestGenerator", message);
    log_dcsa_slow("TestGenerator", message);
}

void log_wv(const std::string &message) {
    log_dcsa("Wave", message);
    log_dcsa_slow("Wave", message);
}
