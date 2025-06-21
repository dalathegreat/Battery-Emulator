#ifndef MEB_BATTERY_H
#define MEB_BATTERY_H
#include <Arduino.h>
#include "../include.h"
#include "CanBattery.h"
#include "MEB-HTML.h"

#ifdef MEB_BATTERY
#define SELECTED_BATTERY_CLASS MebBattery
#endif

class MebBattery : public CanBattery {
 public:
  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  bool supports_real_BMS_status() { return true; }
  bool supports_charged_energy() { return true; }
  static constexpr char* Name = "Volkswagen Group MEB platform via CAN-FD";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  MebHtmlRenderer renderer;
  static const int MAX_PACK_VOLTAGE_84S_DV = 3528;  //5000 = 500.0V
  static const int MIN_PACK_VOLTAGE_84S_DV = 2520;
  static const int MAX_PACK_VOLTAGE_96S_DV = 4032;
  static const int MIN_PACK_VOLTAGE_96S_DV = 2880;
  static const int MAX_PACK_VOLTAGE_108S_DV = 4536;
  static const int MIN_PACK_VOLTAGE_108S_DV = 3240;
  static const int MAX_CELL_DEVIATION_MV = 150;
  static const int MAX_CELL_VOLTAGE_MV = 4250;  //Battery is put into emergency stop if one cell goes over this value
  static const int MIN_CELL_VOLTAGE_MV = 2700;  //Battery is put into emergency stop if one cell goes below this value
  static const int PID_SOC = 0x028C;
  static const int PID_VOLTAGE = 0x1E3B;
  static const int PID_CURRENT = 0x1E3D;
  static const int PID_MAX_TEMP = 0x1E0E;
  static const int PID_MIN_TEMP = 0x1E0F;
  static const int PID_MAX_CHARGE_VOLTAGE = 0x5171;
  static const int PID_MIN_DISCHARGE_VOLTAGE = 0x5170;
  static const int PID_ENERGY_COUNTERS = 0x1E32;
  static const int PID_ALLOWED_CHARGE_POWER = 0x1E1B;
  static const int PID_ALLOWED_DISCHARGE_POWER = 0x1E1C;
  static const int PID_CELLVOLTAGE_CELL_1 = 0x1E40;
  static const int PID_CELLVOLTAGE_CELL_2 = 0x1E41;
  static const int PID_CELLVOLTAGE_CELL_3 = 0x1E42;
  static const int PID_CELLVOLTAGE_CELL_4 = 0x1E43;
  static const int PID_CELLVOLTAGE_CELL_5 = 0x1E44;
  static const int PID_CELLVOLTAGE_CELL_6 = 0x1E45;
  static const int PID_CELLVOLTAGE_CELL_7 = 0x1E46;
  static const int PID_CELLVOLTAGE_CELL_8 = 0x1E47;
  static const int PID_CELLVOLTAGE_CELL_9 = 0x1E48;
  static const int PID_CELLVOLTAGE_CELL_10 = 0x1E49;
  static const int PID_CELLVOLTAGE_CELL_11 = 0x1E4A;
  static const int PID_CELLVOLTAGE_CELL_12 = 0x1E4B;
  static const int PID_CELLVOLTAGE_CELL_13 = 0x1E4C;
  static const int PID_CELLVOLTAGE_CELL_14 = 0x1E4D;
  static const int PID_CELLVOLTAGE_CELL_15 = 0x1E4E;
  static const int PID_CELLVOLTAGE_CELL_16 = 0x1E4F;
  static const int PID_CELLVOLTAGE_CELL_17 = 0x1E50;
  static const int PID_CELLVOLTAGE_CELL_18 = 0x1E51;
  static const int PID_CELLVOLTAGE_CELL_19 = 0x1E52;
  static const int PID_CELLVOLTAGE_CELL_20 = 0x1E53;
  static const int PID_CELLVOLTAGE_CELL_21 = 0x1E54;
  static const int PID_CELLVOLTAGE_CELL_22 = 0x1E55;
  static const int PID_CELLVOLTAGE_CELL_23 = 0x1E56;
  static const int PID_CELLVOLTAGE_CELL_24 = 0x1E57;
  static const int PID_CELLVOLTAGE_CELL_25 = 0x1E58;
  static const int PID_CELLVOLTAGE_CELL_26 = 0x1E59;
  static const int PID_CELLVOLTAGE_CELL_27 = 0x1E5A;
  static const int PID_CELLVOLTAGE_CELL_28 = 0x1E5B;
  static const int PID_CELLVOLTAGE_CELL_29 = 0x1E5C;
  static const int PID_CELLVOLTAGE_CELL_30 = 0x1E5D;
  static const int PID_CELLVOLTAGE_CELL_31 = 0x1E5E;
  static const int PID_CELLVOLTAGE_CELL_32 = 0x1E5F;
  static const int PID_CELLVOLTAGE_CELL_33 = 0x1E60;
  static const int PID_CELLVOLTAGE_CELL_34 = 0x1E61;
  static const int PID_CELLVOLTAGE_CELL_35 = 0x1E62;
  static const int PID_CELLVOLTAGE_CELL_36 = 0x1E63;
  static const int PID_CELLVOLTAGE_CELL_37 = 0x1E64;
  static const int PID_CELLVOLTAGE_CELL_38 = 0x1E65;
  static const int PID_CELLVOLTAGE_CELL_39 = 0x1E66;
  static const int PID_CELLVOLTAGE_CELL_40 = 0x1E67;
  static const int PID_CELLVOLTAGE_CELL_41 = 0x1E68;
  static const int PID_CELLVOLTAGE_CELL_42 = 0x1E69;
  static const int PID_CELLVOLTAGE_CELL_43 = 0x1E6A;
  static const int PID_CELLVOLTAGE_CELL_44 = 0x1E6B;
  static const int PID_CELLVOLTAGE_CELL_45 = 0x1E6C;
  static const int PID_CELLVOLTAGE_CELL_46 = 0x1E6D;
  static const int PID_CELLVOLTAGE_CELL_47 = 0x1E6E;
  static const int PID_CELLVOLTAGE_CELL_48 = 0x1E6F;
  static const int PID_CELLVOLTAGE_CELL_49 = 0x1E70;
  static const int PID_CELLVOLTAGE_CELL_50 = 0x1E71;
  static const int PID_CELLVOLTAGE_CELL_51 = 0x1E72;
  static const int PID_CELLVOLTAGE_CELL_52 = 0x1E73;
  static const int PID_CELLVOLTAGE_CELL_53 = 0x1E74;
  static const int PID_CELLVOLTAGE_CELL_54 = 0x1E75;
  static const int PID_CELLVOLTAGE_CELL_55 = 0x1E76;
  static const int PID_CELLVOLTAGE_CELL_56 = 0x1E77;
  static const int PID_CELLVOLTAGE_CELL_57 = 0x1E78;
  static const int PID_CELLVOLTAGE_CELL_58 = 0x1E79;
  static const int PID_CELLVOLTAGE_CELL_59 = 0x1E7A;
  static const int PID_CELLVOLTAGE_CELL_60 = 0x1E7B;
  static const int PID_CELLVOLTAGE_CELL_61 = 0x1E7C;
  static const int PID_CELLVOLTAGE_CELL_62 = 0x1E7D;
  static const int PID_CELLVOLTAGE_CELL_63 = 0x1E7E;
  static const int PID_CELLVOLTAGE_CELL_64 = 0x1E7F;
  static const int PID_CELLVOLTAGE_CELL_65 = 0x1E80;
  static const int PID_CELLVOLTAGE_CELL_66 = 0x1E81;
  static const int PID_CELLVOLTAGE_CELL_67 = 0x1E82;
  static const int PID_CELLVOLTAGE_CELL_68 = 0x1E83;
  static const int PID_CELLVOLTAGE_CELL_69 = 0x1E84;
  static const int PID_CELLVOLTAGE_CELL_70 = 0x1E85;
  static const int PID_CELLVOLTAGE_CELL_71 = 0x1E86;
  static const int PID_CELLVOLTAGE_CELL_72 = 0x1E87;
  static const int PID_CELLVOLTAGE_CELL_73 = 0x1E88;
  static const int PID_CELLVOLTAGE_CELL_74 = 0x1E89;
  static const int PID_CELLVOLTAGE_CELL_75 = 0x1E8A;
  static const int PID_CELLVOLTAGE_CELL_76 = 0x1E8B;
  static const int PID_CELLVOLTAGE_CELL_77 = 0x1E8C;
  static const int PID_CELLVOLTAGE_CELL_78 = 0x1E8D;
  static const int PID_CELLVOLTAGE_CELL_79 = 0x1E8E;
  static const int PID_CELLVOLTAGE_CELL_80 = 0x1E8F;
  static const int PID_CELLVOLTAGE_CELL_81 = 0x1E90;
  static const int PID_CELLVOLTAGE_CELL_82 = 0x1E91;
  static const int PID_CELLVOLTAGE_CELL_83 = 0x1E92;
  static const int PID_CELLVOLTAGE_CELL_84 = 0x1E93;
  static const int PID_CELLVOLTAGE_CELL_85 = 0x1E94;
  static const int PID_CELLVOLTAGE_CELL_86 = 0x1E95;
  static const int PID_CELLVOLTAGE_CELL_87 = 0x1E96;
  static const int PID_CELLVOLTAGE_CELL_88 = 0x1E97;
  static const int PID_CELLVOLTAGE_CELL_89 = 0x1E98;
  static const int PID_CELLVOLTAGE_CELL_90 = 0x1E99;
  static const int PID_CELLVOLTAGE_CELL_91 = 0x1E9A;
  static const int PID_CELLVOLTAGE_CELL_92 = 0x1E9B;
  static const int PID_CELLVOLTAGE_CELL_93 = 0x1E9C;
  static const int PID_CELLVOLTAGE_CELL_94 = 0x1E9D;
  static const int PID_CELLVOLTAGE_CELL_95 = 0x1E9E;
  static const int PID_CELLVOLTAGE_CELL_96 = 0x1E9F;
  static const int PID_CELLVOLTAGE_CELL_97 = 0x1EA0;
  static const int PID_CELLVOLTAGE_CELL_98 = 0x1EA1;
  static const int PID_CELLVOLTAGE_CELL_99 = 0x1EA2;
  static const int PID_CELLVOLTAGE_CELL_100 = 0x1EA3;
  static const int PID_CELLVOLTAGE_CELL_101 = 0x1EA4;
  static const int PID_CELLVOLTAGE_CELL_102 = 0x1EA5;
  static const int PID_CELLVOLTAGE_CELL_103 = 0x1EA6;
  static const int PID_CELLVOLTAGE_CELL_104 = 0x1EA7;
  static const int PID_CELLVOLTAGE_CELL_105 = 0x1EA8;
  static const int PID_CELLVOLTAGE_CELL_106 = 0x1EA9;
  static const int PID_CELLVOLTAGE_CELL_107 = 0x1EAA;
  static const int PID_CELLVOLTAGE_CELL_108 = 0x1EAB;
  static const int PID_TEMP_POINT_1 = 0x1EAE;
  static const int PID_TEMP_POINT_2 = 0x1EAF;
  static const int PID_TEMP_POINT_3 = 0x1EB0;
  static const int PID_TEMP_POINT_4 = 0x1EB1;
  static const int PID_TEMP_POINT_5 = 0x1EB2;
  static const int PID_TEMP_POINT_6 = 0x1EB3;
  static const int PID_TEMP_POINT_7 = 0x1EB4;
  static const int PID_TEMP_POINT_8 = 0x1EB5;
  static const int PID_TEMP_POINT_9 = 0x1EB6;
  static const int PID_TEMP_POINT_10 = 0x1EB7;
  static const int PID_TEMP_POINT_11 = 0x1EB8;
  static const int PID_TEMP_POINT_12 = 0x1EB9;
  static const int PID_TEMP_POINT_13 = 0x1EBA;
  static const int PID_TEMP_POINT_14 = 0x1EBB;
  static const int PID_TEMP_POINT_15 = 0x1EBC;
  static const int PID_TEMP_POINT_16 = 0x1EBD;
  static const int PID_TEMP_POINT_17 = 0x1EBE;
  static const int PID_TEMP_POINT_18 = 0x1EBF;

  unsigned long previousMillis10ms = 0;   // will store last time a 10ms CAN Message was send
  unsigned long previousMillis20ms = 0;   // will store last time a 20ms CAN Message was send
  unsigned long previousMillis40ms = 0;   // will store last time a 40ms CAN Message was send
  unsigned long previousMillis50ms = 0;   // will store last time a 50ms CAN Message was send
  unsigned long previousMillis100ms = 0;  // will store last time a 100ms CAN Message was send
  unsigned long previousMillis200ms = 0;  // will store last time a 200ms CAN Message was send
  unsigned long previousMillis500ms = 0;  // will store last time a 200ms CAN Message was send
  unsigned long previousMillis1s = 0;     // will store last time a 1s CAN Message was send

  bool toggle = false;
  uint8_t counter_1000ms = 0;
  uint8_t counter_200ms = 0;
  uint8_t counter_100ms = 0;
  uint8_t counter_50ms = 0;
  uint8_t counter_40ms = 0;
  uint8_t counter_20ms = 0;
  uint8_t counter_10ms = 0;
  uint8_t counter_040 = 0;
  uint8_t counter_0F7 = 0;
  uint8_t counter_3b5 = 0;

  uint32_t poll_pid = PID_CELLVOLTAGE_CELL_85;  // We start here to quickly determine the cell size of the pack.
  bool nof_cells_determined = false;
  uint32_t pid_reply = 0;
  uint16_t battery_soc_polled = 0;
  uint16_t battery_voltage_polled = 1480;
  int16_t battery_current_polled = 0;
  int16_t battery_max_temp = 600;
  int16_t battery_min_temp = 600;
  uint16_t battery_max_charge_voltage = 0;
  uint16_t battery_min_discharge_voltage = 0;
  uint16_t battery_allowed_charge_power = 0;
  uint16_t battery_allowed_discharge_power = 0;
  uint16_t cellvoltages_polled[108];
  uint16_t tempval = 0;
  uint8_t BMS_16A954A6_CRC = 0;
  uint8_t BMS_5A2_counter = 0;
  uint8_t BMS_5CA_counter = 0;
  uint8_t BMS_0CF_counter = 0;
  uint8_t BMS_0C0_counter = 0;
  uint8_t BMS_578_counter = 0;
  uint8_t BMS_16A954A6_counter = 0;
  bool BMS_ext_limits_active =
      false;  //The current current limits of the HV battery are expanded to start the combustion engine / confirmation of the request
  uint8_t BMS_mode =
      0x07;  //0: standby; Gates open; Communication active 1: Main contactor closed / HV network activated / normal driving operation
  //2: assigned depending on the project (e.g. balancing, extended DC fast charging) //3: external charging
  uint8_t BMS_HVIL_status = 0;         //0 init, 1 seated, 2 open, 3 fault
  bool BMS_fault_performance = false;  //Error: Battery performance is limited (e.g. due to sensor or fan failure)
  uint16_t BMS_current = 16300;
  bool BMS_fault_emergency_shutdown_crash =
      false;  //Error: Safety-critical error (crash detection) Battery contactors are already opened / will be opened immediately Signal is read directly by the EMS and initiates an AKS of the PWR and an active discharge of the DC link
  uint32_t BMS_voltage_intermediate = 2000;
  uint32_t BMS_voltage = 1480;
  uint8_t BMS_status_voltage_free =
      0;  //0=Init, 1=BMS intermediate circuit voltage-free (U_Zwkr < 20V), 2=BMS intermediate circuit not voltage-free (U_Zwkr >/= 25V, hysteresis), 3=Error
  bool BMS_OBD_MIL = false;
  uint8_t BMS_error_status =
      0x7;  //0 Component_IO, 1 Restricted_CompFkt_Isoerror_I, 2 Restricted_CompFkt_Isoerror_II, 3 Restricted_CompFkt_Interlock, 4 Restricted_CompFkt_SD, 5 Restricted_CompFkt_Performance red, 6 = No component function, 7 = Init
  uint16_t BMS_capacity_ah = 0;
  bool BMS_error_lamp_req = false;
  bool BMS_warning_lamp_req = false;
  uint8_t BMS_Kl30c_Status = 0;  // 0 init, 1 closed, 2 open, 3 fault
  bool service_disconnect_switch_missing = false;
  bool pilotline_open = false;
  bool balancing_request = false;
  uint8_t battery_diagnostic = 0;
  uint16_t battery_Wh_left = 0;
  uint16_t battery_Wh_max = 1000;
  uint8_t battery_potential_status = 0;
  uint8_t battery_temperature_warning = 0;
  uint16_t max_discharge_power_watt = 0;
  uint16_t max_discharge_current_amp = 0;
  uint16_t max_charge_power_watt = 0;
  uint16_t max_charge_current_amp = 0;
  uint16_t battery_SOC = 1;
  uint16_t usable_energy_amount_Wh = 0;
  uint8_t status_HV_line = 0;  //0 init, 1 No open HV line, 2 open HV line detected, 3 fault
  uint8_t warning_support = 0;
  bool battery_heating_active = false;
  uint16_t power_discharge_percentage = 0;
  uint16_t power_charge_percentage = 0;
  uint16_t actual_battery_voltage = 0;
  uint16_t regen_battery = 0;
  uint16_t energy_extracted_from_battery = 0;
  uint16_t max_fastcharging_current_amp = 0;  //maximum DC charging current allowed
  uint8_t BMS_Status_DCLS =
      0;  //Status of the voltage monitoring on the DC charging interface. 0 inactive, 1 I_O , 2 N_I_O , 3 active
  uint16_t DC_voltage_DCLS = 0;  //Factor 1, DC voltage of the charging station. Measurement between the DC HV lines.
  uint16_t DC_voltage_chargeport =
      0;  //Factor 0.5,  Current voltage at the HV battery DC charging terminals; Outlet to the DC charging plug.
  uint8_t BMS_welded_contactors_status =
      0;  //0: Init no diagnostic result, 1: no contactor welded, 2: at least 1 contactor welded, 3: Protection status detection error
  bool BMS_error_shutdown_request =
      false;  // Fault: Fault condition, requires battery contactors to be opened internal battery error; Advance notification of an impending opening of the battery contactors by the BMS
  bool BMS_error_shutdown =
      false;  // Fault: Fault condition, requires battery contactors to be opened Internal battery error, battery contactors opened without notice by the BMS
  uint16_t power_battery_heating_watt = 0;
  uint16_t power_battery_heating_req_watt = 0;
  uint8_t cooling_request =
      0;  //0 = No cooling, 1 = Light cooling, cabin prio, 2= higher cooling, 3 = immediate cooling, 4 = emergency cooling
  uint8_t heating_request = 0;       //0 = init, 1= maintain temp, 2=higher demand, 3 = immediate heat demand
  uint8_t balancing_active = false;  //0 = init, 1 active, 2 not active
  bool charging_active = false;
  uint16_t max_energy_Wh = 0;
  uint16_t max_charge_percent = 0;
  uint16_t min_charge_percent = 0;
  uint16_t isolation_resistance_kOhm = 0;
  bool battery_heating_installed = false;
  bool error_NT_circuit = false;
  uint8_t pump_1_control = 0;             //0x0D not installed, 0x0E init, 0x0F fault
  uint8_t pump_2_control = 0;             //0x0D not installed, 0x0E init, 0x0F fault
  uint8_t target_flow_temperature_C = 0;  //*0,5 -40
  uint8_t return_temperature_C = 0;       //*0,5 -40
  uint8_t status_valve_1 = 0;             //0 not active, 1 active, 5 not installed, 6 init, 7 fault
  uint8_t status_valve_2 = 0;             //0 not active, 1 active, 5 not installed, 6 init, 7 fault
  uint8_t temperature_request =
      0;  //0 high cooling, 1 medium cooling, 2 low cooling, 3 no temp requirement init, 4 low heating , 5 medium heating, 6 high heating, 7 circulation
  uint16_t performance_index_discharge_peak_temperature_percentage = 0;
  uint16_t performance_index_charge_peak_temperature_percentage = 0;
  uint8_t temperature_status_discharge =
      0;                                  //0 init, 1 temp under optimal, 2 temp optimal, 3 temp over optimal, 7 fault
  uint8_t temperature_status_charge = 0;  //0 init, 1 temp under optimal, 2 temp optimal, 3 temp over optimal, 7 fault
  uint8_t isolation_fault =
      0;  //0 init, 1 no iso fault, 2 iso fault threshold1, 3 iso fault threshold2, 4 IWU defective
  uint8_t isolation_status =
      0;  // 0 init, 1 = larger threshold1, 2 = smaller threshold1 total, 3 = smaller threshold1 intern, 4 = smaller threshold2 total, 5 = smaller threshold2 intern, 6 = no measurement, 7 = measurement active
  uint8_t actual_temperature_highest_C = 0;    //*0,5 -40
  uint8_t actual_temperature_lowest_C = 0;     //*0,5 -40
  uint16_t actual_cellvoltage_highest_mV = 0;  //bias 1000
  uint16_t actual_cellvoltage_lowest_mV = 0;   //bias 1000
  uint16_t predicted_power_dyn_standard_watt = 0;
  uint8_t predicted_time_dyn_standard_minutes = 0;
  uint8_t mux = 0;
  uint16_t cellvoltages[160] = {0};
  uint16_t duration_discharge_power_watt = 0;
  uint16_t duration_charge_power_watt = 0;
  uint16_t maximum_voltage = 0;
  uint16_t minimum_voltage = 0;
  uint8_t battery_serialnumber[26];
  uint8_t realtime_overcurrent_monitor = 0;
  uint8_t realtime_CAN_communication_fault = 0;
  uint8_t realtime_overcharge_warning = 0;
  uint8_t realtime_SOC_too_high = 0;
  uint8_t realtime_SOC_too_low = 0;
  uint8_t realtime_SOC_jumping_warning = 0;
  uint8_t realtime_temperature_difference_warning = 0;
  uint8_t realtime_cell_overtemperature_warning = 0;
  uint8_t realtime_cell_undertemperature_warning = 0;
  uint8_t realtime_battery_overvoltage_warning = 0;
  uint8_t realtime_battery_undervoltage_warning = 0;
  uint8_t realtime_cell_overvoltage_warning = 0;
  uint8_t realtime_cell_undervoltage_warning = 0;
  uint8_t realtime_cell_imbalance_warning = 0;
  uint8_t realtime_warning_battery_unathorized = 0;
  bool component_protection_active = false;
  bool shutdown_active = false;
  bool transportation_mode_active = false;
  uint8_t KL15_mode = 0;
  uint8_t bus_knockout_timer = 0;
  uint8_t hybrid_wakeup_reason = 0;
  uint8_t wakeup_type = 0;
  bool instrumentation_cluster_request = false;
  uint8_t seconds = 0;
  uint32_t first_can_msg = 0;
  uint32_t last_can_msg_timestamp = 0;
  bool hv_requested = false;
  int32_t kwh_charge = 0;
  int32_t kwh_discharge = 0;

#define TIME_YEAR 2024
#define TIME_MONTH 8
#define TIME_DAY 20
#define TIME_HOUR 10
#define TIME_MINUTE 5
#define TIME_SECOND 0

#define BMS_TARGET_HV_OFF 0
#define BMS_TARGET_HV_ON 1
#define BMS_TARGET_AC_CHARGING_EXT 3  //(HS + AC_2 contactors closed)
#define BMS_TARGET_AC_CHARGING 4      //(HS + AC contactors closed)
#define BMS_TARGET_DC_CHARGING 6      //(HS + DC contactors closed)
#define BMS_TARGET_INIT 7

#define DC_FASTCHARGE_NO_START_REQUEST 0x00
#define DC_FASTCHARGE_VEHICLE 0x40
#define DC_FASTCHARGE_LS1 0x80
#define DC_FASTCHARGE_LS2 0xC0

  CAN_frame MEB_POLLING_FRAME = {.FD = true,
                                 .ext_ID = true,
                                 .DLC = 8,
                                 .ID = 0x1C40007B,  // SOC 02 8C
                                 .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
  CAN_frame MEB_ACK_FRAME = {.FD = true,
                             .ext_ID = true,
                             .DLC = 8,
                             .ID = 0x1C40007B,  // Ack
                             .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};
  //Messages needed for contactor closing
  CAN_frame MEB_040 = {.FD = true,  // Airbag
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x040,  //Frame5 has HV deactivate request. Needs to be 0x00
                       .data = {0x7E, 0x83, 0x00, 0x01, 0x00, 0x00, 0x15, 0x00}};
  CAN_frame MEB_0C0 = {
      .FD = true,  // EM1 message
      .ext_ID = false,
      .DLC = 32,
      .ID = 0x0C0,  //Frame 5-6 and maybe 7-8 important (external voltage at inverter)
      .data = {0x77, 0x0A, 0xFE, 0xE7, 0x7F, 0x10, 0x27, 0x00, 0xE0, 0x7F, 0xFF, 0xF3, 0x3F, 0xFF, 0xF3, 0x3F,
               0xFC, 0x0F, 0x00, 0x00, 0xC0, 0xFF, 0xFE, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_0FC = {
      .FD = true,  //
      .ext_ID = false,
      .DLC = 48,
      .ID = 0x0FC,  //This message contains emergency regen request?(byte19), battery needs to see this message
      .data = {0x07, 0x08, 0x00, 0x00, 0x7E, 0x00, 0x40, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0xFE, 0xFE, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
               0x00, 0x00, 0x00, 0xF4, 0x01, 0x40, 0xFF, 0xEB, 0x7F, 0x0A, 0x88, 0xE3, 0x81, 0xAF, 0x42}};
  CAN_frame MEB_6B2 = {.FD = true,  // Diagnostics
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x6B2,
                       .data = {0x6A, 0xA7, 0x37, 0x80, 0xC9, 0xBD, 0xF6, 0xC2}};
  CAN_frame MEB_17FC007B_poll = {.FD = true,  // Non period request
                                 .ext_ID = true,
                                 .DLC = 8,
                                 .ID = 0x17FC007B,
                                 .data = {0x03, 0x22, 0x1E, 0x3D, 0x55, 0x55, 0x55, 0x55}};
  CAN_frame MEB_1A5555A6 = {.FD = true,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1A5555A6,
                            .data = {0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_585 = {
      .FD = true,
      .ext_ID = false,
      .DLC = 8,
      .ID = 0x585,
      .data = {0xCF, 0x38, 0xAF, 0x5B, 0x25, 0x00, 0x00, 0x00}};  // CF 38 AF 5B 25 00 00 00 in start4.log
  //                     .data = {0xCF, 0x38, 0x20, 0x02, 0x25, 0xF7, 0x30, 0x00}}; // CF 38 AF 5B 25 00 00 00 in start4.log
  CAN_frame MEB_5F5 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x5F5,
                       .data = {0x23, 0x02, 0x39, 0xC0, 0x1B, 0x8B, 0xC8, 0x1B}};
  CAN_frame MEB_641 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x641,
                       .data = {0x37, 0x18, 0x00, 0x00, 0xF0, 0x00, 0xAA, 0x70}};
  CAN_frame MEB_3C0 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x3C0,
                       .data = {0x66, 0x00, 0x00, 0x00}};  // Klemmen_status_01
  CAN_frame MEB_0FD = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x0FD,  //CRC and counter, otherwise static
                       .data = {0x5F, 0xD0, 0x1F, 0x81, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_16A954FB = {.FD = true,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x16A954FB,
                            .data = {0x00, 0xC0, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_1A555548 = {.FD = true,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1A555548,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_1A55552B = {.FD = true,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1A55552B,
                            .data = {0x00, 0x00, 0x00, 0xA0, 0x02, 0x04, 0x00, 0x30}};
  CAN_frame MEB_569 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x569,  //HVEM
                       .data = {0x00, 0x00, 0x01, 0x3A, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_16A954B4 = {.FD = true,
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x16A954B4,  //eTM
                            .data = {0xFE, 0xB6, 0x0D, 0x00, 0x00, 0xD5, 0x48, 0xFD}};
  CAN_frame MEB_1B000046 = {.FD = false,  // Not FD
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1B000046,  // Klima
                            .data = {0x00, 0x40, 0x08, 0x01, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_1B000010 = {.FD = false,  // Not FD
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1B000010,  // Gateway
                            .data = {0x00, 0x50, 0x08, 0x50, 0x01, 0xFF, 0x30, 0x00}};
  CAN_frame MEB_1B0000B9 = {.FD = false,  // Not FD
                            .ext_ID = true,
                            .DLC = 8,
                            .ID = 0x1B0000B9,  //DC/DC converter
                            .data = {0x00, 0x40, 0x08, 0x08, 0x00, 0x00, 0x00, 0x00}};
  CAN_frame MEB_153 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x153,  // content
                       .data = {0x00, 0x00, 0x00, 0xFF, 0xEF, 0xFE, 0xFF, 0xFF}};
  CAN_frame MEB_5E1 = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x5E1,  // content
                       .data = {0x7F, 0x2A, 0x00, 0x60, 0xFE, 0x00, 0x00, 0x00}};
  CAN_frame MEB_3BE = {.FD = true,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x3BE,  // CRC, otherwise content
                       .data = {0x57, 0x0D, 0x00, 0x00, 0x00, 0x02, 0x04, 0x40}};
  CAN_frame MEB_272 = {.FD = true,  //HVLM_14
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x272,  // content
                       .data = {0x00, 0x00, 0x00, 0x00, 0x48, 0x08, 0x00, 0x94}};
  CAN_frame MEB_503 = {.FD = true,  //HVK_01
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x503,  // Content varies. Frame1 & 3 has HV req
                       .data = {0x5D, 0x61, 0x00, 0xFF, 0x7F, 0x80, 0xE3, 0x03}};
  CAN_frame MEB_14C = {
      .FD = true,  //Motor message
      .ext_ID = false,
      .DLC = 32,
      .ID = 0x14C,  //CRC needed, content otherwise
      .data = {0x38, 0x0A, 0xFF, 0x01, 0x01, 0xFF, 0x01, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
               0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0x25, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE}};

  uint32_t can_msg_received = 0;
};

#endif
