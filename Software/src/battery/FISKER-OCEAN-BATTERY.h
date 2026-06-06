#ifndef FISKER_OCEAN_BATTERY_H
#define FISKER_OCEAN_BATTERY_H

#include "CanBattery.h"

class FiskerOceanBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Fisker Ocean 113/106kWh battery";

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { UserRequestDTCreset = true; }

 private:
  bool UserRequestDTCreset = false;
  unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

  static const uint16_t PID_BATTERY_SUM_VOLTAGE = 0x2003;
  static const uint16_t PID_BATTERY_CURRENT = 0x2004;
  static const uint16_t PID_BATTERY_CURRENT_VALID = 0x2005;
  static const uint16_t PID_DISCHARGE_CURR_LIMIT = 0x2008;
  static const uint16_t PID_CHARGE_CURR_LIMIT = 0x2009;
  static const uint16_t PID_CHARGE_OVER_CURR_LIMIT = 0x2011;
  static const uint16_t PID_HALL_SAMPLE_CURRENT = 0x2016;
  static const uint16_t PID_CSU_SAMPLE_CURRENT = 0x2019;
  static const uint16_t PID_CSU_CURRENT_STATE = 0x2024;
  static const uint16_t PID_INLET_WATER_TEMP = 0x2026;
  static const uint16_t PID_OUTLET_WATER_TEMP = 0x2027;
  static const uint16_t PID_MAX_BALANCE_CIRCUIT_TEMP = 0x2031;
  static const uint16_t PID_SA_LVMD_BAL_TEMP_VALID = 0x2032;
  static const uint16_t PID_MAX_CHIP_TEMP = 0x2033;
  static const uint16_t PID_SA_LVMD_CHIP_INSIDE_TEMP_VALID = 0x2034;
  static const uint16_t PID_AVG_MODULE_TEMP = 0x2038;
  static const uint16_t PID_MAX_MODULE_TEMP = 0x2039;
  static const uint16_t PID_MIN_MODULE_TEMP = 0x2040;
  static const uint16_t PID_MAX_MODULE_TEMP_CMC_AND_POINT_PSTN = 0x2041;
  static const uint16_t PID_MIN_MODULE_TEMP_CMC_AND_POINT_PSTN = 0x2042;
  static const uint16_t PID_MODULE_TEMP_VALID = 0x2043;
  static const uint16_t PID_MAX_VOLT_CELL_SOC_PERCENT = 0x2047;
  static const uint16_t PID_MIN_VOLT_CELL_SOC_PERCENT = 0x2048;
  static const uint16_t PID_AVG_VOLT_CELL_SOC_PERCENT = 0x2049;
  static const uint16_t PID_PACK_DISPLAY_SOC_PERCENT = 0x2050;
  static const uint16_t PID_UNEXPECTED_POWER_DOWN_FAULT = 0x2053;
  static const uint16_t PID_MODULE_TEMP_DAISYCHAIN_UPDATED = 0x2054;
  static const uint16_t PID_CELL_VOLT_DAISYCHAIN_UPDATED = 0x2055;
  static const uint16_t PID_CMC_RESET_ERR_FLAG = 0x2056;
  static const uint16_t PID_VCU_CRASH_MESSAGE_STATUS = 0x2057;
  static const uint16_t PID_HARDWARE_SIG_PWM_PERIOD = 0x2058;
  static const uint16_t PID_HARDWARE_PWM_DUTY_CYCLE = 0x2059;
  static const uint16_t PID_FORCE_FORBIDDEN_ISO_DETECT_CMD = 0x2060;
  static const uint16_t PID_ISOLATION_MEAS_STATUS = 0x2061;
  static const uint16_t PID_ISOLATION_MEAS_STATE = 0x2062;
  static const uint16_t PID_POS_ISO_MEAS_VOLT_RAW = 0x2063;
  static const uint16_t PID_NEG_ISO_MEAS_VOLT_RAW = 0x2064;
  static const uint16_t PID_ISO_MEAS_POS_RES_KOHM = 0x2069;
  static const uint16_t PID_ISO_MEAS_NEG_RES_KOHM = 0x2070;
  static const uint16_t PID_BAL_CIRCUIT_OPEN_ERR_CMC_PSTN = 0x2078;
  static const uint16_t PID_BAL_CIRCUIT_OPEN_ERR_CELL_PSTN = 0x2079;
  static const uint16_t PID_BAL_CIRCUIT_SHORT_ERR_CMC_PSTN = 0x2080;
  static const uint16_t PID_BAL_CIRCUIT_SHORT_ERR_CELL_PSTN = 0x2081;
  static const uint16_t PID_VOLT_OR_CURR_CH0_HIGH_VOLT_MV = 0x2089;
  static const uint16_t PID_VOLT_OR_CURR_CH1_HIGH_VOLT_MV = 0x2090;
  static const uint16_t PID_BATTERY_TO_G0_VOLT = 0x2107;
  static const uint16_t PID_PV_POS_TO_G0_VOLT = 0x2108;
  static const uint16_t PID_MAIN_POS_TO_G0_VOLT = 0x2109;
  static const uint16_t PID_MAIN_POS_TO_G1_VOLT = 0x2117;
  static const uint16_t PID_KL30C_VOLTAGE = 0x2130;
  static const uint16_t PID_MAX_CELL_VOLT_CMC_AND_POINT_PSTN = 0x2133;
  static const uint16_t PID_MIN_CELL_VOLT_CMC_AND_POINT_PSTN = 0x2134;
  static const uint16_t PID_AVG_CELL_VOLTAGE = 0x2136;
  static const uint16_t PID_MAX_CELL_VOLTAGE = 0x2137;
  static const uint16_t PID_MIN_CELL_VOLTAGE = 0x2138;
  static const uint16_t PID_CELL_VOLT_VALID = 0x2143;
  static const uint16_t PID_PV_POS_CONTACTOR_AGING = 0x2144;
  static const uint16_t PID_PR_NEG_CONTACTOR_AGING = 0x2145;
  static const uint16_t PID_TIME_STAMP = 0xEEF6;
  static const uint16_t PID_VEHICLE_SPEED = 0xEEF7;
  static const uint16_t PID_ST_MIN = 0xEFFE;
  static const uint16_t PID_APPLICATION_SOFTWARE_FINGERPRINT = 0xF184;
  static const uint16_t PID_VEHICLE_IDENTIFICATION_NUMBER = 0xF190;

  CAN_frame FISKER_PID_REQUEST = {.FD = false,
                                  .ext_ID = false,
                                  .DLC = 8,
                                  .ID = 0x7E1,
                                  .data = {0x03, 0x22, 0x20, 0x03, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FISKER_DTC_RESET = {.FD = false,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x7E1,
                                .data = {0x04, 0x14, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00}};

  uint16_t currentpoll = PID_BATTERY_SUM_VOLTAGE;
  uint16_t incoming_poll = 0;
  uint8_t poll_index = 0;
  const uint16_t poll_commands[63] = {PID_BATTERY_SUM_VOLTAGE,
                                      PID_BATTERY_CURRENT,
                                      PID_BATTERY_CURRENT_VALID,
                                      PID_DISCHARGE_CURR_LIMIT,
                                      PID_CHARGE_CURR_LIMIT,
                                      PID_CHARGE_OVER_CURR_LIMIT,
                                      PID_HALL_SAMPLE_CURRENT,
                                      PID_CSU_SAMPLE_CURRENT,
                                      PID_CSU_CURRENT_STATE,
                                      PID_INLET_WATER_TEMP,
                                      PID_OUTLET_WATER_TEMP,
                                      PID_MAX_BALANCE_CIRCUIT_TEMP,
                                      PID_SA_LVMD_BAL_TEMP_VALID,
                                      PID_MAX_CHIP_TEMP,
                                      PID_SA_LVMD_CHIP_INSIDE_TEMP_VALID,
                                      PID_AVG_MODULE_TEMP,
                                      PID_MAX_MODULE_TEMP,
                                      PID_MIN_MODULE_TEMP,
                                      PID_MAX_MODULE_TEMP_CMC_AND_POINT_PSTN,
                                      PID_MIN_MODULE_TEMP_CMC_AND_POINT_PSTN,
                                      PID_MODULE_TEMP_VALID,
                                      PID_MAX_VOLT_CELL_SOC_PERCENT,
                                      PID_MIN_VOLT_CELL_SOC_PERCENT,
                                      PID_AVG_VOLT_CELL_SOC_PERCENT,
                                      PID_PACK_DISPLAY_SOC_PERCENT,
                                      PID_UNEXPECTED_POWER_DOWN_FAULT,
                                      PID_MODULE_TEMP_DAISYCHAIN_UPDATED,
                                      PID_CELL_VOLT_DAISYCHAIN_UPDATED,
                                      PID_CMC_RESET_ERR_FLAG,
                                      PID_VCU_CRASH_MESSAGE_STATUS,
                                      PID_HARDWARE_SIG_PWM_PERIOD,
                                      PID_HARDWARE_PWM_DUTY_CYCLE,
                                      PID_FORCE_FORBIDDEN_ISO_DETECT_CMD,
                                      PID_ISOLATION_MEAS_STATUS,
                                      PID_ISOLATION_MEAS_STATE,
                                      PID_POS_ISO_MEAS_VOLT_RAW,
                                      PID_NEG_ISO_MEAS_VOLT_RAW,
                                      PID_ISO_MEAS_POS_RES_KOHM,
                                      PID_ISO_MEAS_NEG_RES_KOHM,
                                      PID_BAL_CIRCUIT_OPEN_ERR_CMC_PSTN,
                                      PID_BAL_CIRCUIT_OPEN_ERR_CELL_PSTN,
                                      PID_BAL_CIRCUIT_SHORT_ERR_CMC_PSTN,
                                      PID_BAL_CIRCUIT_SHORT_ERR_CELL_PSTN,
                                      PID_VOLT_OR_CURR_CH0_HIGH_VOLT_MV,
                                      PID_VOLT_OR_CURR_CH1_HIGH_VOLT_MV,
                                      PID_BATTERY_TO_G0_VOLT,
                                      PID_PV_POS_TO_G0_VOLT,
                                      PID_MAIN_POS_TO_G0_VOLT,
                                      PID_MAIN_POS_TO_G1_VOLT,
                                      PID_KL30C_VOLTAGE,
                                      PID_MAX_CELL_VOLT_CMC_AND_POINT_PSTN,
                                      PID_MIN_CELL_VOLT_CMC_AND_POINT_PSTN,
                                      PID_AVG_CELL_VOLTAGE,
                                      PID_MAX_CELL_VOLTAGE,
                                      PID_MIN_CELL_VOLTAGE,
                                      PID_CELL_VOLT_VALID,
                                      PID_PV_POS_CONTACTOR_AGING,
                                      PID_PR_NEG_CONTACTOR_AGING,
                                      PID_TIME_STAMP,
                                      PID_VEHICLE_SPEED,
                                      PID_ST_MIN,
                                      PID_APPLICATION_SOFTWARE_FINGERPRINT,
                                      PID_VEHICLE_IDENTIFICATION_NUMBER};
};

#endif
