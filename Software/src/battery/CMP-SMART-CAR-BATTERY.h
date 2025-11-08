#ifndef CMP_SMART_CAR_BATTERY_H
#define CMP_SMART_CAR_BATTERY_H
#include "CMP-SMART-CAR-BATTERY-HTML.h"
#include "CanBattery.h"

class CmpSmartCarBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "Stellantis CMP Smart Car Battery";

  bool supports_charged_energy() { return true; }

  bool supports_reset_DTC() { return true; }
  void reset_DTC() { datalayer_extended.stellantisCMPsmart.UserRequestDTCreset = true; }

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  CmpSmartCarHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_100S_DV = 3700;
  static const int MIN_PACK_VOLTAGE_100S_DV = 2900;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 3650;
  static const int MIN_CELL_VOLTAGE_MV = 2800;

  unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
  unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was sent
  unsigned long previousMillis60 = 0;    // will store last time a 60ms CAN Message was sent
  unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
  unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent

  uint8_t precalculated432[16] = {0x12, 0x11, 0x10, 0x1F, 0x1E, 0x1D, 0x1C, 0x1B,
                                  0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14, 0x13};

  CAN_frame CMP_211 = {.FD = false,  //VCU contactor 100ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x211,
                       .data = {0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CMP_351 = {.FD = false,  //VCU 60ms Airbag
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x351,
                       .data = {0x46, 0x14, 0x17, 0x00, 0x00, 0x00, 0x00, 0x0F}};
  CAN_frame CMP_432 = {.FD = false,  //VCU 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x432,
                       .data = {0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x7D, 0x52}};

  //Optional CAN messages to simulate more of the vehicle towards the battery (Not required?)
  /*
    uint8_t checksum217[16] = {0x50, 0x41, 0xB2, 0xA3, 0x14, 0x05, 0xF6, 0xE7,
                             0x58, 0xC9, 0xBA, 0xAB, 0x1C, 0x8D, 0x7E, 0x6F};
  CAN_frame CMP_208 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x208,
                       .data = {0x00, 0x20, 0x00, 0x84, 0x40, 0x21, 0x00, 0x00}};

  CAN_frame CMP_217 = {.FD = false,  //VCU 10ms (Inverter motor speed)
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x217,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0xA6, 0x00, 0x00}};
  CAN_frame CMP_231 = {
      .FD = false,  //VCU preconditioning
      .ext_ID = false,
      .DLC = 8,     //
      .ID = 0x231,  //0b00 : Not active0b01 : Heating function active0b10 : Cooling function active0b11 : Reserve
      .data = {0x98, 0x59, 0x60, 0x00, 0xA3, 0x20, 0x00, 0x00}};  //Last byte, bit pos 1, has precond req
  CAN_frame CMP_241 = {.FD = false,                               //VCU vehicle speed and emg stop 10ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x241,
                       .data = {0x00, 0x00, 0x39, 0x00, 0xC8, 0x00, 0x00, 0x00}};
  CAN_frame CMP_262 = {.FD = false,  //VCU 10ms
                       .ext_ID = false,
                       .DLC = 1,
                       .ID = 0x262,
                       .data = {0x00}};

  CAN_frame CMP_421 = {.FD = false,  //VCU 50ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x421,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame CMP_422 = {.FD = false,  //100ms VCU, Configuration
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x422,  //Fitting, Plant,check,storage,client,APV,showroom etc.
                       .data = {0x00, 0x00, 0x10, 0x00, 0x00, 0x10, 0x00, 0x00}};

  CAN_frame CMP_4A2 = {.FD = false,  //OBC plug 100ms
                       .ext_ID = false,
                       .DLC = 2,
                       .ID = 0x4A2,
                       .data = {0x00, 0x41}};  //second byte, 00 plugged, 64 unplugged, 41vehiclerunning
  CAN_frame CMP_552 = {.FD = false,            //VCU mileage and time 1000ms
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x552,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE}};
  */
  CAN_frame CMP_POLL = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x6B4, .data = {0x03, 0x22, 0xD8, 0x13}};
  CAN_frame CMP_CLEAR_ALL_DTC = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 5,
                                 .ID = 0x6B4,
                                 .data = {0x04, 0x14, 0xFF, 0xFF, 0xFF}};
  uint32_t vehicle_time_counter = 0x088B390B;  //Taken from log on 19thOctober2025
  uint32_t main_contactor_cycle_count = 0;
  uint32_t QC_contactor_cycle_count = 0;
  uint32_t lifetime_kWh_charged = 0;
  uint32_t lifetime_kWh_discharged = 0;
  uint32_t remaining_energy_Wh = 0;
  uint32_t total_energy_when_full_Wh = 41400;
  uint32_t total_coloumb_counting_Ah = 0;
  uint32_t total_coulomb_counting_kWh = 0;

  uint16_t discharge_available_10s_power = 0;
  uint16_t discharge_available_10s_current = 0;
  uint16_t discharge_cont_available_power = 0;
  uint16_t discharge_cont_available_current = 0;
  uint16_t discharge_available_30s_current = 0;
  uint16_t discharge_available_30s_power = 0;
  uint16_t regen_charge_cont_power = 0;
  uint16_t regen_charge_30s_power = 0;
  uint16_t regen_charge_30s_current = 0;
  uint16_t regen_charge_cont_current = 0;
  uint16_t regen_charge_10s_current = 0;
  uint16_t regen_charge_10s_power = 0;
  uint16_t quick_charge_port_voltage = 0;
  uint16_t insulation_resistance_kOhm = 0;
  uint16_t DC_bus_voltage = 0;
  uint16_t charge_max_voltage = 0;
  uint16_t charge_cont_curr_max = 0;
  uint16_t charge_cont_curr_req = 0;
  uint16_t hours_spent_overvoltage = 0;
  uint16_t hours_spent_overtemperature = 0;
  uint16_t hours_spent_undertemperature = 0;
  uint16_t battery_soc = 500;
  uint16_t battery_voltage = 3300;
  uint16_t min_cell_voltage = 3300;
  uint16_t max_cell_voltage = 3300;
  uint16_t nominal_voltage = 0;
  uint16_t charge_continue_power_limit = 0;
  uint16_t charge_energy_amount_requested = 0;
  uint16_t hours_spent_exceeding_charge_power = 0;
  uint16_t hours_spent_exceeding_discharge_power = 0;
  uint16_t SOC_actual = 0;

  int16_t battery_temperature_average = 0;
  int16_t battery_temperature_maximum = 0;
  int16_t coolant_temperature = 0;
  int16_t battery_temperature_minimum = 0;
  int16_t battery_current_dA = 0;

  uint8_t startup_increment = 0;
  uint8_t active_DTC_code = 0;
  uint8_t battery_quickcharge_connect_status = 0;
  uint8_t qc_negative_contactor_status = 0;
  uint8_t qc_positive_contactor_status = 0;
  uint8_t eplug_status = 0;
  uint8_t ev_warning = 0;
  uint8_t battery_state = 0;
  uint8_t battery_fault = 0;
  uint8_t battery_negative_contactor_state = 0;
  uint8_t battery_precharge_contactor_state = 0;
  uint8_t battery_positive_contactor_state = 0;
  uint8_t battery_connect_status = 0;
  uint8_t battery_charging_status = 0;
  uint8_t min_cell_voltage_number = 0;
  uint8_t max_cell_voltage_number = 0;
  uint8_t bulk_SOC_DC_limit = 0;
  uint8_t mux = 0;
  uint8_t startup_counter_432 = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_60ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t SOH_internal_resistance = 0;
  uint8_t SOH_estimated = 100;
  uint8_t max_temperature_probe_number = 0;
  uint8_t min_temperature_probe_number = 0;
  uint8_t number_of_temperature_sensors = 0;
  uint8_t number_of_cells = 0;
  uint8_t coolant_temperature_warning = 0;
  uint8_t heater_relay_status = 0;
  uint8_t preheating_status = 0;
  uint8_t thermal_control = 0;
  uint8_t thermal_runaway = 0;
  uint8_t thermal_runaway_module_ID = 0;
  uint8_t HVIL_status = 0;
  uint8_t hardware_fault_status = 0;
  uint8_t insulation_fault = 0;
  uint8_t temperature = 0;
  uint8_t insulation_circuit_status = 0;
  uint8_t plausibility_error = 0;
  uint8_t service_due = 0;
  uint8_t l3_fault = 0;
  uint8_t master_warning = 0;
  uint8_t hvbat_wakeup_state = 0;
  uint8_t alert_frame3 = 0;
  uint8_t alert_frame4 = 0;

  bool rcd_line_active = false;
  bool power_auth = false;
  bool battery_balancing_active = false;
  bool coolant_alarm = false;
  bool cooling_enabled = false;
  bool battery_minimum_voltage_reached_warning = false;
  bool alert_low_battery_energy = false;
};
#endif
