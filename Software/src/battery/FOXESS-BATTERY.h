#ifndef FOXESS_BATTERY_H
#define FOXESS_BATTERY_H
#include <Arduino.h>
#include "../include.h"

#include "CanBattery.h"

#ifdef FOXESS_BATTERY
#define SELECTED_BATTERY_CLASS FoxessBattery
#endif

class FoxessBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "FoxESS HV2600/ECS4100 OEM battery";

 private:
  static const int MAX_PACK_VOLTAGE_DV = 4672;  //467.2V for HS20.8 (used during startup, refined later)
  static const int MIN_PACK_VOLTAGE_DV = 800;   //80.V for HS5.2 (used during startup, refined later)
  static const int MAX_CELL_DEVIATION_MV = 250;
  static const int MAX_CELL_VOLTAGE_MV = 3800;  //LiFePO4 Prismaticc Cell
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //LiFePO4 Prismatic Cell

  unsigned long previousMillis500 = 0;  // will store last time a 500ms CAN Message was send

  CAN_frame FOX_1871 = {.FD = false,  //Inverter request data from battery. Content varies depending on state
                        .ext_ID = true,
                        .DLC = 8,
                        .ID = 0x1871,
                        .data = {0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint32_t total_watt_hours = 0;
  uint16_t max_charge_power_dA = 0;
  uint16_t max_discharge_power_dA = 0;
  uint16_t cut_mv_max = 0;
  uint16_t cut_mv_min = 0;
  uint16_t cycle_count = 0;
  uint16_t max_ac_voltage = 0;
  uint16_t cellvoltages_mV[128] = {0};
  int16_t temperature_average = 0;
  int16_t pack1_current_sensor = 0;
  int16_t pack2_current_sensor = 0;
  int16_t pack3_current_sensor = 0;
  int16_t pack4_current_sensor = 0;
  int16_t pack5_current_sensor = 0;
  int16_t pack6_current_sensor = 0;
  int16_t pack7_current_sensor = 0;
  int16_t pack8_current_sensor = 0;
  int16_t pack1_temperature_avg_high = 0;
  int16_t pack2_temperature_avg_high = 0;
  int16_t pack3_temperature_avg_high = 0;
  int16_t pack4_temperature_avg_high = 0;
  int16_t pack5_temperature_avg_high = 0;
  int16_t pack6_temperature_avg_high = 0;
  int16_t pack7_temperature_avg_high = 0;
  int16_t pack8_temperature_avg_high = 0;
  int16_t pack1_temperature_avg_low = 0;
  int16_t pack2_temperature_avg_low = 0;
  int16_t pack3_temperature_avg_low = 0;
  int16_t pack4_temperature_avg_low = 0;
  int16_t pack5_temperature_avg_low = 0;
  int16_t pack6_temperature_avg_low = 0;
  int16_t pack7_temperature_avg_low = 0;
  int16_t pack8_temperature_avg_low = 0;
  uint16_t pack1_voltage = 0;
  uint16_t pack2_voltage = 0;
  uint16_t pack3_voltage = 0;
  uint16_t pack4_voltage = 0;
  uint16_t pack5_voltage = 0;
  uint16_t pack6_voltage = 0;
  uint16_t pack7_voltage = 0;
  uint16_t pack8_voltage = 0;
  uint8_t pack1_SOC = 0;
  uint8_t pack2_SOC = 0;
  uint8_t pack3_SOC = 0;
  uint8_t pack4_SOC = 0;
  uint8_t pack5_SOC = 0;
  uint8_t pack6_SOC = 0;
  uint8_t pack7_SOC = 0;
  uint8_t pack8_SOC = 0;
  uint8_t pack_error = 0;
  uint8_t firmware_pack_minor = 0;
  uint8_t firmware_pack_major = 0;
  uint8_t STATUS_OPERATIONAL_PACKS =
      0;  //0x1875 b2 contains status for operational packs (responding) in binary so 01111111 is pack 8 not operational, 11101101 is pack 5 & 2 not operational
  uint8_t NUMBER_OF_PACKS = 0;  //1-8
  uint8_t contactor_status = 0;
  uint8_t statemachine_polling = 0;
  bool charging_disabled = false;
  bool b0_idle = false;
  bool b1_ok_discharge = false;
  bool b2_ok_charge = false;
  bool b3_discharging = false;
  bool b4_charging = false;
  bool b5_operational = false;
  bool b6_active_error = false;
};

#endif
