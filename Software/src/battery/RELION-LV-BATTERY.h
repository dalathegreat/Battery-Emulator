#ifndef RELION_BATTERY_H
#define RELION_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../system_settings.h"
#include "CanBattery.h"
#include "utils/cell_soc_estimator.h"

class RelionBattery : public CanBattery {
 public:
  bool mandatory_charge_taper() { return true; }
  // Use this constructor for the second battery.
  RelionBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, CAN_Interface targetCan, bool* allows_contactor_closing_ptr)
      : CanBattery(targetCan, CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = allows_contactor_closing_ptr;
    battery_total_voltage = 0;
  }

  // Use the default constructor to create the first or single battery.
  RelionBattery() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Relion LV protocol via 250kbps CAN";

 private:
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  uint16_t estimateSOC();

  bool* allows_contactor_closing;

  static const int MAX_PACK_VOLTAGE_DV = 584;  //58.4V recommended charge voltage. BMS protection steps in at 60.8V
  static const int MIN_PACK_VOLTAGE_DV = 440;  //44.0V Recommended LV disconnect. BMS protection steps in at 40.0V
  static const int MAX_CELL_DEVIATION_MV = 300;
  static const int MAX_CELL_VOLTAGE_MV = 3750;
  static const int MIN_CELL_VOLTAGE_MV = 2800;
  static constexpr battery_chemistry_enum CHEMISTRY = battery_chemistry_enum::LFP;

  unsigned long previousMillis500ms = 0;  // will store last time a 500ms CAN Message was sent

  uint16_t SOC_from_max_cell_voltage = 0;
  uint16_t SOC_from_min_cell_voltage = 0;
  uint16_t battery_total_voltage = 500;
  int16_t battery_total_current = 0;
  uint8_t system_state = 0;
  uint8_t battery_soc = 50;
  uint8_t battery_soh = 99;
  uint8_t most_serious_fault = 0;
  uint16_t max_cell_voltage = 3300;
  uint16_t min_cell_voltage = 3300;
  int16_t max_cell_temperature = 0;
  int16_t min_cell_temperature = 0;
  int16_t charge_current_A = 0;
  int16_t regen_charge_current_A = 0;
  int16_t discharge_current_A = 0;

  CAN_frame RELION_CONTACTOR_MESSAGE = {.FD = false,
                                        .ext_ID = true,
                                        .DLC = 8,
                                        .ID = 0x18010081,
                                        .data = {0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}};
};

#endif
