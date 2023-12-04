#include "BMW-I3-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

//TODO: before using
// Map the final values in update_values_i3_battery, set some to static values if not available (current, discharge max , charge max)
// Possible future improvements: Better 13E handling , 2B7 , 2E2 , 2B3, 3A4, 59A 55E 3E4 3C2

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis10 = 0;     // will store last time a 10ms CAN Message was send
static unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
static unsigned long previousMillis30 = 0;     // will store last time a 30ms CAN Message was send
static unsigned long previousMillis40 = 0;     // will store last time a 40ms CAN Message was send
static unsigned long previousMillis50 = 0;     // will store last time a 50ms CAN Message was send
static unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
static unsigned long previousMillis600 = 0;    // will store last time a 600ms CAN Message was send
static unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
static unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
static unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send
static const int interval10 = 10;              // interval (ms) at which send CAN Messages
static const int interval20 = 20;              // interval (ms) at which send CAN Messages
static const int interval30 = 30;              // interval (ms) at which send CAN Messages
static const int interval40 = 40;              // interval (ms) at which send CAN Messages
static const int interval50 = 50;              // interval (ms) at which send CAN Messages
static const int interval100 = 100;            // interval (ms) at which send CAN Messages
static const int interval200 = 200;            // interval (ms) at which send CAN Messages
static const int interval500 = 500;            // interval (ms) at which send CAN Messages
static const int interval600 = 600;            // interval (ms) at which send CAN Messages
static const int interval1000 = 1000;          // interval (ms) at which send CAN Messages
static const int interval5000 = 5000;          // interval (ms) at which send CAN Messages
static const int interval10000 = 10000;        // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;             //counter for checking if CAN is still alive

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

CAN_frame_t BMW_AA = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xAA,
                      .data = {0x00, 0xFC, 0x00, 0x7C, 0xC0, 0x5D, 0xD0, 0xF7}};
CAN_frame_t BMW_AC = {.FIR = {.B =
                                  {
                                      .DLC = 2,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xAC,
                      .data = {0x00, 0xF0}};
CAN_frame_t BMW_AD = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xAD,
                      .data = {0xFF, 0xFF, 0x7F, 0x00, 0x00, 0x00, 0xF0, 0xFF}};
CAN_frame_t BMW_A5 = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xA5,
                      .data = {0x47, 0xF0, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF}};
CAN_frame_t BMW_BB = {.FIR = {.B =
                                  {
                                      .DLC = 3,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xBB,
                      .data = {0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_C3 = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xC3,
                      .data = {0x2E, 0x76, 0x00, 0x40, 0x00, 0x7D, 0xFF, 0xFF}};
CAN_frame_t BMW_D9 = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xD9,
                      .data = {0x76, 0xF6, 0x00, 0x10, 0x00, 0xF0, 0x7F, 0xC0}};
CAN_frame_t BMW_EF = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xEF,
                      .data = {0x3A, 0xF0, 0x00, 0x7D, 0x21, 0xFF, 0x7C, 0xFF}};
CAN_frame_t BMW_F3 = {.FIR = {.B =
                                  {
                                      .DLC = 8,
                                      .FF = CAN_frame_std,
                                  }},
                      .MsgID = 0xF3,
                      .data = {0x8B, 0x00, 0x00, 0xFF, 0xF0, 0xC0, 0xFF, 0xFF}};
CAN_frame_t BMW_100 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x100,
                       .data = {0x95, 0xF0, 0x7F, 0xC0, 0x5D, 0xD9, 0x86, 0x70}};
CAN_frame_t BMW_108 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x108,
                       .data = {0x00, 0x7D, 0xFF, 0xFF, 0x07, 0xF1, 0xFF, 0xFF}};
CAN_frame_t BMW_10B = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x10B,
                       .data = {0xCD, 0x01, 0xFC}};
CAN_frame_t BMW_10E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x10E,
                       .data = {0xFE, 0xE7, 0x7F, 0x00, 0x00, 0x7D, 0x00, 0xF0}};
CAN_frame_t BMW_13E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x13E,
                       .data = {0xFF, 0x2D, 0xFA, 0xFA, 0xFA, 0xFA, 0x00, 0x00}};
CAN_frame_t BMW_153 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x153,
                       .data = {0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xF0}};
CAN_frame_t BMW_15F = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x15F,
                       .data = {0x42, 0xF0, 0x00, 0x7D, 0x00, 0xEE, 0x00, 0x7D}};
CAN_frame_t BMW_197 = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x197,
                       .data = {0x64, 0xE0, 0x0E, 0xC0}};
CAN_frame_t BMW_19B = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x19B,
                       .data = {0x20, 0x40, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_211 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x211,
                       .data = {0x00, 0xFF, 0xFF, 0x44, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_2B3 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2B3,
                       .data = {0x00, 0x08, 0x08, 0x08, 0x0F, 0x21, 0x81, 0x05}};
CAN_frame_t BMW_2B7 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2B7,
                       .data = {0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_2CA = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2CA,
                       .data = {0x74, 0x75}};
CAN_frame_t BMW_2E2 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2E2,
                       .data = {0x4B, 0xDB, 0x7F, 0xB8, 0x57, 0x51, 0xFF, 0x00}};
CAN_frame_t BMW_2E3 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2E3,
                       .data = {0xFE, 0x00, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2E8 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2E8,
                       .data = {0xC0, 0xE8, 0xC3, 0xFF, 0xFF, 0xF4, 0xF0, 0x09}};
CAN_frame_t BMW_3C2 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3C2,
                       .data = {0x10, 0x83, 0x18, 0x86, 0x61, 0x18, 0x86, 0x61}};
CAN_frame_t BMW_32F = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x32F,
                       .data = {0x42, 0x44, 0x43, 0x00, 0x44, 0x44, 0x56, 0x92}};
CAN_frame_t BMW_326 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x326,
                       .data = {0x00, 0x00, 0xE0, 0x15, 0x00, 0x00, 0x00, 0x64}};
CAN_frame_t BMW_380 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x380,
                       .data = {0x56, 0x33, 0x31, 0x30, 0x30, 0x34, 0x32}};
CAN_frame_t BMW_3CA = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3CA,
                       .data = {0x78, 0x50, 0x18, 0x09, 0x09, 0x81, 0xFF, 0xFF}};
CAN_frame_t BMW_512 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x512,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};  //0x512 Network management edme VCU
CAN_frame_t BMW_59A = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x59A,
                       .data = {0x65, 0x02, 0xFF, 0x08, 0x00, 0x00, 0xFF, 0x03}};
CAN_frame_t BMW_03C = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x03C,
                       .data = {0xFF, 0x5F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_1AA = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x11A,
                       .data = {0xAC, 0x0D, 0x40, 0x06, 0x05, 0x8C, 0xF0, 0xFF}};
CAN_frame_t BMW_12F = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x12F,
                       .data = {0xC2, 0x50, 0x8A, 0xDD, 0xF4, 0x35, 0x30, 0xC1}};  //0x12F Wakeup VCU
CAN_frame_t BMW_13D = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x13D,
                       .data = {0xFF, 0xFF, 0x00, 0x60, 0x41, 0x16, 0xF4, 0xFF}};
CAN_frame_t BMW_140 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x140,
                       .data = {0x58, 0x60, 0x40, 0x9C, 0x00, 0x00, 0x94, 0xFF}};
CAN_frame_t BMW_141 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x141,
                       .data = {0x49, 0x70, 0x40, 0x9C, 0x00, 0x00, 0xF4, 0xFF}};
CAN_frame_t BMW_1A1 = {.FIR = {.B =
                                   {
                                       .DLC = 5,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1A1,
                       .data = {0x08, 0xC1, 0x00, 0x00, 0x81}};  //0x1A1 Vehicle speed
CAN_frame_t BMW_1D0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x1D0,
                       .data = {0x4D, 0xF0, 0xAE, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_19E = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x19E,
                       .data = {0x05, 0x00, 0x00, 0x04, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_192 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x192,
                       .data = {0xFF, 0xFF, 0xA3, 0x8F, 0x93, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_199 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x199,
                       .data = {0xFF, 0xFF, 0xD6, 0x7E, 0xFF, 0x3F}};
CAN_frame_t BMW_19A = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x19A,
                       .data = {0xFF, 0xFF, 0xFE, 0xFE, 0xFF, 0x3F}};
CAN_frame_t BMW_19F = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x19F,
                       .data = {0xFF, 0xFF, 0x0C, 0x80, 0xFF, 0x2F}};
CAN_frame_t BMW_150 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x150,
                       .data = {0x21, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x07, 0x00}};
CAN_frame_t BMW_105 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x105,
                       .data = {0x03, 0xF0, 0x7F, 0xE0, 0x2E, 0x00, 0xFC, 0x0F}};
CAN_frame_t BMW_2FA = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2FA,
                       .data = {0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2FC = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2FC,
                       .data = {0x81, 0x00, 0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2A0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2A0,
                       .data = {0x88, 0x88, 0xF8, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_2BE = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x2BE,
                       .data = {0x9B, 0x90, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
CAN_frame_t BMW_302 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x302,
                       .data = {0xFF, 0xFF, 0x54, 0xB0, 0xF1, 0xFF, 0xFF}};
CAN_frame_t BMW_3A0 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3A0,
                       .data = {0xFF, 0xFF, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC}};
CAN_frame_t BMW_3A4 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3A4,
                       .data = {0xCA, 0xA0, 0x5A, 0x03, 0x30, 0xFF, 0x1F, 0x2E}};
CAN_frame_t BMW_3E4 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E4,
                       .data = {0x25, 0x14, 0x00, 0x00, 0xFF, 0xFF}};
CAN_frame_t BMW_3C9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3C9,
                       .data = {0xAC, 0x13, 0x09, 0x0A, 0x92, 0x00, 0xF0, 0x39}};
CAN_frame_t BMW_3D0 = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3D0,
                       .data = {0xFD, 0xFF}};
CAN_frame_t BMW_3E5 = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E5,
                       .data = {0xFD, 0xFF, 0xFF}};
CAN_frame_t BMW_3E8 = {.FIR = {.B =
                                   {
                                       .DLC = 2,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E8,
                       .data = {0xF1, 0xFF}};  //1000ms OBD reset
CAN_frame_t BMW_3E9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3E9,
                       .data = {0xB0, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t BMW_3EC = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3EC,
                       .data = {0xF5, 0x10, 0x00, 0x00, 0x80, 0x25, 0x0F, 0xFC}};
CAN_frame_t BMW_3F9 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3F9,
                       .data = {0x76, 0x30, 0x00, 0xE2, 0xA6, 0x30, 0xC3, 0xFF}};
CAN_frame_t BMW_3FB = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3FB,
                       .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00}};
CAN_frame_t BMW_3FC = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3FC,
                       .data = {0xC0, 0xF9, 0x0F}};
CAN_frame_t BMW_328 = {.FIR = {.B =
                                   {
                                       .DLC = 6,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x328,
                       .data = {0xB0, 0x33, 0xBF, 0x0C, 0xD3, 0x20}};
CAN_frame_t BMW_330 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x330,
                       .data = {0x27, 0x33, 0x01, 0x00, 0x00, 0x00, 0x4D, 0x02}};
CAN_frame_t BMW_397 = {.FIR = {.B =
                                   {
                                       .DLC = 7,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x397,
                       .data = {0x00, 0x2A, 0x00, 0x6C, 0x0F, 0x55, 0x00}};
CAN_frame_t BMW_3FD = {.FIR = {.B =
                                   {
                                       .DLC = 5,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x3FD,
                       .data = {0x84, 0x00, 0x20, 0x00, 0xFF}};
CAN_frame_t BMW_433 = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x433,
                       .data = {0xFF, 0x00, 0x0F, 0xFF}};
CAN_frame_t BMW_418 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x418,
                       .data = {0xFF, 0x7C, 0xFF, 0x00, 0xC0, 0x3F, 0xFF, 0xFF}};
CAN_frame_t BMW_41D = {.FIR = {.B =
                                   {
                                       .DLC = 4,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x41D,
                       .data = {0xFF, 0xF7, 0x7F, 0xFF}};
CAN_frame_t BMW_429 = {.FIR = {.B =
                                   {
                                       .DLC = 3,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x429,
                       .data = {0x00, 0xF0, 0x00}};
CAN_frame_t BMW_51A = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x51A,
                       .data = {0x0, 0x0, 0x0, 0x0, 0x50, 0x0, 0x0, 0x1A}};
CAN_frame_t BMW_510 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x510,
                       .data = {0x40, 0x10, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame_t BMW_515 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x515,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15}};
CAN_frame_t BMW_540 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x540,
                       .data = {0x0, 0x0, 0x0, 0x0, 0xFD, 0x3C, 0xFF, 0x40}};
CAN_frame_t BMW_560 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x560,
                       .data = {0x0, 0x0, 0x0, 0x0, 0xFE, 0x00, 0x00, 0x60}};

//These CAN messages need to be sent towards the battery to keep it alive

static const uint8_t BMW_15F_0[15] = {0x42, 0x1F, 0xF8, 0xA5, 0x2B, 0x76, 0x91, 0xCC,
                                      0x90, 0xCD, 0x2A, 0x77, 0xF9, 0xA4, 0x43};
static const uint8_t BMW_F3_0[15] = {0x8B, 0xD6, 0x31, 0x6C, 0xE2, 0xBF, 0x58, 0x05,
                                     0x59, 0x04, 0xE3, 0xBE, 0x30, 0x6D, 0x8A};
static const uint8_t BMW_D9_0[15] = {0xA5, 0xF8, 0x1F, 0x42, 0xCC, 0x91, 0x76, 0x2B,
                                     0x77, 0x2A, 0xCD, 0x90, 0x1E, 0x43, 0xA4};
static const uint8_t BMW_EF_0[15] = {0x3A, 0x67, 0x80, 0xDD, 0x53, 0x0E, 0xE9, 0xB4,
                                     0xE8, 0xB5, 0x52, 0x0F, 0x81, 0xDC, 0x3B};
static const uint8_t BMW_C3_0[15] = {0xFD, 0xA0, 0x47, 0x1A, 0x94, 0xC9, 0x2E, 0x73,
                                     0x2F, 0x72, 0x95, 0xC8, 0x46, 0x1B, 0xFC};
static const uint8_t BMW_A5_0[15] = {0x47, 0x1A, 0xFD, 0xA0, 0x2E, 0x73, 0x94, 0xC9,
                                     0x95, 0xC8, 0x2F, 0x72, 0xFC, 0xA1, 0x46};
static const uint8_t BMW_10B_0[15] = {0xCD, 0x19, 0x94, 0x6D, 0xE0, 0x34, 0x78, 0xDB,
                                      0x97, 0x43, 0x0F, 0xF6, 0xBA, 0x6E, 0x81};
static const uint8_t BMW_10B_1[15] = {0x01, 0x02, 0x33, 0x34, 0x05, 0x06, 0x07, 0x08,
                                      0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x00};
static const uint8_t BMW_140_0[15] = {0x58, 0xE2, 0x31, 0x8B, 0x8A, 0x30, 0xE3, 0x59,
                                      0x05, 0xBF, 0x6C, 0xD6, 0xD7, 0x6D, 0xBE};
static const uint8_t BMW_141_0[15] = {0x49, 0xF3, 0x20, 0x9A, 0x9B, 0x21, 0xF2, 0x48,
                                      0x14, 0xAE, 0x7D, 0xC7, 0xC6, 0x7C, 0xAD};
static const uint8_t BMW_197_0[15] = {0x89, 0x06, 0x8A, 0x05, 0x8F, 0x00, 0x8C, 0x03,
                                      0x85, 0x0A, 0x86, 0x09, 0x83, 0x0C, 0x80};
static const uint8_t BMW_199_0[15] = {0xD6, 0xCC, 0xE0, 0xD6, 0xD6, 0xD6, 0xD6, 0xD6,
                                      0xE0, 0xD6, 0xD6, 0xD6, 0xCC, 0xD6, 0xD6};
static const uint8_t BMW_19B_0[15] = {0x20, 0x7D, 0x9A, 0xC7, 0x49, 0x14, 0xF3, 0xAE,
                                      0xF2, 0xAF, 0x48, 0x15, 0x9B, 0xC6, 0x21};
static const uint8_t BMW_12F_0[15] = {0xC2, 0x9F, 0x78, 0x25, 0xAB, 0xF6, 0x11, 0x4C,
                                      0x10, 0x4D, 0xAA, 0xF7, 0x79, 0x24, 0xC3};
static const uint8_t BMW_105_0[15] = {0x03, 0x5E, 0xB9, 0xE4, 0x6A, 0x37, 0xD0, 0x8D,
                                      0xD1, 0x8C, 0x6B, 0x36, 0xB8, 0xE5, 0x02};
static const uint8_t BMW_100_0[15] = {0x95, 0xC8, 0x2F, 0x72, 0xFC, 0xA1, 0x46, 0x1B,
                                      0x47, 0x1A, 0xFD, 0xA0, 0x2E, 0x73, 0x94};
static const uint8_t BMW_1D0_0[15] = {0x4D, 0x10, 0xF7, 0xAA, 0x24, 0x79, 0x9E, 0xC3,
                                      0x9F, 0xC2, 0x25, 0x78, 0xF6, 0xAB, 0x4C};
static const uint8_t BMW_2E8_7[15] = {0x09, 0x14, 0x33, 0x2E, 0x7D, 0x60, 0x47, 0x5A,
                                      0xE1, 0xFC, 0xDB, 0xC6, 0x95, 0x88, 0xAF};
static const uint8_t BMW_3A4_0[15] = {0xCA, 0x60, 0x20, 0xC7, 0xF3, 0xFD, 0x1A, 0x14,
                                      0x1B, 0xDB, 0xA1, 0x61, 0xEF, 0xAF, 0xBC};
static const uint8_t BMW_3F9_0[15] = {0x76, 0x2B, 0xCC, 0x91, 0x1F, 0x42, 0xA5, 0xF8,
                                      0xA4, 0xF9, 0x1E, 0x43, 0xCD, 0x90, 0x77};
static const uint8_t BMW_3EC_0[15] = {0xF5, 0xA8, 0x4F, 0x12, 0x9C, 0xC1, 0x26, 0x7B,
                                      0x27, 0x7A, 0x9D, 0xC0, 0x4E, 0x13, 0xF4};
static const uint8_t BMW_3FD_0[15] = {0x84, 0x19, 0xA3, 0x3E, 0xCA, 0x57, 0xED, 0x70,
                                      0x18, 0x85, 0x3F, 0xA2, 0x56, 0xCB, 0x71};
static const uint8_t BMW_2E3_0[15] = {0xFE, 0xA3, 0x44, 0x19, 0x97, 0xCA, 0x2D, 0x70,
                                      0x2C, 0x71, 0x96, 0xCB, 0x45, 0x18, 0xFF};
static const uint8_t BMW_2BE_0[15] = {0x9B, 0xC6, 0x21, 0x7C, 0xF2, 0xAF, 0x48, 0x15,
                                      0x49, 0x14, 0xF3, 0xAE, 0x20, 0x7D, 0x9A};
static const uint8_t BMW_F0_FE[15] = {0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7,
                                      0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE};
static const uint8_t BMW_A0_AE[15] = {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7,
                                      0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE};
static const uint8_t BMW_30_3E[15] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
                                      0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E};
static const uint8_t BMW_10_1E[15] = {0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                                      0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E};
static const uint8_t BMW_C0_CE[15] = {0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
                                      0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE};
static const uint8_t BMW_90_9E[15] = {0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
                                      0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E};
static const uint8_t BMW_40_4E[15] = {0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
                                      0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E};
static const uint8_t BMW_50_5E[15] = {0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
                                      0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E};
static const uint8_t BMW_70_7E[15] = {0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
                                      0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E};
static const uint8_t BMW_60_6E_spec[15] = {0x60, 0x62, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E,
                                           0x61, 0x63, 0x65, 0x67, 0x69, 0x6B, 0x6D};
static const uint8_t BMW_70_7E_spec[15] = {0x70, 0x72, 0x74, 0x76, 0x78, 0x7A, 0x7C, 0x7E,
                                           0x71, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7D};

static const uint8_t BMW_19E_0[6] = {0x05, 0x00, 0x05, 0x07, 0x0A, 0x0A};
static uint8_t BMW_20ms_counter = 0;
static uint8_t BMW_1AA_counter = 0;
static uint8_t BMW_10ms_counter = 0;
static uint8_t BMW_153_counter = 0;
static uint8_t BMW_1D0_counter = 0;
static uint8_t BMW_197_counter = 0;
static uint8_t BMW_19E_counter = 0;
static uint8_t BMW_10_counter = 0;
static uint8_t BMW_13D_counter = 0;
static uint8_t BMW_13E_counter = 0;
static uint8_t BMW_100ms_counter = 0;
static uint8_t BMW_3F9_counter = 0;
static uint8_t BMW_3FC_counter = 0;
static uint8_t BMW_200ms_counter = 0;
static uint8_t BMW_380_counter = 4;
static uint8_t BMW_429_counter = 0;
static uint32_t BMW_328_counter = 0;

static int16_t Battery_Current = 0;
static uint16_t Battery_Capacity_kWh = 0;
static uint16_t Voltage_Setpoint = 0;
static uint16_t Low_SOC = 0;
static uint16_t High_SOC = 0;
static uint16_t Display_SOC = 0;
static uint16_t Calculated_SOC = 0;
static uint16_t Battery_Volts = 0;
static uint16_t HVBatt_SOC = 0;
static uint16_t Battery_Status = 0;
static uint16_t DC_link = 0;
static int16_t Battery_Power = 0;

void update_values_i3_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;             //Startout in active mode

  //Calculate the SOC% value to send to inverter
  Calculated_SOC = (Display_SOC * 10);  //Increase decimal amount
  Calculated_SOC =
      LB_MIN_SOC + (LB_MAX_SOC - LB_MIN_SOC) * (Calculated_SOC - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE);
  if (Calculated_SOC < 0) {  //We are in the real SOC% range of 0-20%, always set SOC sent to inverter as 0%
    Calculated_SOC = 0;
  }
  if (Calculated_SOC > 1000) {  //We are in the real SOC% range of 80-100%, always set SOC sent to inverter as 100%
    Calculated_SOC = 1000;
  }
  SOC = (Calculated_SOC * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  battery_voltage = Battery_Volts;  //Unit V+1 (5000 = 500.0V)

  battery_current = Battery_Current;

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh = (Battery_Capacity_kWh * 1000);

  if (SOC > 9900)  //If Soc is over 99%, stop charging
  {
    max_target_charge_power = 0;
  } else {
    max_target_charge_power = 5000;  //Hardcoded value for testing. TODO: read real value from battery when discovered
  }

  if (SOC < 500)  //If Soc is under 5%, stop dicharging
  {
    max_target_discharge_power = 0;
  } else {
    max_target_discharge_power =
        5000;  //Hardcoded value for testing. TODO: read real value from battery when discovered
  }

  Battery_Power = (Battery_Current * (Battery_Volts / 10));

  stat_batt_power = Battery_Power;  //TODO:, is mapping OK?

  temperature_min;  //hardcoded to 5*C in startup, TODO:, find from battery CAN later

  temperature_max;  //hardcoded to 6*C in startup, TODO:, find from battery CAN later

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB
  Serial.print("SOC% battery: ");
  Serial.print(Display_SOC);
  Serial.print(" SOC% sent to inverter: ");
  Serial.print(SOC);
  Serial.print(" Battery voltage: ");
  Serial.print(battery_voltage);
  Serial.print(" Remaining Wh: ");
  Serial.print(remaining_capacity_Wh);
  Serial.print(" Max charge power: ");
  Serial.print(max_target_charge_power);
  Serial.print(" Max discharge power: ");
  Serial.print(max_target_discharge_power);
#endif
}

void receive_can_i3_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;

  Serial.print(rx_frame.MsgID);
  Serial.print(" ");

  switch (rx_frame.MsgID) {
    case 0x431:  //Battery capacity [200ms]
      Battery_Capacity_kWh = (((rx_frame.data.u8[1] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
      break;
    case 0x432:  //SOC% charged [200ms]
      Voltage_Setpoint = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      Low_SOC = (rx_frame.data.u8[2] / 2);
      High_SOC = (rx_frame.data.u8[3] / 2);
      Display_SOC = (rx_frame.data.u8[4] / 2);
      break;
    case 0x112:  //BMS status [10ms]
      CANstillAlive = 12;
      Battery_Current = ((rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) / 10) - 819;  //Amps
      Battery_Volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);                 //500.0 V
      HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 4 | rx_frame.data.u8[4]) / 10;
      Battery_Status = (rx_frame.data.u8[6] & 0x0F);
      DC_link = rx_frame.data.u8[7];
      break;
    case 0x430:  //BMS
      break;
    case 0x1FA:  //BMS
      break;
    case 0x40D:  //BMS
      break;
    case 0x2FF:  //BMS
      break;
    case 0x239:  //BMS
      break;
    case 0x2BD:  //BMS
      break;
    case 0x2F5:  //BMS
      break;
    case 0x3EB:  //BMS
      break;
    case 0x363:  //BMS
      break;
    case 0x507:
      break;
    case 0x41C:  //BMS
      break;
    default:
      break;
  }
}
void send_can_i3_battery() {
  unsigned long currentMillis = millis();
  //Send 10ms message
  if (currentMillis - previousMillis10 >= interval10) {
    previousMillis10 = currentMillis;

    BMW_105.data.u8[0] = BMW_105_0[BMW_10ms_counter];
    BMW_105.data.u8[1] = BMW_F0_FE[BMW_10ms_counter];

    BMW_A5.data.u8[0] = BMW_A5_0[BMW_10ms_counter];
    BMW_A5.data.u8[1] = BMW_F0_FE[BMW_10ms_counter];

    BMW_C3.data.u8[0] = BMW_C3_0[BMW_10ms_counter];
    BMW_C3.data.u8[1] = BMW_70_7E[BMW_10ms_counter];

    BMW_100.data.u8[0] = BMW_100_0[BMW_10ms_counter];
    BMW_100.data.u8[1] = BMW_F0_FE[BMW_10ms_counter];

    BMW_10ms_counter++;
    if (BMW_10ms_counter > 14) {
      BMW_10ms_counter = 0;
    }

    BMW_10_counter++;  //The three first frames of these messages are special
    if (BMW_10_counter > 3) {
      BMW_10_counter = 3;

      BMW_BB.data.u8[0] = 0x7D;

      BMW_AD.data.u8[2] = 0xFE;
      BMW_AD.data.u8[3] = 0xE7;
      BMW_AD.data.u8[4] = 0x7F;
      BMW_AD.data.u8[5] = 0xFE;
    }

    BMW_13D_counter++;

    if (BMW_13D_counter > 3) {
      BMW_13D_counter = 5;
      BMW_13D.data.u8[0] = 0xFF;
      BMW_13D.data.u8[1] = 0xFF;
      BMW_13D.data.u8[2] = 0xFE;
      BMW_13D.data.u8[3] = 0x27;
      BMW_13D.data.u8[4] = 0x9F;
      BMW_13D.data.u8[5] = 0x0A;
      BMW_13D.data.u8[6] = 0xF6;
      BMW_13D.data.u8[7] = 0xFF;
    }

    ESP32Can.CANWriteFrame(&BMW_C3);
    ESP32Can.CANWriteFrame(&BMW_BB);
    ESP32Can.CANWriteFrame(&BMW_AA);
    ESP32Can.CANWriteFrame(&BMW_A5);
    ESP32Can.CANWriteFrame(&BMW_AD);
    ESP32Can.CANWriteFrame(&BMW_100);
    ESP32Can.CANWriteFrame(&BMW_105);
    ESP32Can.CANWriteFrame(&BMW_150);
    ESP32Can.CANWriteFrame(&BMW_13D);
  }
  //Send 20ms message
  if (currentMillis - previousMillis20 >= interval20) {
    previousMillis20 = currentMillis;

    BMW_10B.data.u8[0] = BMW_10B_0[BMW_20ms_counter];
    BMW_10B.data.u8[1] = BMW_10B_1[BMW_20ms_counter];

    BMW_EF.data.u8[0] = BMW_EF_0[BMW_20ms_counter];
    BMW_EF.data.u8[1] = BMW_F0_FE[BMW_20ms_counter];

    BMW_140.data.u8[0] = BMW_140_0[BMW_20ms_counter];
    BMW_140.data.u8[1] = BMW_60_6E_spec[BMW_20ms_counter];

    BMW_141.data.u8[0] = BMW_141_0[BMW_20ms_counter];
    BMW_141.data.u8[1] = BMW_70_7E_spec[BMW_20ms_counter];

    BMW_D9.data.u8[0] = BMW_D9_0[BMW_20ms_counter];
    BMW_D9.data.u8[1] = BMW_F0_FE[BMW_20ms_counter];

    BMW_F3.data.u8[0] = BMW_F3_0[BMW_20ms_counter];
    BMW_F3.data.u8[1] = BMW_20ms_counter;

    BMW_15F.data.u8[0] = BMW_15F_0[BMW_20ms_counter];
    BMW_15F.data.u8[1] = BMW_F0_FE[BMW_20ms_counter];

    BMW_199.data.u8[2] = BMW_199_0[BMW_20ms_counter];

    BMW_20ms_counter++;
    if (BMW_20ms_counter > 14) {
      BMW_20ms_counter = 0;
    }

    BMW_153_counter++;

    if (BMW_153_counter > 2) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[6] = 0xF0;
    }
    if (BMW_153_counter > 4) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[6] = 0xF6;
    }
    if (BMW_153_counter > 30) {
      BMW_153.data.u8[2] = 0x01;
      BMW_153.data.u8[5] = 0x20;
      BMW_153.data.u8[6] = 0xF7;
      BMW_153_counter == 31;  //Stop the counter, maybe this is enough
    }

    BMW_13E_counter++;
    BMW_13E.data.u8[4] = BMW_13E_counter;

    ESP32Can.CANWriteFrame(&BMW_10B);
    ESP32Can.CANWriteFrame(&BMW_1A1);
    ESP32Can.CANWriteFrame(&BMW_153);
    ESP32Can.CANWriteFrame(&BMW_13E);
    ESP32Can.CANWriteFrame(&BMW_10E);
    ESP32Can.CANWriteFrame(&BMW_EF);
    ESP32Can.CANWriteFrame(&BMW_140);
    ESP32Can.CANWriteFrame(&BMW_141);
    ESP32Can.CANWriteFrame(&BMW_199);
    ESP32Can.CANWriteFrame(&BMW_19A);
    ESP32Can.CANWriteFrame(&BMW_D9);
    ESP32Can.CANWriteFrame(&BMW_AC);
    ESP32Can.CANWriteFrame(&BMW_19F);
    ESP32Can.CANWriteFrame(&BMW_F3);
    ESP32Can.CANWriteFrame(&BMW_15F);
  }
  //Send 30ms message
  if (currentMillis - previousMillis30 >= interval30) {
    previousMillis30 = currentMillis;

    BMW_197.data.u8[0] = BMW_197_0[BMW_197_counter];
    BMW_197.data.u8[1] = BMW_197_counter;
    BMW_197_counter++;
    if (BMW_197_counter > 14) {
      BMW_197_counter = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_197);
  }
  //Send 40ms message
  if (currentMillis - previousMillis40 >= interval40) {
    previousMillis40 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_302);
  }
  //Send 50ms message
  if (currentMillis - previousMillis50 >= interval50) {
    previousMillis50 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_429);
    ESP32Can.CANWriteFrame(&BMW_1AA);

    BMW_1AA_counter++;
    if (BMW_1AA_counter > 19) {
      BMW_1AA.data.u8[2] = 0x90;
      BMW_1AA.data.u8[3] = 0xC6;
      BMW_1AA.data.u8[4] = 0x08;
      BMW_1AA.data.u8[5] = 0xA0;
    }

    BMW_429_counter++;
    if (BMW_429_counter < 2) {
      BMW_429.data.u8[0] = 0x88;
      BMW_429.data.u8[1] = 0xF8;
    } else {
      BMW_429.data.u8[0] = 0x2C;
      BMW_429.data.u8[1] = 0xF8;
      BMW_429_counter = 3;  //stop the counter
    }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    BMW_2E8.data.u8[6] = BMW_F0_FE[BMW_100ms_counter];
    BMW_2E8.data.u8[7] = BMW_2E8_7[BMW_100ms_counter];

    BMW_3FD.data.u8[0] = BMW_3FD_0[BMW_100ms_counter];
    BMW_3FD.data.u8[1] = BMW_100ms_counter;

    BMW_2E3.data.u8[0] = BMW_2E3_0[BMW_100ms_counter];
    BMW_2E3.data.u8[1] = BMW_100ms_counter;

    BMW_2BE.data.u8[0] = BMW_2BE_0[BMW_100ms_counter];
    BMW_2BE.data.u8[1] = BMW_90_9E[BMW_100ms_counter];

    BMW_12F.data.u8[0] = BMW_12F_0[BMW_100ms_counter];
    BMW_12F.data.u8[1] = BMW_50_5E[BMW_100ms_counter];

    BMW_100ms_counter++;
    if (BMW_100ms_counter > 14) {
      BMW_100ms_counter = 0;
    }

    if (BMW_380_counter > 0) {
      ESP32Can.CANWriteFrame(&BMW_380);
      BMW_380_counter--;
    }

    ESP32Can.CANWriteFrame(&BMW_12F);
    ESP32Can.CANWriteFrame(&BMW_2FC);
    ESP32Can.CANWriteFrame(&BMW_108);
    ESP32Can.CANWriteFrame(&BMW_2B7);
    ESP32Can.CANWriteFrame(&BMW_2E8);
    ESP32Can.CANWriteFrame(&BMW_2B3);
    ESP32Can.CANWriteFrame(&BMW_3FD);
    ESP32Can.CANWriteFrame(&BMW_2E3);
    ESP32Can.CANWriteFrame(&BMW_211);
    ESP32Can.CANWriteFrame(&BMW_2BE);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= interval200) {
    previousMillis200 = currentMillis;

    BMW_3A4.data.u8[0] = BMW_3A4_0[BMW_200ms_counter];
    BMW_3A4.data.u8[1] = BMW_A0_AE[BMW_200ms_counter];

    BMW_19B.data.u8[0] = BMW_19B_0[BMW_200ms_counter];
    BMW_19B.data.u8[1] = BMW_40_4E[BMW_200ms_counter];

    BMW_3C2.data.u8[1] = BMW_10_1E[BMW_200ms_counter];

    BMW_200ms_counter++;
    if (BMW_200ms_counter > 14) {
      BMW_200ms_counter = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_03C);
    ESP32Can.CANWriteFrame(&BMW_3E9);
    ESP32Can.CANWriteFrame(&BMW_2A0);  //Only in LIM code
    ESP32Can.CANWriteFrame(&BMW_397);  //Only in LIM code
    ESP32Can.CANWriteFrame(&BMW_510);  //Only in LIM code
    ESP32Can.CANWriteFrame(&BMW_540);  //Only in LIM code
    ESP32Can.CANWriteFrame(&BMW_560);  //Only in LIM code
    ESP32Can.CANWriteFrame(&BMW_32F);
    ESP32Can.CANWriteFrame(&BMW_3A4);
    ESP32Can.CANWriteFrame(&BMW_3C9);
    ESP32Can.CANWriteFrame(&BMW_59A);
    ESP32Can.CANWriteFrame(&BMW_19B);
    ESP32Can.CANWriteFrame(&BMW_3C2);
  }
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= interval500) {
    previousMillis500 = currentMillis;

    BMW_19E.data.u8[0] = BMW_19E_0[BMW_19E_counter];
    BMW_19E.data.u8[3] = 0x04;
    if (BMW_19E_counter == 1) {
      BMW_19E.data.u8[3] = 0x00;
    }
    if (BMW_19E_counter < 5) {
      BMW_19E_counter++;
    }

    ESP32Can.CANWriteFrame(&BMW_19E);
    ESP32Can.CANWriteFrame(&BMW_326);
  }
  // Send 600ms CAN Message
  if (currentMillis - previousMillis600 >= interval600) {
    previousMillis600 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_512);
    ESP32Can.CANWriteFrame(&BMW_515);
    ESP32Can.CANWriteFrame(&BMW_51A);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= interval1000) {
    previousMillis1000 = currentMillis;

    BMW_328_counter++;
    BMW_328.data.u8[0] = BMW_328_counter;  //rtc msg. needs to be every 1 sec. first 32 bits are 1 second wrap counter
    BMW_328.data.u8[1] = BMW_328_counter << 8;
    BMW_328.data.u8[2] = BMW_328_counter << 16;
    BMW_328.data.u8[3] = BMW_328_counter << 24;

    BMW_1D0.data.u8[0] = BMW_1D0_0[BMW_1D0_counter];
    BMW_1D0.data.u8[1] = BMW_F0_FE[BMW_1D0_counter];

    BMW_3F9.data.u8[0] = BMW_3F9_0[BMW_3F9_counter];
    BMW_3F9.data.u8[1] = BMW_30_3E[BMW_3F9_counter];

    BMW_3EC.data.u8[0] = BMW_3EC_0[BMW_3F9_counter];
    BMW_3EC.data.u8[1] = BMW_10_1E[BMW_3F9_counter];

    if (BMW_328_counter > 1) {
      BMW_433.data.u8[1] = 0x01;
    }

    BMW_3F9_counter++;
    if (BMW_3F9_counter > 14) {
      BMW_3F9_counter = 0;
    }

    ESP32Can.CANWriteFrame(&BMW_328);
    ESP32Can.CANWriteFrame(&BMW_3E8);
    ESP32Can.CANWriteFrame(&BMW_2FA);
    ESP32Can.CANWriteFrame(&BMW_1D0);
    ESP32Can.CANWriteFrame(&BMW_433);
    ESP32Can.CANWriteFrame(&BMW_192);
    ESP32Can.CANWriteFrame(&BMW_2E2);
    ESP32Can.CANWriteFrame(&BMW_3F9);
    ESP32Can.CANWriteFrame(&BMW_3FB);
    ESP32Can.CANWriteFrame(&BMW_3EC);
    ESP32Can.CANWriteFrame(&BMW_418);
    ESP32Can.CANWriteFrame(&BMW_41D);
    ESP32Can.CANWriteFrame(&BMW_3D0);
    ESP32Can.CANWriteFrame(&BMW_2CA);
    ESP32Can.CANWriteFrame(&BMW_3CA);
    ESP32Can.CANWriteFrame(&BMW_3E4);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= interval5000) {
    previousMillis5000 = currentMillis;

    BMW_3FC_counter++;
    if (BMW_3FC_counter > 14) {
      BMW_3FC_counter = 0;
    }

    BMW_3FC.data.u8[1] = BMW_C0_CE[BMW_3FC_counter];

    ESP32Can.CANWriteFrame(&BMW_3A0);
    ESP32Can.CANWriteFrame(&BMW_330);
    ESP32Can.CANWriteFrame(&BMW_3FC);
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= interval10000) {
    previousMillis10000 = currentMillis;

    ESP32Can.CANWriteFrame(&BMW_3E5);
  }
}
