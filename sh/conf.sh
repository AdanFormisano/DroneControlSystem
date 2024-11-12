#!/bin/bash

# Nomi monitor ad indici
declare -A MONITOR_NAMES=(
    [1]="AreaCoverageMonitor"
    [2]="RechargeTimeMonitor"
    [3]="SystemPerformanceMonitor"
    [4]="DroneChargeMonitor"
    [5]="WaveCoverageMonitor"
)

REDIS_PORT=7777

manage_redis() {
    # echo "Vedo se ci sono istanze di Redis in esecuzione..."
    if pgrep redis-server > /dev/null; then
        echo "
Redis è già in esecuzione su queste porte:"
        redis_ports=$(ss -lntp | grep redis-server | awk '{print $4}' | awk -F':' '{print $NF}' | sort -u)
        if [ -n "$redis_ports" ]; then
            echo "$redis_ports"
        else
            echo "Errore: Nessuna porta trovata, ma Redis sembra essere in esecuzione."
        fi
        echo "
Termino tutte le istanze di Redis..."
        echo "-----------------------------------"
        pkill redis-server
        sleep 1.5

        # Verifica se Redis è stato terminato correttamente
        if pgrep redis-server > /dev/null; then
            echo "Errore: Impossibile terminare tutte le istanze di Redis."
            exit 1
        else
            echo "Ogni istanza di Redis è stata chiusa."
        fi
    fi

    echo -e "\n ☑️ Avvio Redis sulla porta $REDIS_PORT..."
    sleep 0.2
    redis-server ./redis.conf --port $REDIS_PORT --daemonize yes &>/dev/null
    sleep 0.2
}