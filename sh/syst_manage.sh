#!/bin/bash

source sh/strings.sh

cleanup() {
    echo -e "\n☑️ Termino i processi..."
    
    # Chiude il processo Drone
    if [ -n "$DRONE_PID" ] && kill -0 $DRONE_PID 2>/dev/null; then
        kill -- -$DRONE_PID
    fi
    
    # Chiude il processo DCS
    if [ "$DCS_RUNNING" = true ] && [ -n "$DCS_PID" ] && kill -0 $DCS_PID 2>/dev/null; then
        kill $DCS_PID
    fi
    
    # Chiude il processo DCSA
    if [ "$DCSA_RUNNING" = true ] && [ -n "$DCSA_PID" ] && kill -0 $DCSA_PID 2>/dev/null; then
        kill $DCSA_PID
    fi

    # Chiude il processo MONITOR
    if [ "$MONITOR_RUNNING" = true ] && [ -n "$MONITOR_LOG_PID" ] && kill -0 $MONITOR_LOG_PID 2>/dev/null; then
        kill $MONITOR_LOG_PID
    fi
    
    # Chiude il processo DCSA Fast
    if [ "$DCSAF_RUNNING" = true ] && [ -n "$DCSAF_PID" ] && kill -0 $DCSAF_PID 2>/dev/null; then
        kill $DCSAF_PID
    fi
    
    # Chiude i monitor specifici
    for pid in "${MONITOR_PIDS[@]}"; do
        if [ -n "$pid" ] && kill -0 $pid 2>/dev/null; then
            kill $pid
        fi
    done

    # Attesa per garantire che i processi siano terminati
    wait $DRONE_PID $DCS_PID $DCSA_PID $MONITOR_LOG_PID $DCSAF_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    if pgrep redis-server > /dev/null; then
        redis-cli -p $REDIS_PORT shutdown
    fi

    echo -e "\n✅ Programma terminato\n"
    exit
    # pkill -f "tail -f ../log"
    pkill -F "tail -F ../log"
    # pkill -f "less +F ../log"

}

cleanup_ipc() {
    echo "Cleaning up IPC resources..."

    # Rimuovi le code di messaggi specifiche
    rm -rf /dev/shm/charge_fault_queue
    rm -rf /dev/shm/drone_fault_queue

    # Rimuovi i semafori nominati specifici
    rm -f /dev/shm/sem.sem_sync_dc
    rm -f /dev/shm/sem.sem_sync_sc
    rm -f /dev/shm/sem.sem_sync_cb
    rm -f /dev/shm/sem.sem_dc
    rm -f /dev/shm/sem.sem_sc
    rm -f /dev/shm/sem.sem_cb

    echo "Cleanup completed: IPC resources removed"
}

trap cleanup SIGINT SIGTERM

# show_help() {
#     center_title_with_message "Hai premuto i e visualizzi..." "AIUTO"
#     echo "$help_message"
#     read -n 1 -r
# }