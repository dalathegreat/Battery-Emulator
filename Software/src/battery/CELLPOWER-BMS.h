#ifndef CELLPOWER_BMS_H
#define CELLPOWER_BMS_H
#include <Arduino.h>
#include "../include.h"
#include "CELLPOWER-HTML.h"
#include "CanBattery.h"

#ifdef CELLPOWER_BMS
#define SELECTED_BATTERY_CLASS CellPowerBms
#endif

class CellPowerBms : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr char* Name = "Cellpower BMS";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  CellpowerHtmlRenderer renderer;
  /* Tweak these according to your battery build */
  static const int MAX_PACK_VOLTAGE_DV = 5000;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 1500;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  unsigned long previousMillis1s = 0;  // will store last time a 1s CAN Message was sent

  //Actual content messages
  // Optional add-on charger module. Might not be needed to send these towards the BMS to keep it happy.
  CAN_frame CELLPOWER_18FF50E9 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 5,
                                  .ID = 0x18FF50E9,
                                  .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CELLPOWER_18FF50E8 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 5,
                                  .ID = 0x18FF50E8,
                                  .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CELLPOWER_18FF50E7 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 5,
                                  .ID = 0x18FF50E7,
                                  .data = {0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CELLPOWER_18FF50E5 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 5,
                                  .ID = 0x18FF50E5,
                                  .data = {0x00, 0x00, 0x00, 0x00, 0x00}};

  bool system_state_discharge = false;
  bool system_state_charge = false;
  bool system_state_cellbalancing = false;
  bool system_state_tricklecharge = false;
  bool system_state_idle = false;
  bool system_state_chargecompleted = false;
  bool system_state_maintenancecharge = false;
  bool IO_state_main_positive_relay = false;
  bool IO_state_main_negative_relay = false;
  bool IO_state_charge_enable = false;
  bool IO_state_precharge_relay = false;
  bool IO_state_discharge_enable = false;
  bool IO_state_IO_6 = false;
  bool IO_state_IO_7 = false;
  bool IO_state_IO_8 = false;
  bool error_Cell_overvoltage = false;
  bool error_Cell_undervoltage = false;
  bool error_Cell_end_of_life_voltage = false;
  bool error_Cell_voltage_misread = false;
  bool error_Cell_over_temperature = false;
  bool error_Cell_under_temperature = false;
  bool error_Cell_unmanaged = false;
  bool error_LMU_over_temperature = false;
  bool error_LMU_under_temperature = false;
  bool error_Temp_sensor_open_circuit = false;
  bool error_Temp_sensor_short_circuit = false;
  bool error_SUB_communication = false;
  bool error_LMU_communication = false;
  bool error_Over_current_IN = false;
  bool error_Over_current_OUT = false;
  bool error_Short_circuit = false;
  bool error_Leak_detected = false;
  bool error_Leak_detection_failed = false;
  bool error_Voltage_difference = false;
  bool error_BMCU_supply_over_voltage = false;
  bool error_BMCU_supply_under_voltage = false;
  bool error_Main_positive_contactor = false;
  bool error_Main_negative_contactor = false;
  bool error_Precharge_contactor = false;
  bool error_Midpack_contactor = false;
  bool error_Precharge_timeout = false;
  bool error_Emergency_connector_override = false;
  bool warning_High_cell_voltage = false;
  bool warning_Low_cell_voltage = false;
  bool warning_High_cell_temperature = false;
  bool warning_Low_cell_temperature = false;
  bool warning_High_LMU_temperature = false;
  bool warning_Low_LMU_temperature = false;
  bool warning_SUB_communication_interfered = false;
  bool warning_LMU_communication_interfered = false;
  bool warning_High_current_IN = false;
  bool warning_High_current_OUT = false;
  bool warning_Pack_resistance_difference = false;
  bool warning_High_pack_resistance = false;
  bool warning_Cell_resistance_difference = false;
  bool warning_High_cell_resistance = false;
  bool warning_High_BMCU_supply_voltage = false;
  bool warning_Low_BMCU_supply_voltage = false;
  bool warning_Low_SOC = false;
  bool warning_Balancing_required_OCV_model = false;
  bool warning_Charger_not_responding = false;
  uint16_t cell_voltage_max_mV = 3700;
  uint16_t cell_voltage_min_mV = 3700;
  int8_t pack_temperature_high_C = 0;
  int8_t pack_temperature_low_C = 0;
  uint16_t battery_pack_voltage_dV = 3700;
  int16_t battery_pack_current_dA = 0;
  uint8_t battery_SOH_percentage = 99;
  uint8_t battery_SOC_percentage = 50;
  uint16_t battery_remaining_dAh = 0;
  uint8_t cell_with_highest_voltage = 0;
  uint8_t cell_with_lowest_voltage = 0;
  uint16_t requested_charge_current_dA = 0;
  uint16_t average_charge_current_dA = 0;
  uint16_t actual_charge_current_dA = 0;
  bool requested_exceeding_average_current = 0;
  bool error_state = false;
};

/* Do not modify any rows below*/
#define NATIVECAN_250KBPS

#endif
