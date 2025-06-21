#ifndef FOXESS_CAN_H
#define FOXESS_CAN_H
#include "../include.h"

#include "CanInverterProtocol.h"

#ifdef FOXESS_CAN
#define SELECTED_INVERTER_CLASS FoxessCanInverter
#endif

class FoxessCanInverter : public CanInverterProtocol {
 public:
  void setup();
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr char* Name = "FoxESS compatible HV2600/ECS4100 battery";

 private:
  int16_t temperature_average = 0;
  uint16_t voltage_per_pack = 0;
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
                           .ID = 0x1881,
                           .data = {0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}};
  CAN_frame FOXESS_1882 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1882,
                           .data = {0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}};
  CAN_frame FOXESS_1883 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1883,
                           .data = {0x10, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07}};

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
  CAN_frame FOXESS_0C1D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C1D,                                               //Cell 1-4
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C21 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C21,                                               //Cell 5-8
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C25 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C25,                                               //Cell 9-12
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C29 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C29,                                               //Cell 13-16
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C2D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C2D,                                               //Cell 17-20
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C31 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C31,                                               //Cell 21-24
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C35 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C35,                                               //Cell 25-28
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C39 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C39,                                               //Cell 29-32
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C3D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C3D,                                               //Cell 33-36
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C41 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C41,                                               //Cell 37-40
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C45 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C45,                                               //Cell 41-44
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C49 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C49,                                               //Cell 45-48
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C4D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C4D,                                               //Cell 49-52
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C51 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C51,                                               //Cell 53-56
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C55 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C55,                                               //Cell 57-60
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C59 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C59,                                               //Cell 61-64
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C5D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C5D,                                               //Cell 65-68
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C61 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C61,                                               //Cell 69-72
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C65 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C65,                                               //Cell 73-76
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C69 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C69,                                               //Cell 77-80
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C6D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C6D,                                               //Cell 81-84
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C71 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C71,                                               //Cell 85-88
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C75 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C75,                                               //Cell 89-92
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C79 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C79,                                               //Cell 93-96
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C7D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C7D,                                               //Cell 97-100
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C81 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C81,                                               //Cell 101-104
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C85 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C85,                                               //Cell 105-108
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C89 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C89,                                               //Cell 109-112
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C8D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C8D,                                               //Cell 113-116
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C91 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C91,                                               //Cell 117-120
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C95 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C95,                                               //Cell 121-124
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C99 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C99,                                               //Cell 125-128
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0C9D = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0C9D,                                               //Cell 129-132
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0CA1 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0CA1,                                               //Cell 133-136
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0CA5 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0CA5,                                               //Cell 137-140
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV
  CAN_frame FOXESS_0CA9 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0CA9,                                               //Cell 141-144
                           .data = {0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C, 0xE4, 0x0C}};  //All cells init to 3300mV

  // Temperatures
  CAN_frame FOXESS_0D21 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D21,  //Celltemperatures Pack 1
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D29 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D29,  //Celltemperatures Pack 2
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D31 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D31,  //Celltemperatures Pack 3
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D39 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D39,  //Celltemperatures Pack 4
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D41 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D41,  //Celltemperatures Pack 5
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D49 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D49,  //Celltemperatures Pack 6
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D51 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D51,  //Celltemperatures Pack 7
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
  CAN_frame FOXESS_0D59 = {.FD = false,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x0D59,  //Celltemperatures Pack 8
                           .data = {0x49, 0x48, 0x47, 0x47, 0x48, 0x49, 0x46, 0x47}};
};

#endif
