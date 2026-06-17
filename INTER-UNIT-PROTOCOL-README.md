# Inter-Unit Controller/Node CAN Protocol

Documentation for the Battery Emulator's internal Controller/Node communication protocol.

Relevant source files:
- `Software/src/communication/can/INTER-UNIT-PROTOCOL.h` — CAN ID macros and payload definitions
- `Software/src/communication/can/CONTROLLER-CAN.cpp` / `CONTROLLER-CAN.h` — Controller-side logic
- `Software/src/communication/can/BATTERY-NODE-CAN.cpp` / `BATTERY-NODE-CAN.h` — Battery-node-side logic

---

## Overview

A Battery Emulator can be configured as **Controller** or **Battery Node** via the settings page.

| Role | Function |
|------|----------|
| **Controller** | Communicates with the inverter. Aggregates data from all nodes. Controls contactors. |
| **Battery Node** | Communicates with a battery. Sends battery data to the controller. Opens/closes contactor on controller command. |

Up to **24 battery nodes** are supported (Node ID 1–24).

CAN bus, **500 kbps**: the controller runs the inter-unit protocol on its **battery** interface (`BATTCOMM`), the node on its **inverter** interface (`INVCOMM`). There is no separate inter-unit bus.

---

## Hardware Topology

Each unit is a separate ESP32 board (e.g. LilyGo). The controller and all nodes share one inter-unit CAN bus. Each node connects to its own real battery; the controller connects to the inverter.

```
        ┌───────────┐
        │  Inverter │
        └─────┬─────┘
              │ CAN
        ┌─────┴─────────────┐
        │ CONTROLLER LilyGo │
        └─────┬─────────────┘
              │
   ═══════════╪═══════════════════════════╗   Inter-Unit CAN bus
              │                            ║   500 kbps
        ┌─────┴───────────┐   ┌────────────╫────┐
        │ NODE 1 LilyGo   │   │ NODE 2 LilyGo    │   ... up to 24 nodes
        └─────┬───────────┘   └────────────┬────┘
              │ CAN                         │ CAN
        ┌─────┴─────┐               ┌───────┴───┐
        │  Battery  │               │  Battery  │
        │ (BMW i3)  │               │ (BMW i3)  │
        └───────────┘               └───────────┘
```

- All inter-unit nodes share one 500 kbps bus; terminate it with 120 Ω at both physical ends like any CAN bus, and tie all node grounds together.

---

## CAN ID Allocation

| CAN ID | Direction | Description |
|--------|-----------|-------------|
| `0x300` | Controller → Broadcast | Heartbeat (no payload) |
| `0x301`–`0x318` | Controller → Node N | Contactor command to Node N |
| `0x110 + N×0x10` | Node N → Controller | STATUS message (msg 0x00) |
| `0x111 + N×0x10` | Node N → Controller | POWER message (msg 0x01) |
| `0x112 + N×0x10` | Node N → Controller | INFO message (msg 0x02) |
| `0x113 + N×0x10` | Node N → Controller | IP address message (msg 0x03) |
| `0x114 + N×0x10` | Node N → Controller | CELL message (msg 0x04) |
| `0x115 + N×0x10` | Node N → Controller | IDENT message (msg 0x05, startup only) |

Example for Node 1 (N=1): STATUS = `0x110`, POWER = `0x111`, etc.

---

## Timing

| Message | Sent by | Interval |
|---------|---------|----------|
| Heartbeat | Controller | Every 1 second |
| STATUS | Node | Every heartbeat (1s) |
| POWER | Node | Every heartbeat (1s) |
| CELL | Node | Every 2nd heartbeat (2s) |
| INFO | Node | Every 10th heartbeat (10s) |
| IP | Node | First 3 heartbeats + every 10 minutes |
| IDENT | Node | First 3 heartbeats + every 10 minutes |

**Collision avoidance:** Nodes reply with a delay of `NodeID × 5 ms` after heartbeat.  
Node 1 = 5 ms, Node 2 = 10 ms, ..., Node 24 = 120 ms.

---

## Message Layouts

### Heartbeat — `0x300` (0 bytes)
No payload. Used only to signal that the controller is online.

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
| 5 | `IU_FAULT_BATTERY_TIMEOUT` | Battery CAN timeout on node | **Error (red)** |
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
| [7] | uint8 | Node power flags — see `IU_NODE_PFLAG_*` (bit 0 = offline balancing) |

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

Only sent if the node is connected to WiFi.

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

At startup the controller holds all contactors **open** for 20 seconds regardless of node state.  
This gives all nodes time to announce their voltage before the inverter begins charging or discharging, preventing inrush current from large voltage differences between packs.

---

### 2. Node Offline Detection
**Timeout:** 60 seconds (`CAN_STILL_ALIVE`, decremented at 1 Hz)

The controller maintains a countdown from 60 for each node, reset on every received message.  
When the counter reaches 0:
- Node is marked **offline**
- Contactor is **blocked**
- Event `EVENT_BATTERY_NODE_MISSING` (WARNING) is raised

---

### 3. Stale Data Detection (Toggle Bit)
**Timeout:** 3 seconds (`IU_STATUS_STALE_SECONDS`)

The STATUS message contains a toggle bit (bit 7) that flips on **every** frame.  
The controller monitors whether bit 7 is changing. If it has not changed for 3 seconds the data is considered frozen (e.g. node software hung):
- Contactor is **blocked**
- Event `EVENT_BATTERY_NODE_STATUS_STALE` (WARNING) is raised
- Automatically cleared when the toggle bit starts changing again

---

### 4. Firmware and Battery Type Mismatch (IDENT)

At startup the node sends an IDENT message containing its firmware version and battery type ID.  
The controller verifies:
- **Firmware version** matches the controller's compiled version (`IU_FW_VERSION_NUM`)
- **Battery type** matches the first node that reported one (all nodes must use the same battery type)

On mismatch:
- Contactor is **blocked**
- Event `EVENT_BATTERY_NODE_IDENT_MISMATCH` (WARNING) is raised
- Automatically cleared if subsequent IDENT data matches

> **Note:** `IU_FW_VERSION_MAJOR` and `IU_FW_VERSION_MINOR` in `INTER-UNIT-PROTOCOL.h` must be updated manually whenever the firmware version changes in `Software.cpp`.

---

### 5. Fault Flag → Event Mapping

Fault bits in the STATUS flags byte are automatically translated to events on the controller:

| Fault | Classification | Consequence | Event |
|-------|---------------|-------------|-------|
| BMS fault | **ERROR** | Contactor blocked | `EVENT_BATTERY_NODE_FAULT` |
| Battery CAN timeout | **ERROR** | Contactor blocked | `EVENT_BATTERY_NODE_FAULT` |
| Contactor failed | **ERROR** | Contactor blocked | `EVENT_BATTERY_NODE_FAULT` |
| Cell over-voltage | WARNING | Advisory only | `EVENT_BATTERY_NODE_WARNING` |
| Cell under-voltage | WARNING | Advisory only | `EVENT_BATTERY_NODE_WARNING` |
| Over-temperature | WARNING | Advisory only | `EVENT_BATTERY_NODE_WARNING` |

Events are automatically cleared when the fault condition disappears.

---

### 6. Voltage Synchronisation Before Contactor Allow

Before the controller allows a node to close its contactor it must verify that the node's pack voltage is close enough to the already-active nodes. There are two paths depending on how far apart the voltages are.

#### Direct-close path (diff ≤ 1.5 V)
When the voltage difference is at or below `VOLTAGE_DIFF_THRESHOLD_dV` (1.5 V) for `VOLTAGE_DIFF_SECONDS_LIMIT` (10 s) the contactor is allowed — no power capping is applied. This is the normal case after a brief disconnection.

#### Prejoin path (1.5 V < diff ≤ 1.8 V with inverter active)
When the raw voltage difference is between 1.5 V and 1.8 V (`PREJOIN_ENTER_DIFF_dV`) and the inverter is delivering meaningful power (absolute load > `PREJOIN_LOW_POWER_ABORT_W`, 300 W), the controller enters **prejoin mode** for that node. Prejoin does **not** cap or reduce inverter power. The ongoing charge/discharge current naturally pulls the idle pack's voltage toward the active pack; prejoin simply monitors that convergence and imposes a stricter, direction-aware close gate so the contactor only closes once the gap has shrunk into a safe window.

How it works:
1. Prejoin activates when the diff first falls to ≤ 1.8 V while the inverter is working.
2. While active, the controller tracks a **close window** that depends on the load direction:
   - **Charging** (load ≥ 0): close window is diff ≤ `PREJOIN_CLOSE_RAW_DIFF_dV` (0.5 V).
   - **Discharging** (load < 0): close window requires the node voltage to be at or above the reference **and** diff ≤ 0.7 V, so the joining pack is never pulled down by an already-loaded bus.
3. The diff must stay inside the close window for `PREJOIN_CLOSE_DWELL_S` (2 s) of stable dwell.
4. The contactor is allowed when **both** the standard voltage timer (diff ≤ 1.5 V for 10 s) and the prejoin gate (close window satisfied + dwell met) pass.
5. Prejoin aborts if the absolute inverter load stays below `PREJOIN_LOW_POWER_ABORT_W` (300 W) for `PREJOIN_LOW_POWER_ABORT_S` (30 s) — e.g. when solar disappears mid-prejoin — and the node falls back to waiting for the direct-close path.

If neither path can close the contactor (diff > 1.8 V, or the inverter is idle) the node remains blocked.

The first node that comes online is used as the reference voltage and is always allowed.  
All voltage checks are skipped during the startup grace period.

> **Safety override order:** Voltage safety runs first (may allow), then stale detection, IDENT mismatch, and fault flags each run afterwards — so any blocking condition always has the final say over `contactor_allowed`.

---

### 7. Node Data Aggregation to Inverter

The controller combines data from all online nodes whose contactor is actually **engaged** (physically closed). A node that is merely allowed but has not closed its contactor is not part of the pack and is excluded — otherwise its zero power limits would wrongly cap the whole pack:

| Parameter | Method |
|-----------|--------|
| Total capacity | Sum of all engaged nodes |
| Remaining capacity | Sum of all engaged nodes |
| Max charge power | Sum (0 if any engaged node blocks charging) |
| Max discharge power | Sum (0 if any engaged node blocks discharging) |
| Voltage | First available node (parallel = same for all) |
| Current | Sum of all engaged nodes |
| Reported SOC | Lowest SOC normally. When the highest node SOC reaches 95%, the reported value blends linearly from lowest toward highest as the top node rises from 95% to 100%, so the inverter sees a smooth rise rather than a sudden jump at full charge. |
| Temperature max/min | Highest max, lowest min across all engaged nodes |
| Max design voltage | **Lowest** across all engaged nodes (protects against overcharge) |
| Min design voltage | **Highest** across all engaged nodes (protects against over-discharge) |

---

## Web UI

The controller web UI shows a **Battery Nodes** section with a colour-coded status box:

| Colour | Meaning |
|--------|---------|
| Blue-grey `#303E47` | Normal operation, no faults |
| Yellow `#F5CC00` | Warning on at least one node |
| Red `#A70107` | Error on at least one node (contactor blocked) |
| Blue `#2B35AF` | OTA firmware update in progress |

Each node is shown as an individual card with SOC, voltage, current, power, temperature, remaining capacity, max charge/discharge power, and contactor status.

---

## Configuration

Settings are applied via the web UI under **Settings**:

| Setting | Description |
|---------|-------------|
| Battery type = **Inter-Unit Controller** | Selects Controller role. The controller's `BATTCOMM` interface carries the inter-unit protocol. |
| Inverter protocol = **Inter-Unit Node** | Selects Battery Node role. The node's `INVCOMM` interface carries the inter-unit protocol. |
| Battery node ID | Node ID for this battery node (1–24). Stored under the NVM key `SLAVENODEID` (kept for backward compatibility). |

> The node role is derived from the battery/inverter selection — there is no separate `NODE_MODE` setting.
