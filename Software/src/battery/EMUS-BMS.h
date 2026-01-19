#ifndef EMUS_BMS_H
#define EMUS_BMS_H

#include "../datalayer/datalayer.h"
#include "CanBattery.h"

class EmusBms : public CanBattery {
 public:
  // Use this constructor for the second battery.
  EmusBms(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* contactor_closing_allowed_ptr, CAN_Interface targetCan)
      : CanBattery(targetCan, CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = datalayer_ptr;
    contactor_closing_allowed = contactor_closing_allowed_ptr;
    allows_contactor_closing = nullptr;
  }

  // Use the default constructor to create the first or single battery.
  EmusBms() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    contactor_closing_allowed = nullptr;
  }

  virtual ~EmusBms() = default;

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "EMUS BMS compatible battery";

 private:
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELLS = 192;  // Maximum cells supported
  static const uint32_t EMUS_BASE_ID = 0x10B50000;  // EMUS extended ID base
  static const uint32_t EMUS_SOC_PARAMS_ID = 0x10B50500;  // Base + 0x05 (State of Charge parameters)
  static const uint32_t EMUS_ENERGY_PARAMS_ID = 0x10B50600;  // Base + 0x06 (Energy parameters)
  static const uint32_t CELL_VOLTAGE_BASE_ID = 0x10B50100;  // Base CAN ID for cell voltages
  static const uint32_t CELL_BALANCING_BASE_ID = 0x10B50300;  // Base CAN ID for balancing status

  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  // If not null, this battery listens to this boolean to determine whether contactor closing is allowed
  bool* contactor_closing_allowed;

  unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent
  unsigned long previousMillis5000 = 0;  // will store last time a 5s CAN Message was sent

  //Actual content messages
  CAN_frame PYLON_3010 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x3010,
                          .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_8200 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x8200,
                          .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_8210 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x8210,
                          .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame PYLON_4200 = {.FD = false,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x4200,
                          .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // EMUS request for individual cell voltages (group 0 = all cells)
  CAN_frame EMUS_CELL_VOLTAGE_REQUEST = {.FD = false,
                                          .ext_ID = true,
                                          .DLC = 0,
                                          .ID = 0x19B50100,
                                          .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  // EMUS request for individual cell balancing status (group 0 = all cells)
  CAN_frame EMUS_CELL_BALANCING_REQUEST = {.FD = false,
                                            .ext_ID = true,
                                            .DLC = 0,
                                            .ID = 0x19B50300,
                                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

  int16_t celltemperature_max_dC;
  int16_t celltemperature_min_dC;
  int16_t current_dA;
  uint16_t voltage_dV = 0;
  uint16_t cellvoltage_max_mV = 3300;
  uint16_t cellvoltage_min_mV = 3300;
  uint16_t charge_cutoff_voltage = 0;
  uint16_t discharge_cutoff_voltage = 0;
  int16_t max_charge_current = 0;
  int16_t max_discharge_current = 0;
  uint8_t ensemble_info_ack = 0;
  uint8_t battery_module_quantity = 0;
  uint8_t battery_modules_in_series = 0;
  uint8_t cell_quantity_in_module = 0;
  uint8_t voltage_level = 0;
  uint8_t ah_number = 0;
  uint8_t SOC = 50;
  uint8_t SOH = 100;
  uint8_t charge_forbidden = 0;
  uint8_t discharge_forbidden = 0;
  uint8_t actual_cell_count = 0;  // Actual number of cells detected
  uint8_t stable_data_cycles = 0;  // Counter for stable voltage data
  uint16_t last_min_voltage = 3300;  // Track previous min for stability check
  uint16_t last_max_voltage = 3300;  // Track previous max for stability check

  // EMUS estimated remaining charge (0.1Ah units) from Base+0x05.
  uint16_t est_charge_0p1Ah = 0;
  bool est_charge_valid = false;
  unsigned long est_charge_last_ms = 0;
  static const unsigned long EST_CHARGE_TIMEOUT_MS = 10000;  // Treat estimate as stale after 10s

  // EMUS estimated remaining energy (10Wh units) from Base+0x06 (optional diagnostics).
  uint16_t est_energy_10Wh = 0;
  bool est_energy_valid = false;
};

#endif
