#ifndef FOXESS_EP_CAN_H
#define FOXESS_EP_CAN_H

#include "CanInverterProtocol.h"
#include "INVERTERS.h"

class FoxessEpCanInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  bool setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "FoxESS EP-Series battery";

 private:
  void transmit_cell_voltage_frame(uint32_t frame_id, uint16_t first_cell_index);

  void transmit_temperature_frame(uint32_t frame_id, uint8_t first_virtual_sensor_index);

  // EP controller and single virtual-unit identity.
  static const int FIRMWARE_VERSION_MAIN_BMS = 0x0C;
  static const int FIRMWARE_VERSION_SUBSTACKS = 0x1C;
  static const int MAIN = 0;
  static const int DEFAULT_NUMBER_OF_MODULES = 1;
  static const int DEFAULT_BATTERY_TYPE = 0x6E;
  static const int DEFAULT_BATTERY_SUBTYPE = 0xFF;
  static const uint8_t STATUS_OPERATIONAL_PACKS = 0x01;

  uint16_t configured_number_of_modules = 0;
  uint16_t configured_battery_type = 0;
  uint16_t configured_battery_subtype = 0;

  int16_t temperature_average = 0;
  uint8_t temperature_max_per_pack = 0;
  uint8_t temperature_min_per_pack = 0;
  uint8_t current_pack_info = 0;
  // Installation energy accounting measured by Battery-Emulator.
  // Charged and discharged counters are the authoritative paired totals.
  // The throughput counter remains temporarily for 0x1878 compatibility.
  uint64_t foxess_throughput_energy_Wh = 0ULL;
  uint64_t foxess_installation_charged_energy_Wh = 0ULL;
  uint64_t foxess_installation_discharged_energy_Wh = 0ULL;
  uint64_t foxess_charged_energy_remainder = 0;
  uint64_t foxess_discharged_energy_remainder = 0;
  uint64_t foxess_charged_capacity_dAh = 0ULL;
  uint64_t foxess_discharged_capacity_dAh = 0ULL;
  uint64_t foxess_charged_capacity_remainder_dAms = 0ULL;
  uint64_t foxess_discharged_capacity_remainder_dAms = 0ULL;
  unsigned long foxess_previous_energy_millis = 0;
  bool foxess_energy_counter_initialised = false;

  // Batch send of CAN message variables
  const uint8_t delay_between_batches_ms = 10;
  bool send_bms_info = false;
  bool send_individual_pack_status = false;
  bool send_serial_numbers = false;
  bool send_cellvoltages = false;
  bool send_celltemperatures = false;
  unsigned long previousMillisCellvoltage = 0;
  unsigned long previousMillisCelltemperature = 0;
  unsigned long previousMillisSerialNumber = 0;
  unsigned long previousMillisBMSinfo = 0;
  unsigned long previousMillisIndividualPacks = 0;
  uint8_t can_message_cellvolt_index = 0;
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
                           .data = {0x00, 0x00, 0x00, 0x00, 0x82, 0x00, 0x20, 0x50}};  //BMS_ErrorsBrand
  CAN_frame FOXESS_1878 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1878,
                           .data = {0x07, 0x0A, 0x00, 0x00, 0xD0, 0xFF, 0x4E, 0x00}};  //BMS_PackStats
  CAN_frame FOXESS_1879 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1879,
                           .data = {0x00, 0x35, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Reserved EP field
  // Charged and discharged installation energy - 0x187A.
  CAN_frame FOXESS_187A = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x187A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FOXESS_187B = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x187B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //BMS_ExtendedData
  CAN_frame FOXESS_187F = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x187F,
                           .data = {0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame FOXESS_1900 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1900,
                           .data = {0x2C, 0x01, 0xA0, 0x0F, 0x00, 0x00, 0x3C, 0x46}};
  CAN_frame FOXESS_1901 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1901,
                           .data = {0xF0, 0x0F, 0x00, 0x00, 0x40, 0x00, 0x33, 0x42}};
  CAN_frame FOXESS_1902 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1902,
      .data = {0},
  };
  CAN_frame FOXESS_1903 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1903,
      .data = {0},
  };
  CAN_frame FOXESS_1904 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1904,
      .data = {0},
  };
  CAN_frame FOXESS_1905 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1905,
      .data = {0},
  };
  CAN_frame FOXESS_1906 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1906,
      .data = {0x51, 0x00, 0x00, 0x00, 0xCA, 0x03, 0x00, 0x00},
  };
  CAN_frame FOXESS_1907 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1907,
      .data = {0},
  };
  CAN_frame FOXESS_1908 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1908,
      .data = {0},
  };
  CAN_frame FOXESS_1909 = {
      .FD = false,
      .ext_ID = true,
      .DLC = 8,
      .ID = 0x1909,
      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
  };
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
