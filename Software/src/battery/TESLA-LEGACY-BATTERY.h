#ifndef TESLA_LEGACY_BATTERY_H
#define TESLA_LEGACY_BATTERY_H
#include "CanBattery.h"

class TeslaLegacyBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Tesla Model S/X 2012-2020";

 private:
  static const int MAX_PACK_VOLTAGE_60_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_60_DV = 3000;
  static const int MAX_PACK_VOLTAGE_70_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_70_DV = 3000;
  static const int MAX_PACK_VOLTAGE_75_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_75_DV = 3000;
  static const int MAX_PACK_VOLTAGE_85_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_85_DV = 3000;
  static const int MAX_PACK_VOLTAGE_90_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_90_DV = 3000;
  static const int MAX_PACK_VOLTAGE_100_DV = 5000;  //TODO, set
  static const int MIN_PACK_VOLTAGE_100_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4200;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 3400;  //Battery is put into emergency stop if one cell goes below this value
  CAN_frame TESLA_25C = {.FD = false,           // fast charge status
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x25C,
                         .data = {0x00, 0x02, 0x2A, 0x09, 0x40, 0xC7, 0x72, 0x81}};
  CAN_frame TESLA_2C8 = {.FD = false,  // GTW status
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2C8,
                         .data = {0x6F, 0xE8, 0x13, 0x71, 0x1D, 0x24, 0x80, 0x7B}};
  CAN_frame TESLA_21C = {.FD = false,  // charger status
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x21C,
                         .data = {0x31, 0x58, 0x20, 0x89, 0x8C, 0x08, 0x03, 0x08}};
  CAN_frame TESLA_20E = {.FD = false,  // charge port status
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x20E,
                         .data = {0x05, 0x56, 0x22, 0x00, 0xC3, 0x00, 0x02, 0x08}};
  // Keep alive
  CAN_frame TESLA_408 = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x408, .data = {0x00}};
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was send
  uint32_t BMS_CAC_min = 23160000;
  uint16_t battery_cell_max_v = 3300;
  uint16_t battery_cell_min_v = 3300;
  uint16_t battery_soc_ui = 0;
  uint8_t battery_BMS_state = 0;
  bool cellvoltagesRead = false;
  //0x102: BMS_hvBusStatus
  uint16_t battery_volts = 0;
  int16_t battery_amps = 0;
  //0x2d2: 722 BMSVAlimits
  uint16_t battery_max_discharge_current = 0;
  uint16_t battery_max_charge_current = 0;
  uint16_t battery_bms_max_voltage = 0;
  uint16_t battery_bms_min_voltage = 0;
  //0x3d2: 978 BMS_kwhCounter
  uint32_t battery_total_discharge = 0;
  uint32_t battery_total_charge = 0;
  //0x332: 818 BattBrickMinMax:BMS_bmbMinMax
  int16_t battery_max_temp = 0;  // C*
  int16_t battery_min_temp = 0;  // C*
  uint16_t battery_BrickVoltageMax = 0;
  uint16_t battery_BrickVoltageMin = 0;
  uint8_t battery_BrickTempMaxNum = 0;
  uint8_t battery_BrickTempMinNum = 0;
  uint8_t battery_BrickModelTMax = 0;
  uint8_t battery_BrickModelTMin = 0;
  uint8_t battery_BrickVoltageMaxNum = 0;
  uint8_t battery_BrickVoltageMinNum = 0;
  //0x5D2
  uint8_t battery_hwID = 0;
  //0x212: 530 BMS_status
  // Nieuwe variablen voor Classic:
  bool battery_BMS_rapidDCLinkDchgRequest = false;
  bool battery_BMS_chargingActiveOrTrans = false;
  bool battery_BMS_dcdcEnableOn = false;
  bool battery_BMS_hvilStatus = false;
  bool battery_BMS_okToShipByAir = false;
  bool battery_BMS_okToShipByLand = false;
  bool battery_BMS_activeHeatingWorthwhile = false;
  bool battery_BMS_notEnoughPowerForSupport = false;
  bool battery_BMS_hvacPowerRequest = false;
  bool battery_BMS_notEnoughPowerForDrive = false;
  bool battery_BMS_hvilOn = false;
  uint8_t battery_BMS_highestFaultCategory = 0;
  uint8_t battery_BMS_contactorStateFC = 0;
  uint8_t battery_BMS_cpChargeStatus = 0;
  uint16_t battery_BMS_isolationResistance = 0;
  uint8_t battery_BMS_contactorState = 0;
};

#endif
