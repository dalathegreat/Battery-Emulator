#ifndef BYD_KOSTAL_RS485_H
#define BYD_KOSTAL_RS485_H
#include <Arduino.h>
#include "../include.h"

#include "Rs485InverterProtocol.h"

#ifdef BYD_KOSTAL_RS485
#define RS485_INVERTER_SELECTED
#define SELECTED_INVERTER_CLASS KostalInverterProtocol
#endif

//#define DEBUG_KOSTAL_RS485_DATA  // Enable this line to get TX / RX printed out via logging
//#define DEBUG_KOSTAL_RS485_DATA_USB  // Enable this line to get TX / RX printed out via USB

#if defined(DEBUG_KOSTAL_RS485_DATA) && !defined(DEBUG_LOG)
#error "enable LOG_TO_SD, DEBUG_VIA_USB or DEBUG_VIA_WEB in order to use DEBUG_KOSTAL_RS485_DATA"
#endif

class KostalInverterProtocol : public Rs485InverterProtocol {
 public:
  void setup();
  void receive();
  void update_values();
  static constexpr char* Name = "BYD battery via Kostal RS485";

 private:
  int baud_rate() { return 57600; }
  void float2frame(byte* arr, float value, byte framepointer);
  bool check_kostal_frame_crc(int len);

  // How many value updates we can go without inverter gets reported as missing \
    // e.g. value set to 12, 12*5sec=60seconds without comm before event is raised
  const int RS485_HEALTHY = 12;

  const uint8_t KOSTAL_FRAMEHEADER[5] = {0x62, 0xFF, 0x02, 0xFF, 0x29};
  const uint8_t KOSTAL_FRAMEHEADER2[5] = {0x63, 0xFF, 0x02, 0xFF, 0x29};
  uint16_t nominal_voltage_dV = 0;
  int16_t average_temperature_dC = 0;
  uint8_t incoming_message_counter = RS485_HEALTHY;
  int8_t f2_startup_count = 0;

  boolean B1_delay = false;
  unsigned long B1_last_millis = 0;
  unsigned long currentMillis;
  unsigned long startupMillis = 0;
  unsigned long contactorMillis = 0;

  uint16_t rx_index = 0;
  boolean RX_allow = false;

  union f32b {
    float f;
    byte b[4];
  };

  // clang-format off
uint8_t BATTERY_INFO[40] = {
    0x00,                         // First zero byte pointer
    0xE2, 0xFF, 0x02, 0xFF, 0x29, // Frame header
    0x00, 0x00, 0x80, 0x43,       // 256.063 Nominal voltage / 5*51.2=256
    0xE4, 0x70, 0x8A, 0x5C,       // Manufacture date (Epoch time) (BYD: GetBatteryInfo this[0x10ac])
    0xB5, 0x00, 0xD3, 0x00,       // Battery Serial number? Modbus register 527 - 0x10b0
    0x00, 0x00, 0xC8, 0x41,       // Nominal Capacity (0x10b4)
    0xC2, 0x18,                   // Battery Firmware, modbus register 586  (0x10b8)
    0x00,                         // (BYD: GetBatteryInfo this[0x10ba])
    0x00,                         // ?
    0x59, 0x42,                   // Vendor identifier
                                  //       0x59 0x42 -> 'YB' -> BYD
                                  //       0x59 0x44 -> 'YD' -> Dyness
    0x00, 0x00,                   // (BYD: GetBatteryInfo this[0x10be])
    0x00, 0x00,
    0x05, 0x00,                   // Number of blocks in series (uint16)
    0xA0, 0x00, 0x00, 0x00,
    0x4D, // CRC
    0x00};
  // clang-format on

  // values in CYCLIC_DATA will be overwritten at update_values()

  // clang-format off
uint8_t CYCLIC_DATA[64] = {
    0x00,                          // First zero byte pointer
    0xE2, 0xFF, 0x02, 0xFF, 0x29,  // Frame header
    0x1D, 0x5A, 0x85, 0x43,        // Current Voltage            (float32)   Bytes  6- 9    Modbus register 216
    0x00, 0x00, 0x8D, 0x43,        // Max Voltage                (float32)   Bytes 10-13
    0x00, 0x00, 0xAC, 0x41,        // Battery Temperature        (float32)   Bytes 14-17    Modbus register 214
    0x00, 0x00, 0x00, 0x00,        // Peak Current (1s period?)  (float32)   Bytes 18-21
    0x00, 0x00, 0x00, 0x00,        // Avg current  (1s period?)  (float32)   Bytes 22-25
    0x00, 0x00, 0x48, 0x42,        // Max discharge current      (float32)   Bytes 26-29    Sunspec: ADisChaMax
    0x00, 0x00, 0xC8, 0x41,        // Battery gross capacity, Ah (float32)   Bytes 30-33    Modbus register 512
    0x00, 0x00, 0xA0, 0x41,        // Max charge current         (float32)   Bytes 34-37    0.0f when SoC is 100%, Sunspec: AChaMax
    0xCD, 0xCC, 0xB4, 0x41,        // MaxCellTemp                (float32)   Bytes 38-41
    0x00, 0x00, 0xA4, 0x41,        // MinCellTemp                (float32)   Bytes 42-45
    0xA4, 0x70, 0x55, 0x40,        // MaxCellVolt                (float32)   Bytes 46-49
    0x7D, 0x3F, 0x55, 0x40,        // MinCellVolt                (float32)   Bytes 50-53

    0xFE, 0x04,  // Bytes 54-55, Cycle count (uint16)
    0x00,        // Byte 56, charge/discharge control, 0=disable, 1=enable
    0x00,        // Byte 57, When SoC is 100%, seen as 0x40
    0x64,        // Byte 58, SoC (uint8)
    0x00,        // Byte 59, Unknown
    0x00,        // Byte 60, Unknown
    0x01,        // Byte 61, Unknown, 1 only at first frame, 0 otherwise
    0x00,        // Byte 62, CRC
    0x00};
  // clang-format on

  // FE 04 01 40 xx 01 01 02 yy (fully charged)
  // FE 02 01 02 xx 01 01 02 yy (charging or discharging)

  uint8_t STATUS_FRAME[9] = {
      0x00, 0xE2, 0xFF, 0x02, 0xFF, 0x29,  //header
      0x06,                                //Unknown (battery status/error?)
      0xEF,                                //CRC
      0x00                                 //endbyte
  };

  uint8_t ACK_FRAME[8] = {0x07, 0xE3, 0xFF, 0x02, 0xFF, 0x29, 0xF4, 0x00};

  uint8_t RS485_RXFRAME[300];

  bool register_content_ok = false;
};

#endif
