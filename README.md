# DroneControlSystem
Drone Control System (in seguito DCS) è un software sviluppato per il corso di Informatica Ingegneria del software.

## Traccia progetto
La traccia del progetto è la seguente:

Si progetti il centro di controllo per una formazione di droni che deve sorvegliare un'area di dati. Ogni drone ha un'autonomia di 30 minuti di volo ed impiega un tempo di minimo 2h e massimo 3h per ricaricarsi. Il tempo di ricarica è scelto ad ogni ricarica uniformemente a random nell'intervallo [2h, 3h]. Ogni drone si muove alla velocità di 30 Km/h. L'area da monitorare misura 6 × 6 Km. Il centro di controllo e ricarica si trova al centro dell'area da sorvegliare. Il centro di controllo manda istruzioni ai droni in modo da garantire che per ogni punto dell'area sorvegliata sia verificato almeno ogni 5 minuti. Un punto è verificato al tempo t se al tempo t c'è almeno un drone a distanza inferiore a 10m dal punto. Il progetto deve includere i seguenti componenti:

1. Un modello (test generator) per i droni
2. Un modello per il centro di controllo
3. Un DB per i dati (ad esempio, stato di carica dei droni) ed i log
4. Monitors per almeno tre proprietà funzionali
5. Monitors per almeno due proprietà non funzionali

## Dipendenze e software usato
- C++.
- Redis
- PostgreSQL
- spdlog (https://github.com/gabime/spdlog)

