#ifndef AIWAYS_U5_BATTERY_H
#define AIWAYS_U5_BATTERY_H
#include "../datalayer/datalayer_extended.h"
#include "../devboard/hal/hal.h"
#include "UdsCanBattery.h"

class AiwaysU5Battery : public UdsCanBattery {
 public:
  // Use this constructor for the second battery.
  AiwaysU5Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan) : UdsCanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    dtc = &datalayer_battery->dtc;
  }

  // Use the default constructor to create the first or single battery.
  AiwaysU5Battery() : UdsCanBattery() {
    datalayer_battery = &datalayer.battery;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Aiways U5 Battery";

  String get_uds_info_html() override;
  const char* get_dtc_json_filename() override { return "aiways_u5_dtc.json"; }

 protected:
  // Called by the UDS superclass for each successful PID query response.
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  static const int MAX_PACK_VOLTAGE_DV = 4176;  // 96 x 4.35 V
  static const int MIN_PACK_VOLTAGE_DV = 2688;  // 96 x 2.80 V
  static const int MAX_CELL_VOLTAGE_MV = 4350;
  static const int MIN_CELL_VOLTAGE_MV = 2800;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int NUM_CELLS = 96;
  static const int NUM_TEMPS = 24;
  // Power limits 0x2CD: raw unit NOT confirmed — 0.1 kW assumed (baseline 806 = 80.6 kW continuous charge).
  static const int POWER_LIMIT_W_PER_BIT = 100;
  // Current 0x09D: 0.1 A/bit, offset -700 A. Sign vs charging NOT confirmed (emulation cannot tell): +1 or -1.
  static const int CURRENT_SIGN = 1;

  static const uint8_t STATE_STANDBY = 2;
  static const uint8_t STATE_REQUESTED = 3;
  static const uint8_t STATE_HV_ON = 4;
  static const uint8_t STATE_OFF_FAULT = 8;

  // received
  uint8_t state = 0;
  uint8_t fault_level = 0;
  uint8_t soc_percent = 0;
  uint8_t soh_percent = 100;
  uint16_t pack_voltage_dV = 0;  // 0x2CA, 0.1 V/bit = dV
  bool hv_down_request = false;
  int16_t current_dA = 0;          // 0x09D
  uint16_t cont_charge_power = 0;  // 0x2CD raw
  uint16_t peak_charge_power = 0;
  uint16_t cont_discharge_power = 0;
  uint16_t peak_discharge_power = 0;
  bool limits_seen = false;
  uint16_t cell_max_mV = 0;
  uint16_t cell_min_mV = 0;
  uint16_t cell_mV[NUM_CELLS] = {0};
  int16_t temp_C[NUM_TEMPS] = {0};
  bool temps_seen = false;

  // sent
  bool request_on = false;
  uint8_t counter_0B5 = 0, counter_2A3 = 0, counter_2A5 = 0, counter_2A7 = 0;
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent

  // UDS PIDs
  //The BMS (0x7E1) carries a live measurement block around DIDs 0x8800–0x887F — roughly 120 aggregate battery values
  static const uint16_t PID_PACK_VOLTAGE = 0x8871;
  static const uint16_t PID_PACK_CURRENT = 0x887B;
  static const uint16_t PID_CELL_VOLTAGE = 0x882D;

  uint16_t pid_pack_voltage = 0;
  int16_t pid_pack_current = 0;
  uint16_t pid_cellvoltage = 0;

  CAN_frame U5_0B5 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x0B5,
                      .data = {0xFA, 0x0F, 0xA0, 0x7F, 0xF1, 0x00, 0x00, 0x00}};
  CAN_frame U5_2A3 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x2A3,  // byte 1: 0x40 HV off, 0x20 HV on
                      .data = {0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame U5_2A5 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x2A5,
                      .data = {0x84, 0x00, 0x00, 0x00, 0x05, 0xA7, 0x00, 0x00}};
  CAN_frame U5_2A7 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x2A7,
                      .data = {0x0C, 0x80, 0x08, 0x00, 0x64, 0x00, 0x00, 0x00}};
  CAN_frame U5_2AD = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x2AD,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame U5_130 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x130,  // DC-link voltage, 10 bit, 1 V/bit
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  static uint8_t crc8_j1850(const uint8_t* data, uint8_t len);
  static void protect(CAN_frame* frame, uint8_t* counter);
  static uint32_t motorola(const uint8_t* d, int start, int len);
};
#endif
