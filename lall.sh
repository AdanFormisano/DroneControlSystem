#!/bin/bash

echo "
 ########################
 ## DroneControlSystem ##
 ########################
"
echo " Benvenuto in DroneControlSystem! 🤖

  Questo programma verifica che ogni punto di una area
  6×6Km sia verificato almeno ogni 5 minuti.

  Per più info sul progetto, vai qui
   → https://tinyurl.com/ym49mmqw ←
"

echo "-----------------------------------"
echo "
 Premi Invio per avviare la simulazione
 (o Ctrl+C per terminare il programma)"
read
echo "-----------------------------------"

# Redis è in esecuzione sulla porta 7777?
REDIS_PORT=7777
if lsof -i:$REDIS_PORT | grep LISTEN; then
    echo "
Redis è già in esecuzione sulla porta $REDIS_PORT.
Lo chiudo e lo riapro..."
echo "-----------------------------------"
    redis-cli -p $REDIS_PORT shutdown
    sleep 2
fi


echo -e "\n ☑️ Avvio Redis sulla porta $REDIS_PORT..."
# ./ls.sh &
redis-server ./redis.conf --daemonize yes &> /dev/null # &> /dev/null & evita che Redis rompa con un warning inutile


cleanup() {
    echo -e "\n\n☑️ Termino i processi..."
    # echo "Killing Drone PID: $DRONE_PID, Monitor PID: $MONITOR_PID, Tail PID: $TAIL_PID"
    kill -- -$DRONE_PID
    kill $MONITOR_PID
    if [ "$TAIL_RUNNING" = true ]; then
        kill $TAIL_PID
    fi
    wait $DRONE_PID $MONITOR_PID $TAIL_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    redis-cli -p $REDIS_PORT shutdown

    echo -e "\n✅ Programma terminato."
    exit
}

trap cleanup SIGINT SIGTERM

toggle_tail() {
    if [ "$TAIL_RUNNING" = true ]; then
        echo -e "

Hai bloccato la visualizzazione di DroneControlSystem.

Premi l'iniziale per:
 • [v]isualizzare l'output (sbloccandolo)
 • [n]ascondere l'output
 • [c]hiudere il programma
"
        kill $TAIL_PID
        wait $TAIL_PID 2>/dev/null
        TAIL_RUNNING=false
    else
        echo "Visualizzazione di DroneControlSystem."
        tail -f drone_output.log &
        TAIL_PID=$!
        TAIL_RUNNING=true
    fi
}

echo " ☑️ Avvio il DroneControlSystem..."
cd build
setsid ./DroneControlSystem > drone_output.log 2>&1 &
DRONE_PID=$!

echo " ☑️ Avvio i monitor..."
gnome-terminal -- python3 Monitors/Monitor.py
MONITOR_PID=$!

TAIL_RUNNING=false
TAIL_PID=0

# echo -e "\nSimulazione avviata.\n\nComandi:\n • [V]isualizza output\n • [N]ascondi\n • [C]hiudi programma\n"
echo "
 ✅ Simulazione avviata.
"
echo "-----------------------------------"

echo "
Premi l'iniziale per:
 • [v]isualizzare l'output
 • [n]ascondere l'output
 • [c]hiudere il programma
"

while true; do
    read -n 1 -r key
    case $key in
    [Vv])
        toggle_tail
        ;;
    [Nn])
        if [ "$TAIL_RUNNING" = true ]; then
            toggle_tail  # Ferma tail se sta girando
        fi
        clear
        echo "
Output di DroneControlSystem nascosto.

Premi l'iniziale per:
• [v]isualizzare l'output
• [n]ascondere l'output
• [c]hiudere il programma
        "
        ;;
    [Cc])
        cleanup
        ;;
    esac
done
