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

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  CmpSmartCarHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_100S_DV = 3700;
  static const int MIN_PACK_VOLTAGE_100S_DV = 2900;
  static const int MAX_CELL_DEVIATION_MV = 100;
  static const int MAX_CELL_VOLTAGE_MV = 3650;
  static const int MIN_CELL_VOLTAGE_MV = 2800;

  unsigned long previousMillis10 = 0;  // will store last time a 10ms CAN Message was sent
  uint8_t mux = 0;
  int16_t temperature_sensors[16];
  uint16_t cell_voltages_mV[100];
  uint16_t battery_soc = 5000;
  uint16_t battery_voltage = 3300;
  uint8_t battery_state = 0;
  uint8_t battery_fault = 0;
  int16_t battery_current = 0;
  uint8_t battery_negative_contactor_state = 0;
  uint8_t battery_precharge_contactor_state = 0;
  uint8_t battery_positive_contactor_state = 0;
  uint8_t battery_connect_status = 0;
  uint8_t battery_charging_status = 0;
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
  uint8_t battery_quickcharge_connect_status = 0;
  uint16_t regen_charge_10s_current = 0;
  uint16_t regen_charge_10s_power = 0;
  uint16_t quick_charge_port_voltage = 0;
  uint8_t qc_negative_contactor_status = 0;
  uint8_t qc_positive_contactor_status = 0;
  bool battery_balancing_active = false;
  uint8_t eplug_status = 0;
  uint8_t ev_warning = 0;
  bool power_auth = false;
  uint8_t HVIL_status = 0;
  uint8_t hardware_fault_status = 0;
  uint8_t insulation_fault = 0;
  uint8_t temperature = 0;
  uint8_t insulation_circuit_status = 0;
  uint8_t plausibility_error = 0;
  uint8_t service_due = 0;
  uint8_t l3_fault = 0;
  uint8_t master_warning = 0;
  uint16_t insulation_resistance_kOhm = 0;
  uint16_t DC_bus_voltage = 0;
  uint16_t charge_max_voltage = 0;
  uint16_t charge_cont_curr_max = 0;
  uint16_t charge_cont_curr_req = 0;
  uint16_t hours_spent_overvoltage = 0;
  uint16_t hours_spent_overtemperature = 0;
  uint16_t hours_spent_undertemperature = 0;
  uint32_t total_coloumb_counting_Ah = 0;
  uint32_t total_coulomb_counting_kWh = 0;
  int16_t battery_temperature_maximum = 0;
  uint16_t min_cell_voltage = 3300;
  uint16_t max_cell_voltage = 3300;
  uint8_t min_cell_voltage_number = 0;
  uint8_t max_cell_voltage_number = 0;
  uint16_t nominal_voltage = 0;
  uint16_t charge_continue_power_limit = 0;
  uint16_t charge_energy_amount_requested = 0;
  uint8_t bulk_SOC_DC_limit = 0;
  uint32_t lifetime_kWh_charged = 0;
  uint32_t lifetime_kWh_discharged = 0;
  uint16_t hours_spent_exceeding_charge_power = 0;
  uint16_t hours_spent_exceeding_discharge_power = 0;
  uint16_t SOC_actual = 0;
  bool alert_low_battery_energy = 0;
  int16_t battery_temperature_average = 0;
  bool battery_minimum_voltage_reached_warning = 0;
  uint32_t remaining_energy_Wh = 0;
  uint32_t total_energy_when_full_Wh = 41400;
  uint8_t SOH_internal_resistance = 0;
  uint8_t SOH_estimated = 0;
  int16_t battery_temperature_minimum = 0;
};
#endif
