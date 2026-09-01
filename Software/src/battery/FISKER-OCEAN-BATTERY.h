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

  void transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble, uint8_t xor_out);
};

#endif
