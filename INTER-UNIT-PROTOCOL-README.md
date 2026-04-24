# Inter-Unit Master/Slave CAN Protocol

Documentation for the Battery Emulator's internal Master/Slave communication protocol.

Relevant source files:
- `Software/src/communication/can/INTER-UNIT-PROTOCOL.h` — CAN ID macros and payload definitions
- `Software/src/communication/can/MASTER-CAN.cpp` / `MASTER-CAN.h` — Master-side logic
- `Software/src/communication/can/SLAVE-CAN.cpp` / `SLAVE-CAN.h` — Slave-side logic

---

## Overview

A Battery Emulator can be configured as **Master** or **Slave** via the settings page.

| Role | Function |
|------|----------|
| **Master** | Communicates with the inverter. Aggregates data from all slaves. Controls contactors. |
| **Slave** | Communicates with a battery. Sends battery data to the master. Opens/closes contactor on master command. |

Up to **24 slave nodes** are supported (Node ID 1–24).

CAN bus: MCP2515 addon bus (configured via `IUCOMM` setting), **500 kbps**.

---

## CAN ID Allocation

| CAN ID | Direction | Description |
|--------|-----------|-------------|
| `0x300` | Master → Broadcast | Heartbeat (no payload) |
| `0x301`–`0x318` | Master → Slave N | Contactor command to Slave N |
| `0x110 + N×0x10` | Slave N → Master | STATUS message (msg 0x00) |
| `0x111 + N×0x10` | Slave N → Master | POWER message (msg 0x01) |
| `0x112 + N×0x10` | Slave N → Master | INFO message (msg 0x02) |
| `0x113 + N×0x10` | Slave N → Master | IP address message (msg 0x03) |
| `0x114 + N×0x10` | Slave N → Master | CELL message (msg 0x04) |
| `0x115 + N×0x10` | Slave N → Master | IDENT message (msg 0x05, startup only) |

Example for Slave 1 (N=1): STATUS = `0x110`, POWER = `0x111`, etc.

---

## Timing

| Message | Sent by | Interval |
|---------|---------|----------|
| Heartbeat | Master | Every 1 second |
| STATUS | Slave | Every heartbeat (1s) |
| POWER | Slave | Every heartbeat (1s) |
| CELL | Slave | Every 2nd heartbeat (2s) |
| INFO | Slave | Every 10th heartbeat (10s) |
| IP | Slave | First 3 heartbeats + every 10 minutes |
| IDENT | Slave | First 3 heartbeats + every 10 minutes |

**Collision avoidance:** Slaves reply with a delay of `NodeID × 5 ms` after heartbeat.  
Slave 1 = 5 ms, Slave 2 = 10 ms, ..., Slave 24 = 120 ms.

---

## Message Layouts

### Heartbeat — `0x300` (0 bytes)
No payload. Used only to signal that the master is online.

---

### Contactor Command — `0x300 + N` (1 byte)

| Byte | Content |
|------|---------|
| [0] | `0x01` = allow contactor closing, `0x00` = open contactor |

---

### STATUS — `0x110 + N×0x10` (8 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–1] | uint16 | Pack voltage in deciVolts (3700 = 370.0 V) |
| [2–3] | uint16 | SOC in 0.01% units (9550 = 95.50%) |
| [4–5] | int16 | Current in deciAmpere (positive = charging) |
| [6] | int8 | Max temperature in °C |
| [7] | uint8 | Flags — see table below |

#### FLAGS byte (data[7])

| Bit | Constant | Meaning | Classification |
|-----|----------|---------|----------------|
| 0 | `IU_FAULT_BMS_FAULT` | BMS reports a fault | **Error (red)** |
| 1 | `IU_FAULT_CELL_OVERVOLTAGE` | Cell over-voltage | Warning (yellow) |
| 2 | `IU_FAULT_CELL_UNDERVOLTAGE` | Cell under-voltage | Warning (yellow) |
| 3 | `IU_FAULT_OVERTEMPERATURE` | Over-temperature (>55°C) | Warning (yellow) |
| 4 | `IU_FAULT_CONTACTOR_FAILED` | Contactor failed to close | **Error (red)** |
| 5 | `IU_FAULT_BATTERY_TIMEOUT` | Battery CAN timeout on slave | **Error (red)** |
| 6 | `IU_FLAG_CONTACTOR_ENGAGED` | Contactor is physically closed (not a fault) | Info |
| 7 | `IU_FLAG_STATUS_TOGGLE` | Toggle bit — flips every frame (stale detection) | Internal |

---

### POWER — `0x111 + N×0x10` (8 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–1] | uint16 | Max charge power in Watts |
| [2–3] | uint16 | Max discharge power in Watts |
| [4–5] | uint16 | Remaining capacity in Wh |
| [6] | int8 | Min temperature in °C |
| [7] | — | Reserved |

---

### INFO — `0x112 + N×0x10` (8 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–1] | uint16 | Total pack capacity in Wh |
| [2–3] | uint16 | Max design voltage in dV |
| [4–5] | uint16 | Min design voltage in dV |
| [6–7] | uint16 | State of Health in 0.01% units (9900 = 99.00%) |

---

### IP — `0x113 + N×0x10` (4 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–3] | uint32 | IPv4 address, big-endian (192.168.1.10 = 0xC0A8010A) |

Only sent if the slave is connected to WiFi.

---

### CELL — `0x114 + N×0x10` (8 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–1] | uint16 | Highest cell voltage in mV |
| [2–3] | uint16 | Lowest cell voltage in mV |
| [4–7] | — | Reserved |

---

### IDENT — `0x115 + N×0x10` (8 bytes)

| Bytes | Type | Content |
|-------|------|---------|
| [0–1] | uint16 | Firmware version: `(major << 8) | minor` — e.g. 10.6 = `0x0A06` |
| [2–3] | uint16 | Battery type ID (`BatteryType` enum cast to uint16) |
| [4–7] | — | Reserved |

---

## Safety Features

### 1. Startup Grace Period
**Duration:** 20 seconds (`IU_STARTUP_GRACE_S`)

At startup the master holds all contactors **open** for 20 seconds regardless of slave state.  
This gives all slaves time to announce their voltage before the inverter begins charging or discharging, preventing inrush current from large voltage differences between packs.

---

### 2. Slave Offline Detection
**Timeout:** 60 seconds (`IU_OFFLINE_TIMEOUT_S`)

The master maintains a countdown from 60 for each slave, reset on every received message.  
When the counter reaches 0:
- Slave is marked **offline**
- Contactor is **blocked**
- Event `EVENT_SLAVE_BATTERY_MISSING` (WARNING) is raised

---

### 3. Stale Data Detection (Toggle Bit)
**Timeout:** 3 seconds (`IU_STATUS_STALE_SECONDS`)

The STATUS message contains a toggle bit (bit 7) that flips on **every** frame.  
The master monitors whether bit 7 is changing. If it has not changed for 3 seconds the data is considered frozen (e.g. slave software hung):
- Contactor is **blocked**
- Event `EVENT_STALE_VALUE` (ERROR) is raised
- Automatically cleared when the toggle bit starts changing again

---

### 4. Firmware and Battery Type Mismatch (IDENT)

At startup the slave sends an IDENT message containing its firmware version and battery type ID.  
The master verifies:
- **Firmware version** matches the master's compiled version (`IU_FW_VERSION_NUM`)
- **Battery type** matches the first slave that reported one (all slaves must use the same battery type)

On mismatch:
- Contactor is **blocked**
- Event `EVENT_SLAVE_IDENT_MISMATCH` (ERROR) is raised
- Automatically cleared if subsequent IDENT data matches

> **Note:** `IU_FW_VERSION_MAJOR` and `IU_FW_VERSION_MINOR` in `INTER-UNIT-PROTOCOL.h` must be updated manually whenever the firmware version changes in `Software.cpp`.

---

### 5. Fault Flag → Event Mapping

Fault bits in the STATUS flags byte are automatically translated to events on the master:

| Fault | Classification | Consequence | Event |
|-------|---------------|-------------|-------|
| BMS fault | **ERROR** | Contactor blocked | `EVENT_SLAVE_FAULT` |
| Battery CAN timeout | **ERROR** | Contactor blocked | `EVENT_SLAVE_FAULT` |
| Contactor failed | **ERROR** | Contactor blocked | `EVENT_SLAVE_FAULT` |
| Cell over-voltage | WARNING | Advisory only | `EVENT_SLAVE_WARNING` |
| Cell under-voltage | WARNING | Advisory only | `EVENT_SLAVE_WARNING` |
| Over-temperature | WARNING | Advisory only | `EVENT_SLAVE_WARNING` |

Events are automatically cleared when the fault condition disappears.

---

### 6. Voltage Synchronisation Before Contactor Allow

Before the master allows a slave to close its contactor it checks the voltage difference against the already-active slaves:
- Difference ≤ 1.5 V (`VOLTAGE_DIFF_THRESHOLD_dV = 15`) → contactor allowed
- Difference > 1.5 V → contactor remains blocked until voltages equalise

The first slave that comes online is used as the reference and is always allowed.  
The voltage check is skipped during the startup grace period.

> **Safety override order:** Voltage safety runs first (may allow), then stale detection, IDENT mismatch, and fault flags each run afterwards — so any blocking condition always has the final say over `contactor_allowed`.

---

### 7. Slave Data Aggregation to Inverter

The master combines data from all online and contactor-allowed slaves:

| Parameter | Method |
|-----------|--------|
| Total capacity | Sum of all slaves |
| Remaining capacity | Sum of all slaves |
| Max charge power | Sum (0 if any slave blocks charging) |
| Max discharge power | Sum (0 if any slave blocks discharging) |
| Voltage | First available slave (parallel = same for all) |
| Current | Sum of all slaves |
| Reported SOC | Lowest SOC — unless any slave is >99%, then highest is used |
| Temperature max/min | Highest max, lowest min across all slaves |
| Max design voltage | **Lowest** across all slaves (protects against overcharge) |
| Min design voltage | **Highest** across all slaves (protects against over-discharge) |

---

## Web UI

The master web UI shows a **Slave Nodes** section with a colour-coded status box:

| Colour | Meaning |
|--------|---------|
| Blue-grey `#303E47` | Normal operation, no faults |
| Yellow `#F5CC00` | Warning on at least one slave |
| Red `#A70107` | Error on at least one slave (contactor blocked) |
| Blue `#2B35AF` | OTA firmware update in progress |

Each slave is shown as an individual card with SOC, voltage, current, power, temperature, remaining capacity, max charge/discharge power, and contactor status.

---

## Configuration

Settings are applied via the web UI under **Settings**:

| Setting | Description |
|---------|-------------|
| `NODE_MODE` | `NODE_MASTER` or `NODE_SLAVE` |
| `SLAVE_NODE_ID` | Node ID for this slave (1–24) |
| `IUCOMM` | CAN interface for inter-unit communication (default: `CAN_ADDON_MCP2515`) |

Relevante kildefiler:
- `INTER-UNIT-PROTOCOL.h` — CAN ID-makroer og payload-definitioner
- `MASTER-CAN.cpp` / `MASTER-CAN.h` — Master-logik
- `SLAVE-CAN.cpp` / `SLAVE-CAN.h` — Slave-logik

---

## Oversigt

En Battery Emulator kan konfigureres som **Master** eller **Slave** via indstillingssiden.

| Rolle | Funktion |
|-------|----------|
| **Master** | Kommunikerer med inverteren. Aggregerer data fra alle slaver. Styrer kontaktorer. |
| **Slave** | Kommunikerer med et batteri. Sender batteridata til masteren. Åbner/lukker kontaktor på masterens kommando. |

Op til **24 slave-noder** understøttes (Node ID 1–24).

CAN-bus: MCP2515 addon-bus (konfigureret via `IUCOMM`-indstilling), **500 kbps**.

---

## CAN ID-tildeling

| CAN ID | Retning | Beskrivelse |
|--------|---------|-------------|
| `0x300` | Master → Broadcast | Heartbeat (ingen payload) |
| `0x301`–`0x318` | Master → Slave N | Kontaktor-kommando til Slave N |
| `0x110 + N×0x10` | Slave N → Master | STATUS-besked (msg 0x00) |
| `0x111 + N×0x10` | Slave N → Master | POWER-besked (msg 0x01) |
| `0x112 + N×0x10` | Slave N → Master | INFO-besked (msg 0x02) |
| `0x113 + N×0x10` | Slave N → Master | IP-adresse-besked (msg 0x03) |
| `0x114 + N×0x10` | Slave N → Master | CELL-besked (msg 0x04) |
| `0x115 + N×0x10` | Slave N → Master | IDENT-besked (msg 0x05, kun ved opstart) |

Eksempel for Slave 1 (N=1): STATUS = `0x110`, POWER = `0x111`, osv.

---

## Timing

| Besked | Sendes af | Interval |
|--------|-----------|----------|
| Heartbeat | Master | Hvert 1 sekund |
| STATUS | Slave | Hvert heartbeat (1s) |
| POWER | Slave | Hvert heartbeat (1s) |
| CELL | Slave | Hvert 2. heartbeat (2s) |
| INFO | Slave | Hvert 10. heartbeat (10s) |
| IP | Slave | De første 3 heartbeats + hvert 10. minut |
| IDENT | Slave | De første 3 heartbeats + hvert 10. minut |

**Kollisionsundgåelse:** Slaver svarer med en forsinkelse på `NodeID × 5 ms` efter heartbeat.  
Slave 1 = 5 ms, Slave 2 = 10 ms, ..., Slave 24 = 120 ms.

---

## Beskedlayout

### Heartbeat — `0x300` (0 bytes)
Ingen payload. Bruges kun til at signalere at masteren er online.

---

### Kontaktor-kommando — `0x300 + N` (1 byte)

| Byte | Indhold |
|------|---------|
| [0] | `0x01` = tillad kontaktorlukning, `0x00` = åbn kontaktor |

---

### STATUS — `0x110 + N×0x10` (8 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–1] | uint16 | Pakspænding i deciVolt (3700 = 370,0 V) |
| [2–3] | uint16 | SOC i 0,01% (9550 = 95,50%) |
| [4–5] | int16 | Strøm i deciAmpere (positiv = opladning) |
| [6] | int8 | Maks. temperatur i °C |
| [7] | uint8 | Flags — se tabellen nedenfor |

#### FLAGS-byte (data[7])

| Bit | Konstant | Betydning | Klassifikation |
|-----|----------|-----------|----------------|
| 0 | `IU_FAULT_BMS_FAULT` | BMS rapporterer fejl | **Fejl (rød)** |
| 1 | `IU_FAULT_CELL_OVERVOLTAGE` | Celle-overspænding | Advarsel (gul) |
| 2 | `IU_FAULT_CELL_UNDERVOLTAGE` | Celle-underspænding | Advarsel (gul) |
| 3 | `IU_FAULT_OVERTEMPERATURE` | Overtemperatur (>55°C) | Advarsel (gul) |
| 4 | `IU_FAULT_CONTACTOR_FAILED` | Kontaktor fejlede at lukke | **Fejl (rød)** |
| 5 | `IU_FAULT_BATTERY_TIMEOUT` | Batteri-CAN timeout på slaven | **Fejl (rød)** |
| 6 | `IU_FLAG_CONTACTOR_ENGAGED` | Kontaktor er fysisk lukket (ikke fejl) | Info |
| 7 | `IU_FLAG_STATUS_TOGGLE` | Toggle-bit — skifter hver frame (stale-detektion) | Intern |

---

### POWER — `0x111 + N×0x10` (8 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–1] | uint16 | Maks. ladeeffekt i Watt |
| [2–3] | uint16 | Maks. afladningseffekt i Watt |
| [4–5] | uint16 | Resterende kapacitet i Wh |
| [6] | int8 | Min. temperatur i °C |
| [7] | — | Reserveret |

---

### INFO — `0x112 + N×0x10` (8 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–1] | uint16 | Total pakkapacitet i Wh |
| [2–3] | uint16 | Maks. designspænding i dV |
| [4–5] | uint16 | Min. designspænding i dV |
| [6–7] | uint16 | SOH i 0,01% (9900 = 99,00%) |

---

### IP — `0x113 + N×0x10` (4 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–3] | uint32 | IPv4-adresse, big-endian (192.168.1.10 = 0xC0A8010A) |

Sendes kun hvis slaven er forbundet til WiFi.

---

### CELL — `0x114 + N×0x10` (8 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–1] | uint16 | Højeste celle-spænding i mV |
| [2–3] | uint16 | Laveste celle-spænding i mV |
| [4–7] | — | Reserveret |

---

### IDENT — `0x115 + N×0x10` (8 bytes)

| Bytes | Type | Indhold |
|-------|------|---------|
| [0–1] | uint16 | Firmware-version: `(major << 8) | minor` — eks. 10.6 = `0x0A06` |
| [2–3] | uint16 | Batteri-type ID (`BatteryType` enum cast til uint16) |
| [4–7] | — | Reserveret |

---

## Sikkerhedsfunktioner

### 1. Opstartskarantæne (Startup Grace Period)
**Varighed:** 20 sekunder (`IU_STARTUP_GRACE_S`)

Ved opstart holder masteren alle kontaktorer **åbne** i 20 sekunder, uanset slavernes tilstand.  
Dette giver alle slaver tid til at annoncere deres spænding, inden inverteren begynder at lade/aflade — og forhindrer strømstød ved store spændingsforskelle.

---

### 2. Slave offline-detektion
**Timeout:** 60 sekunder (`IU_OFFLINE_TIMEOUT_S`)

Masteren tæller ned fra 60 for hver slave, og nulstiller tælleren, når der modtages en besked.  
Når tælleren rammer 0:
- Slave markeres som **offline**
- Kontaktor **blokeres**
- Event `EVENT_SLAVE_BATTERY_MISSING` (WARNING) udløses

---

### 3. Stale data-detektion (Toggle-bit)
**Timeout:** 3 sekunder (`IU_STATUS_STALE_SECONDS`)

STATUS-beskeden indeholder et toggle-bit (bit 7) der skifter ved **hver** frame.  
Masteren overvåger om bit 7 ændrer sig. Hvis det ikke har ændret sig i 3 sekunder, er data "frosset" (f.eks. software hængt):
- Kontaktor **blokeres**
- Event `EVENT_STALE_VALUE` (ERROR) udløses
- Automatisk clearet når toggle-bit begynder at skifte igen

---

### 4. Firmware- og batterityp-mismatch (IDENT)

Ved opstart sender slaven en IDENT-besked med firmware-version og batteri-type ID.  
Masteren verificerer:
- **Firmware-version** matcher masterens kompilerede version (`IU_FW_VERSION_NUM`)
- **Batteri-type** matcher den første slave (alle slaver skal have samme batteri-type)

Ved mismatch:
- Kontaktor **blokeres**
- Event `EVENT_SLAVE_IDENT_MISMATCH` (ERROR) udløses
- Automatisk clearet hvis IDENT-data efterfølgende matcher

> **Bemærk:** `IU_FW_VERSION_MAJOR` og `IU_FW_VERSION_MINOR` i `INTER-UNIT-PROTOCOL.h` skal opdateres manuelt når firmware-versionen ændres i `Software.cpp`.

---

### 5. Fault-flag → Events mapping

Fejl-bits i STATUS-flagsbyte oversættes automatisk til events på masteren:

| Fejl | Klassifikation | Konsekvens | Event |
|------|---------------|------------|-------|
| BMS fault | **ERROR** | Kontaktor blokeres | `EVENT_SLAVE_FAULT` |
| Battery CAN timeout | **ERROR** | Kontaktor blokeres | `EVENT_SLAVE_FAULT` |
| Contactor failed | **ERROR** | Kontaktor blokeres | `EVENT_SLAVE_FAULT` |
| Cell overvoltage | WARNING | Kun advarsel | `EVENT_SLAVE_WARNING` |
| Cell undervoltage | WARNING | Kun advarsel | `EVENT_SLAVE_WARNING` |
| Overtemperature | WARNING | Kun advarsel | `EVENT_SLAVE_WARNING` |

Events cleares automatisk når fejlen forsvinder.

---

### 6. Spændingssynkronisering ved kontaktor-tilladelse

Inden masteren tillader en slave at lukke kontaktoren, tjekker den spændingsforskellen til de allerede aktive slaver:
- Forskel ≤ 1,5 V (`VOLTAGE_DIFF_THRESHOLD_dV = 15`) → kontaktor tillades
- Forskel > 1,5 V → kontaktor forbliver blokeret til spændingerne udlignes

Den første slave der melder sig online sættes som reference og tillades altid.  
Spændingskontrollen springer over i opstartskarantæneperioden.

---

### 7. Aggregering af slave-data til inverter

Masteren kombinerer data fra alle online og contactor-allowed slaver:

| Parameter | Metode |
|-----------|--------|
| Total kapacitet | Sum af alle slaver |
| Resterende kapacitet | Sum af alle slaver |
| Maks. ladeeffekt | Sum (0 hvis én slave blokerer opladning) |
| Maks. afladningseffekt | Sum (0 hvis én slave blokerer afladning) |
| Spænding | Første tilgængelige slave (parallel = samme) |
| Strøm | Sum af alle slaver |
| SOC rapporteret | Laveste SOC — medmindre én slave er >99%, så brugs højeste |
| Temperatur max/min | Højeste max, laveste min på tværs af slaver |
| Maks. designspænding | **Laveste** på tværs (beskytter mod overopladning) |
| Min. designspænding | **Højeste** på tværs (beskytter mod overdyb afladning) |

---

## Web UI

Masterens web-UI viser en **Slave Nodes**-sektion med en farvekodet statusboks:

| Farve | Betydning |
|-------|-----------|
| Blågrå `#303E47` | Normaldrift, ingen fejl |
| Gul `#F5CC00` | Advarsel på mindst én slave |
| Rød `#A70107` | Fejl på mindst én slave (kontaktor blokeret) |
| Blå `#2B35AF` | OTA firmware-opdatering |

Hver slave vises som en individuel boks med SOC, spænding, strøm, effekt, temperatur, resterende kapacitet, maks. lade-/afladningseffekt og kontaktor-status.

---

## Konfiguration

Indstillinger sættes via web-UI under **Settings**:

| Indstilling | Beskrivelse |
|-------------|-------------|
| `NODE_MODE` | `NODE_MASTER` eller `NODE_SLAVE` |
| `SLAVE_NODE_ID` | Node ID for denne slave (1–24) |
| `IUCOMM` | CAN-interface til inter-unit kommunikation (standard: `CAN_ADDON_MCP2515`) |
