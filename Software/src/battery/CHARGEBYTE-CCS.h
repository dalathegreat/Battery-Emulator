#ifndef CHARGEBYTE_CCS_BATTERY_H
#define CHARGEBYTE_CCS_BATTERY_H
#include "../devboard/hal/hal.h"
#include "CanBattery.h"

#include "driver/i2c_master.h"

class ChargebyteCCSBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_precharge(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Chargebyte CCS V2X";

 private:
  void dump_data();
  void setPrechargeVoltage(uint16_t value, bool writeEEPROM = false);

  static constexpr const char* EVSE_ID = "DEDIYE42";
  static const int EVSE_MIN_VOLTAGE = 1500;  // in dV
  static const int EVSE_MAX_VOLTAGE = 4500;  // in dV
  static const int EVSE_MAX_CURRENT = 320;   // in dA
  static const int EVSE_MAX_POWER = 100;     // in 100W
  static const int MIN_PACK_VOLTAGE_DV = 3000;

  // the following configures pins for precharging the vehicle side
  // a MCP4725 is used for generating a voltage 0-3.3V
  // a QC05-5C is used for converting 0-3.3V to 0-500V
  static const int PRECHARGE_DAC_SDA = 13;        // pin 13 for SDA of precharge MCP4725
  static const int PRECHARGE_DAC_SCL = 19;        // pin 19 for SCL of precharge MCP4725
  static const int PRECHARGE_DAC_ADDRESS = 0x61;  // I2C address of precharge MCP4725

  // the following configures pins for precharging the inverter side
  // battery voltage and two DC contactors are used. negative is connected all the time
  // we differ from the official STARK CMR pins
  virtual gpio_num_t CCS_PRECHARGE_CONTACTOR_PIN() { return esp32hal->POSITIVE_CONTACTOR_PIN(); }
  virtual gpio_num_t CCS_MAIN_CONCTACTOR_PIN() { return esp32hal->NEGATIVE_CONTACTOR_PIN(); }
  virtual gpio_num_t CCS_CP_PP_DISCONNECT_PIN() { return esp32hal->BMS_POWER(); }

  CAN_frame CHARGEBYTE_307 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x307,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CHARGEBYTE_306 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x306,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CHARGEBYTE_305 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x305,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CHARGEBYTE_304 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x304,
                              .data = {
                                  EVSE_ID[0],
                                  EVSE_ID[1],
                                  EVSE_ID[2],
                                  EVSE_ID[3],
                                  EVSE_ID[4],
                                  EVSE_ID[5],
                                  EVSE_ID[6],
                                  EVSE_ID[7],
                              }};
  CAN_frame CHARGEBYTE_303 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x303,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CHARGEBYTE_302 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x302,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x09, 0x01, 0x00, 0x00}};
  CAN_frame CHARGEBYTE_301 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x301,
                              .data = {
                                  0x00,
                                  0x00,
                                  EVSE_MIN_VOLTAGE & 0xff,
                                  EVSE_MIN_VOLTAGE >> 8,
                                  0x32,
                                  0x32,
                                  0x00,
                                  0x00,
                              }};
  CAN_frame CHARGEBYTE_300 = {.FD = false,
                              .ext_ID = true,
                              .DLC = 8,
                              .ID = 0x300,
                              .data = {
                                  EVSE_MAX_CURRENT & 0xff,
                                  EVSE_MAX_CURRENT >> 8,
                                  EVSE_MAX_VOLTAGE & 0xff,
                                  EVSE_MAX_VOLTAGE >> 8,
                                  EVSE_MAX_POWER & 0xff,
                                  EVSE_MAX_POWER >> 8,
                                  0xE8,
                                  0x03,
                              }};

  bool hasChargebyteError = false;
  bool hasLowLevelError = false;
  bool inPrecharge = false;
  bool inCharge = false;
  uint16_t precharge_dV = 0;
  uint16_t targetVoltage_dV = 0;
  uint16_t targetCurrent_dA = 0;
  uint16_t maxVoltage_dV = 0;
  uint16_t maxCurrent_dA = 0;
  uint32_t maxPower_W = 0;
  uint16_t soc = 0;
  uint32_t energyCapacity_Wh = 20000;

  i2c_master_bus_handle_t prechargeI2CBus = nullptr;
  i2c_master_dev_handle_t prechargeDacDev = nullptr;
};

#endif
