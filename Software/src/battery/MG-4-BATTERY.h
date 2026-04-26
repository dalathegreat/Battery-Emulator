#ifndef MG_4_BATTERY_H
#define MG_4_BATTERY_H

#include "UdsCanBattery.h"

class Mg4Battery : public UdsCanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual uint32_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length, UdsStatus status);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  virtual uint32_t calculate_max_discharge_power_W();
  virtual uint32_t calculate_max_charge_power_W();
  //virtual uint32_t calculate_pack_voltage_limit_max_dV();

  static constexpr const char* Name = "MG4 battery";

 private:
  static const uint16_t MAX_CELL_DEVIATION_MV = 150;

  int32_t working_cell_min_mV = 0;
  int32_t working_cell_max_mV = 0;
  bool below_working_min = false;
  int32_t cell_voltage_freshness = 0;
  int32_t soc_freshness = 0;

  int16_t module_temperatures_dC[12] = {0};
  int16_t module_temps_received = 0;

  int sendPhase = 0;
  bool reportsFDVoltages = false;
  bool reportsSoC = false;
  bool coulombCounting = false;

  uint32_t total_discharge_dC = 0;  // in deci-Coulombs
  bool total_discharge_initialized = false;

  unsigned long lastTickMillis = 0;

  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was send

  uint32_t* nonvolatile_cookie = 0;
  uint32_t* nonvolatile_total_discharge_dC = 0;
  static const uint32_t NONVOLATILE_COOKIE_VALUE = 0x7734b1f5;

  static const uint16_t POLL_BATTERY_VOLTAGE = 0xB042;
  static const uint16_t POLL_BATTERY_CURRENT = 0xB043;
  static const uint16_t POLL_BATTERY_SOC = 0xB046;
  static const uint16_t POLL_MIN_CELL_TEMPERATURE = 0xB057;
  static const uint16_t POLL_MAX_CELL_TEMPERATURE = 0xB056;

  CAN_frame MG4_4F3 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x4F3,
                       .data = {0xF3, 0x10, 0x48, 0x00, 0xFF, 0xFF, 0x00, 0x11}};
  CAN_frame MG4_047 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x047,
                       .data = {0x00, 0x00, 0x45, 0x7D, 0x7F, 0xFF, 0xFF, 0xFE}};

  CAN_frame MG4_4F3_FD = {.FD = true,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x4F3,
                          .data = {0xF3, 0x10, 0x48, 0x00, 0xFF, 0xFF, 0x00, 0x11}};
  CAN_frame MG4_047_FD = {.FD = true,
                          .ext_ID = false,
                          .DLC = 24,
                          .ID = 0x047,
                          .data = {0x00, 0x01, 0x27, 0x08, 0xF4, 0xF0, 0x80, 0x04, 0x00, 0x5F, 0x3C, 0x00,
                                   0x00, 0x01, 0x48, 0x08, 0x61, 0xF0, 0x6A, 0x06, 0xA0, 0xFF, 0xF0, 0xFF}};
};

#endif
