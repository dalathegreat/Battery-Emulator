#ifndef GROWATT_HV_ARK_BATTERY_H
#define GROWATT_HV_ARK_BATTERY_H

#include "../datalayer/datalayer.h"
#include "CanBattery.h"

// Battery-facing integration for Growatt HV batteries.
//
// Role:
//  - This implementation behaves as the "Storage Inverter" (PCS) side.
//  - It sends PCS->Battery frames (0x3010/0x3020/0x3030) at 1 Hz.
//  - It parses Battery->PCS frames (0x3110..0x3290, 0x3F00) and populates the datalayer.
//
// Protocol reference:
//  Communication Protocol of Growatt High Voltage Battery CAN (V1.10.x)
//  500 kbit/s, extended identifier used in this codebase.
class GrowattHvArkBattery : public CanBattery {
 public:
  // Use the default constructor to create the first or single battery.
  GrowattHvArkBattery() {}

  ~GrowattHvArkBattery() {}

  void setup(void) override;
  void handle_incoming_can_frame(CAN_frame rx_frame) override;
  void update_values() override;
  void transmit_can(unsigned long currentMillis) override;

  static constexpr const char* Name = "Growatt HV ARK battery (battery-facing CAN)";

 private:
  // --- Outgoing (PCS -> Battery) ---
  CAN_frame PCS_3010 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x3010,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PCS_3020 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x3020,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PCS_3030 = {.FD = false,
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x3030,
                        .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  unsigned long previousMillis1000 = 0;
  uint16_t send_times = 0;
  uint32_t epoch_time_s = 0;

  // --- Parsed battery state ---
  uint16_t max_charge_voltage_dV = 0;
  uint16_t discharge_cutoff_voltage_dV = 0;
  int16_t max_charge_current_dA = 0;
  int16_t max_discharge_current_dA = 0;

  uint16_t pack_voltage_dV = 0;
  int16_t pack_current_dA = 0;
  int16_t temp_max_dC = 0;
  int16_t temp_min_dC = 0;
  uint8_t soc_pct = 0;
  uint8_t soh_pct = 0;

  uint16_t cell_max_mV = 0;
  uint16_t cell_min_mV = 0;

  uint16_t remaining_capacity_10mAh = 0;  // unit 0.01Ah
  uint16_t full_capacity_10mAh = 0;       // unit 0.01Ah

  bool battery_sleeping = false;
  bool battery_fault_present = false;
  bool battery_no_charge = false;
  bool battery_no_discharge = false;
};

#endif
