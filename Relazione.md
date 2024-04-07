# Drone Control System
***
## Drone Control System
Drone Control System è un progetto simulante un sistema di sorveglianza basato su droni volanti che monitorano un'area di $6\times6\,\mathrm{Km}$.

Il sistema è sviluppato come progetto d'esame per [Ingegneria del software](https://corsidilaurea.uniroma1.it/it/view-course-details/2023/29923/20190322090929/1c0d2a0e-d989-463c-a09a-00b823557edd/8e637351-4a3a-47a1-ab11-dfe4ad47e446/4f7bd2b2-2f8e-4c38-b15f-7f3c310550b6/8bcc378c-9ff1-4263-87b7-04a394485a9f?guid_cv=8e637351-4a3a-47a1-ab11-dfe4ad47e446&current_erogata=1c0d2a0e-d989-463c-a09a-00b823557edd), corso tenuto dal prof [Enrico Tronci](https://corsidilaurea.uniroma1.it/it/users/enricotronciuniroma1it) a [La Sapienza](https://www.uniroma1.it/), ed è basato sul progetto gentilmente proposto dal prof nel main.pdf [qui](https://drive.google.com/drive/folders/15HrKGosqsuBBe8qWCm1qB_PvIbRLohqZ), al punto *4.2 Controllo formazione droni*.
***
## Descrizione generale

### Fini del sistema
Il sistema di occupa di verificare che ogni punto dell'area sia sorvegliato ogni cinque minuti, e, in caso contrario, segnala eventuali anomalie. Difatti, e con precisione, se allo scoccare dell'ennesimo intervallo suddetto anche solo un punto dell'area non risulta come [checked](sap/crs/ing/checked), nel log del sistema verrà riportato lo stato di `check-failed` relativo a quel punto e all'annesso timestamp di verifica in cui si è verificato il mancato controllo.

## User requirements
Questi requisiti riflettono le esigenze e le aspettative degli utenti finali del sistema.

1. **(1) Area di Sorveglianza**: L’area da monitorare misura $6×6\,\mathrm{Km}$.
2. **(2) Posizione del Centro di Controllo e Ricarica**: Il centro di controllo e ricarica si trova al centro dell’area da sorvegliare.

## System requirements
Questi requisiti dettagliano le specifiche tecniche e le funzionalità necessarie per implementare il sistema.

1. **(1.1) Sistema di Copertura dell'Area di Sorveglianza**: Il sistema deve programmare e coordinare i percorsi di volo dei droni per garantire una copertura completa e costante dell'area di sorveglianza di 6×6 Km.
2. **(1.2) Monitoraggio e Verifica del Territorio**: Il sistema deve assicurare che ogni punto dell'area sia verificato almeno una volta ogni 5 minuti, monitorando la posizione e l'attività di ciascun drone.
3. **(1.3) Gestione Autonoma dei Droni**: Il sistema deve gestire autonomamente l'autonomia di volo di ciascun drone, coordinando i tempi di rientro per la ricarica basandosi sul livello di carica della batteria.
4. **(2.1) Progettazione e Implementazione del Centro di Controllo**: Il centro di controllo e ricarica deve essere fisicamente situato al centro dell'area da sorvegliare. Il sistema deve essere configurato per utilizzare questa posizione centrale come punto di partenza per la pianificazione delle missioni e per l'ottimizzazione dei percorsi di ritorno per la ricarica.
5. **(2.2) Posizionamento e Funzionalità del Centro di Controllo**: Il centro di controllo, situato al centro dell'area di sorveglianza, deve gestire tutte le operazioni dei droni, inclusa la pianificazione delle missioni, il monitoraggio in tempo reale e la gestione delle emergenze.
6. **(2.3) Interfaccia di Controllo e Comando**: Il sistema deve fornire un'interfaccia utente intuitiva e funzionale per permettere agli operatori di controllare e monitorare facilmente tutte le operazioni dei droni, e specie eventuali punti che essi non dovessero riuscire a sorvegliare

## Implementation
### Implementazione
Il sistema è implementato in [C++](https://isocpp.org/), e fa uso di [Redis](https://redis.io/) e di [PostgreSQL](https://www.postgresql.org/).
Redis è disponibile in C++ come client grazie a [redis-plus-plus](https://github.com/sewenew/redis-plus-plus), ed è quello che è stato usato.
Redis è stato usato per gestire i flussi di dati dei thread, compresi quelli dei droni, e per la comunicazione col database PostgreSQL.

### Struttura dell'area sorvegliata
Il sistema gestisce l'area da sorvegliare dividendola in varie colonne, ognuna delle quali è divisa in zone rettangolari impilate virtualmente una sopra la base superiore dell'altra.
In ogni zona figurano $124$ celle, ciascuna delle quali rappresenta un'area quadrata di $20\times20$ $\mathrm{m}$ sorvegliata da un drone al suo centro. 

### Droni e verifica dei punti
Come richiesto dalla traccia del progetto, ogni punto dell'area deve essere _verificato_ almeno ogni $5$ minuti, ed un punto è _verificato_ al tempo $t$ se al tempo $t$ c'è almeno un drone a distanza inferiore a $10\,\mathrm{m}$ dal punto.
Per questa ragione abbiamo pensato di dividere l'area, a livello più basso della nostra astrazione, in celle.
Ogni *cella* è un quadrato di lato $20$ metri, al centro (ossia nel punto in cui le diagonali del quadrato si incrociano) del quale è posizionato un drone. Il drone, in tale posizione riuscendo a coprire con un sensore a $360°$ l'intera area della cella, verifica la copertura totale di quest'ultima quando passa sul punto e verifica esso.
Più celle vanno a formare una _zona_. Più precisamente due file (una sopra l'altra) di $62$ celle adiacenti creano una zona, che quindi è composta da $124$ celle.
Ogni zona è sorvegliata contemporaneamente da $2$ droni, i quali partendo dalle celle "centrali" (da sinistra: la $32\mathrm{esima}$ per il drone nella fila in alto, e la $30\mathrm{esima}$ per il drone nella fila in basso) attraversano tutte le celle che li separano dalla cella di partenza dell'altro drone nella zona, e raggiungono quindi taluna.
In tal modo i due droni assegnati alla zona riescono a coprire, coadiuvando il loro lavoro, tutta la zona. E così fanno il resto dei droni nelle altre zone di ogni colonna. Le zone sono in totale $150$ per colonna, e le colonne sono $5$. Le prime $4$ colonne contando da sinistra sono larghe, giustappunto, $62$ celle ciascuna, mentre l'ultima a destra ha dimensione minore. Considerando che lo spazio rimanente da coprire era di meno, abbiamo scelto di rendere minore la dimensione di una delle colonne ai lati per semplificarci i calcoli sulle logiche di movimento dei droni, evitando di creare un'area piccola centrale (o altrove posta) che si occupasse di recuperare lo spazio non occupato da eventuali colonne tutte uguali ai suoi lati.

Di [[#Implementation]] manca:
1. Una descrizione con pseudo-codice per tutte le componenti del sistema.
2. Lo schema del (o dei) DB usati.
3. Una descrizione delle connessioni con Redis.

### Risultati Sperimentali
Descrivere i risultati ottenuti dalla simulazione del sistema.