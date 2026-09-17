#ifndef FISKER_OCEAN_BATTERY_H
#define FISKER_OCEAN_BATTERY_H

#include "FISKER-OCEAN-HTML.h"
#include "UdsCanBattery.h"

class FiskerOceanBattery : public UdsCanBattery {
 public:
  FiskerOceanBattery() : renderer(&datalayer_extended.fiskerOcean) { dtc = &datalayer.battery.dtc; }

  void setup() override;
  void handle_incoming_can_frame(CAN_frame rx_frame) override;
  void update_values() override;
  void transmit_can(unsigned long currentMillis) override;
  static constexpr const char* Name = "Fisker Ocean 113/106kWh battery";

  bool supports_contactor_close() override { return true; }
  bool uses_main_page_contactor_control() override { return true; }
  bool supports_reset_BMS() override { return true; }
  void reset_BMS() override;
  void request_open_contactors() override { datalayer_extended.fiskerOcean.wake_transmit_active = false; }
  void request_close_contactors() override { datalayer_extended.fiskerOcean.wake_transmit_active = true; }
  BatteryHtmlRenderer& get_status_renderer() override { return renderer; }

 protected:
  uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) override;

 private:
  FiskerOceanHtmlRenderer renderer;
  unsigned long previousMillis093 = 0;
  unsigned long previousMillis333 = 0;

  static constexpr uint16_t CELLVOLTAGE_FRAME_START = 0x6B0;
  static constexpr uint16_t NUM_CELLS = 102;

  static constexpr int MAX_PACK_VOLTAGE_113S_DV = 5000;
  static constexpr int MIN_PACK_VOLTAGE_106S_DV = 2500;
  static constexpr int MAX_CELL_DEVIATION_MV = 250;
  static constexpr int MAX_CELL_VOLTAGE_MV = 4250;
  static constexpr int MIN_CELL_VOLTAGE_MV = 2900;
  static constexpr uint16_t TEST_CURRENT_LIMIT_DA = 500;  // 50.0 A until BMS limits are decoded

  int16_t cell_temperature_max_C = 0;
  int16_t cell_temperature_min_C = 0;
  uint16_t pack_voltage = 37000;

  static constexpr uint16_t poll_commands[DATALAYER_INFO_FISKER_OCEAN::DID_COUNT] = {
      0x2003, 0x2004, 0x2005, 0x2008, 0x2009, 0x2011, 0x2016, 0x2019, 0x2024, 0x2026, 0x2027, 0x2031,
      0x2032, 0x2033, 0x2034, 0x2038, 0x2039, 0x2040, 0x2041, 0x2042, 0x2043, 0x2047, 0x2048, 0x2049,
      0x2050, 0x2053, 0x2054, 0x2055, 0x2056, 0x2057, 0x2058, 0x2059, 0x2060, 0x2061, 0x2062, 0x2063,
      0x2064, 0x2069, 0x2070, 0x2078, 0x2079, 0x2080, 0x2081, 0x2089, 0x2090, 0x2091, 0x2092, 0x2093,
      0x2094, 0x2107, 0x2108, 0x2109, 0x2117, 0x2130, 0x2133, 0x2134, 0x2136, 0x2137, 0x2138, 0x2143,
      0x2144, 0x2145, 0xEFF6, 0xEFF7, 0xEFF8, 0xEFF9, 0xEFFE, 0xF040, 0xF055, 0xF060, 0xF184, 0xF190};

  CAN_frame FISKER_READY_093 = {
      .FD = true,
      .ext_ID = false,
      .DLC = 16,
      .ID = 0x093,
      .data = {0x8A, 0xE0, 0x11, 0xFF, 0x22, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}};
  CAN_frame FISKER_READY_333 = {.FD = true,
                                .ext_ID = false,
                                .DLC = 8,
                                .ID = 0x333,
                                .data = {0x03, 0xD0, 0x55, 0xAD, 0x96, 0xFF, 0xFF, 0xFF}};
  struct ReadyCandidate {
    CAN_frame frame;
    uint16_t period_ms;
    uint8_t counter_high_nibble;
    uint8_t crc_xor_out;
    bool protected_frame;
    unsigned long previous_millis;
    uint8_t counter;
  };

  ReadyCandidate ready_candidates[DATALAYER_INFO_FISKER_OCEAN::READY_CANDIDATE_COUNT] = {
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x150, .data = {0x00, 0x10, 0x80, 0x00, 0xAF, 0xF7, 0xF2, 0xC0}},
       INTERVAL_10_MS,
       0x10,
       0x47,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x151, .data = {0x00, 0x10, 0x80, 0x00, 0xAF, 0xFB, 0x52, 0xC0}},
       INTERVAL_10_MS,
       0x10,
       0x22,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x1B6, .data = {0x00, 0x00, 0x00, 0x1E, 0xCB, 0x3C, 0xF0, 0x07}},
       INTERVAL_10_MS,
       0x00,
       0xF1,
       true,
       0,
       0},
      {{.FD = true,
        .ext_ID = false,
        .DLC = 16,
        .ID = 0x214,
        .data = {0x00, 0x30, 0x51, 0x41, 0x00, 0x03, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}},
       INTERVAL_10_MS,
       0x30,
       0xD2,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x236, .data = {0x00, 0x10, 0x04, 0x28, 0x00, 0x00, 0x00, 0xF8}},
       INTERVAL_20_MS,
       0x10,
       0xB1,
       true,
       0,
       0},
      {{.FD = true,
        .ext_ID = false,
        .DLC = 16,
        .ID = 0x260,
        .data = {0x00, 0xF0, 0x56, 0xA3, 0x00, 0x00, 0xF5, 0x1B, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF}},
       INTERVAL_20_MS,
       0xF0,
       0xCE,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x311, .data = {0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00}},
       INTERVAL_20_MS,
       0x00,
       0xE9,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x318, .data = {0x00, 0x90, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x80}},
       INTERVAL_20_MS,
       0x90,
       0x80,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x354, .data = {0x00, 0x30, 0xCA, 0xF5, 0x52, 0x49, 0x27, 0xFF}},
       INTERVAL_10_MS,
       0x30,
       0x9E,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x355, .data = {0x00, 0x30, 0xCA, 0xF5, 0x52, 0x49, 0x27, 0xFF}},
       INTERVAL_10_MS,
       0x30,
       0xC1,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x358, .data = {0x00, 0x40, 0x68, 0xFF, 0x80, 0xA2, 0xF1, 0xFF}},
       INTERVAL_100_MS,
       0x40,
       0x2B,
       true,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x365, .data = {0x39, 0xF0, 0x05, 0xC1, 0x20, 0x02, 0x08, 0xFF}},
       INTERVAL_10_MS,
       0,
       0,
       false,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x366, .data = {0x39, 0xF0, 0x05, 0xC1, 0x20, 0x02, 0x08, 0xFF}},
       INTERVAL_10_MS,
       0,
       0,
       false,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x507, .data = {0x4F, 0x0C, 0x70, 0x84, 0x00, 0x00, 0x35, 0xC3}},
       INTERVAL_100_MS,
       0,
       0,
       false,
       0,
       0},
      {{.FD = true, .ext_ID = false, .DLC = 8, .ID = 0x511, .data = {0x00, 0x20, 0x01, 0x00, 0xAF, 0xE4, 0xC0, 0x00}},
       INTERVAL_100_MS,
       0,
       0,
       false,
       0,
       0}};

  void transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble, uint8_t xor_out);
  void transmit_optional_ready_frames(unsigned long currentMillis);
};

#endif
