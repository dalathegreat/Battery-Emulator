#ifndef _DATALAYER_EXTENDED_H_
#define _DATALAYER_EXTENDED_H_

#include "../include.h"

typedef struct {
  /** uint16_t */
  /** SOC% raw battery value. Might not always reach 100% */
  uint16_t SOC_raw = 0;
  /** uint16_t */
  /** SOC% instrumentation cluster value. Will always reach 100% */
  uint16_t SOC_dash = 0;
  /** uint16_t */
  /** SOC% OBD2 value, polled actively */
  uint16_t SOC_OBD2 = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_ext = 0;
  /** uint8_t */
  /** Status isolation external, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_iso_int = 0;
  /** uint8_t */
  /** Status cooling valve error, 0 not evaluated, 1 OK valve closed, 2 error active valve open, 3 Invalid signal*/
  uint8_t ST_valve_cooling = 0;
  /** uint8_t */
  /** Status interlock error, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_interlock = 0;
  /** uint8_t */
  /** Status precharge, 0 no statement, 1 Not active closing not blocked, 2 error precharge blocked, 3 Invalid signal*/
  uint8_t ST_precharge = 0;
  /** uint8_t */
  /** Status DC switch, 0 contactors open, 1 precharge ongoing, 2 contactors engaged, 3 Invalid signal*/
  uint8_t ST_DCSW = 0;
  /** uint8_t */
  /** Status emergency, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_EMG = 0;
  /** uint8_t */
  /** Status welding detection, 0 Contactors OK, 1 One contactor welded, 2 Two contactors welded, 3 Invalid signal*/
  uint8_t ST_WELD = 0;
  /** uint8_t */
  /** Status isolation, 0 not evaluated, 1 OK, 2 error active, 3 Invalid signal*/
  uint8_t ST_isolation = 0;
  /** uint8_t */
  /** Status cold shutoff valve, 0 OK, 1 Short circuit to GND, 2 Short circuit to 12V, 3 Line break, 6 Driver error, 12 Stuck, 13 Stuck, 15 Invalid Signal*/
  uint8_t ST_cold_shutoff_valve = 0;
} DATALAYER_INFO_BMWI3;

typedef struct {
  /** uint16_t */
  /** SOC% estimate. Estimated from total pack voltage */
  uint16_t SOC_estimated = 0;
  /** uint16_t */
  /** SOC% raw battery value. Highprecision. Can be locked if pack is crashed */
  uint16_t SOC_highprec = 0;
  /** uint16_t */
  /** SOC% polled OBD2 value. Can be locked if pack is crashed */
  uint16_t SOC_polled = 0;
  /** uint16_t */
  /** Voltage raw battery value */
  uint16_t voltage_periodic = 0;
  /** uint16_t */
  /** Voltage polled OBD2*/
  uint16_t voltage_polled = 0;

} DATALAYER_INFO_BYDATTO3;

typedef struct {
  /** bool */
  /** All values either True or false */
  bool system_state_discharge = false;
  bool system_state_charge = false;
  bool system_state_cellbalancing = false;
  bool system_state_tricklecharge = false;
  bool system_state_idle = false;
  bool system_state_chargecompleted = false;
  bool system_state_maintenancecharge = false;
  bool IO_state_main_positive_relay = false;
  bool IO_state_main_negative_relay = false;
  bool IO_state_charge_enable = false;
  bool IO_state_precharge_relay = false;
  bool IO_state_discharge_enable = false;
  bool IO_state_IO_6 = false;
  bool IO_state_IO_7 = false;
  bool IO_state_IO_8 = false;
  bool error_Cell_overvoltage = false;
  bool error_Cell_undervoltage = false;
  bool error_Cell_end_of_life_voltage = false;
  bool error_Cell_voltage_misread = false;
  bool error_Cell_over_temperature = false;
  bool error_Cell_under_temperature = false;
  bool error_Cell_unmanaged = false;
  bool error_LMU_over_temperature = false;
  bool error_LMU_under_temperature = false;
  bool error_Temp_sensor_open_circuit = false;
  bool error_Temp_sensor_short_circuit = false;
  bool error_SUB_communication = false;
  bool error_LMU_communication = false;
  bool error_Over_current_IN = false;
  bool error_Over_current_OUT = false;
  bool error_Short_circuit = false;
  bool error_Leak_detected = false;
  bool error_Leak_detection_failed = false;
  bool error_Voltage_difference = false;
  bool error_BMCU_supply_over_voltage = false;
  bool error_BMCU_supply_under_voltage = false;
  bool error_Main_positive_contactor = false;
  bool error_Main_negative_contactor = false;
  bool error_Precharge_contactor = false;
  bool error_Midpack_contactor = false;
  bool error_Precharge_timeout = false;
  bool error_Emergency_connector_override = false;
  bool warning_High_cell_voltage = false;
  bool warning_Low_cell_voltage = false;
  bool warning_High_cell_temperature = false;
  bool warning_Low_cell_temperature = false;
  bool warning_High_LMU_temperature = false;
  bool warning_Low_LMU_temperature = false;
  bool warning_SUB_communication_interfered = false;
  bool warning_LMU_communication_interfered = false;
  bool warning_High_current_IN = false;
  bool warning_High_current_OUT = false;
  bool warning_Pack_resistance_difference = false;
  bool warning_High_pack_resistance = false;
  bool warning_Cell_resistance_difference = false;
  bool warning_High_cell_resistance = false;
  bool warning_High_BMCU_supply_voltage = false;
  bool warning_Low_BMCU_supply_voltage = false;
  bool warning_Low_SOC = false;
  bool warning_Balancing_required_OCV_model = false;
  bool warning_Charger_not_responding = false;
} DATALAYER_INFO_CELLPOWER;

typedef struct {
  /** uint8_t */
  /** Contactor status */
  uint8_t status_contactor = 0;
  /** uint8_t */
  /** Contactor status */
  uint8_t hvil_status = 0;
  /** uint8_t */
  /** Negative contactor state */
  uint8_t packContNegativeState = 0;
  /** uint8_t */
  /** Positive contactor state */
  uint8_t packContPositiveState = 0;
  /** uint8_t */
  /** Set state of contactors */
  uint8_t packContactorSetState = 0;
  /** uint8_t */
  /** Battery pack allows closing of contacors */
  uint8_t packCtrsClosingAllowed = 0;
  /** uint8_t */
  /** Pyro test in progress */
  uint8_t pyroTestInProgress = 0;

} DATALAYER_INFO_TESLA;

typedef struct {
  /** uint8_t */
  /** Enum, ZE0 = 0, AZE0 = 1, ZE1 = 2 */
  uint8_t LEAF_gen = 0;
  /** uint16_t */
  /** 77Wh per gid. LEAF specific unit */
  uint16_t GIDS = 0;
  /** uint16_t */
  /** Max regen power in kW */
  uint16_t ChargePowerLimit = 0;
  /** int16_t */
  /** Max charge power in kW */
  int16_t MaxPowerForCharger = 0;
  /** bool */
  /** Interlock status */
  bool Interlock = false;
  /** uint8_t */
  /** battery_FAIL status */
  uint8_t RelayCutRequest = 0;
  /** uint8_t */
  /** battery_STATUS status */
  uint8_t FailsafeStatus = 0;
  /** bool */
  /** True if fully charged */
  bool Full = false;
  /** bool */
  /** True if battery empty */
  bool Empty = false;
  /** bool */
  /** Battery pack allows closing of contacors */
  bool MainRelayOn = false;
  /** bool */
  /** True if heater exists */
  bool HeatExist = false;
  /** bool */
  /** Heater stopped */
  bool HeatingStop = false;
  /** bool */
  /** Heater starting */
  bool HeatingStart = false;
  /** bool */
  /** Heat request sent*/
  bool HeaterSendRequest = false;

} DATALAYER_INFO_NISSAN_LEAF;

typedef struct {
  /** uint8_t */
  /** Service disconnect switch status */
  bool SDSW = 0;
  /** uint8_t */
  /** Pilotline status */
  bool pilotline = 0;
  /** uint8_t */
  /** Transportation mode status */
  bool transportmode = 0;
  /** uint8_t */
  /** Componentprotection mode status */
  bool componentprotection = 0;
  /** uint8_t */
  /** Shutdown status */
  bool shutdown_active = 0;
  /** uint8_t */
  /** Battery heating status */
  bool battery_heating = 0;
  /** uint8_t */
  /** All realtime_ warnings have same enumeration, 0 = no fault, 1 = error level 1, 2 error level 2, 3 error level 3 */
  uint8_t rt_overcurrent = 0;
  uint8_t rt_CAN_fault = 0;
  uint8_t rt_overcharge = 0;
  uint8_t rt_SOC_high = 0;
  uint8_t rt_SOC_low = 0;
  uint8_t rt_SOC_jumping = 0;
  uint8_t rt_temp_difference = 0;
  uint8_t rt_cell_overtemp = 0;
  uint8_t rt_cell_undertemp = 0;
  uint8_t rt_battery_overvolt = 0;
  uint8_t rt_battery_undervol = 0;
  uint8_t rt_cell_overvolt = 0;
  uint8_t rt_cell_undervol = 0;
  uint8_t rt_cell_imbalance = 0;
  uint8_t rt_battery_unathorized = 0;
  /** uint8_t */
  /** HVIL status, 0 = Init, 1 = Closed, 2 = Open!, 3 = Fault */
  uint8_t HVIL = 0;
  /** uint8_t */
  /** 0 = HV inactive, 1 = HV active, 2 = Balancing, 3 = Extern charging, 4 = AC charging, 5 = Battery error, 6 = DC charging, 7 = init */
  uint8_t BMS_mode = 0;
  /** uint8_t */
  /** 1 = Battery display, 4 = Battery display OK, 4 = Display battery charging, 6 = Display battery check, 7 = Fault */
  uint8_t battery_diagnostic = 0;
  /** uint8_t */
  /** 0 = init, 1 = no open HV line detected, 2 = open HV line , 3 = fault */
  uint8_t status_HV_line = 0;
  /** uint8_t */
  /** 0 = OK, 1 = Not OK, 0x06 = init, 0x07 = fault */
  uint8_t warning_support = 0;
  /** uint32_t */
  /** Isolation resistance in kOhm */
  uint32_t isolation_resistance = 0;

} DATALAYER_INFO_MEB;

class DataLayerExtended {
 public:
  DATALAYER_INFO_BMWI3 bmwi3;
  DATALAYER_INFO_BYDATTO3 bydAtto3;
  DATALAYER_INFO_CELLPOWER cellpower;
  DATALAYER_INFO_TESLA tesla;
  DATALAYER_INFO_NISSAN_LEAF nissanleaf;
  DATALAYER_INFO_MEB meb;
};

extern DataLayerExtended datalayer_extended;

#endif
