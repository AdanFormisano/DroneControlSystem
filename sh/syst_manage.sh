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
    # echo "Cleaning up IPC resources..."
    # echo "Controllo che l'area da sorvegliare sia ok..."

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

    # echo "Cleanup completed: IPC resources removed"
    echo "L'area da sorvegliare è ok"
}

get_DB() {
    # Legge i dati dal file JSON
    CONFIG_FILE="src/db_config.json"
    if ! [ -f "$CONFIG_FILE" ]; then
        echo "Errore: file di configurazione '$CONFIG_FILE' non trovato."
        exit 1
    fi

    DB_USER=$(jq -r '.db_user' "$CONFIG_FILE")
    DB_PASSWORD=$(jq -r '.db_password' "$CONFIG_FILE")
    DB_NAME=$(jq -r '.db_name' "$CONFIG_FILE")

    # Verifica che l'utente PostgreSQL esista
    echo "Verifica utente e DB PostgreSQL: permessi necessari..."
    user_exists=$(sudo -H -i -u postgres psql -tAc "SELECT 1 FROM pg_roles WHERE rolname='$DB_USER'")

    # Creazione dell'utente solo se non esiste già
    if [ "$user_exists" != "1" ]; then
        echo "L'utente PostgreSQL '$DB_USER' non esiste. Lo creo..."
        echo "Inserisci la password per l'utente PostgreSQL:"
        read -s DB_PASSWORD
        sudo -H -i -u postgres psql -d postgres -c "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD';"
    else
        echo "L'utente PostgreSQL esiste già"
    fi

    # Funzione per verificare se un database esiste
    db_exists=$(sudo -H -i -u postgres psql -d postgres -tAc "SELECT 1 FROM pg_database WHERE datname='$DB_NAME'")

    # Creazione del database solo se non esiste già
    if [ "$db_exists" == "1" ]; then
        echo "Il DB esiste già"
    else
        echo "Il DB non esiste. Lo creo..."
        sudo -H -i -u postgres psql -d postgres -c "CREATE DATABASE $DB_NAME OWNER $DB_USER;"

        echo "DB creato"
    fi
}

trap cleanup SIGINT SIGTERM

# show_help() {
#     center_title_with_message "Hai premuto i e visualizzi..." "AIUTO"
#     echo "$help_message"
#     read -n 1 -r
# }