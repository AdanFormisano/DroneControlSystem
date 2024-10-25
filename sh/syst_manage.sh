#!/bin/bash

source sh/strings.sh

cleanup() {
    echo -e "\n☑️ Termino i processi..."
    if [ -n "$DRONE_PID" ]; then
        kill -- -$DRONE_PID
    fi
    if [ "$DCS_RUNNING" = true ] && [ -n "$DCS_PID" ]; then
        kill $DCS_PID
    fi
    if [ "$MONITOR_RUNNING" = true ] && [ -n "$MONITOR_LOG_PID" ]; then
        kill $MONITOR_LOG_PID
    fi
    if [ "$DCSAF_RUNNING" = true ] && [ -n "$DCSAF_PID" ]; then
    kill $DCSAF_PID
    fi
    for pid in "${MONITOR_PIDS[@]}"; do
        if [ -n "$pid" ]; then
            kill $pid
        fi
    done
    wait $DRONE_PID $DCS_PID $MONITOR_LOG_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    if pgrep redis-server > /dev/null; then
        # pkill redis-server
        redis-cli -p $REDIS_PORT shutdown
    fi

    echo -e "\n✅ Programma terminato\n"
    exit
    # pkill -f "tail -f ../log"
    pkill -F "tail -f ../log"
    # pkill -f "less +F ../log"

}

trap cleanup SIGINT SIGTERM

show_help() {
    center_title_with_message "Hai premuto i e visualizzi..." "AIUTO"
    echo "$help_message"
    read -n 1 -r
}