#ifndef GEELY_GEOMETRY_C_BATTERY_H
#define GEELY_GEOMETRY_C_BATTERY_H
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../include.h"
#include "CanBattery.h"
#include "GEELY-GEOMETRY-C-HTML.h"

#ifdef GEELY_GEOMETRY_C_BATTERY
#define SELECTED_BATTERY_CLASS GeelyGeometryCBattery
#endif

class GeelyGeometryCBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "Geely Geometry C";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  static const int POLL_SOC = 0x4B35;
  static const int POLL_CC2_VOLTAGE = 0x4BCF;
  static const int POLL_CELL_MAX_VOLTAGE_NUMBER = 0x4B1E;
  static const int POLL_CELL_MIN_VOLTAGE_NUMBER = 0x4B20;
  static const int POLL_AMOUNT_CELLS = 0x4B07;
  static const int POLL_SPECIFICIAL_VOLTAGE = 0x4B05;
  static const int POLL_UNKNOWN_1 = 0x4BDA;  //245 on two batteries
  static const int POLL_RAW_SOC_MAX = 0x4BC3;
  static const int POLL_RAW_SOC_MIN = 0x4BC4;
  static const int POLL_UNKNOWN_4 = 0xDF00;  //144 (the other battery 143)
  static const int POLL_CAPACITY_MODULE_MAX = 0x4B3D;
  static const int POLL_CAPACITY_MODULE_MIN = 0x4B3E;
  static const int POLL_UNKNOWN_7 = 0x4B24;  //1 (the other battery 23)
  static const int POLL_UNKNOWN_8 = 0x4B26;  //16 (the other battery 33)
  static const int POLL_MULTI_TEMPS = 0x4B0F;
  static const int POLL_MULTI_UNKNOWN_2 = 0x4B10;
  static const int POLL_MULTI_UNKNOWN_3 = 0x4B53;
  static const int POLL_MULTI_UNKNOWN_4 = 0x4B54;
  static const int POLL_MULTI_HARDWARE_VERSION = 0x4B6B;
  static const int POLL_MULTI_SOFTWARE_VERSION = 0x4B6C;

  GeelyGeometryCHtmlRenderer renderer;

  static const int MAX_PACK_VOLTAGE_70_DV = 4420;  //70kWh
  static const int MIN_PACK_VOLTAGE_70_DV = 2860;
  static const int MAX_PACK_VOLTAGE_53_DV = 4160;  //53kWh
  static const int MIN_PACK_VOLTAGE_53_DV = 2700;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_GEELY_GEOMETRY_C* datalayer_geometryc;

  CAN_frame GEELY_191 = {.FD = false,  //PAS_APA_Status , 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x191,
                         .data = {0x00, 0x00, 0x81, 0x20, 0x00, 0x00, 0x00, 0x01}};
  CAN_frame GEELY_2D2 = {.FD = false,  //DSCU 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x2D2,
                         .data = {0x60, 0x8E, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_0A6 = {.FD = false,  //VCU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x0A6,
                         .data = {0xFA, 0x0F, 0xA0, 0x00, 0x00, 0xFA, 0x00, 0xE4}};
  CAN_frame GEELY_160 = {.FD = false,  //VCU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x160,
                         .data = {0x00, 0x01, 0x67, 0xF7, 0xC0, 0x19, 0x00, 0x20}};
  CAN_frame GEELY_165 = {.FD = false,  //VCU_ModeControl 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x165,
                         .data = {0x00, 0x81, 0xA1, 0x00, 0x00, 0x1E, 0x00, 0xD6}};
  CAN_frame GEELY_1A4 = {.FD = false,  //VCU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1A4,
                         .data = {0x17, 0x73, 0x17, 0x70, 0x02, 0x1C, 0x00, 0x56}};
  CAN_frame GEELY_162 = {.FD = false,  //VCU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x162,
                         .data = {0x00, 0x05, 0x06, 0x81, 0x00, 0x09, 0x00, 0xC6}};
  CAN_frame GEELY_1A5 = {.FD = false,  //VCU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1A5,
                         .data = {0x17, 0x70, 0x24, 0x0B, 0x00, 0x00, 0x00, 0xF9}};
  CAN_frame GEELY_1B2 = {.FD = false,  //??? 50ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1B2,
                         .data = {0x17, 0x70, 0x24, 0x0B, 0x00, 0x00, 0x00, 0xF9}};
  CAN_frame GEELY_221 = {.FD = false,  //OBC 50ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x221,
                         .data = {0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0x00}};
  CAN_frame GEELY_220 = {.FD = false,  //OBC 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x220,
                         .data = {0x0B, 0x43, 0x69, 0xF3, 0x3A, 0x10, 0x00, 0x31}};
  CAN_frame GEELY_1A3 = {.FD = false,  //FRS 50ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1A3,
                         .data = {0xFF, 0x18, 0x20, 0x00, 0x00, 0x00, 0x00, 0x4F}};
  CAN_frame GEELY_1A7 = {.FD = false,  //??? 50ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1A7,
                         .data = {0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_0A8 = {.FD = false,  //IPU 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x0A8,
                         .data = {0x00, 0x2E, 0xDC, 0x4E, 0x20, 0x00, 0x20, 0xA2}};
  CAN_frame GEELY_1F2 = {.FD = false,  //??? 50ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1F2,
                         .data = {0x9B, 0xA3, 0x99, 0xA2, 0x41, 0x42, 0x41, 0x42}};
  CAN_frame GEELY_222 = {.FD = false,  //OBC 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x222,
                         .data = {0x00, 0x00, 0x00, 0xFF, 0xF8, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_1A6 = {.FD = false,  //OBC 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x1A6,
                         .data = {0x00, 0x7F, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_145 = {.FD = false,  //EGSM 20ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x145,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A}};
  CAN_frame GEELY_0E0 = {.FD = false,  //IPU 10ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x0E0,
                         .data = {0xFF, 0x09, 0x00, 0xE0, 0x00, 0x8F, 0x00, 0x00}};
  CAN_frame GEELY_0F9 = {.FD = false,  //??? 20ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x0F9,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_292 = {.FD = false,  //T-BOX 100ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x292,
                         .data = {0x00, 0x00, 0x00, 0x1F, 0xE7, 0xE7, 0x00, 0xBC}};
  CAN_frame GEELY_0FA = {.FD = false,  //??? 20ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x0FA,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_197 = {.FD = false,  //??? 20ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x197,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6A}};
  CAN_frame GEELY_150 = {.FD = false,  //EPS 20ms
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x150,
                         .data = {0x7E, 0x00, 0x24, 0x00, 0x01, 0x01, 0x00, 0xA9}};
  CAN_frame GEELY_POLL = {.FD = false,  //Polling frame
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x7E2,
                          .data = {0x03, 0x22, 0x4B, 0xDA, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame GEELY_ACK = {.FD = false,  //Ack frame
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x7E2,
                         .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  uint16_t poll_pid = POLL_SOC;
  uint16_t incoming_poll = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_100ms = 0;
  unsigned long previousMillis10 = 0;   // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis20 = 0;   // will store last time a 20ms CAN Message was sent
  unsigned long previousMillis50 = 0;   // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was sent
  uint8_t mux = 0;
  uint16_t battery_voltage = 3700;
  int16_t maximum_temperature = 0;
  int16_t minimum_temperature = 0;
  uint8_t HVIL_signal = 0;
  uint8_t serialnumbers[28] = {0};
  uint16_t maximum_cell_voltage = 3700;
  uint16_t discharge_power_allowed = 0;
  uint16_t poll_soc = 0;
  uint16_t poll_cc2_voltage = 0;
  uint16_t poll_cell_max_voltage_number = 0;
  uint16_t poll_cell_min_voltage_number = 0;
  uint16_t poll_amount_cells = 0;
  uint16_t poll_specificial_voltage = 0;
  uint16_t poll_unknown1 = 0;
  uint16_t poll_raw_soc_max = 0;
  uint16_t poll_raw_soc_min = 0;
  uint16_t poll_unknown4 = 0;
  uint16_t poll_cap_module_max = 0;
  uint16_t poll_cap_module_min = 0;
  uint16_t poll_unknown7 = 0;
  uint16_t poll_unknown8 = 0;
  int16_t poll_temperature[6] = {0};
  static const int TEMP_OFFSET = 30;  //TODO, not calibrated yet, best guess
  uint8_t poll_software_version[16] = {0};
  uint8_t poll_hardware_version[16] = {0};
};

#endif
