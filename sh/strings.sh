#!/bin/bash

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

prompt="
Premi Invio per avviare la simulazione
(o Ctrl+C per terminare il programma)"

sep1="--------------------------"
sep2="-----------------------------------"

instructions="
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

help_message="
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

closing_message="
----------------------
  Hai premuto \"c\"
 Chiusura in corso...
----------------------
"

boot_dcs_msg=" ☑️ Avvio DroneControlSystem..."

init_msg() {
    while IFS= read -r line; do
        center_text "$line"
    done <<< "$header"
    while IFS= read -r line; do
        center_text "$line"
    done <<< "$welcome"
    center_text "$sep1"
    while IFS= read -r line; do
        center_text "$line"
    done <<< "$prompt"
    read
    center_text "$sep1"
}

dcs_start_msg() {
  echo "$boot_dcs_msg"
}

sim_start_msg(){
  echo "
 ✅ Simulazione avviata
"
  echo "$sep2"
}

instr_msg() {
  echo "$instructions"
}