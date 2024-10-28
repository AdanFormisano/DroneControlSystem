#!/bin/bash

# Termina tail esistenti
# pkill -f "tail -f ../log"
# if pgrep -F "tail -F ../log" > /dev/null; then
#     pkill -F "tail -F ../log"
# fi
# pkill -f "less +F ../log"

# Altri sh che servono
source sh/utils.sh
source sh/log_manage.sh
source sh/syst_manage.sh
source sh/conf.sh
source sh/strings.sh

# Prompt DCS iniziale
init_msg

# Avvia Redis
manage_redis

# Crea cartelle log
mkdir_log

# Avvia DCS
dcs_start_msg
start_dcs

# Stat-vars monitor
constants

# Sim avviata
sim_start_msg

# Premi per...
instr_msg

# Ciclo principale input utente
while true; do
    read -n 1 -r key
    case $key in
    [d])
        toggle_dcs
        ;;
    [D])
        toggle_dcsa_fast
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
        toggle_hide
        ;;
    [Cc])
        echo "$closing_message"
        cleanup
        ;;
    esac
done