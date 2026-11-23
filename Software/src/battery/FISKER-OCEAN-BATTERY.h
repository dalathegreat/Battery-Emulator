#ifndef FISKER_OCEAN_BATTERY_H
#define FISKER_OCEAN_BATTERY_H

#include <Arduino.h>
#include "../datalayer/datalayer.h"
#include "UdsCanBattery.h"

// Fisker Ocean (NMC) high-voltage battery.
//
// The pack is kept awake and its contactors are closed by replaying the three
// frames the vehicle sends on the battery bus:
//   - 0x093  CAN-FD, every 20 ms  (rolling counter + CRC)
//   - 0x333  CAN-FD, every 50 ms  (rolling counter + CRC)
//   - 0x6A5  classic CAN, every 1 s (static operating-mode vector)
// 0x093/0x333 keep the contactors closed; 0x6A5 is the operating-mode vector
// that takes the BMS out of its inactive mode so it will balance and run its HV
// housekeeping. Transmission starts only after a Close Contactors request.
//
// Measurements (voltage, current, SOC, temperatures and cell voltages) are read
// from the BMS broadcast frames. DTCs are read and cleared over UDS on
// 0x7E1/0x7E9.

class FiskerOceanBattery : public UdsCanBattery {
 public:
  FiskerOceanBattery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    dtc = &datalayer_battery->dtc;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "Fisker Ocean 106/113kWh battery";

  // Contactor closing is driven over CAN: Close Contactors starts the wake
  // traffic, Open Contactors stops it. Buttons are shown on the battery page.
  bool supports_contactor_close() override { return true; }
  void request_open_contactors() override { wake_transmit_active = false; }
  void request_close_contactors() override { wake_transmit_active = true; }

  // Human-readable names for the DTCs read over UDS.
  const char* get_dtc_json_filename() override { return "fisker_ocean_dtc.json"; }

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // Output flag the GPIO contactor control waits on. Set true once in setup():
  // the BMS owns its own contactors, the emulator only has to permit closing.
  bool* allows_contactor_closing;

  bool wake_transmit_active = false;
  unsigned long previousMillis093 = 0;
  unsigned long previousMillis333 = 0;
  unsigned long previousMillis6A5 = 0;
  uint8_t wake_093_counter = 0;
  uint8_t wake_333_counter = 0;

  int16_t cell_temperature_max_C = 0;
  int16_t cell_temperature_min_C = 0;
  uint16_t pack_voltage = 37000;  // dV*10, replaced by the first 0x0E9 reading
  uint8_t broadcast_soc_percent = 0;
  bool broadcast_soc_valid = false;

  static constexpr uint16_t CELLVOLTAGE_FRAME_START = 0x6B0;
  static constexpr uint16_t NUM_CELLS = 102;

  static constexpr int MAX_PACK_VOLTAGE_113S_DV = 5000;
  static constexpr int MIN_PACK_VOLTAGE_106S_DV = 2500;
  static constexpr int MAX_CELL_DEVIATION_MV = 250;
  static constexpr int MAX_CELL_VOLTAGE_MV = 4250;
  static constexpr int MIN_CELL_VOLTAGE_MV = 2900;
  // BMS charge/discharge limits are not yet decoded, so the integration keeps a
  // conservative development ceiling and otherwise follows the operator limits.
  static constexpr uint16_t TEST_CURRENT_LIMIT_DA = 500;  // 50.0 A

  // 0x093 and 0x333 carry a rolling counter and CRC, so bytes 0-1 are rewritten
  // on every transmission by transmit_ready_frame(); the values below are the
  // initial counter/CRC placeholders.
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
  // Operating-mode vector. On the car this is produced on the propulsion bus and
  // received by the BMS. The gate nibble (bytes 0-1) is asserted in every awake
  // vehicle state and takes the BMS out of its inactive mode so it will balance
  // and run its HV housekeeping. No rolling counter/CRC is present, so a static
  // replay is sufficient; the vehicle sends it about every 3 s, 1 s gives margin.
  static constexpr CAN_frame FISKER_MODE_6A5 = {.FD = false,
                                                .ext_ID = false,
                                                .DLC = 8,
                                                .ID = 0x6A5,
                                                .data = {0x30, 0x30, 0x06, 0x38, 0xCB, 0xD2, 0x50, 0x04}};

  void transmit_ready_frame(CAN_frame* frame, uint8_t& counter, uint8_t high_nibble, uint8_t xor_out);
};

#endif
