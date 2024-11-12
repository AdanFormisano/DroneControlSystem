#!/bin/bash

constants(){
    DCS_RUNNING=false
    DCS_PID=0
    DCSA_RUNNING=false
    DCSA_PID=0
    DCSAF_RUNNING=false
    DCSAF_PID=0
    MONITOR_RUNNING=false
    MONITOR_LOG_PID=0
    declare -A MONITOR_PIDS
    MONITOR2_RUNNING=false
    MONITOR2_LOG_PID=0
}

# Funzione per centrare il testo
center_text() {
    local term_width=$(tput cols)
    local text="$1"
    local text_width=${#text}
    local padding=$(( (term_width - text_width) / 2 ))
    printf "%*s%s\n" $padding "" "$text"
}

center_title_with_message() {
    local term_width=$(tput cols)
    local message="$1"
    local text="$2"
    local message_width=${#message}
    local text_width=${#text}
    local message_padding=$(( (term_width - message_width) / 2 ))
    local text_padding=$(( (term_width - text_width) / 2 ))
    
    printf "\n%*s%s\n" $message_padding "" "$message"
    printf '%*s\n' "$term_width" '' | tr ' ' '-'
    printf "%*s%s\n" $text_padding "" "$text"
    printf '%*s\n' "$term_width" '' | tr ' ' '-'
}

generate_dashes() {
    local name=$1
    local name_length=${#name}
    printf '%*s' "$name_length" '' | tr ' ' '-'
}

mkdir_log() {
    mkdir -p log
    mkdir -p log/mon
}

is_process_running() {
    local pid=$1
    if [ -n "$pid" ] && kill -0 $pid 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

start_dcs() {
    cd build
    setsid ./DroneControlSystem >../log/dcs.log 2>&1 &
    DRONE_PID=$!
    sleep 0.2
}

build_dcs() {
    mkdir -p build && cd build
    echo -e "\n ☑️ Building DCS..."
    cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --config Release
    cd ..
    echo -e "\n ☑️ DCS built successfully\n"
}

hide_all_views() {
    if [ "$DCS_RUNNING" = true ]; then
        kill $DCS_PID
        wait $DCS_PID 2>/dev/null || true
        DCS_RUNNING=false
    fi
    if [ "$DCSA_RUNNING" = true ]; then
        kill $DCSA_PID
        wait $DCSA_PID 2>/dev/null || true
        DCSA_RUNNING=false
    fi
    if [ "$MONITOR_RUNNING" = true ]; then
        kill $MONITOR_LOG_PID
        wait $MONITOR_LOG_PID 2>/dev/null || true
        MONITOR_RUNNING=false
    fi
    if [ "$DCSAF_RUNNING" = true ]; then
        kill $DCSAF_PID
        wait $DCSAF_PID 2>/dev/null || true
        DCSAF_RUNNING=false
    fi
    for index in "${!MONITOR_PIDS[@]}"; do
        pid=${MONITOR_PIDS[$index]}
        if ps -p $pid > /dev/null; then
            kill $pid
            wait $pid 2>/dev/null || true
            if ps -p $pid > /dev/null; then
                kill -9 $pid # Chiusura forzata se necessario
            fi
        fi
        unset MONITOR_PIDS[$index]
    done
    MONITOR_RUNNING=false
    if [ "$MONITOR2_RUNNING" = true ]; then
        kill $MONITOR2_LOG_PID
        wait $MONITOR2_LOG_PID 2>/dev/null || true
        MONITOR2_RUNNING=false
    fi
}