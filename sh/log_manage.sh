#!/bin/bash

toggle_dcs() {
    local name="DroneControlSystem"
    local dashes=$(generate_dashes "$name")

    if [ "$DCS_RUNNING" = true ]; then
        center_title_with_message "Hai premuto \"d\" e hai reso" "$name nascosto"
        kill $DCS_PID
        wait $DCS_PID 2>/dev/null
        DCS_RUNNING=false
    else
        center_title_with_message "Hai premuto \"d\" e visualizzi" "$name"
        # tail -f ../log/dcs_slow.log &
        tail -F ../log/dcs_slow.log &
        # less +F ../log/dcs_slow.log &
        DCS_PID=$!
        DCS_RUNNING=true
    fi
}

toggle_dcsa() {
    local name="Vista generale"
    local dashes=$(generate_dashes "$name")

    if [ "$DCSA_RUNNING" = true ]; then
        center_title_with_message "Hai premuto \"a\" e hai reso" "$name nascosta"
        
        # Controllo e chiusura PID
        if ps -p $DCSA_PID > /dev/null; then
            kill $DCSA_PID
            wait $DCSA_PID 2>/dev/null || true
            if ps -p $DCSA_PID > /dev/null; then
                kill -9 $DCSA_PID # Chiusura forzata se necessario
            fi
        fi
        
        DCSA_RUNNING=false
        DCSA_PID=0
    else
        center_title_with_message "Hai premuto \"a\" e visualizzi" "$name"

        # Usa `tail -f` anziché `tail -F`
        tail -f ../log/dcsa_slow.log &
        DCSA_PID=$!
        DCSA_RUNNING=true
    fi
}

toggle_dcsa_fast() {
    local name="DCSA Fast"
    local dashes=$(generate_dashes "$name")

    if [ "$DCSAF_RUNNING" = true ]; then
        center_title_with_message "Hai premuto \"D\" e hai reso" "$name nascosta"
        
        # Controllo e chiusura PID
        if ps -p $DCSAF_PID > /dev/null; then
            kill $DCSAF_PID
            wait $DCSAF_PID 2>/dev/null || true
            if ps -p $DCSAF_PID > /dev/null; then
                kill -9 $DCSAF_PID # Chiusura forzata se necessario
            fi
        fi
        
        DCSAF_RUNNING=false
        DCSAF_PID=0
    else
        center_title_with_message "Hai premuto \"D\" e visualizzi" "$name"

        # Usa `tail -f` anziché `tail -F`
        tail -f ../log/dcsa.log &
        DCSAF_PID=$!
        DCSAF_RUNNING=true
    fi
}

toggle_monitor() {
    local name="Monitor"
    local dashes=$(generate_dashes "$name")

    if [ "$MONITOR_RUNNING" = true ]; then
        center_title_with_message "Hai premuto \"m\" e hai reso" "$name nascosti"
        kill $MONITOR_LOG_PID
        wait $MONITOR_LOG_PID 2>/dev/null
        MONITOR_RUNNING=false
    else
        center_title_with_message "Hai premuto \"m\" e visualizzi" "$name"
        # tail -f ../log/monitor.log &
        tail -F ../log/monitor.log &
        # less +F ../log/monitor.log &
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
        center_title_with_message "Hai premuto \"$monitor_index\" e visualizzi" "$monitor_name"
        # tail -f $log_file &
        tail -F $log_file &
        # less +F $log_file &
        MONITOR_PIDS[$monitor_index]=$!
    else
        center_title_with_message "Hai premuto \"$monitor_index\" e hai reso" "$monitor_name nascosto"
        if ps -p ${MONITOR_PIDS[$monitor_index]} > /dev/null; then
            kill ${MONITOR_PIDS[$monitor_index]}
            wait ${MONITOR_PIDS[$monitor_index]} 2>/dev/null
            unset MONITOR_PIDS[$monitor_index]
        fi
        unset MONITOR_PIDS[$monitor_index]
    fi
}

toggle_hide() {
    if [ "$DCS_RUNNING" = true ]; then
        toggle_dcs
    fi
    if [ "$DCSA_RUNNING" = true ]; then
        toggle_dcsa
    fi
    if [ "$MONITOR_RUNNING" = true ]; then
        toggle_monitor
    fi
    if [ "$DCSAF_RUNNING" = true ]; then
        toggle_dcsa_fast
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
    # sleep 0.2 # Evita sovrapposizioni processi
    clear
    center_title_with_message "Hai premuto \"h\" e hai reso" "Output nascosto"
    echo "$instructions"
}
