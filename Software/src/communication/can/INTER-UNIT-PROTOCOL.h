#ifndef _INTER_UNIT_PROTOCOL_H_
#define _INTER_UNIT_PROTOCOL_H_

#include <stdint.h>

#include "../../devboard/utils/common_functions.h"  // crc8_table_SAE_J1850_ZER0

/**
 * Inter-Unit CAN Protocol for Controller/Node Battery Emulator Network
 *
 * Controller uses can_config.battery (BATTCOMM), node uses can_config.inverter (INVCOMM). Speed: 500 kbps.
 *
 * Integrity (protocol v2+):
 *   Every inter-unit frame carries a 1-byte application CRC in its LAST data byte
 *   (data[DLC-1]), computed by iu_crc8() over the preceding bytes seeded with the CAN ID's
 *   low byte. The receiver re-checks it before using any field; a mismatch drops the frame.
 *   This sits on top of classic CAN's own 15-bit CRC and additionally guards against ID
 *   aliasing (a foreign device on an IU ID), mis-delivered frames and software/buffer faults.
 *   Because the three node frames that were already 8 bytes full (STATUS/POWER/INFO) had no
 *   spare byte, a few fields are packed tighter on the wire to free the CRC byte — see the
 *   per-message layouts below. All units run the same firmware, so this is a hard cutover:
 *   a node on an older (CRC-less) firmware is rejected by the controller and stays offline.
 *
 * Timing:
 *   - Controller sends heartbeat every 1 second
 *   - Nodes reply after (NodeID * 5ms) to avoid collisions:
 *       Node 1 = 5ms, Node 2 = 10ms ... Node 8 = 40ms
 *   - Nodes send STATUS + POWER on every heartbeat
 *   - Nodes send INFO every 10th heartbeat (~10 seconds)
 *   - Safety: Node opens contactors if no heartbeat for 60 seconds
 *   - Safety: Controller marks node offline if no reply for 60 seconds
 *
 * CAN ID allocation:
 *   0x300        : Controller heartbeat broadcast
 *   0x301-0x318  : Controller -> Node N contactor command
 *   0x110+N*0x10 : Node N -> Controller, STATUS message  (msg 0x00)
 *   0x111+N*0x10 : Node N -> Controller, POWER message   (msg 0x01)
 *   0x112+N*0x10 : Node N -> Controller, INFO message    (msg 0x02)
 *   0x113+N*0x10 : Node N -> Controller, IP message      (msg 0x03)
 *   0x114+N*0x10 : Node N -> Controller, CELL message    (msg 0x04)
 *   0x115+N*0x10 : Node N -> Controller, IDENT message   (msg 0x05, startup only)
 *   Where N = node ID (1..24)
 *
 * (Authoritative per-message wire layouts — including the CRC byte — are at the bottom of
 *  this header. The 0x_2/0x_4/0x_5 frames now carry a CRC in their last data byte.)
 */

/* ---- Firmware version encoding ---- */
// Single source of truth: parse version_number ("10.11.dev") from Software.cpp at runtime.
// Both controller and node compile the same firmware, so they derive the same value and can
// compare versions over the IDENT message without any hardcoded duplicate.
extern const char* version_number;  // defined in Software.cpp

// Encode "MAJOR.MINOR[.suffix]" as (major << 8) | minor, e.g. "10.11.dev" -> 0x0A0B.
inline uint16_t iu_fw_version_num() {
  uint16_t major = 0;
  uint16_t minor = 0;
  const char* p = version_number;
  while (*p >= '0' && *p <= '9') {
    major = (uint16_t)(major * 10 + (*p - '0'));
    p++;
  }
  if (*p == '.') {
    p++;
    while (*p >= '0' && *p <= '9') {
      minor = (uint16_t)(minor * 10 + (*p - '0'));
      p++;
    }
  }
  return (uint16_t)(((major & 0xFFu) << 8u) | (minor & 0xFFu));
}

/* ---- Protocol version ---- */
// Bumped when the on-wire layout changes incompatibly. v2 = per-frame application CRC.
// Informational/documentation only — all units run the same firmware (hard cutover).
#define IU_PROTOCOL_VERSION 2u

/* ---- On-wire field scaling (to free a byte for the CRC on full 8-byte frames) ---- */
// STATUS soc / INFO soh travel as 1 byte in 0.5% steps; receiver multiplies back to 0.01% units.
#define IU_SOC_WIRE_SCALE 50u  // wire 0..200  <->  real_soc 0..10000 (0.01%)
#define IU_SOH_WIRE_SCALE 50u  // wire 0..198  <->  soh_pptt 0..9900 (0.01%)
// POWER remaining_Wh travels as a 15-bit value in 2 Wh steps (bit15 reused as the balancing flag).
#define IU_REM_WH_WIRE_SCALE 2u  // wire 0..32767  <->  remaining_Wh 0..65534

/* ---- Application CRC ---- */
// CRC8 (SAE J1850, poly 0x1D) over data[0..len-1], seeded with the CAN ID's low byte so a frame
// delivered on the wrong ID (aliasing / mis-delivery) fails the check. Stored in data[DLC-1];
// compute over the first DLC-1 bytes and compare against the last byte on receive.
inline uint8_t iu_crc8(uint32_t can_id, const uint8_t* data, uint8_t len) {
  uint8_t crc = (uint8_t)(can_id & 0xFFu);
  for (uint8_t i = 0; i < len; i++) {
    crc = crc8_table_SAE_J1850_ZER0[crc ^ data[i]];
  }
  return crc;
}

// Validate a received inter-unit frame: the last data byte must equal the CRC over the rest.
// dlc must be >= 1. Returns false (reject) for an empty frame.
inline bool iu_crc_valid(uint32_t can_id, const uint8_t* data, uint8_t dlc) {
  if (dlc < 1u) {
    return false;
  }
  return iu_crc8(can_id, data, (uint8_t)(dlc - 1u)) == data[dlc - 1u];
}

// Stamp the CRC into the last data byte of a frame being sent. dlc must be >= 1.
inline void iu_crc_stamp(uint32_t can_id, uint8_t* data, uint8_t dlc) {
  data[dlc - 1u] = iu_crc8(can_id, data, (uint8_t)(dlc - 1u));
}

/* ---- Controller → Broadcast ---- */
#define IU_CONTROLLER_HEARTBEAT_ID 0x300u  // Heartbeat, no payload needed

/* ---- Controller → Node N ---- */
#define IU_CONTROLLER_CONTACTOR_ID(n) (0x300u + (uint32_t)(n))  // n = 1..24

/* ---- Node N → Controller ---- */
#define IU_NODE_STATUS_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x00u)  // Status (8 bytes, every 1s)
#define IU_NODE_POWER_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x01u)   // Power  (8 bytes, every 1s)
#define IU_NODE_INFO_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x02u)    // Info   (8 bytes, every 10s)
#define IU_NODE_IP_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x03u)      // IP addr (5 bytes, every 10s)
#define IU_NODE_CELL_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x04u)    // Cell info (5 bytes, every 2s)
#define IU_NODE_IDENT_ID(n) (0x100u + ((uint32_t)(n) * 0x10u) + 0x05u)   // Ident (8 bytes, startup only)

/* ---- Node ID range detection ---- */
#define IU_NODE_MSG_MIN_ID 0x110u  // Node 1, msg 0x00
#define IU_NODE_MSG_MAX_ID 0x285u  // Node 24, msg 0x05

/* ---- Contactor command byte (controller → node, data[0]) ---- */
#define IU_CONTACTOR_ALLOW 0x01u  // Node may close contactor
#define IU_CONTACTOR_OPEN 0x00u   // Node must open contactor

/* ---- FLAGS byte in STATUS message (data[7]) ---- */
#define IU_FAULT_BMS_FAULT 0x01u          // bit 0: BMS reports fault
#define IU_FAULT_CELL_OVERVOLTAGE 0x02u   // bit 1: Cell over-voltage
#define IU_FAULT_CELL_UNDERVOLTAGE 0x04u  // bit 2: Cell under-voltage
#define IU_FAULT_OVERTEMPERATURE 0x08u    // bit 3: Over-temperature
#define IU_FAULT_CONTACTOR_FAILED 0x10u   // bit 4: Contactor failed to close
#define IU_FAULT_BATTERY_TIMEOUT 0x20u    // bit 5: Battery CAN timeout on node
#define IU_FLAG_CONTACTOR_ENGAGED 0x40u   // bit 6: Contactor is physically engaged (not a fault)
#define IU_FLAG_STATUS_TOGGLE 0x80u       // bit 7: Alternating toggle bit — flips each STATUS frame for stale detection

/* ---- Fault classification masks ---- */
#define IU_FAULT_ERROR_MASK (IU_FAULT_BMS_FAULT | IU_FAULT_BATTERY_TIMEOUT | IU_FAULT_CONTACTOR_FAILED)
#define IU_FAULT_WARNING_MASK (IU_FAULT_CELL_OVERVOLTAGE | IU_FAULT_CELL_UNDERVOLTAGE | IU_FAULT_OVERTEMPERATURE)

/* ---- Balancing flag in POWER message ---- */
// Node is performing offline balancing — exclude from aggregation. Carried in bit15 of the
// remaining_Wh word [4..5] (the old dedicated pflags byte was reclaimed for the CRC).
#define IU_NODE_REM_BALANCING_BIT 0x8000u  // bit15 of [4..5]
#define IU_NODE_REM_VALUE_MASK 0x7FFFu     // bits0..14 = remaining_Wh / IU_REM_WH_WIRE_SCALE

/* ---- Stale data detection ---- */
#define IU_STATUS_STALE_SECONDS 3u  // Flag stale if STATUS toggle has not changed for this many seconds

/* ---- Delay constants ---- */
#define IU_NODE_REPLY_DELAY_MS(node_id) ((node_id) * 5u)  // Collision avoidance
#define IU_INFO_INTERVAL_HEARTBEATS 10u    // Send INFO every 10th heartbeat (heartbeat cadence = INTERVAL_1_S)
#define IU_CONTROLLER_REBOOT_GAP_MS 5000u  // Heartbeat gap > this => controller restarted, re-announce IDENT/IP
#define IU_STARTUP_GRACE_S \
  20u  // Controller holds all contactors OPEN at startup
       // so all nodes can announce at matching voltages
       // before the inverter starts charging/discharging

/*
 * Wire layouts (protocol v2). Every frame's LAST data byte is the iu_crc8() CRC.
 *
 * HEARTBEAT — IU_CONTROLLER_HEARTBEAT_ID, 1 byte:
 *   [0]    : uint8_t   CRC
 *
 * CONTACTOR COMMAND — IU_CONTROLLER_CONTACTOR_ID(n), 2 bytes:
 *   [0]    : uint8_t   command — IU_CONTACTOR_ALLOW or IU_CONTACTOR_OPEN
 *   [1]    : uint8_t   CRC
 *
 * STATUS message layout — IU_NODE_STATUS_ID(n), 8 bytes:
 *   [0..1] : uint16_t  voltage_dV        — pack voltage in deciVolts (3700 = 370.0 V)
 *   [2]    : uint8_t   soc_wire          — SOC / IU_SOC_WIRE_SCALE (0.5% steps; *50 -> 0.01%)
 *   [3..4] : int16_t   current_dA        — current in deciAmpere (positive = charging)
 *   [5]    : int8_t    temp_max_dC_div10 — max temperature (°C, as int8: 25 = 25°C, -10 = -10°C)
 *   [6]    : uint8_t   flags             — see IU_FAULT_* and IU_FLAG_* above
 *   [7]    : uint8_t   CRC
 *
 * POWER message layout — IU_NODE_POWER_ID(n), 8 bytes:
 *   [0..1] : uint16_t  max_charge_W      — max charge power in Watts
 *   [2..3] : uint16_t  max_discharge_W   — max discharge power in Watts
 *   [4..5] : uint16_t  rem_word          — bit15 = balancing (IU_NODE_REM_BALANCING_BIT),
 *                                          bits0..14 = remaining_Wh / IU_REM_WH_WIRE_SCALE (2 Wh)
 *   [6]    : int8_t    temp_min_dC       — min temperature (°C, as int8)
 *   [7]    : uint8_t   CRC
 *
 * INFO message layout — IU_NODE_INFO_ID(n), 8 bytes:
 *   [0..1] : uint16_t  total_capacity_Wh     — total pack capacity in Wh
 *   [2..3] : uint16_t  max_design_voltage_dV — max design voltage in dV
 *   [4..5] : uint16_t  min_design_voltage_dV — min design voltage in dV
 *   [6]    : uint8_t   soh_wire              — soh_pptt / IU_SOH_WIRE_SCALE (0.5% steps; *50 -> 0.01%)
 *   [7]    : uint8_t   CRC
 *
 * IP message layout — IU_NODE_IP_ID(n), 5 bytes:
 *   [0..3] : uint32_t  IPv4 address (big-endian, e.g. 192.168.1.10 = 0xC0A8010A)
 *   [4]    : uint8_t   CRC
 *
 * CELL message layout — IU_NODE_CELL_ID(n), 5 bytes:
 *   [0..1] : uint16_t  cell_max_voltage_mV
 *   [2..3] : uint16_t  cell_min_voltage_mV
 *   [4]    : uint8_t   CRC
 *
 * IDENT message layout — IU_NODE_IDENT_ID(n), 8 bytes (startup only):
 *   [0..1] : uint16_t  fw_version_num   — (major << 8) | minor, e.g. 10.6 -> 0x0A06
 *   [2..3] : uint16_t  battery_type_id  — BatteryType enum cast to uint16_t
 *   [4..6] : reserved (must be 0)
 *   [7]    : uint8_t   CRC
 */

#endif  // _INTER_UNIT_PROTOCOL_H_
