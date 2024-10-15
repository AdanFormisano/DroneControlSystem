#!/bin/bash
echo "-----------------------------------"
echo "
########################
## DroneControlSystem ##
########################
"
echo "Benvenuto in DroneControlSystem! 🤖

Questo programma verifica che ogni punto di una area
6×6 Km sia verificato almeno ogni 5 minuti

Per più info sul progetto, vai qui
→ https://tinyurl.com/ym49mmqw ←
"

echo "-----------------------------------"
echo "
Premi Invio per avviare la simulazione
(o Ctrl+C per terminare il programma)"
read
echo "-----------------------------------"

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
    if [ "$TAIL_RUNNING" = true ]; then
        kill $TAIL_PID
    fi
    if [ "$MONITOR_RUNNING" = true ]; then
        kill $MONITOR_LOG_PID
    fi
    for pid in "${MONITOR_PIDS[@]}"; do
        kill $pid
    done
    wait $DRONE_PID $TAIL_PID $MONITOR_LOG_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    redis-cli -p $REDIS_PORT shutdown

    echo -e "\n✅ Programma terminato\n"
    exit
}

trap cleanup SIGINT SIGTERM

############################################
# Toggle viste log
############################################

toggle_tail() {
    if [ "$TAIL_RUNNING" = true ]; then
        echo "

--------------------
 DroneControlSystem
      nascosto
--------------------

"
        kill $TAIL_PID
        wait $TAIL_PID 2>/dev/null
        TAIL_RUNNING=false
    else
        echo "

------------------
DroneControlSystem
------------------

"
        tail -f dcsa.log &
        TAIL_PID=$!
        TAIL_RUNNING=true
    fi
}

toggle_monitor() {
    if [ "$MONITOR_RUNNING" = true ]; then
        echo "

----------------
 M o n i t o r
    nascosti
----------------

"
        kill $MONITOR_LOG_PID
        wait $MONITOR_LOG_PID 2>/dev/null
        MONITOR_RUNNING=false
    else
        echo "

---------------
 M o n i t o r
---------------

"
        tail -f monitor.log &
        MONITOR_LOG_PID=$!
        MONITOR_RUNNING=true
    fi
}

toggle_individual_monitor() {
    local monitor_index=$1
    local log_file
    local monitor_name=${MONITOR_NAMES[$monitor_index]}

    case $monitor_index in
    1) log_file="coverage.log" ;;
    2) log_file="recharge.log" ;;
    3) log_file="system.log" ;;
    4) log_file="charge.log" ;;
    *) return ;;
    esac

    if [ -z "${MONITOR_PIDS[$monitor_index]}" ]; then
        echo "

---------------
$monitor_name
---------------

"
        tail -f $log_file &
        MONITOR_PIDS[$monitor_index]=$!
    else
        echo "

----------------
$monitor_name
   nascosto
----------------

"
        if ps -p ${MONITOR_PIDS[$monitor_index]} > /dev/null; then
            kill ${MONITOR_PIDS[$monitor_index]}
            wait ${MONITOR_PIDS[$monitor_index]} 2>/dev/null
            unset MONITOR_PIDS[$monitor_index]
        fi
        unset MONITOR_PIDS[$monitor_index]
    fi
}

############################################
# Avvio componenti
############################################

echo " ☑️ Avvio DroneControlSystem..."
cd build
setsid ./DroneControlSystem >dcsa.log 2>&1 &
DRONE_PID=$!
sleep 0.2

# Variabili di stato per i log
TAIL_RUNNING=false
TAIL_PID=0
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

  • [c] per chiudere tutto

  • [d] per mostrare DroneControlSystem
  
  • [m] per mostrare i Monitor

  • Per i singoli Monitor
       - [1] Coverage Area
       - [2] Recharge Time
       - [3] System Performance
       - [4] Drone Charge

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
        toggle_tail
        ;;
    [Mm])
        toggle_monitor
        ;;
    [1-4])
        toggle_individual_monitor $key
        ;;
    [Hh])
        if [ "$TAIL_RUNNING" = true ]; then
            toggle_tail
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
        echo "
        
---------------
Output nascosto
---------------

 Premi:
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [m] per mostrare i Monitor
  • 1 - Coverage Area
  • 2 - Recharge Time
  • 3 - System Performance
  • 4 - Drone Charge

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
