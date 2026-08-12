#ifndef FOXESS_CAN_H
#define FOXESS_CAN_H

#include "CanInverterProtocol.h"
#include "INVERTERS.h"

class FoxessCanInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  bool setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "FoxESS compatible HV2600/ECS4100 battery";

 private:
  static const int FIRMWARE_VERSION_MAIN_BMS = 0x12;
  static const int FIRMWARE_VERSION_SUBSTACKS = 0x1F;
  //for the PACK_ID (b7 =10,20,30,40,50,60,70,80) then FIRMWARE_VERSION 0x1F = 0001 1111, version is v1.15, and if FIRMWARE_VERSION was 0x20 = 0010 0000 then = v2.0
  static const int MAIN = 0;                       //Main BMS has ID 0
  static const int DEFAULT_NUMBER_OF_MODULES = 8;  //Configurable for user, 1-8 modules, default is 8
  static const int DEFAULT_BATTERY_TYPE = 0x52;    //0x52 is HV2600 V2 BMS main controlled
  static const int DEFAULT_BATTERY_SUBTYPE =
      0x84;  //0x82 is HV2600 V1, 0x83 is ECS4100 v1, 0x84 is HV2600 V2 , the attached sub-stacks
  uint16_t configured_number_of_modules = 0;
  uint16_t configured_battery_type = 0;
  uint16_t configured_battery_subtype = 0;
  static const int MAX_AC_VOLTAGE = 2567;              //256.7VAC max
  static const int TOTAL_LIFETIME_WH_ACCUMULATED = 0;  //We dont have this value in the emulator
  static const uint8_t STATUS_OPERATIONAL_PACKS =
      0b11111111;  //0x1875 b2 contains status for operational packs (responding) in binary so 01111111 is pack 8 not operational, 11101101 is pack 5 & 2 not operational

  int16_t temperature_average = 0;
  uint16_t voltage_per_pack = 0;
  uint16_t cell_tweaked_max_voltage_mV = 3300;
  uint16_t cell_tweaked_min_voltage_mV = 3300;
  int16_t current_per_pack = 0;
  uint8_t temperature_max_per_pack = 0;
  uint8_t temperature_min_per_pack = 0;
  uint8_t current_pack_info = 0;

  // Batch send of CAN message variables
  const uint8_t delay_between_batches_ms = 10;  //TODO, tweak to as low as possible before performance issues appear
  bool send_bms_info = false;
  bool send_individual_pack_status = false;
  bool send_serial_numbers = false;
  bool send_cellvoltages = false;
  unsigned long currentMillis = 0;
  unsigned long previousMillisCellvoltage = 0;
  unsigned long previousMillisSerialNumber = 0;
  unsigned long previousMillisBMSinfo = 0;
  unsigned long previousMillisIndividualPacks = 0;
  uint8_t can_message_cellvolt_index = 0;
  uint8_t can_message_serial_index = 0;
  uint8_t can_message_individualpack_index = 0;
  uint8_t can_message_bms_index = 0;

  //CAN message translations from this amazing repository: https://github.com/rand12345/FOXESS_can_bus

  CAN_frame FOXESS_1872 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1872,
                           .data = {0x40, 0x12, 0x80, 0x0C, 0xCD, 0x00, 0xF4, 0x01}};  //BMS_Limits
  CAN_frame FOXESS_1873 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1873,
                           .data = {0xA3, 0x10, 0x0D, 0x00, 0x5D, 0x00, 0x77, 0x07}};  //BMS_PackData
  CAN_frame FOXESS_1874 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1874,
                           .data = {0xA3, 0x10, 0x0D, 0x00, 0x5D, 0x00, 0x77, 0x07}};  //BMS_CellData
  CAN_frame FOXESS_1875 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1875,
                           .data = {0xF9, 0x00, 0xFF, 0x08, 0x01, 0x00, 0x8E, 0x00}};  //BMS_Status
  CAN_frame FOXESS_1876 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1876,
                           .data = {0x01, 0x00, 0x07, 0x0D, 0x0, 0x0, 0xFE, 0x0C}};  //BMS_PackTemps
  CAN_frame FOXESS_1877 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1877,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x82, 0x00, 0x20, 0x50}};  //BMS_Unk1
  CAN_frame FOXESS_1878 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1878,
                           .data = {0x07, 0x0A, 0x00, 0x00, 0xD0, 0xFF, 0x4E, 0x00}};  //BMS_PackStats
  CAN_frame FOXESS_1879 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1879,
                           .data = {0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //BMS_Unk2
  CAN_frame FOXESS_1881 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1881,  //First byte specifies which pack the serial number is for, 0-7
                           .data = {0x00, '6', '0', 'E', 'P', '0', '0', '5'}};
  CAN_frame FOXESS_1882 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1882,  //First byte specifies which pack the serial number is for, 0-7
                           .data = {0x00, '0', '4', '7', 'M', 'A', '0', '5'}};
  CAN_frame FOXESS_1883 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1883,  //First byte specifies which pack the serial number is for, 0-7
                           .data = {0x00, '2', '\0', '\0', '\0', '\0', '\0', '\0'}};
  CAN_frame FOXESS_0C05 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C05,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C06 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C06,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C07 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C07,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C08 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C08,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C09 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C09,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C0A = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C0A,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C0B = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C0B,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  CAN_frame FOXESS_0C0C = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C0C,
                           .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xD0, 0xD0}};
  // Cellvoltages
  CAN_frame FOXESS_CELLVOLTAGES = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x0C1D,                                               //Cell XXX
      .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  // Temperatures
  CAN_frame FOXESS_CELLTEMPERATURES = {.FD = false,
                                       .ext_ID = true,
                                       .DLC = 8,
                                       .ID = 0x0D21,  //Celltemperatures Pack X
                                       .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
};

#endif
