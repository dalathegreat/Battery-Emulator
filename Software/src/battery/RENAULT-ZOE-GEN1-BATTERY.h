#ifndef RENAULT_ZOE_GEN1_BATTERY_H
#define RENAULT_ZOE_GEN1_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "RENAULT-ZOE-GEN1-HTML.h"

class RenaultZoeGen1Battery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  RenaultZoeGen1Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_ZOE* extended, CAN_Interface targetCan)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = nullptr;
    datalayer_zoe = extended;

    calculated_total_pack_voltage_mV = 0;
  }

  // Use the default constructor to create the first or single battery.
  RenaultZoeGen1Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    datalayer_zoe = &datalayer_extended.zoe;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Renault Zoe Gen1 22/40kWh";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  RenaultZoeGen1HtmlRenderer renderer;

  static const int MAX_PACK_VOLTAGE_DV = 4040;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_DV = 3000;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4220;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_ZOE* datalayer_zoe;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  unsigned long previousMillis100 = 0;  // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis250 = 0;  // will store last time a 250ms CAN Message was sent
  uint8_t counter_423 = 0;

  CAN_frame ZOE_423 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x423,
                       .data = {0x07, 0x1d, 0x00, 0x02, 0x5d, 0x80, 0x5d, 0xc8}};
  CAN_frame ZOE_POLL_79B = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x79B,
                            .data = {0x02, 0x21, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame ZOE_ACK_79B = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x79B,
                           .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

#define GROUP1_CELLVOLTAGES_1_POLL 0x41
#define GROUP2_CELLVOLTAGES_2_POLL 0x42
#define GROUP3_METRICS 0x61
#define GROUP4_SOC 0x03
#define GROUP5_TEMPERATURE_POLL 0x04
#define GROUP6_BALANCING 0x07

  uint16_t LB_SOC = 50;
  uint16_t LB_Display_SOC = 50;
  uint16_t LB_SOH = 99;
  int16_t LB_Average_Temperature = 0;
  uint32_t LB_Charging_Power_W = 0;
  uint32_t LB_Regen_allowed_W = 0;
  uint32_t LB_Discharge_allowed_W = 0;
  int16_t LB_Current = 0;
  int16_t LB_Cell_minimum_temperature = 0;
  int16_t LB_Cell_maximum_temperature = 0;
  uint16_t LB_Cell_minimum_voltage = 3700;
  uint16_t LB_Cell_maximum_voltage = 3700;
  uint16_t LB_kWh_Remaining = 0;
  uint16_t LB_Battery_Voltage = 3700;
  uint8_t LB_Heartbeat = 0;
  uint8_t LB_CUV = 0;
  uint8_t LB_HVBIR = 0;
  uint8_t LB_HVBUV = 0;
  uint8_t LB_EOCR = 0;
  uint8_t LB_HVBOC = 0;
  uint8_t LB_HVBOT = 0;
  uint8_t LB_HVBOV = 0;
  uint8_t LB_COV = 0;
  uint8_t frame0 = 0;
  uint8_t current_poll = 0;
  uint8_t requested_poll = 0;
  uint8_t group = 0;
  uint16_t cellvoltages[96];
  uint32_t calculated_total_pack_voltage_mV = 370000;
  uint8_t highbyte_cell_next_frame = 0;
  uint16_t SOC_polled = 5000;
  int16_t cell_1_temperature_polled = 0;
  int16_t cell_2_temperature_polled = 0;
  int16_t cell_3_temperature_polled = 0;
  int16_t cell_4_temperature_polled = 0;
  int16_t cell_5_temperature_polled = 0;
  int16_t cell_6_temperature_polled = 0;
  int16_t cell_7_temperature_polled = 0;
  int16_t cell_8_temperature_polled = 0;
  int16_t cell_9_temperature_polled = 0;
  int16_t cell_10_temperature_polled = 0;
  int16_t cell_11_temperature_polled = 0;
  int16_t cell_12_temperature_polled = 0;
  uint16_t battery_mileage_in_km = 0;
  uint16_t kWh_from_beginning_of_battery_life = 0;
  bool looping_over_20 = false;
};

#endif
