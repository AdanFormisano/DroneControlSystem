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