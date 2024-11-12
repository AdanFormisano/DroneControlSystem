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
       [1] AreaCoverageMonitor
       [2] RechargeTimeMonitor
       [3] SystemPerformanceMonitor
       [4] DroneChargeMonitor
       [5] WaveCoverageMonitor

  -------------------------------
   Puoi premerli anche
   mentre vedi l'output

"

help_message="

Ecco cosa accade alla pressione di ogni tasto:

[a] per mostrare tutto
  Mostra DCSA (DroneControlSystemAll) ossia DCS più monitor e componenti secondari (come Wave, Database, ecc...)

[c] per chiudere tutto
  Chiude DroneControlSystem terminando la simulazione

[d] per mostrare DroneControlSystem
  Mostra i componenti principali di DroneControlSystem: DroneControl, ScannerManager, TestGenerator, ChargeBase

[D] per mostrare DCSA Fast
  Una versione più veloce di DCSA (mostra tutti i tick dei processi di DCSA)

[h] per nascondere tutto
  Nasconde tutto. Utile per pulire lo schermo - nascondendo l'output visualizzato - e rivedere i comandi disponibili

[m] per mostrare i Monitor
  Mostra i log dei monitor tutti assieme
  
  Premendo un numero, si può vedere il log di un monitor specifico
  • [1] AreaCoverageMonitor: Mostra i log dell'area di copertura
  • [2] RechargeTimeMonitor: Mostra i log del tempo di ricarica
  • [3] SystemPerformanceMonitor: Mostra i log delle prestazioni del sistema
  • [4] DroneChargeMonitor: Mostra i log della carica dei droni
  • [5] WaveCoverageMonitor: Mostra i log della copertura delle onde

----------------------

 PREMI

  • [h] per tornare indietro

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