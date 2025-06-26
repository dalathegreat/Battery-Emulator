#ifndef BMW_I3_BATTERY_H
#define BMW_I3_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../include.h"
#include "BMW-I3-HTML.h"
#include "CanBattery.h"

#ifdef BMW_I3_BATTERY
#define SELECTED_BATTERY_CLASS BmwI3Battery
#endif

class BmwI3Battery : public CanBattery {
 public:
  // Use this constructor for the second battery.
  BmwI3Battery(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* contactor_closing_allowed_ptr, CAN_Interface targetCan,
               int wakeup)
      : CanBattery(targetCan) {
    datalayer_battery = datalayer_ptr;
    contactor_closing_allowed = contactor_closing_allowed_ptr;
    allows_contactor_closing = nullptr;
    wakeup_pin = wakeup;

    //Init voltage to 0 to allow contactor check to operate without fear of default values colliding
    battery_volts = 0;
  }

  // Use the default constructor to create the first or single battery.
  BmwI3Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    contactor_closing_allowed = nullptr;
    wakeup_pin = WUP_PIN1;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr char* Name = "BMW i3";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  BmwI3HtmlRenderer renderer;

 private:
  const int MAX_CELL_VOLTAGE_60AH = 4110;   // Battery is put into emergency stop if one cell goes over this value
  const int MIN_CELL_VOLTAGE_60AH = 2700;   // Battery is put into emergency stop if one cell goes below this value
  const int MAX_CELL_VOLTAGE_94AH = 4140;   // Battery is put into emergency stop if one cell goes over this value
  const int MIN_CELL_VOLTAGE_94AH = 2700;   // Battery is put into emergency stop if one cell goes below this value
  const int MAX_CELL_VOLTAGE_120AH = 4190;  // Battery is put into emergency stop if one cell goes over this value
  const int MIN_CELL_VOLTAGE_120AH = 2790;  // Battery is put into emergency stop if one cell goes below this value
  const int MAX_CELL_DEVIATION_MV = 250;    // LED turns yellow on the board if mv delta exceeds this value
  const int MAX_PACK_VOLTAGE_60AH = 3950;   // Charge stops if pack voltage exceeds this value
  const int MIN_PACK_VOLTAGE_60AH = 2590;   // Discharge stops if pack voltage exceeds this value
  const int MAX_PACK_VOLTAGE_94AH = 3980;   // Charge stops if pack voltage exceeds this value
  const int MIN_PACK_VOLTAGE_94AH = 2590;   // Discharge stops if pack voltage exceeds this value
  const int MAX_PACK_VOLTAGE_120AH = 4030;  // Charge stops if pack voltage exceeds this value
  const int MIN_PACK_VOLTAGE_120AH = 2680;  // Discharge stops if pack voltage exceeds this value
  const int NUMBER_OF_CELLS = 96;

  DATALAYER_BATTERY_TYPE* datalayer_battery;

  // If not null, this battery decides when the contactor can be closed and writes the value here.
  bool* allows_contactor_closing;

  // If not null, this battery listens to this boolean to determine whether contactor closing is allowed
  bool* contactor_closing_allowed;

  int wakeup_pin;

  unsigned long previousMillis20 = 0;     // will store last time a 20ms CAN Message was send
  unsigned long previousMillis100 = 0;    // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
  unsigned long previousMillis500 = 0;    // will store last time a 500ms CAN Message was send
  unsigned long previousMillis640 = 0;    // will store last time a 600ms CAN Message was send
  unsigned long previousMillis1000 = 0;   // will store last time a 1000ms CAN Message was send
  unsigned long previousMillis5000 = 0;   // will store last time a 5000ms CAN Message was send
  unsigned long previousMillis10000 = 0;  // will store last time a 10000ms CAN Message was send

  static const int ALIVE_MAX_VALUE = 14;  // BMW CAN messages contain alive counter, goes from 0...14

  uint8_t increment_alive_counter(uint8_t counter);

  enum BatterySize { BATTERY_60AH, BATTERY_94AH, BATTERY_120AH };
  BatterySize detectedBattery = BATTERY_60AH;

  enum CmdState { SOH, CELL_VOLTAGE_MINMAX, SOC, CELL_VOLTAGE_CELLNO, CELL_VOLTAGE_CELLNO_LAST };

  CmdState cmdState = SOC;

  /* CAN messages from PT-CAN2 not needed to operate the battery
     0AA 105 13D 0BB 0AD 0A5 150 100 1A1 10E 153 197 429 1AA 12F 59A 2E3 2BE 211 2b3 3FD 2E8 2B7 108 29D 29C 29B 2C0 330
     3E9 32F 19E 326 55E 515 509 50A 51A 2F5 3A4 432 3C9 
     */

  CAN_frame BMW_10B = {.FD = false,
                       .ext_ID = false,
                       .DLC = 3,
                       .ID = 0x10B,
                       .data = {0xCD, 0x00, 0xFC}};  // Contactor closing command
  CAN_frame BMW_12F = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x12F,
                       .data = {0xE6, 0x24, 0x86, 0x1A, 0xF1, 0x31, 0x30, 0x00}};  //0x12F Wakeup VCU
  CAN_frame BMW_13E = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x13E,
                       .data = {0xFF, 0x31, 0xFA, 0xFA, 0xFA, 0xFA, 0x0C, 0x00}};
  CAN_frame BMW_192 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x192,
                       .data = {0xFF, 0xFF, 0xA3, 0x8F, 0x93, 0xFF, 0xFF, 0xFF}};
  CAN_frame BMW_19B = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x19B,
                       .data = {0x20, 0x40, 0x40, 0x55, 0xFD, 0xFF, 0xFF, 0xFF}};
  CAN_frame BMW_1D0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x1D0,
                       .data = {0x4D, 0xF0, 0xAE, 0xF8, 0xFF, 0xFF, 0xFF, 0xFF}};
  CAN_frame BMW_2CA = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x2CA, .data = {0x57, 0x57}};
  CAN_frame BMW_2E2 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x2E2,
                       .data = {0x4F, 0xDB, 0x7F, 0xB9, 0x07, 0x51, 0xff, 0x00}};
  CAN_frame BMW_30B = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x30B,
                       .data = {0xe1, 0xf0, 0xff, 0xff, 0xf1, 0xff, 0xff, 0xff}};
  CAN_frame BMW_328 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x328,
                       .data = {0xB0, 0xE4, 0x87, 0x0E, 0x30, 0x22}};
  CAN_frame BMW_37B = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x37B,
                       .data = {0x40, 0x00, 0x00, 0xFF, 0xFF, 0x00}};
  CAN_frame BMW_380 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 7,
                       .ID = 0x380,
                       .data = {0x56, 0x5A, 0x37, 0x39, 0x34, 0x34, 0x34}};
  CAN_frame BMW_3A0 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3A0,
                       .data = {0xFF, 0xFF, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC}};
  CAN_frame BMW_3A7 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 7,
                       .ID = 0x3A7,
                       .data = {0x05, 0xF5, 0x0A, 0x00, 0x4F, 0x11, 0xF0}};
  CAN_frame BMW_3C5 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3C5,
                       .data = {0x30, 0x05, 0x47, 0x70, 0x2c, 0xce, 0xc3, 0x34}};
  CAN_frame BMW_3CA = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3CA,
                       .data = {0x87, 0x80, 0x30, 0x0C, 0x0C, 0x81, 0xFF, 0xFF}};
  CAN_frame BMW_3D0 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x3D0, .data = {0xFD, 0xFF}};
  CAN_frame BMW_3E4 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x3E4,
                       .data = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF}};
  CAN_frame BMW_3E5 = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x3E5, .data = {0xFC, 0xFF, 0xFF}};
  CAN_frame BMW_3E8 = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x3E8, .data = {0xF0, 0xFF}};  //1000ms OBD reset
  CAN_frame BMW_3EC = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3EC,
                       .data = {0xF5, 0x10, 0x00, 0x00, 0x80, 0x25, 0x0F, 0xFC}};
  CAN_frame BMW_3F9 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3F9,
                       .data = {0xA7, 0x2A, 0x00, 0xE2, 0xA6, 0x30, 0xC3, 0xFF}};
  CAN_frame BMW_3FB = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x3FB,
                       .data = {0xFF, 0xFF, 0xFF, 0xFF, 0x5F, 0x00}};
  CAN_frame BMW_3FC = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x3FC, .data = {0xC0, 0xF9, 0x0F}};
  CAN_frame BMW_418 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x418,
                       .data = {0xFF, 0x7C, 0xFF, 0x00, 0xC0, 0x3F, 0xFF, 0xFF}};
  CAN_frame BMW_41D = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x41D, .data = {0xFF, 0xF7, 0x7F, 0xFF}};
  CAN_frame BMW_433 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x433,
                       .data = {0xFF, 0x00, 0x0F, 0xFF}};  // HV specification
  CAN_frame BMW_512 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x512,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x12}};  // 0x512 Network management
  CAN_frame BMW_592_0 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x592,
                         .data = {0x86, 0x10, 0x07, 0x21, 0x6e, 0x35, 0x5e, 0x86}};
  CAN_frame BMW_592_1 = {.FD = false,
                         .ext_ID = false,
                         .DLC = 8,
                         .ID = 0x592,
                         .data = {0x86, 0x21, 0xb4, 0xdd, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame BMW_5F8 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x5F8,
                       .data = {0x64, 0x01, 0x00, 0x0B, 0x92, 0x03, 0x00, 0x05}};
  CAN_frame BMW_6F1_CELL = {.FD = false,
                            .ext_ID = false,
                            .DLC = 5,
                            .ID = 0x6F1,
                            .data = {0x07, 0x03, 0x22, 0xDD, 0xBF}};
  CAN_frame BMW_6F1_SOH = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0x63, 0x35}};
  CAN_frame BMW_6F1_SOC = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x6F1, .data = {0x07, 0x03, 0x22, 0xDD, 0xBC}};
  CAN_frame BMW_6F1_CELL_VOLTAGE_AVG = {.FD = false,
                                        .ext_ID = false,
                                        .DLC = 5,
                                        .ID = 0x6F1,
                                        .data = {0x07, 0x03, 0x22, 0xDF, 0xA0}};
  CAN_frame BMW_6F1_CONTINUE = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x6F1, .data = {0x07, 0x30, 0x00, 0x02}};
  CAN_frame BMW_6F4_CELL_VOLTAGE_CELLNO = {.FD = false,
                                           .ext_ID = false,
                                           .DLC = 7,
                                           .ID = 0x6F4,
                                           .data = {0x07, 0x05, 0x31, 0x01, 0xAD, 0x6E, 0x01}};
  CAN_frame BMW_6F4_CELL_CONTINUE = {.FD = false,
                                     .ext_ID = false,
                                     .DLC = 6,
                                     .ID = 0x6F4,
                                     .data = {0x07, 0x04, 0x31, 0x03, 0xAD, 0x6E}};

  //The above CAN messages need to be sent towards the battery to keep it alive

  uint8_t startup_counter_contactor = 0;
  uint8_t alive_counter_20ms = 0;
  uint8_t alive_counter_100ms = 0;
  uint8_t alive_counter_200ms = 0;
  uint8_t alive_counter_500ms = 0;
  uint8_t alive_counter_1000ms = 0;
  uint8_t alive_counter_5000ms = 0;
  uint8_t BMW_1D0_counter = 0;
  uint8_t BMW_13E_counter = 0;
  uint8_t BMW_380_counter = 0;
  uint32_t BMW_328_seconds = 243785948;  // Initialized to make the battery think vehicle was made 7.7years ago
  uint16_t BMW_328_days =
      9244;  //Time since 1.1.2000. Hacky implementation to make it think current date is 23rd April 2025
  uint32_t BMS_328_seconds_to_day = 0;  //Counter to keep track of days uptime

  bool battery_awake = false;
  bool battery_info_available = false;
  bool skipCRCCheck = false;
  bool CRCCheckPassedPreviously = false;

  uint16_t cellvoltage_temp_mV = 0;
  uint32_t battery_serial_number = 0;
  uint32_t battery_available_power_shortterm_charge = 0;
  uint32_t battery_available_power_shortterm_discharge = 0;
  uint32_t battery_available_power_longterm_charge = 0;
  uint32_t battery_available_power_longterm_discharge = 0;
  uint32_t battery_BEV_available_power_shortterm_charge = 0;
  uint32_t battery_BEV_available_power_shortterm_discharge = 0;
  uint32_t battery_BEV_available_power_longterm_charge = 0;
  uint32_t battery_BEV_available_power_longterm_discharge = 0;
  uint16_t battery_energy_content_maximum_Wh = 0;
  uint16_t battery_display_SOC = 0;
  uint16_t battery_volts = 0;
  uint16_t battery_HVBatt_SOC = 0;
  uint16_t battery_DC_link_voltage = 0;
  uint16_t battery_max_charge_voltage = 0;
  uint16_t battery_min_discharge_voltage = 0;
  uint16_t battery_predicted_energy_charge_condition = 0;
  uint16_t battery_predicted_energy_charging_target = 0;
  uint16_t battery_actual_value_power_heating = 0;  //0 - 4094 W
  uint16_t battery_prediction_voltage_shortterm_charge = 0;
  uint16_t battery_prediction_voltage_shortterm_discharge = 0;
  uint16_t battery_prediction_voltage_longterm_charge = 0;
  uint16_t battery_prediction_voltage_longterm_discharge = 0;
  uint16_t battery_prediction_duration_charging_minutes = 0;
  uint16_t battery_target_voltage_in_CV_mode = 0;
  uint16_t battery_soc = 0;
  uint16_t battery_soc_hvmax = 0;
  uint16_t battery_soc_hvmin = 0;
  uint16_t battery_capacity_cah = 0;
  int16_t battery_temperature_HV = 0;
  int16_t battery_temperature_heat_exchanger = 0;
  int16_t battery_temperature_max = 0;
  int16_t battery_temperature_min = 0;
  int16_t battery_max_charge_amperage = 0;
  int16_t battery_max_discharge_amperage = 0;
  int16_t battery_current = 0;
  uint8_t battery_status_error_isolation_external_Bordnetz = 0;
  uint8_t battery_status_error_isolation_internal_Bordnetz = 0;
  uint8_t battery_request_cooling = 0;
  uint8_t battery_status_valve_cooling = 0;
  uint8_t battery_status_error_locking = 0;
  uint8_t battery_status_precharge_locked = 0;
  uint8_t battery_status_disconnecting_switch = 0;
  uint8_t battery_status_emergency_mode = 0;
  uint8_t battery_request_service = 0;
  uint8_t battery_error_emergency_mode = 0;
  uint8_t battery_status_error_disconnecting_switch = 0;
  uint8_t battery_status_warning_isolation = 0;
  uint8_t battery_status_cold_shutoff_valve = 0;
  uint8_t battery_request_open_contactors = 0;
  uint8_t battery_request_open_contactors_instantly = 0;
  uint8_t battery_request_open_contactors_fast = 0;
  uint8_t battery_charging_condition_delta = 0;
  uint8_t battery_status_service_disconnection_plug = 0;
  uint8_t battery_status_measurement_isolation = 0;
  uint8_t battery_request_abort_charging = 0;
  uint8_t battery_prediction_time_end_of_charging_minutes = 0;
  uint8_t battery_request_operating_mode = 0;
  uint8_t battery_request_charging_condition_minimum = 0;
  uint8_t battery_request_charging_condition_maximum = 0;
  uint8_t battery_status_cooling_HV = 0;      //1 works, 2 does not start
  uint8_t battery_status_diagnostics_HV = 0;  // 0 all OK, 1 HV protection function error, 2 diag not yet expired
  uint8_t battery_status_diagnosis_powertrain_maximum_multiplexer = 0;
  uint8_t battery_status_diagnosis_powertrain_immediate_multiplexer = 0;
  uint8_t battery_ID2 = 0;
  uint8_t battery_soh = 99;

  uint8_t message_data[50];
  uint8_t next_data = 0;
  uint8_t current_cell_polled = 0;
};

#endif
