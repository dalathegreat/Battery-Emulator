#ifndef GROWATT_WIT_CAN_H
#define GROWATT_WIT_CAN_H

#include <Arduino.h>
#include "CanInverterProtocol.h"

/* GROWATT BATTERY BMS CAN COMMUNICATION PROTOCOL V1.1 2024.7.19
 * CAN 2.0B Extended Frame (29-bit identifier)
 * 500 kbps
 * Intel (Little Endian) byte order
 *
 * CAN ID Structure (29-bit):
 * | PRI (28-26) | PG (25-24) | FSN (23-16) | TA (15-8) | SA (7-0) |
 *
 * Address definitions:
 * - 0xF1: Inverter (PCS)
 * - 0xF3: Battery monitoring (BMS)
 * - 0xFF: Broadcast to all nodes
 *
 * Example: 0x1AC3FFF3 = PRI:6, PG:2, FSN:0xC3, TA:0xFF, SA:0xF3
 */

// PCS -> BMS message FSN codes (for matching incoming messages)
#define FSN_HEARTBEAT 0xB5    // 1AB5: Heartbeat from PCS
#define FSN_DATETIME 0xB6     // 1AB6: Date/Time from PCS
#define FSN_PCS_STATUS 0xB7   // 1AB7: PCS status
#define FSN_BUS_VOLTAGE 0xB8  // 1AB8: Bus voltage setting
#define FSN_PCS_PRODUCT 0xBE  // 1ABE: PCS product info (triggers event messages)

// Temperature offset: Actual = Raw * 0.1 - 40.0
#define TEMP_OFFSET_DC 400  // 40.0°C in deciCelsius

// Current offset: Actual = Raw * 0.1 - 1000.0
#define GROWATT_CURRENT_OFFSET_DA 10000  // 1000.0A in deciAmpere

class GrowattWitInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Growatt WIT compatible battery via CAN";

 private:
  // Helper to extract FSN from 29-bit CAN ID
  uint8_t get_fsn_from_id(uint32_t can_id) { return (can_id >> 16) & 0xFF; }

  // Timing variables
  unsigned long previousMillis100ms = 0;
  unsigned long previousMillis500ms = 0;
  unsigned long previousMillis1000ms = 0;
  unsigned long previousMillis2000ms = 0;

  uint16_t pcs_frame_count = 0;
  uint16_t pcs_bus_voltage_dV = 0;
  uint8_t pcs_working_status = 0;
  uint8_t pcs_fault_status = 0;
  uint8_t sn_frame_counter = 0;  // For multi-frame SN transmission

  bool inverter_alive = false;

  /* BMS -> PCS Messages (Basic Functions - Required) */

  // 1AC3: Current/Voltage Limits (100ms)
  CAN_frame GROWATT_1AC3 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC3FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC4: CV Voltage (100ms)
  CAN_frame GROWATT_1AC4 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC4FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC5: Working Status (100ms)
  CAN_frame GROWATT_1AC5 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC5FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC6: SOC/SOH/Capacity (500ms)
  CAN_frame GROWATT_1AC6 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC6FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC7: Voltage/Current (100ms)
  CAN_frame GROWATT_1AC7 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC7FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  /* BMS -> PCS Messages (Data Monitoring - Optional but recommended) */

  // 1AC0: System Composition (2000ms)
  CAN_frame GROWATT_1AC0 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC0FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC2: Product Version (Event - triggered by PCS 1ABE)
  CAN_frame GROWATT_1AC2 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC2FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC8: Software Version (500ms)
  CAN_frame GROWATT_1AC8 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC8FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AC9: Max Module Voltage (1000ms)
  CAN_frame GROWATT_1AC9 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AC9FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1ACA: Min Module Voltage (1000ms)
  CAN_frame GROWATT_1ACA = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1ACAFFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1ACC: Max Cell Temperature (1000ms)
  CAN_frame GROWATT_1ACC = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1ACCFFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1ACD: Min Cell Temperature (1000ms)
  CAN_frame GROWATT_1ACD = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1ACDFFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1ACE: Max Cell Voltage (100ms)
  CAN_frame GROWATT_1ACE = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1ACEFFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1ACF: Min Cell Voltage (100ms)
  CAN_frame GROWATT_1ACF = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1ACFFFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AD0: Cluster SOC Info (1000ms)
  CAN_frame GROWATT_1AD0 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AD0FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AD1: Cumulative Energy (1000ms)
  CAN_frame GROWATT_1AD1 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AD1FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AD8: Fault Information 3 (100ms)
  CAN_frame GROWATT_1AD8 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AD8FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1AD9: Fault Information 4 (100ms)
  CAN_frame GROWATT_1AD9 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1AD9FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1A80: BMS Software Version (Event)
  CAN_frame GROWATT_1A80 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1A80FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  // 1A82: Battery Serial Number (Event - multi-frame)
  CAN_frame GROWATT_1A82 = {.FD = false,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1A82FFF3,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
};

#endif
