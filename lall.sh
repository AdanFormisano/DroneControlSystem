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

# Redis è in esecuzione sulla porta 7777?
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
sleep 0.2 # non serve, è solo estetico
redis-server ./redis.conf --daemonize yes &>/dev/null
sleep 0.2 # non serve, è solo estetico

cleanup() {
    echo -e "\n\n☑️ Termino i processi..."
    kill -- -$DRONE_PID
    kill $MONITOR_PID
    if [ "$TAIL_RUNNING" = true ]; then
        kill $TAIL_PID
    fi
    if [ "$MONITOR_RUNNING" = true ]; then
        kill $MONITOR_LOG_PID
    fi
    wait $DRONE_PID $MONITOR_PID $TAIL_PID $MONITOR_LOG_PID 2>/dev/null

    echo -e "☑️ Termino Redis..."
    redis-cli -p $REDIS_PORT shutdown

    echo -e "\n✅ Programma terminato"
    exit
}

trap cleanup SIGINT SIGTERM

toggle_tail() {
    if [ "$TAIL_RUNNING" = true ]; then
        echo "

-------------------------------
Hai bloccato la visualizzazione
    di DroneControlSystem
-------------------------------


 Premi:
  • [c] per chiudere il programma
  • [d] per mostrare ancora DroneControlSystem
  • [m] per mostrare i monitor
  • [h] per nascondere l'output

"
        kill $TAIL_PID
        wait $TAIL_PID 2>/dev/null
        TAIL_RUNNING=false
    else
        echo "

------------------
Visualizzazione di
DroneControlSystem
------------------

"
        tail -f drone_output.log &
        TAIL_PID=$!
        TAIL_RUNNING=true
    fi
}

toggle_monitor() {
    if [ "$MONITOR_RUNNING" = true ]; then
        echo "

-------------------------
Hai bloccato la visualiz-
  -zione dei monitor
-------------------------

 Premi:
  • [c] per chiudere il programma
  • [d] per mostrare DroneControlSystem
  • [m] per mostrare ancora i monitor
  • [h] per nascondere l'output

"
        kill $MONITOR_LOG_PID
        wait $MONITOR_LOG_PID 2>/dev/null
        MONITOR_RUNNING=false
    else
        echo "

---------------
Visualizzazione
  dei monitor
---------------

"
        tail -f monitor_output.log &
        MONITOR_LOG_PID=$!
        MONITOR_RUNNING=true
    fi
}

echo " ☑️ Avvio il DroneControlSystem..."
cd build
setsid ./DroneControlSystem >drone_output.log 2>&1 &
DRONE_PID=$!
sleep 0.2

echo " ☑️ Avvio i monitor..."
python3 ../Monitors/Monitor.py >monitor_output.log 2>&1 &
MONITOR_PID=$!
sleep 0.2

TAIL_RUNNING=false
TAIL_PID=0
MONITOR_RUNNING=false
MONITOR_LOG_PID=0

echo "
 ✅ Simulazione avviata
"
echo "-----------------------------------"

echo "
 Premi:
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [m] per mostrare i monitor"
echo "    ----------------------
     Puoi premerli anche
      mentre vedi l'output

"

while true; do
    read -n 1 -r key
    case $key in
    [Dd])
        toggle_tail
        ;;
    [Mm])
        toggle_monitor
        ;;
    [Hh])
        if [ "$TAIL_RUNNING" = true ]; then
            toggle_tail # Ferma tail se sta girando
        fi
        if [ "$MONITOR_RUNNING" = true ]; then
            toggle_monitor # Ferma tail del monitor se sta girando
        fi
        clear
        echo "
        
---------------
Output nascosto
---------------

 Premi:
  • [c] per chiudere tutto
  • [d] per mostrare DroneControlSystem
  • [m] per mostrare i monitor

"
        ;;
    [Cc])
        cleanup
        ;;
    esac
done
