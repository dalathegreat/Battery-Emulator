#ifndef _INTER_UNIT_PROTOCOL_H_
#define _INTER_UNIT_PROTOCOL_H_

/**
 * Inter-Unit CAN Protocol for Master/Slave Battery Emulator Network
 *
 * Master uses can_config.battery (BATTCOMM), slave uses can_config.inverter (INVCOMM). Speed: 500 kbps.
 *
 * Timing:
 *   - Master sends heartbeat every 1 second
 *   - Slaves reply after (NodeID * 5ms) to avoid collisions:
 *       Slave 1 = 5ms, Slave 2 = 10ms ... Slave 8 = 40ms
 *   - Slaves send STATUS + POWER on every heartbeat
 *   - Slaves send INFO every 10th heartbeat (~10 seconds)
 *   - Safety: Slave opens contactors if no heartbeat for 60 seconds
 *   - Safety: Master marks slave offline if no reply for 60 seconds
 *
 * CAN ID allocation:
 *   0x300        : Master heartbeat broadcast
 *   0x301-0x318  : Master -> Slave N contactor command
 *   0x110+N*0x10 : Slave N -> Master, STATUS message  (msg 0x00)
 *   0x111+N*0x10 : Slave N -> Master, POWER message   (msg 0x01)
 *   0x112+N*0x10 : Slave N -> Master, INFO message    (msg 0x02)
 *   0x113+N*0x10 : Slave N -> Master, IP message      (msg 0x03)
 *   0x114+N*0x10 : Slave N -> Master, CELL message    (msg 0x04)
 *   0x115+N*0x10 : Slave N -> Master, IDENT message   (msg 0x05, startup only)
 *   Where N = node ID (1..24)
 *
 * INFO Message Content:
 *   [0..1] : total_capacity_Wh
 *   [2..3] : max_design_voltage_dV
 *   [4..5] : min_design_voltage_dV
 *   [6..7] : soh_pptt (State of Health in 0.01% units, e.g. 9900 = 99.00%)
 *
 * CELL Message Content:
 *   [0..1] : cell_max_voltage_mV
 *   [2..3] : cell_min_voltage_mV
 *
 * IDENT Message Content (sent at startup, same timing as IP frame):
 *   [0..1] : uint16_t  fw_version_num   — (major << 8) | minor, e.g. 10.6 -> 0x0A06
 *   [2..3] : uint16_t  battery_type_id  — BatteryType enum cast to uint16_t
 *   [4..7] : reserved
 */

/* ---- Firmware version encoding ---- */
// Update these when version_number in Software.cpp changes (e.g. "10.11.dev" -> major=10, minor=11)
#define IU_FW_VERSION_MAJOR 10u
#define IU_FW_VERSION_MINOR 11u
#define IU_FW_VERSION_NUM ((IU_FW_VERSION_MAJOR << 8u) | IU_FW_VERSION_MINOR)  // 0x0A0B

/* ---- Master → Broadcast ---- */
#define IU_MASTER_HEARTBEAT_ID 0x300u  // Heartbeat, no payload needed

/* ---- Master → Slave N ---- */
#define IU_MASTER_CONTACTOR_ID(n) (0x300u + (uint32_t)(n))  // n = 1..24

/* ---- Slave N → Master ---- */
#define IU_SLAVE_STATUS_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x00u)  // Status (8 bytes, every 1s)
#define IU_SLAVE_POWER_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x01u)   // Power  (8 bytes, every 1s)
#define IU_SLAVE_INFO_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x02u)    // Info   (8 bytes, every 10s)
#define IU_SLAVE_IP_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x03u)      // IP addr (4 bytes, every 10s)
#define IU_SLAVE_CELL_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x04u)    // Cell info (8 bytes, every 2s)
#define IU_SLAVE_IDENT_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x05u)   // Ident (8 bytes, startup only)

/* ---- Slave ID range detection ---- */
#define IU_SLAVE_MSG_MIN_ID 0x110u  // Slave 1, msg 0x00
#define IU_SLAVE_MSG_MAX_ID 0x285u  // Slave 24, msg 0x05

/* ---- Contactor command byte (master → slave, data[0]) ---- */
#define IU_CONTACTOR_ALLOW 0x01u  // Slave may close contactor
#define IU_CONTACTOR_OPEN 0x00u   // Slave must open contactor

/* ---- FLAGS byte in STATUS message (data[7]) ---- */
#define IU_FAULT_BMS_FAULT 0x01u          // bit 0: BMS reports fault
#define IU_FAULT_CELL_OVERVOLTAGE 0x02u   // bit 1: Cell over-voltage
#define IU_FAULT_CELL_UNDERVOLTAGE 0x04u  // bit 2: Cell under-voltage
#define IU_FAULT_OVERTEMPERATURE 0x08u    // bit 3: Over-temperature
#define IU_FAULT_CONTACTOR_FAILED 0x10u   // bit 4: Contactor failed to close
#define IU_FAULT_BATTERY_TIMEOUT 0x20u    // bit 5: Battery CAN timeout on slave
#define IU_FLAG_CONTACTOR_ENGAGED 0x40u   // bit 6: Contactor is physically engaged (not a fault)
#define IU_FLAG_STATUS_TOGGLE 0x80u       // bit 7: Alternating toggle bit — flips each STATUS frame for stale detection

/* ---- Fault classification masks ---- */
#define IU_FAULT_ERROR_MASK (IU_FAULT_BMS_FAULT | IU_FAULT_BATTERY_TIMEOUT | IU_FAULT_CONTACTOR_FAILED)
#define IU_FAULT_WARNING_MASK (IU_FAULT_CELL_OVERVOLTAGE | IU_FAULT_CELL_UNDERVOLTAGE | IU_FAULT_OVERTEMPERATURE)

/* ---- FLAGS byte in POWER message (data[7]) ---- */
#define IU_SLAVE_PFLAG_BALANCING 0x01u  // bit 0: Slave is performing offline balancing — exclude from aggregation

/* ---- Stale data detection ---- */
#define IU_STATUS_STALE_SECONDS 3u  // Flag stale if STATUS toggle has not changed for this many seconds

/* ---- Delay constants ---- */
#define IU_SLAVE_REPLY_DELAY_MS(node_id) ((node_id) * 5u)  // Collision avoidance
#define IU_INFO_INTERVAL_HEARTBEATS 10u  // Send INFO every 10th heartbeat (heartbeat cadence = INTERVAL_1_S)
#define IU_MASTER_REBOOT_GAP_MS 5000u    // Heartbeat gap > this => master restarted, re-announce IDENT/IP
#define IU_STARTUP_GRACE_S \
  20u  // Master holds all contactors OPEN at startup
       // so all slaves can announce at matching voltages
       // before the inverter starts charging/discharging

/*
 * STATUS message layout — IU_SLAVE_STATUS_ID(n), 8 bytes:
 *   [0..1] : uint16_t  voltage_dV        — pack voltage in deciVolts (3700 = 370.0 V)
 *   [2..3] : uint16_t  real_soc          — SOC in 0.01% (9550 = 95.50%)
 *   [4..5] : int16_t   current_dA        — current in deciAmpere (positive = charging)
 *   [6]    : int8_t    temp_max_dC_div10 — max temperature (°C, as int8: 25 = 25°C, -10 = -10°C)
 *   [7]    : uint8_t   flags             — see IU_FAULT_* and IU_FLAG_* above
 *
 * POWER message layout — IU_SLAVE_POWER_ID(n), 8 bytes:
 *   [0..1] : uint16_t  max_charge_W      — max charge power in Watts
 *   [2..3] : uint16_t  max_discharge_W   — max discharge power in Watts
 *   [4..5] : uint16_t  remaining_Wh      — remaining capacity in Wh (max 65535 Wh)
 *   [6]    : int8_t    temp_min_dC       — min temperature (°C, as int8)
 *   [7]    : uint8_t   slave_pflags      — see IU_SLAVE_PFLAG_* below
 *
 * INFO message layout — IU_SLAVE_INFO_ID(n), 8 bytes:
 *   [0..1] : uint16_t  total_capacity_Wh    — total pack capacity in Wh
 *   [2..3] : uint16_t  max_design_voltage_dV — max design voltage in dV
 *   [4..5] : uint16_t  min_design_voltage_dV — min design voltage in dV
 *   [6..7] : uint16_t  reserved
 *
 * IP message layout — IU_SLAVE_IP_ID(n), 4 bytes:
 *   [0..3] : uint32_t  IPv4 address (big-endian, e.g. 192.168.1.10 = 0xC0A8010A)
 *
 * CONTACTOR COMMAND layout — IU_MASTER_CONTACTOR_ID(n), 1 byte:
 *   [0]    : uint8_t   command — IU_CONTACTOR_ALLOW or IU_CONTACTOR_OPEN
 */

#endif  // _INTER_UNIT_PROTOCOL_H_
