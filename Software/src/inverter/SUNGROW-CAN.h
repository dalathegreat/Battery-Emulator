#ifndef SUNGROW_CAN_H
#define SUNGROW_CAN_H

#include "CanInverterProtocol.h"

class SungrowInverter : public CanInverterProtocol {
 public:
  const char* name() override { return Name; }
  // Constructor: request 250 kbps on the inverter CAN interface
  SungrowInverter() : CanInverterProtocol(CAN_Speed::CAN_SPEED_250KBPS) {}
  void update_values();
  void transmit_can(unsigned long currentMillis);
  void map_can_frame_to_variable(CAN_frame rx_frame);
  static constexpr const char* Name = "Sungrow SBRXXX emulation over CAN bus";

 private:
  unsigned long previousMillisBatch = 0;
  unsigned long previousMillis1s = 0;
  unsigned long previousMillis10s = 0;
  unsigned long previousMillis60s = 0;
  bool transmit_can_init = true;
  const uint8_t delay_between_batches_ms = INTERVAL_20_MS;

  uint8_t mux = 0;
  uint8_t version_char[14] = {0};
  uint8_t manufacturer_char[14] = {0};
  uint8_t model_char[14] = {0};
  uint32_t remaining_wh = 0;
  uint32_t capacity_wh = 0;
  uint8_t batch_send_index = 0;
  static constexpr uint16_t NAMEPLATE_WH = 9600;

  // Cached signed current in deci-amps (range: int16_t)
  int16_t current_dA = 0;

  // Clamp an int32 -> int16 safely
  static constexpr int16_t clamp_i32_to_i16(int32_t value) {
    if (value > 32767) {
      return 32767;
    }

    if (value < -32768) {
      return -32768;
    }

    return static_cast<int16_t>(value);
  }

  //Actual content messages
  CAN_frame SUNGROW_000 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x000,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_001 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x001,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_002 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x002,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_003 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x003,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_004 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x004,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_005 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x005,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_006 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x006,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_007 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x007,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_008_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x008,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_008_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x008,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_009 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x009,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00A_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x00A,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00A_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x00A,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_00E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x00E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_013 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x013,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_014 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x014,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_015 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x015,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_016 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x016,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_017 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x017,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_018 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x018,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_019 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x019,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01C = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01C,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_01E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x01E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_1E0_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x1E0,
                              .data = {0x01, 0x04, 0x04, 0x01, 0xF4, 0x00, 0x00, 0xBB}};
  CAN_frame SUNGROW_1E0_01 = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x1E0, .data = {0x8A}};
  CAN_frame SUNGROW_400 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x400,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_401 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x401,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_500 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x500,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_501 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x501,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_502 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x502,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_503 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x503,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_504 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x504,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_505 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x505,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_506 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x506,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_512 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x512,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_700 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x700,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_701 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x701,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_702 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x702,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_703 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x703,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_704 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x704,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_705 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x705,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_706 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x706,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_707 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x707,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_708_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x708,
                              .data = {0x00, 0x53, 0x32, 0x33, 0x31, 0x30, 0x31, 0x32}};  // "S231012"
  CAN_frame SUNGROW_708_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x708,
                              .data = {0x01, 0x33, 0x34, 0x35, 0x36, 0x00, 0x00, 0x53}};  // "3456" S suffix
  CAN_frame SUNGROW_709 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x709,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70A_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70A,
                              .data = {0x00, 0x53, 0x55, 0x4E, 0x47, 0x52, 0x4F, 0x57}};
  CAN_frame SUNGROW_70A_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70A,
                              .data = {0x01, 0x00, 0x42, 0x52, 0x58, 0x58, 0x58, 0x00}};
  CAN_frame SUNGROW_70B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70B,
                           .data = {0x00, 0x53, 0x42, 0x52, 0x58, 0x58, 0x58, 0x00}};
  CAN_frame SUNGROW_70D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70D,
                           .data = {0x0F, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x70E,
                           .data = {0x07, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_00 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_01 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_02 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_03 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_04 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_05 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_06 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_70F_07 = {.FD = false,
                              .ext_ID = false,
                              .DLC = 8,
                              .ID = 0x70F,
                              .data = {0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_713 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x713,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_714 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x714,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_715 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x715,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_716 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x716,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_717 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x717,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_718 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x718,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_719 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x719,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71A = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71A,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71B,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71C = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71C,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71D = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71D,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71E = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x71E,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame SUNGROW_71F_01_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x01, 0x45, 0x4D, 0x30, 0x33, 0x32, 0x44}};  // "EM032D"
  CAN_frame SUNGROW_71F_01_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x02, 0x32, 0x33, 0x31, 0x30, 0x31, 0x32}};  // "231012"
  CAN_frame SUNGROW_71F_01_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x01, 0x03, 0x33, 0x34, 0x36, 0x31, 0x44, 0x46}};  // "3461DF"
  CAN_frame SUNGROW_71F_02_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x01, 0x45, 0x4D, 0x30, 0x33, 0x32, 0x44}};  // "EM032D"
  CAN_frame SUNGROW_71F_02_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x02, 0x32, 0x33, 0x31, 0x30, 0x31, 0x32}};  // "231012"
  CAN_frame SUNGROW_71F_02_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x02, 0x03, 0x33, 0x34, 0x36, 0x32, 0x44, 0x46}};  // "3462DF"
  CAN_frame SUNGROW_71F_03_01 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x01, 0x45, 0x4D, 0x30, 0x33, 0x32, 0x44}};  // "EM032D"
  CAN_frame SUNGROW_71F_03_02 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x02, 0x32, 0x33, 0x31, 0x30, 0x31, 0x32}};  // "231012"
  CAN_frame SUNGROW_71F_03_03 = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x71F,
                                 .data = {0x03, 0x03, 0x33, 0x34, 0x36, 0x33, 0x44, 0x46}};  // "3463DF"
};

#endif
