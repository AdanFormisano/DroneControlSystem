#!/bin/bash

# Funzione per centrare il testo
center_text() {
    local term_width=$(tput cols)
    local text="$1"
    local text_width=${#text}
    local padding=$(( (term_width - text_width) / 2 ))
    printf "%*s%s\n" $padding "" "$text"
}

# center_title() {
#     local term_width=$(tput cols)
#     local text="$1"
#     local text_width=${#text}
#     local dashes=$(( (term_width - text_width - 2) / 2 ))
#     printf '%*s' "$dashes" '' | tr ' ' '-'
#     printf " %s " "$text"
#     printf '%*s\n' "$dashes" '' | tr ' ' '-'
# }

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


# Testo da centrare
header="
########################
## DroneControlSystem ##
########################
"
welcome="🤖 Benvenuto in DroneControlSystem! 🤖

Questo programma verifica che ogni punto di una
area 6×6 Km sia verificato almeno ogni 5 minuti

Per più info sul progetto, vai qui
→ https://tinyurl.com/ym49mmqw ←
"
prompt="Premi Invio per avviare la simulazione
(o Ctrl+C per terminare il programma)"
separator="--------------------------"

# Stampa il testo centrato
# center_text "$separator"
while IFS= read -r line; do
    center_text "$line"
done <<< "$header"
# center_text "$separator"
while IFS= read -r line; do
    center_text "$line"
done <<< "$welcome"
center_text "$separator"
while IFS= read -r line; do
    center_text "$line"
done <<< "$prompt"
read
center_text "$separator"

############################################
# Nomi monitor ad indici
############################################

declare -A MONITOR_NAMES=(
    [1]="Coverage Area"
    [2]="Recharge Time"
    [3]="System Performance"
    [4]="Drone Charge"
)

############################################
# Redis
############################################

REDIS_PORT=7777
if lsof -i:$REDIS_PORT | grep LISTEN; then
    echo "
Redis è già in esecuzione sulla porta $REDIS_PORT.
Lo chiudo e lo riapro..."
    echo "-----------------------------------"
    redis-cli -p $REDIS_PORT shutdown
    sleep 1.5
fi

echo -e "\n ☑️ Avvio Redis sulla porta $REDIS_PORT..."
sleep 0.2
redis-server ./redis.conf --daemonize yes &>/dev/null
sleep 0.2

############################################
# Cleanup
############################################

cleanup() {
    echo -e "\n☑️ Termino i processi..."
    kill -- -$DRONE_PID
    if [ "$DCS_RUNNING" = true ]; then
        kill $DCS_PID
    fi
    if [ "$MONITOR_RUNNING" = true ]; then
        kill $MONITOR_LOG_PID
    fi
    for pid in "${MONITOR_PIDS[@]}"; do
        kill $pid
    done
    wait $DRONE_PID $DCS_PID $MONITOR_LOG_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    redis-cli -p $REDIS_PORT shutdown

    echo -e "\n✅ Programma terminato\n"
    exit
}

trap cleanup SIGINT SIGTERM

############################################
# Toggle viste log
############################################

generate_dashes() {
    local name=$1
    local name_length=${#name}
    printf '%*s' "$name_length" '' | tr ' ' '-'
}

toggle_dcs() {
    local name="DroneControlSystem"
    local dashes=$(generate_dashes "$name")

    if [ "$DCS_RUNNING" = true ]; then
        center_title_with_message "Hai premuto d e visualizzi..." "$name nascosto"
        kill $DCS_PID
        wait $DCS_PID 2>/dev/null
        DCS_RUNNING=false
    else
        center_title_with_message "Hai premuto d e visualizzi..." "$name"
        tail -f ../log/dcs_slow.log &
        DCS_PID=$!
        DCS_RUNNING=true
    fi
}

toggle_dcsa() {
    local name="Vista generale"
    local dashes=$(generate_dashes "$name")

    if [ "$DCSA_RUNNING" = true ]; then
        center_title_with_message "Hai premuto a e visualizzi..." "$name nascosta"
        kill $DCSA_PID
        wait $DCSA_PID 2>/dev/null
        DCSA_RUNNING=false
    else
        center_title_with_message "Hai premuto a e visualizzi..." "$name"
        tail -f ../log/dcsa_slow.log &
        DCSA_PID=$!
        DCSA_RUNNING=true
    fi
}

DCSA_RUNNING=false
DCSA_PID=0

toggle_monitor() {
    local name="Monitor"
    local dashes=$(generate_dashes "$name")

    if [ "$MONITOR_RUNNING" = true ]; then
        center_title_with_message "Hai premuto m e visualizzi..." "$name nascosti"
        kill $MONITOR_LOG_PID
        wait $MONITOR_LOG_PID 2>/dev/null
        MONITOR_RUNNING=false
    else
        center_title_with_message "Hai premuto m e visualizzi..." "$name"
        tail -f ../log/monitor.log &
        MONITOR_LOG_PID=$!
        MONITOR_RUNNING=true
    fi
}

toggle_single_mon() {
    local monitor_index=$1
    local log_file
    local monitor_name=${MONITOR_NAMES[$monitor_index]}

    local dashes=$(generate_dashes "$monitor_name")

    case $monitor_index in
    1) log_file="../log/mon/coverage.log" ;;
    2) log_file="../log/mon/recharge.log" ;;
    3) log_file="../log/mon/system.log" ;;
    4) log_file="../log/mon/charge.log" ;;
    *) return ;;
    esac

    if [ -z "${MONITOR_PIDS[$monitor_index]}" ]; then
        center_title_with_message "Hai premuto $monitor_index e visualizzi..." "$monitor_name"
        tail -f $log_file &
        MONITOR_PIDS[$monitor_index]=$!
    else
        center_title_with_message "Hai premuto $monitor_index e visualizzi..." "$monitor_name nascosto"
        if ps -p ${MONITOR_PIDS[$monitor_index]} > /dev/null; then
            kill ${MONITOR_PIDS[$monitor_index]}
            wait ${MONITOR_PIDS[$monitor_index]} 2>/dev/null
            unset MONITOR_PIDS[$monitor_index]
        fi
        unset MONITOR_PIDS[$monitor_index]
    fi
}

show_help() {
    center_title_with_message "Hai premuto i e visualizzi..." "AIUTO"
    echo "

DroneControlSystem:
  - Mostra i log del sistema di controllo dei droni.

Vista generale:
  - Mostra una vista generale del sistema.

Monitor:
  - Mostra i log dei monitor specifici.

  • [1] Coverage Area: Mostra i log dell'area di copertura.
  • [2] Recharge Time: Mostra i log del tempo di ricarica.
  • [3] System Performance: Mostra i log delle prestazioni del sistema.
  • [4] Drone Charge: Mostra i log della carica dei droni.

----------------------

 PREMI

  • [a] per mostrare tutto
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [h] per nascondere tutto
  • [m] per mostrare i Monitor
       [1] Coverage Area
       [2] Recharge Time
       [3] System Performance
       [4] Drone Charge

  -------------------------------
   Puoi premerli anche
   mentre vedi l'output
"
    read -n 1 -r
}

############################################
# Avvio componenti
############################################

echo " ☑️ Avvio DroneControlSystem..."
cd build
setsid ./DroneControlSystem >../log/dcs.log 2>&1 &
DRONE_PID=$!
sleep 0.2

# Variabili di stato per i log
DCS_RUNNING=false
DCS_PID=0
MONITOR_RUNNING=false
MONITOR_LOG_PID=0
declare -A MONITOR_PIDS

echo "
 ✅ Simulazione avviata
"
echo "-----------------------------------"

############################################
# Istruzioni per l'utente
############################################
echo "
 PREMI
 
  • [a] per mostrare tutto
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [h] per nascondere tutto
  • [i] per info su ciascuna vista
  • [m] per mostrare i Monitor
       [1] Coverage Area
       [2] Recharge Time
       [3] System Performance
       [4] Drone Charge

  -------------------------------
   Puoi premerli anche
   mentre vedi l'output

"

############################################
# Ciclo principale di input dell'utente
############################################
while true; do
    read -n 1 -r key
    case $key in
    [Dd])
        toggle_dcs
        ;;
    [Aa])
        toggle_dcsa
        ;;
    [Mm])
        toggle_monitor
        ;;
    [1-4])
        toggle_single_mon $key
        ;;
    [Ii])
        show_help
        ;;
    [Hh])
        if [ "$DCS_RUNNING" = true ]; then
            toggle_dcs
        fi
        if [ "$DCSA_RUNNING" = true ]; then
            toggle_dcsa
        fi
        if [ "$MONITOR_RUNNING" = true ]; then
            toggle_monitor
        fi
        for index in "${!MONITOR_PIDS[@]}"; do
            pid=${MONITOR_PIDS[$index]}
            if ps -p $pid > /dev/null; then
                kill $pid
                wait $pid 2>/dev/null
            fi
            unset MONITOR_PIDS[$index]
        done
        MONITOR_RUNNING=false
        clear
        center_title_with_message "Hai premuto h e visualizzi..." "Output nascosto"
        echo "
 PREMI

  • [a] per mostrare tutto
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [i] per info su ciascuna vista
  • [m] per mostrare i Monitor
       [1] Coverage Area
       [2] Recharge Time
       [3] System Performance
       [4] Drone Charge

  -------------------------------
   Puoi premerli anche
   mentre vedi l'output


"
        ;;
    [Cc])
        echo "

----------------------
  Hai premuto \"c\"
 Chiusura in corso...
----------------------
"    
        cleanup
        ;;
    esac
done