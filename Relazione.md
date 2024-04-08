# Drone Control System

## Indice
1. [Drone Control System](#dcs)
2. [Descrizione generale](#gen-descr)
3. [User requirements](#usr-req)
4. [System requirements](#sys-req)
5. [Implementation](#implem)
6. [Risultati Sperimentali](#exp-res)

## [Drone Control System](#dcs)
Drone Control System è un progetto simulante un sistema di sorveglianza basato su droni volanti che monitorano un'area di $6\times6\,\mathrm{Km}$.

Il sistema è sviluppato come progetto d'esame per [Ingegneria del software](https://corsidilaurea.uniroma1.it/it/view-course-details/2023/29923/20190322090929/1c0d2a0e-d989-463c-a09a-00b823557edd/8e637351-4a3a-47a1-ab11-dfe4ad47e446/4f7bd2b2-2f8e-4c38-b15f-7f3c310550b6/8bcc378c-9ff1-4263-87b7-04a394485a9f?guid_cv=8e637351-4a3a-47a1-ab11-dfe4ad47e446&current_erogata=1c0d2a0e-d989-463c-a09a-00b823557edd), corso tenuto dal prof [Enrico Tronci](https://corsidilaurea.uniroma1.it/it/users/enricotronciuniroma1it) a [La Sapienza](https://www.uniroma1.it/), ed è basato sul progetto gentilmente proposto dal prof nel main.pdf [qui](https://drive.google.com/drive/folders/15HrKGosqsuBBe8qWCm1qB_PvIbRLohqZ), al punto *4.2 Controllo formazione droni*.

## [Descrizione generale](#gen-descr)

### Di cosa si occupa Drone Control System
Il sistema progettato è basato, come detto in apertura, su una delle tracce di progetto fornite dal prof Tronci. La traccia è la seguente:
>Si progetti il centro di controllo per una formazione di droni che deve sorvegliare un'area di dati. Ogni drone ha un'autonomia di $30$ minuti di volo ed impiega un tempo di minimo $2h$ e massimo $3h$ per ricaricarsi. Il tempo di ricarica è scelto ad ogni ricarica uniformemente a random nell'intervallo $[2h, 3h]$. Ogni drone si muove alla velocità di $30 Km/h$. L’area da monitorare misura $6\times6$ Km. Il centro di controllo e ricarica si trova al centro dell’area da sorvegliare. Il centro di controllo manda istruzioni ai droni in modo da garantire che per ogni punto dell’area sorvegliata sia verificato almeno ogni $5$ minuti. Un punto è verificato al tempo $t$ se al tempo $t$ c'è almeno un drone a distanza inferiore a $10$ m dal punto. Il progetto deve includere i seguenti componenti:
>1. Un modello (test generator) per i droni
>2. Un modello per il centro di controllo
>3. Un DB per i dati (ad esempio, stato di carica dei droni) ed i log
>4. Monitors per almeno tre proprietà funzionali
>5. Monitors per almeno due proprietà non-funzionali


### Fini del sistema
Il sistema si occupa quindi di verificare che ogni punto dell'area sia sorvegliato ogni cinque minuti, e, in caso contrario, segnala eventuali anomalie. Difatti, e con precisione, se allo scoccare dell'ennesimo intervallo suddetto anche solo un punto dell'area non risulta come [checked](sap/crs/ing/checked), nel log del sistema verrà riportato lo stato di `check-failed` relativo a quel punto e all'annesso timestamp di verifica in cui si è verificato il mancato controllo.

### Schema del sistema
La seguente è una vista ad alto livello delle componenti del sistema

#### Area da sorvegliare
![[Area da sorvegliare]](res/area_view.jpg)

#### Contesto del sistema
![[Contesto del sistema]](res/cntxt_view.png)

## [User requirements](#usr-req)
Questi requisiti riflettono le esigenze e le aspettative degli utenti finali del sistema.

- **(1) Area di Sorveglianza**: L’area da monitorare misura $6×6\,\mathrm{Km}$.
- **(2) Posizione del Centro di Controllo e Ricarica**: Il centro di controllo e ricarica si trova al centro dell’area da sorvegliare.

## [System requirements](#sys-req)
Questi requisiti dettagliano le specifiche tecniche e le funzionalità necessarie per implementare il sistema.

- **(1.1) Sistema di Copertura dell'Area di Sorveglianza**: Il sistema deve programmare e coordinare i percorsi di volo dei droni per garantire una copertura completa e costante dell'area di sorveglianza di 6×6 Km.
- **(1.2) Monitoraggio e Verifica del Territorio**: Il sistema deve assicurare che ogni punto dell'area sia verificato almeno una volta ogni 5 minuti, monitorando la posizione e l'attività di ciascun drone.
- **(1.3) Gestione Autonoma dei Droni**: Il sistema deve gestire autonomamente l'autonomia di volo di ciascun drone, coordinando i tempi di rientro per la ricarica basandosi sul livello di carica della batteria.
- **(2.1) Progettazione e Implementazione del Centro di Controllo**: Il centro di controllo e ricarica deve essere fisicamente situato al centro dell'area da sorvegliare. Il sistema deve essere configurato per utilizzare questa posizione centrale come punto di partenza per la pianificazione delle missioni e per l'ottimizzazione dei percorsi di ritorno per la ricarica.
- **(2.2) Posizionamento e Funzionalità del Centro di Controllo**: Il centro di controllo, situato al centro dell'area di sorveglianza, deve gestire tutte le operazioni dei droni, inclusa la pianificazione delle missioni, il monitoraggio in tempo reale e la gestione delle emergenze.
- **(2.3) Interfaccia di Controllo e Comando**: Il sistema deve fornire un'interfaccia utente intuitiva e funzionale per permettere agli operatori di controllare e monitorare facilmente tutte le operazioni dei droni, e specie eventuali punti che essi non dovessero riuscire a sorvegliare

## [Implementation](#implem)
### Implementazione software
Il sistema è implementato in [C++](https://isocpp.org/), e fa uso di [Redis](https://redis.io/) e di [PostgreSQL](https://www.postgresql.org/).
Redis è disponibile in C++ come client grazie a [redis-plus-plus](https://github.com/sewenew/redis-plus-plus), ed è quello che è stato usato.
Redis è stato usato per gestire i flussi di dati dei thread, compresi quelli dei droni, e per la comunicazione col database PostgreSQL.

### Struttura dell'area sorvegliata
Il sistema gestisce l'area da sorvegliare dividendola in varie colonne, ognuna delle quali è divisa in zone rettangolari impilate virtualmente una sopra l'altra.

In ogni zona figurano $124$ celle. Ogni *cella* è un quadrato di lato $20$ metri, al centro (ossia nel punto a terra in cui le diagonali del quadrato si incrociano) del quale è posizionato un punto (sensore di movimento) che il drone usa, passandovi sopra in volo a distanza sufficientemente ravvicinata, per effettuare la verifica dell'area delimitata dalla cella. 
Più celle vanno a formare una _zona_. Più precisamente due file (una sopra l'altra) di $62$ celle adiacenti creano una zona, che quindi è composta da $124$ celle.

Le zone sono in totale $150$ per colonna, e le colonne sono $5$. Le prime $4$ colonne contando da sinistra sono larghe, giustappunto, $62$ celle ciascuna, mentre l'ultima a destra ha larghezza minore di $52$ celle. Considerando che lo spazio rimanente da coprire era di meno, abbiamo scelto di rendere minore la dimensione di una delle colonne ai lati per semplificarci i calcoli sulle logiche di movimento dei droni, evitando di creare un'area piccola centrale (o altrove posta) che si occupasse di recuperare lo spazio non occupato da eventuali colonne tutte uguali ai suoi lati.

### Droni e verifica dei punti
Come richiesto dalla traccia del progetto, ogni punto dell'area deve essere _verificato_ almeno ogni $5$ minuti, ed un punto è _verificato_ al tempo $t$ se al tempo $t$ c'è almeno un drone a distanza inferiore a $10\,\mathrm{m}$ dal punto.
Per questa ragione abbiamo pensato di dividere l'area, a livello più basso della nostra astrazione, in celle e in zone dopodiché.

Quando un drone ha raggiunto il punto e si trova su di esso in volo verifica la copertura totale dell'area della cella. Per far ciò transita sul punto per il tempo necessario a:
1. effettuare una ripresa (col sensore di immagine con capacità di registrazione video a $360$°) completa dell'intera area delimitata dalla cella
2. scambiare il messaggio di avvenuta verifica del punto ricevendo un input dal sensore posto su quest'ultimo e confermando al centro di controllo, perciò, di averlo effettivamente verificato

Ogni zona è sorvegliata contemporaneamente da $2$ droni, i quali partendo dalle celle "centrali" (da sinistra: la $32\mathrm{esima}$ per il drone nella fila in alto, e la $30\mathrm{esima}$ per il drone nella fila in basso) attraversano tutte le celle che li separano dalla cella di partenza dell'altro drone nella zona, e raggiungono quindi taluna.
In tal modo i due droni assegnati alla zona riescono a coprire, coadiuvando il loro lavoro, tutta la zona. E così fanno il resto dei droni nelle altre zone di ogni colonna.

### Outsourcing
Nell'implementazione del sistema abbiamo dato per scontato l'uso di altre tecnologie e soluzioni di cui esso è inevitabilmente e anche composto, quali quelle del:
- drone
  - sistema di comunicazione a lungo raggio (LTE o 5G): per trasmettere dati e conferme al centro di controllo
  - sistema di ricezione: per ricevere segnali dai sensori a terra
  - sistema di navigazione e posizionamento GPS: per determinare con precisione la posizione del drone
- punto (sensore a terra)
  - sensore di movimento o RFID a lungo raggio: rileva la presenza del drone e invia un segnale di conferma
  - trasmettitore: Invia segnali di conferma al drone per la verifica
- centro di controllo
  - sistema di comunicazione per ricevere dati dai droni: assicura il flusso costante di informazioni dal campo

Sebbene alcune di queste tecnologie e componenti siano parte dell'environment del sistema (come il GPS), ognuna di esse rimane esterna ad esso, ed è naturalmente legata a misure di outsourcing in ogni caso imprescindibili.

Di [Implementation](#implem) manca:
1. Una descrizione con pseudo-codice per tutte le componenti del sistema.
2. Lo schema del (o dei) DB usati.
3. Una descrizione delle connessioni con Redis.

### [Risultati Sperimentali](#exp-res)
Descrivere i risultati ottenuti dalla simulazione del sistema.
