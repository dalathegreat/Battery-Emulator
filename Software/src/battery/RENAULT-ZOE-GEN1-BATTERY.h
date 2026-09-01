#ifndef RENAULT_ZOE_GEN1_BATTERY_H
#define RENAULT_ZOE_GEN1_BATTERY_H

#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

class RenaultZoeGen1Battery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  RenaultZoeGen1Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : UdsCanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    dtc = &datalayer_battery->dtc;
    calculated_total_pack_voltage_mV = 0;
  }

  // Use the default constructor to create the first or single battery.
  RenaultZoeGen1Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Renault Zoe Gen1 22/40kWh";

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "renault_zoe_gen1_dtc.json"; }
  bool get_dtc_standard_code_string() override { return false; }
  void read_DTC() override;

 protected:
  // Called by the UDS superclass for every successful PID response. `data`
  // points at the raw value bytes, starting right after the echoed local
  // identifier. Return 0 to continue the scan list in order.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;
  void on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4220;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  // One-byte KWP2000 local identifiers polled from the BMS (0x21 service).
  static const int GROUP1_CELLVOLTAGES_1_POLL = 0x41;  // Cells 1-62
  static const int GROUP2_CELLVOLTAGES_2_POLL = 0x42;  // Cells 63-96
  static const int GROUP3_METRICS = 0x61;              // Mileage + alltime energy
  static const int GROUP6_BALANCING = 0x07;            // Balancing status bits

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000_69f = 0;
  uint8_t counter_423 = 0;
  uint8_t zoe_19F_counter = 0;
  uint16_t zoe_436_counter = 0xAAEA;

  CAN_frame ZOE_423 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x423,
                       .data = {0x07, 0x1d, 0x00, 0x02, 0x5d, 0x80, 0x5d, 0xc8}};

  // Cyclic Broadcast Frames for Zoe Gen1 Powertrain (Eliminates U1000 / D000 CAN Loss Faults)
  CAN_frame ZOE_19F_INVERTER = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x19F,
                                .data = {0x00, 0x00, 0x7D, 0x04, 0x00, 0x6A, 0x90, 0xFE}};

  CAN_frame ZOE_426_POWER_MUX = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x426,
                                 .data = {0x00, 0x60, 0x01, 0x00, 0x4B, 0xC8, 0x00, 0x40}};

  CAN_frame ZOE_436_VEHICLE_STATUS = {.FD = false,
                                      .ext_ID = false,
                                      .DLC = 6,
                                      .ID = 0x436,
                                      .data = {0x86, 0x14, 0xAA, 0xEA, 0xFF, 0xDC}};

  CAN_frame ZOE_69F_BCM_GATEWAY = {.FD = false,
                                   .ext_ID = false,
                                   .DLC = 4,
                                   .ID = 0x69F,
                                   .data = {0x71, 0x30, 0x28, 0x2F}};

  uint16_t LB_SOC = 50;
  uint16_t LB_Display_SOC = 50;
  uint16_t LB_SOH = 99;
  int16_t LB_Average_Temperature = 0;
  uint32_t LB_Charging_Power_W = 0;
  uint32_t LB_Regen_allowed_W = 0;
  uint32_t LB_Discharge_allowed_W = 0;
  int16_t LB_Current_raw = 2000;  // 0x155 raw; 2000 => 0 A
  int16_t LB_Cell_minimum_temperature = 0;
  int16_t LB_Cell_maximum_temperature = 0;
  uint16_t LB_Cell_minimum_voltage = 3700;
  uint16_t LB_Cell_maximum_voltage = 3700;
  uint16_t LB_kWh_Remaining = 0;
  uint16_t LB_Battery_Voltage = 3700;
  uint8_t LB_Heartbeat = 0;
  uint8_t LB_CUV = 0;
  uint8_t LB_HVBIR = 0;
  uint8_t LB_HVBUV = 0;
  uint8_t LB_EOCR = 0;
  uint8_t LB_HVBOC = 0;
  uint8_t LB_HVBOT = 0;
  uint8_t LB_HVBOV = 0;
  uint8_t LB_COV = 0;
  uint32_t calculated_total_pack_voltage_mV = 370000;
  uint16_t battery_mileage_in_km = 0;
  uint16_t kWh_from_beginning_of_battery_life = 0;
};

#endif
