#include "GROWATT-WIT-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* GROWATT BATTERY BMS CAN COMMUNICATION PROTOCOL V1.1 2024.7.19
 *
 * Protocol specifications:
 * - CAN 2.0B Extended Frame (29-bit identifier)
 * - 500 kbps
 * - Intel (Little Endian) byte order
 * - Cycle tolerance: ±5%
 *
 * This implementation emulates a Growatt-compatible battery BMS.
 */

void GrowattWitInverter::update_values() {

  /* ============================================================
   * 1AC3: Current/Voltage Limits (100ms cycle)
   * Basic function - Required for power control
   * ============================================================ */

  // Byte 0-1: Max Charge Current (0.1A, 0-10000 = 0-1000A)
  // Little Endian: low byte first
  GROWATT_1AC3.data.u8[0] = (datalayer.battery.status.max_charge_current_dA & 0xFF);
  GROWATT_1AC3.data.u8[1] = (datalayer.battery.status.max_charge_current_dA >> 8);

  // Byte 2-3: Max Discharge Current (0.1A, 0-10000)
  GROWATT_1AC3.data.u8[2] = (datalayer.battery.status.max_discharge_current_dA & 0xFF);
  GROWATT_1AC3.data.u8[3] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  // Byte 4-5: Max Charge Voltage (0.1V, 0-15000)
  uint16_t max_charge_voltage_dV;
  if (datalayer.battery.settings.user_set_voltage_limits_active) {
    max_charge_voltage_dV = datalayer.battery.settings.max_user_set_charge_voltage_dV;
  } else {
    max_charge_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  }
  GROWATT_1AC3.data.u8[4] = (max_charge_voltage_dV & 0xFF);
  GROWATT_1AC3.data.u8[5] = (max_charge_voltage_dV >> 8);

  // Byte 6-7: Min Discharge Voltage (0.1V, 0-15000)
  uint16_t min_discharge_voltage_dV;
  if (datalayer.battery.settings.user_set_voltage_limits_active) {
    min_discharge_voltage_dV = datalayer.battery.settings.max_user_set_discharge_voltage_dV;
  } else {
    min_discharge_voltage_dV = datalayer.battery.info.min_design_voltage_dV;
  }
  GROWATT_1AC3.data.u8[6] = (min_discharge_voltage_dV & 0xFF);
  GROWATT_1AC3.data.u8[7] = (min_discharge_voltage_dV >> 8);

  /* ============================================================
   * 1AC4: CV Voltage (100ms cycle)
   * Constant voltage charging threshold
   * ============================================================ */

  // Byte 0-3: Reserved
  GROWATT_1AC4.data.u8[0] = 0x00;
  GROWATT_1AC4.data.u8[1] = 0x00;
  GROWATT_1AC4.data.u8[2] = 0x00;
  GROWATT_1AC4.data.u8[3] = 0x00;

  // Byte 4-5: CV Voltage (0.1V)
  // Set CV voltage slightly below max charge voltage (10V = 100 dV below)
  uint16_t cv_voltage_dV = max_charge_voltage_dV > 100 ? max_charge_voltage_dV - 100 : max_charge_voltage_dV;
  GROWATT_1AC4.data.u8[4] = (cv_voltage_dV & 0xFF);
  GROWATT_1AC4.data.u8[5] = (cv_voltage_dV >> 8);

  // Byte 6-7: Reserved
  GROWATT_1AC4.data.u8[6] = 0x00;
  GROWATT_1AC4.data.u8[7] = 0x00;

  /* ============================================================
   * 1AC5: Working Status (100ms cycle)
   * BMS status and charge/discharge flags
   * ============================================================ */

  // Byte 0: BMS Working Status
  // 0=Init, 1=Standby, 2=Charging, 3=Discharging, 4=Shutdown, 5=Fault, 6=Upgrade
  if (datalayer.battery.status.bms_status == FAULT) {
    GROWATT_1AC5.data.u8[0] = 5;  // Fault
  } else if (datalayer.battery.status.current_dA > 5) {
    GROWATT_1AC5.data.u8[0] = 2;  // Charging (current > 0.5A)
  } else if (datalayer.battery.status.current_dA < -5) {
    GROWATT_1AC5.data.u8[0] = 3;  // Discharging (current < -0.5A)
  } else {
    GROWATT_1AC5.data.u8[0] = 1;  // Standby
  }

  // Byte 1: Charge Flag
  // Bit 0: 0=Allow charge, 1=Prohibit charge
  // Bit 1: 0=Force charge OFF, 1=Force charge ON
  uint8_t charge_flag = 0x00;
  if (datalayer.battery.status.max_charge_current_dA == 0 || datalayer.battery.status.reported_soc >= 10000 ||  // 100%
      datalayer.battery.status.bms_status == FAULT) {
    charge_flag |= 0x01;  // Prohibit charge
  }
  GROWATT_1AC5.data.u8[1] = charge_flag;

  // Byte 2: Discharge Flag
  // Bit 0: 0=Allow discharge, 1=Prohibit discharge
  // Bit 1: 0=Force discharge OFF, 1=Force discharge ON
  // Bit 3: 0=Normal, 1=Soft starting
  uint8_t discharge_flag = 0x00;
  if (datalayer.battery.status.max_discharge_current_dA == 0 || datalayer.battery.status.reported_soc == 0 ||
      datalayer.battery.status.bms_status == FAULT) {
    discharge_flag |= 0x01;  // Prohibit discharge
  }
  GROWATT_1AC5.data.u8[2] = discharge_flag;

  // Byte 3-7: Reserved
  GROWATT_1AC5.data.u8[3] = 0x00;
  GROWATT_1AC5.data.u8[4] = 0x00;
  GROWATT_1AC5.data.u8[5] = 0x00;
  GROWATT_1AC5.data.u8[6] = 0x00;
  GROWATT_1AC5.data.u8[7] = 0x00;

  /* ============================================================
   * 1AC6: SOC/SOH/Capacity (500ms cycle)
   * ============================================================ */

  // Byte 0: SOC (0-100%)
  GROWATT_1AC6.data.u8[0] = (uint8_t)(datalayer.battery.status.reported_soc / 100);

  // Byte 1: SOH (0-100%, bit7 = scrap warning if 1)
  uint8_t soh = (uint8_t)(datalayer.battery.status.soh_pptt / 100);
  if (soh < 50) {
    soh |= 0x80;  // Set scrap warning bit if SOH < 50%
  }
  GROWATT_1AC6.data.u8[1] = soh;

  // Byte 2-3: Rated Capacity (0.1Ah, 0-50000)
  // Convert Wh to Ah: Ah = Wh / V
  uint16_t rated_capacity_dAh = 0;
  if (datalayer.battery.status.voltage_dV > 0) {
    // total_capacity_Wh / (voltage_dV / 10) = capacity_Ah
    // capacity_dAh = capacity_Ah * 10 = total_capacity_Wh * 100 / voltage_dV
    uint32_t capacity_calc = (datalayer.battery.info.total_capacity_Wh * 100UL) / datalayer.battery.status.voltage_dV;
    // Clamp to uint16_t max (50000 per protocol, but allow up to 65535)
    rated_capacity_dAh = (capacity_calc > 50000) ? 50000 : (uint16_t)capacity_calc;
  }
  GROWATT_1AC6.data.u8[2] = (rated_capacity_dAh & 0xFF);
  GROWATT_1AC6.data.u8[3] = (rated_capacity_dAh >> 8);

  // Byte 4-7: Reserved
  GROWATT_1AC6.data.u8[4] = 0x00;
  GROWATT_1AC6.data.u8[5] = 0x00;
  GROWATT_1AC6.data.u8[6] = 0x00;
  GROWATT_1AC6.data.u8[7] = 0x00;

  /* ============================================================
   * 1AC7: Voltage/Current (100ms cycle)
   * ============================================================ */

  // Byte 0-1: Reserved
  GROWATT_1AC7.data.u8[0] = 0x00;
  GROWATT_1AC7.data.u8[1] = 0x00;

  // Byte 2-3: Battery Voltage (0.1V, 0-15000)
  GROWATT_1AC7.data.u8[2] = (datalayer.battery.status.voltage_dV & 0xFF);
  GROWATT_1AC7.data.u8[3] = (datalayer.battery.status.voltage_dV >> 8);

  // Byte 4-5: Battery Current (0.1A, offset -1000A)
  // Raw = (Actual + 1000) * 10 = Actual_dA + 10000
  int16_t current_dA = datalayer.battery.status.current_dA;
  uint16_t current_raw = (uint16_t)(current_dA + CURRENT_OFFSET_DA);
  GROWATT_1AC7.data.u8[4] = (current_raw & 0xFF);
  GROWATT_1AC7.data.u8[5] = (current_raw >> 8);

  // Byte 6-7: Reserved
  GROWATT_1AC7.data.u8[6] = 0x00;
  GROWATT_1AC7.data.u8[7] = 0x00;

  /* ============================================================
   * 1AC0: System Composition (2000ms cycle)
   * ============================================================ */

  // Byte 0: Stack Sum (1-10)
  GROWATT_1AC0.data.u8[0] = 1;

  // Byte 1: Cluster Sum (1-16)
  GROWATT_1AC0.data.u8[1] = 1;

  // Byte 2: Modules per Cluster (1-32)
  GROWATT_1AC0.data.u8[2] = 1;

  // Byte 3: Cells per Module (1-32, clamp to protocol limits)
  uint8_t cells_per_module = datalayer.battery.info.number_of_cells;
  if (cells_per_module == 0) {
    cells_per_module = 16;  // Default
  } else if (cells_per_module > 32) {
    cells_per_module = 32;  // Protocol max
  }
  GROWATT_1AC0.data.u8[3] = cells_per_module;

  // Byte 4-5: Module Rated Voltage (0.01V)
  // Use pack voltage as module voltage for single module setup
  // Clamp to prevent overflow: max 6553.5V (65535 cV)
  uint32_t voltage_cV_calc = (uint32_t)datalayer.battery.status.voltage_dV * 10;
  uint16_t module_voltage_cV = (voltage_cV_calc > 65535) ? 65535 : (uint16_t)voltage_cV_calc;
  GROWATT_1AC0.data.u8[4] = (module_voltage_cV & 0xFF);
  GROWATT_1AC0.data.u8[5] = (module_voltage_cV >> 8);

  // Byte 6-7: Module Rated Capacity (0.1Ah)
  GROWATT_1AC0.data.u8[6] = (rated_capacity_dAh & 0xFF);
  GROWATT_1AC0.data.u8[7] = (rated_capacity_dAh >> 8);

  /* ============================================================
   * 1AC2: Product Version (Event - triggered by PCS)
   * ============================================================ */

  // Byte 0: Cell Model (1=LFP_50Ah, 2=NCM_50Ah, 3=LFP_100Ah, 4=LFP_280Ah)
  GROWATT_1AC2.data.u8[0] = 3;  // LFP_100Ah as default

  // Byte 1: Cell Manufacturer (1=RIPO, 2=EVE, 3=BYD, 4=ATL, 5=CATL, 6=SUNWODA)
  GROWATT_1AC2.data.u8[1] = 5;  // CATL as default

  // Byte 2: Pack Foundry (0=default)
  GROWATT_1AC2.data.u8[2] = 0;

  // Byte 3: Pack Solution Provider (0=default)
  GROWATT_1AC2.data.u8[3] = 0;

  // Byte 4: Pack Version (1=AL1, etc.)
  GROWATT_1AC2.data.u8[4] = 1;

  // Byte 5: BMS HW Version
  GROWATT_1AC2.data.u8[5] = 1;

  // Byte 6: BMS HW Mode (0=default)
  GROWATT_1AC2.data.u8[6] = 0;

  // Byte 7: Reserved
  GROWATT_1AC2.data.u8[7] = 0;

  /* ============================================================
   * 1AC8: Software Version (500ms cycle)
   * ============================================================ */

  // Byte 0-3: Software Code (ASCII, e.g., "EMUL")
  GROWATT_1AC8.data.u8[0] = 'E';
  GROWATT_1AC8.data.u8[1] = 'M';
  GROWATT_1AC8.data.u8[2] = 'U';
  GROWATT_1AC8.data.u8[3] = 'L';

  // Byte 4-5: Monitor Version
  GROWATT_1AC8.data.u8[4] = 0x01;
  GROWATT_1AC8.data.u8[5] = 0x00;

  // Byte 6: BOOT Version
  GROWATT_1AC8.data.u8[6] = 0x01;

  // Byte 7: Software Subversion
  GROWATT_1AC8.data.u8[7] = 0x00;

  /* ============================================================
   * 1AC9: Max Module Voltage (1000ms cycle)
   * ============================================================ */

  // Byte 0: Stack Number (1-10)
  GROWATT_1AC9.data.u8[0] = 1;

  // Byte 1: Cluster Number (1-10)
  GROWATT_1AC9.data.u8[1] = 1;

  // Byte 2: Module Number (1-100)
  GROWATT_1AC9.data.u8[2] = 1;

  // Byte 3: Reserved
  GROWATT_1AC9.data.u8[3] = 0;

  // Byte 4-5: Max Module Voltage (0.01V)
  // For single module, use pack voltage
  GROWATT_1AC9.data.u8[4] = (module_voltage_cV & 0xFF);
  GROWATT_1AC9.data.u8[5] = (module_voltage_cV >> 8);

  // Byte 6-7: Avg Module Voltage (0.01V)
  GROWATT_1AC9.data.u8[6] = (module_voltage_cV & 0xFF);
  GROWATT_1AC9.data.u8[7] = (module_voltage_cV >> 8);

  /* ============================================================
   * 1ACA: Min Module Voltage (1000ms cycle)
   * ============================================================ */

  // Byte 0-2: Stack/Cluster/Module Number
  GROWATT_1ACA.data.u8[0] = 1;
  GROWATT_1ACA.data.u8[1] = 1;
  GROWATT_1ACA.data.u8[2] = 1;

  // Byte 3: Reserved
  GROWATT_1ACA.data.u8[3] = 0;

  // Byte 4-5: Min Module Voltage (0.01V)
  GROWATT_1ACA.data.u8[4] = (module_voltage_cV & 0xFF);
  GROWATT_1ACA.data.u8[5] = (module_voltage_cV >> 8);

  // Byte 6-7: Reserved
  GROWATT_1ACA.data.u8[6] = 0;
  GROWATT_1ACA.data.u8[7] = 0;

  /* ============================================================
   * 1ACC: Max Cell Temperature (1000ms cycle)
   * ============================================================ */

  // Byte 0-3: Stack/Cluster/Module/Cell Number
  GROWATT_1ACC.data.u8[0] = 1;
  GROWATT_1ACC.data.u8[1] = 1;
  GROWATT_1ACC.data.u8[2] = 1;
  GROWATT_1ACC.data.u8[3] = 1;

  // Byte 4-5: Max Cell Temp (0.1°C, offset -40°C)
  // Raw = (Actual + 40) * 10 = Actual_dC + 400
  // Clamp to valid range: -40°C to 125°C (-400 to 1250 dC, raw 0 to 1650)
  int16_t temp_max_dC = datalayer.battery.status.temperature_max_dC;
  int32_t temp_max_calc = temp_max_dC + TEMP_OFFSET_DC;
  uint16_t temp_max_raw = (temp_max_calc < 0) ? 0 : (temp_max_calc > 1650) ? 1650 : (uint16_t)temp_max_calc;
  GROWATT_1ACC.data.u8[4] = (temp_max_raw & 0xFF);
  GROWATT_1ACC.data.u8[5] = (temp_max_raw >> 8);

  // Byte 6-7: Avg Cell Temp (0.1°C, offset -40°C)
  // Use average of min and max
  int16_t temp_avg_dC = (datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2;
  int32_t temp_avg_calc = temp_avg_dC + TEMP_OFFSET_DC;
  uint16_t temp_avg_raw = (temp_avg_calc < 0) ? 0 : (temp_avg_calc > 1650) ? 1650 : (uint16_t)temp_avg_calc;
  GROWATT_1ACC.data.u8[6] = (temp_avg_raw & 0xFF);
  GROWATT_1ACC.data.u8[7] = (temp_avg_raw >> 8);

  /* ============================================================
   * 1ACD: Min Cell Temperature (1000ms cycle)
   * ============================================================ */

  // Byte 0-3: Stack/Cluster/Module/Cell Number
  GROWATT_1ACD.data.u8[0] = 1;
  GROWATT_1ACD.data.u8[1] = 1;
  GROWATT_1ACD.data.u8[2] = 1;
  GROWATT_1ACD.data.u8[3] = 2;

  // Byte 4-5: Min Cell Temp (0.1°C, offset -40°C)
  // Clamp to valid range: -40°C to 125°C
  int16_t temp_min_dC = datalayer.battery.status.temperature_min_dC;
  int32_t temp_min_calc = temp_min_dC + TEMP_OFFSET_DC;
  uint16_t temp_min_raw = (temp_min_calc < 0) ? 0 : (temp_min_calc > 1650) ? 1650 : (uint16_t)temp_min_calc;
  GROWATT_1ACD.data.u8[4] = (temp_min_raw & 0xFF);
  GROWATT_1ACD.data.u8[5] = (temp_min_raw >> 8);

  // Byte 6-7: Environment Temp (use min temp as approximation)
  GROWATT_1ACD.data.u8[6] = (temp_min_raw & 0xFF);
  GROWATT_1ACD.data.u8[7] = (temp_min_raw >> 8);

  /* ============================================================
   * 1ACE: Max Cell Voltage (100ms cycle)
   * ============================================================ */

  // Byte 0-3: Stack/Cluster/Module/Cell Number
  GROWATT_1ACE.data.u8[0] = 1;
  GROWATT_1ACE.data.u8[1] = 1;
  GROWATT_1ACE.data.u8[2] = 1;
  GROWATT_1ACE.data.u8[3] = 1;

  // Byte 4-5: Max Cell Voltage (1mV, 0-5000)
  uint16_t cell_max_mV = datalayer.battery.status.cell_max_voltage_mV;
  GROWATT_1ACE.data.u8[4] = (cell_max_mV & 0xFF);
  GROWATT_1ACE.data.u8[5] = (cell_max_mV >> 8);

  // Byte 6-7: Avg Cell Voltage (1mV, 0-5000)
  uint16_t cell_avg_mV =
      (datalayer.battery.status.cell_max_voltage_mV + datalayer.battery.status.cell_min_voltage_mV) / 2;
  GROWATT_1ACE.data.u8[6] = (cell_avg_mV & 0xFF);
  GROWATT_1ACE.data.u8[7] = (cell_avg_mV >> 8);

  /* ============================================================
   * 1ACF: Min Cell Voltage (100ms cycle)
   * ============================================================ */

  // Byte 0-3: Stack/Cluster/Module/Cell Number
  GROWATT_1ACF.data.u8[0] = 1;
  GROWATT_1ACF.data.u8[1] = 1;
  GROWATT_1ACF.data.u8[2] = 1;
  GROWATT_1ACF.data.u8[3] = 2;

  // Byte 4-5: Min Cell Voltage (1mV, 0-5000)
  uint16_t cell_min_mV = datalayer.battery.status.cell_min_voltage_mV;
  GROWATT_1ACF.data.u8[4] = (cell_min_mV & 0xFF);
  GROWATT_1ACF.data.u8[5] = (cell_min_mV >> 8);

  // Byte 6-7: Reserved
  GROWATT_1ACF.data.u8[6] = 0;
  GROWATT_1ACF.data.u8[7] = 0;

  /* ============================================================
   * 1AD0: Cluster SOC Info (1000ms cycle)
   * ============================================================ */

  uint8_t soc_percent = (uint8_t)(datalayer.battery.status.reported_soc / 100);

  // Byte 0-1: Max SOC Stack/Cluster
  GROWATT_1AD0.data.u8[0] = 1;
  GROWATT_1AD0.data.u8[1] = 1;

  // Byte 2-3: Min SOC Stack/Cluster
  GROWATT_1AD0.data.u8[2] = 1;
  GROWATT_1AD0.data.u8[3] = 1;

  // Byte 4: Max SOC (0-100%)
  GROWATT_1AD0.data.u8[4] = soc_percent;

  // Byte 5: Min SOC (0-100%)
  GROWATT_1AD0.data.u8[5] = soc_percent;

  // Byte 6: Avg SOC (0-100%)
  GROWATT_1AD0.data.u8[6] = soc_percent;

  // Byte 7: Reserved
  GROWATT_1AD0.data.u8[7] = 0;

  /* ============================================================
   * 1AD1: Cumulative Energy (1000ms cycle)
   * ============================================================ */

  // Byte 0-2: Accumulated Charge (0.1kWh, U24)
  // Convert Wh to 0.1kWh: value = Wh / 100
  uint32_t acc_charge_01kWh = 0;
  if (datalayer.battery.status.total_charged_battery_Wh > 0) {
    acc_charge_01kWh = (uint32_t)(datalayer.battery.status.total_charged_battery_Wh / 100);
  }
  GROWATT_1AD1.data.u8[0] = (acc_charge_01kWh & 0xFF);
  GROWATT_1AD1.data.u8[1] = ((acc_charge_01kWh >> 8) & 0xFF);
  GROWATT_1AD1.data.u8[2] = ((acc_charge_01kWh >> 16) & 0xFF);

  // Byte 3-5: Accumulated Discharge (0.1kWh, U24)
  uint32_t acc_discharge_01kWh = 0;
  if (datalayer.battery.status.total_discharged_battery_Wh > 0) {
    acc_discharge_01kWh = (uint32_t)(datalayer.battery.status.total_discharged_battery_Wh / 100);
  }
  GROWATT_1AD1.data.u8[3] = (acc_discharge_01kWh & 0xFF);
  GROWATT_1AD1.data.u8[4] = ((acc_discharge_01kWh >> 8) & 0xFF);
  GROWATT_1AD1.data.u8[5] = ((acc_discharge_01kWh >> 16) & 0xFF);

  // Byte 6-7: Charge/Discharge Power (1W, offset -32000W)
  // Raw = Power_W + 32000, valid range 0-65535 (represents -32000W to +33535W)
  int32_t power_W = datalayer.battery.status.active_power_W;
  int32_t power_calc = power_W + 32000;
  uint16_t power_raw;
  if (power_calc < 0) {
    power_raw = 0;  // Clamp to min (-32000W)
  } else if (power_calc > 65535) {
    power_raw = 65535;  // Clamp to max (+33535W)
  } else {
    power_raw = (uint16_t)power_calc;
  }
  GROWATT_1AD1.data.u8[6] = (power_raw & 0xFF);
  GROWATT_1AD1.data.u8[7] = (power_raw >> 8);

  /* ============================================================
   * 1AD8: Fault Information 3 (100ms cycle)
   * Charge/Discharge Alarms and Protections
   * ============================================================ */

  uint16_t charge_alarm = 0;
  uint16_t discharge_alarm = 0;
  uint16_t charge_protection = 0;
  uint16_t discharge_protection = 0;

  // Check for fault conditions and set appropriate bits
  if (datalayer.battery.status.bms_status == FAULT) {
    // Set general fault indicators
    charge_protection |= 0x01;     // Cell overvoltage protection
    discharge_protection |= 0x01;  // Cell undervoltage protection
  }

  // Check cell voltage limits
  if (datalayer.battery.status.cell_max_voltage_mV >= datalayer.battery.info.max_cell_voltage_mV) {
    charge_alarm |= 0x01;       // Cell overvoltage alarm
    charge_protection |= 0x01;  // Cell overvoltage protection
  }
  if (datalayer.battery.status.cell_min_voltage_mV <= datalayer.battery.info.min_cell_voltage_mV) {
    discharge_alarm |= 0x01;       // Cell undervoltage alarm
    discharge_protection |= 0x01;  // Cell undervoltage protection
  }

  // Check temperature limits (assuming reasonable limits)
  if (datalayer.battery.status.temperature_max_dC > 550) {  // > 55°C
    charge_alarm |= 0x10;                                   // Charging temp too high
    discharge_alarm |= 0x10;                                // Discharge temp too high
    charge_protection |= 0x10;
    discharge_protection |= 0x10;
  }
  if (datalayer.battery.status.temperature_min_dC < 0) {  // < 0°C
    charge_alarm |= 0x08;                                 // Charging temp too low
    charge_protection |= 0x08;                            // Low charging temp protection
  }

  // Check SOC limits
  if (soc_percent <= 5) {
    discharge_alarm |= 0x40;       // SOC too low
    discharge_protection |= 0x40;  // SOC too low protection
  }

  // Byte 0-1: Charging Alarm
  GROWATT_1AD8.data.u8[0] = (charge_alarm & 0xFF);
  GROWATT_1AD8.data.u8[1] = (charge_alarm >> 8);

  // Byte 2-3: Discharge Alarm
  GROWATT_1AD8.data.u8[2] = (discharge_alarm & 0xFF);
  GROWATT_1AD8.data.u8[3] = (discharge_alarm >> 8);

  // Byte 4-5: Charging Protection
  GROWATT_1AD8.data.u8[4] = (charge_protection & 0xFF);
  GROWATT_1AD8.data.u8[5] = (charge_protection >> 8);

  // Byte 6-7: Discharge Protection
  GROWATT_1AD8.data.u8[6] = (discharge_protection & 0xFF);
  GROWATT_1AD8.data.u8[7] = (discharge_protection >> 8);

  /* ============================================================
   * 1AD9: Fault Information 4 (100ms cycle)
   * Fault codes and generic alarms/protections
   * ============================================================ */

  uint32_t fault_code = 0;
  uint16_t generic_alarm = 0;
  uint16_t generic_protection = 0;

  // Set fault code if BMS is in fault state
  if (datalayer.battery.status.bms_status == FAULT) {
    fault_code |= 0x01;  // Cell fault
  }

  // Check cell voltage deviation
  uint16_t cell_deviation = datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV;
  if (cell_deviation > datalayer.battery.info.max_cell_voltage_deviation_mV) {
    generic_alarm |= 0x01;       // Cell voltage difference excessive
    generic_protection |= 0x01;  // Cell voltage difference protection
  }

  // Check temperature difference
  int16_t temp_diff = datalayer.battery.status.temperature_max_dC - datalayer.battery.status.temperature_min_dC;
  if (temp_diff > 150) {    // > 15°C difference
    generic_alarm |= 0x40;  // Cell temp difference too high
  }

  // Byte 0-3: Fault code (U32)
  GROWATT_1AD9.data.u8[0] = (fault_code & 0xFF);
  GROWATT_1AD9.data.u8[1] = ((fault_code >> 8) & 0xFF);
  GROWATT_1AD9.data.u8[2] = ((fault_code >> 16) & 0xFF);
  GROWATT_1AD9.data.u8[3] = ((fault_code >> 24) & 0xFF);

  // Byte 4-5: Generic Alarm
  GROWATT_1AD9.data.u8[4] = (generic_alarm & 0xFF);
  GROWATT_1AD9.data.u8[5] = (generic_alarm >> 8);

  // Byte 6-7: Generic Protection
  GROWATT_1AD9.data.u8[6] = (generic_protection & 0xFF);
  GROWATT_1AD9.data.u8[7] = (generic_protection >> 8);

  /* ============================================================
   * 1A80: BMS Software Version (Event)
   * ============================================================ */

  // Byte 0-3: BMS Software Code (ASCII)
  GROWATT_1A80.data.u8[0] = 'B';
  GROWATT_1A80.data.u8[1] = 'E';
  GROWATT_1A80.data.u8[2] = 'M';
  GROWATT_1A80.data.u8[3] = 'U';

  // Byte 4-5: BMS Version
  GROWATT_1A80.data.u8[4] = 0x01;
  GROWATT_1A80.data.u8[5] = 0x00;

  // Byte 6-7: DTC (Battery Type) - 13000 = third-party battery
  uint16_t dtc = 13000;
  GROWATT_1A80.data.u8[6] = (dtc & 0xFF);
  GROWATT_1A80.data.u8[7] = (dtc >> 8);

  /* ============================================================
   * 1A82: Battery Serial Number (Event - multi-frame)
   * ============================================================ */
  // Will be sent in transmit_can when triggered
}

void GrowattWitInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  // Validate extended frame (29-bit ID required for Growatt WIT protocol)
  if (!rx_frame.ext_ID) {
    return;  // Ignore standard 11-bit frames
  }

  // Validate minimum DLC for message parsing
  if (rx_frame.DLC < 2) {
    return;  // Need at least 2 bytes for most messages
  }

  // Extract FSN from 29-bit CAN ID
  uint8_t fsn = get_fsn_from_id(rx_frame.ID);

  // Update last message timestamp for timeout detection
  last_inverter_msg_ms = millis();

  switch (fsn) {
    case FSN_HEARTBEAT:  // 1AB5: Heartbeat from PCS (1000ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      // Byte 0-1: Frame count (increments each send)
      pcs_frame_count = rx_frame.data.u8[0] | ((uint16_t)rx_frame.data.u8[1] << 8);
      break;

    case FSN_DATETIME:  // 1AB6: Date/Time from PCS (1000ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      // Could extract date/time here if needed
      // Byte 0: Year (20-99, actual = 2000 + value)
      // Byte 1: Month (1-12)
      // Byte 2: Day (1-31)
      // Byte 3: Hour (0-24)
      // Byte 4: Minute (0-59)
      // Byte 5: Second (0-59)
      break;

    case FSN_PCS_STATUS:  // 1AB7: PCS Status (1000ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      // Byte 0: PCS Working Status (0=Init, 1=Standby, 2=Charging, 3=Discharging, 4=Shutdown, 5=Fault, 6=Upgrade)
      pcs_working_status = rx_frame.data.u8[0];
      // Byte 1: PCS Fault Status (0=Init, 1=Fault, 2=Alarm)
      pcs_fault_status = rx_frame.data.u8[1];
      break;

    case FSN_BUS_VOLTAGE:  // 1AB8: Bus Voltage Setting (50ms)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      // Byte 0-1: Bus Voltage Setting (0.1V, 3400-10000 = 340-1000V)
      pcs_bus_voltage_dV = rx_frame.data.u8[0] | ((uint16_t)rx_frame.data.u8[1] << 8);
      break;

    case FSN_PCS_PRODUCT:  // 1ABE: PCS Product Info (Event)
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_alive = true;
      // This message triggers BMS to send event messages (1AC2, 1A80, 1A82)
      transmit_can_frame(&GROWATT_1AC2);
      transmit_can_frame(&GROWATT_1A80);

      // Send multi-frame serial number (3 frames)
      for (sn_frame_counter = 0; sn_frame_counter < 3; sn_frame_counter++) {
        GROWATT_1A82.data.u8[0] = sn_frame_counter;  // Frame number
        // SN characters (placeholder - "BATTERY-EMULATOR")
        switch (sn_frame_counter) {
          case 0:
            GROWATT_1A82.data.u8[1] = 'B';
            GROWATT_1A82.data.u8[2] = 'A';
            GROWATT_1A82.data.u8[3] = 'T';
            GROWATT_1A82.data.u8[4] = 'T';
            GROWATT_1A82.data.u8[5] = 'E';
            GROWATT_1A82.data.u8[6] = 'R';
            GROWATT_1A82.data.u8[7] = 'Y';
            break;
          case 1:
            GROWATT_1A82.data.u8[1] = '-';
            GROWATT_1A82.data.u8[2] = 'E';
            GROWATT_1A82.data.u8[3] = 'M';
            GROWATT_1A82.data.u8[4] = 'U';
            GROWATT_1A82.data.u8[5] = 'L';
            GROWATT_1A82.data.u8[6] = 'A';
            GROWATT_1A82.data.u8[7] = 'T';
            break;
          case 2:
            GROWATT_1A82.data.u8[1] = 'O';
            GROWATT_1A82.data.u8[2] = 'R';
            GROWATT_1A82.data.u8[3] = '0';
            GROWATT_1A82.data.u8[4] = '0';
            GROWATT_1A82.data.u8[5] = '1';
            GROWATT_1A82.data.u8[6] = 0;
            GROWATT_1A82.data.u8[7] = 0;
            break;
        }
        transmit_can_frame(&GROWATT_1A82);
      }
      break;

    default:
      break;
  }
}

void GrowattWitInverter::transmit_can(unsigned long currentMillis) {

  // Don't send until inverter is alive (has sent heartbeat)
  if (!inverter_alive) {
    return;
  }

  /* ============================================================
   * 100ms messages (Basic functions - Required)
   * ============================================================ */
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    // Basic required messages
    transmit_can_frame(&GROWATT_1AC3);  // Current/Voltage limits
    transmit_can_frame(&GROWATT_1AC4);  // CV voltage
    transmit_can_frame(&GROWATT_1AC5);  // Working status
    transmit_can_frame(&GROWATT_1AC7);  // Voltage/Current

    // Optional monitoring - Cell voltages
    transmit_can_frame(&GROWATT_1ACE);  // Max cell voltage
    transmit_can_frame(&GROWATT_1ACF);  // Min cell voltage

    // Optional monitoring - Fault information
    transmit_can_frame(&GROWATT_1AD8);  // Fault info 3
    transmit_can_frame(&GROWATT_1AD9);  // Fault info 4
  }

  /* ============================================================
   * 500ms messages
   * ============================================================ */
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&GROWATT_1AC6);  // SOC/SOH/Capacity
    transmit_can_frame(&GROWATT_1AC8);  // Software version
  }

  /* ============================================================
   * 1000ms messages
   * ============================================================ */
  if (currentMillis - previousMillis1000ms >= INTERVAL_1_S) {
    previousMillis1000ms = currentMillis;

    // Module voltage info
    transmit_can_frame(&GROWATT_1AC9);  // Max module voltage
    transmit_can_frame(&GROWATT_1ACA);  // Min module voltage

    // Temperature info
    transmit_can_frame(&GROWATT_1ACC);  // Max cell temperature
    transmit_can_frame(&GROWATT_1ACD);  // Min cell temperature

    // Cluster and energy info
    transmit_can_frame(&GROWATT_1AD0);  // Cluster SOC info
    transmit_can_frame(&GROWATT_1AD1);  // Cumulative energy
  }

  /* ============================================================
   * 2000ms messages
   * ============================================================ */
  if (currentMillis - previousMillis2000ms >= INTERVAL_2_S) {
    previousMillis2000ms = currentMillis;

    transmit_can_frame(&GROWATT_1AC0);  // System composition
  }
}
