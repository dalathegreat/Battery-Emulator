#include "GROWATT-HV-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* TODO:
   This protocol has not been tested with any inverter. Proceed with extreme caution.
   Search the file for "TODO" to see all the places that might require work

   Growatt BMS CAN-Bus-protocol High Voltage V1.10 2023-11-06
   29-bit identifier
   500kBit/sec
   Big-endian

   Terms and abbreviations:
   PCS - Power conversion system (the Storage Inverter)
   Cell - A single battery cell
   Module - A battery module composed of 16 strings of cells
   Pack - A battery pack composed of the BMS and battery modules connected in parallel and series, which can work independently
   FCC - Full charge capacity
   RM - Remaining capacity
   BMS - Battery Information Collector */

void GrowattHvInverter::update_values() {
  // This function maps all the values fetched from battery CAN to the correct CAN messages

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
    // (WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
    ampere_hours_full = ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) * 100);
    // (WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }

  // Map values to CAN messages
  // Battery operating parameters and status information
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  // If user is requesting a specific voltage
    // User specified charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_3110.data.u8[0] = (datalayer.battery.settings.max_user_set_charge_voltage_dV >> 8);
    GROWATT_3110.data.u8[1] = (datalayer.battery.settings.max_user_set_charge_voltage_dV & 0x00FF);
  } else {
    // Battery max voltage used as charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_3110.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV >> 8);
    GROWATT_3110.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  }

  // Charge limited current, 125 =12.5A (0.1, A) (Min 0, Max 300A)
  GROWATT_3110.data.u8[2] = (datalayer.battery.status.max_charge_current_dA >> 8);
  GROWATT_3110.data.u8[3] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  // Discharge limited current, 500 = 50A, (0.1, A)
  GROWATT_3110.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  GROWATT_3110.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);

  // Status bits (see documentation for all bits, most important are mapped)
  GROWATT_3110.data.u8[7] = 0x00;                      // Clear all bits
  if (datalayer.battery.status.active_power_W < -1) {  // Discharging
    GROWATT_3110.data.u8[7] |= 0b00000011;
  } else if (datalayer.battery.status.active_power_W > 1) {  // Charging
    GROWATT_3110.data.u8[7] |= 0b00000010;
  } else {  // Idle
    GROWATT_3110.data.u8[7] |= 0b00000001;
  }

  if ((datalayer.battery.status.max_charge_current_dA == 0) || (datalayer.battery.status.reported_soc == 10000) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    GROWATT_3110.data.u8[7] |= 0b01000000;  // No Charge
  } else {
    GROWATT_3110.data.u8[7] |= 0b00000000;  // Charge allowed
  }

  if ((datalayer.battery.status.max_discharge_current_dA == 0) || (datalayer.battery.status.reported_soc == 0) ||
      (datalayer.battery.status.bms_status == FAULT)) {
    GROWATT_3110.data.u8[7] |= 0b00100000;  // No Discharge
  } else {
    GROWATT_3110.data.u8[7] |= 0b00000000;  // Discharge allowed
  }

  GROWATT_3110.data.u8[6] |= 0b00100000;  // ISO Detection status: Detected
  GROWATT_3110.data.u8[6] |= 0b00010000;  // Battery status: Normal

  // Battery protection and alarm information
  // Fault and warning status bits. TODO, map these according to documentation.
  // GROWATT_3120.data.u8[0] =
  // GROWATT_3120.data.u8[1] =
  // GROWATT_3120.data.u8[2] =
  // GROWATT_3120.data.u8[3] =
  // GROWATT_3120.data.u8[4] =
  // GROWATT_3120.data.u8[5] =
  // GROWATT_3120.data.u8[6] =
  // GROWATT_3120.data.u8[7] =

  // Battery operation information
  // Voltage of the pack (0.1V) [0-1000V]
  GROWATT_3130.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  GROWATT_3130.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  // Total current (0.1A -300 to 300A)
  GROWATT_3130.data.u8[2] = (datalayer.battery.status.reported_current_dA >> 8);
  GROWATT_3130.data.u8[3] = (datalayer.battery.status.reported_current_dA & 0x00FF);
  // Cell max temperature (0.1C) [-40 to 120*C]
  GROWATT_3130.data.u8[4] = (datalayer.battery.status.temperature_max_dC >> 8);
  GROWATT_3130.data.u8[5] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  // SOC (%) [0-100]
  GROWATT_3130.data.u8[6] = (datalayer.battery.status.reported_soc / 100);
  // SOH (%) (Bit 0~ Bit6 SOH Counters) Bit7 SOH flag (Indicates that battery is in unsafe use)
  GROWATT_3130.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  // Battery capacity information
  // Remaining capacity (10 mAh) [0.0 ~ 500000.0 mAH]
  GROWATT_3140.data.u8[0] = ((ampere_hours_remaining * 100) >> 8);
  GROWATT_3140.data.u8[1] = ((ampere_hours_remaining * 100) & 0x00FF);
  // Fully charged capacity (10 mAh) [0.0 ~ 500000.0 mAH]
  GROWATT_3140.data.u8[2] = ((ampere_hours_full * 100) >> 8);
  GROWATT_3140.data.u8[3] = ((ampere_hours_full * 100) & 0x00FF);
  // Manufacturer code
  GROWATT_3140.data.u8[4] = MANUFACTURER_ASCII_0;
  GROWATT_3140.data.u8[5] = MANUFACTURER_ASCII_1;
  // Cycle count (h)
  GROWATT_3140.data.u8[6] = 0;
  GROWATT_3140.data.u8[7] = 0;

  // Battery working parameters and module number information
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  // If user is requesting a specific voltage
    // Use user specified voltage as Discharge cutoff voltage (0.1V) [0-1000V]
    GROWATT_3150.data.u8[0] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV >> 8);
    GROWATT_3150.data.u8[1] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV & 0x00FF);
  } else {
    // Use battery min design voltage as Discharge cutoff voltage (0.1V) [0-1000V]
    GROWATT_3150.data.u8[0] = (datalayer.battery.info.min_design_voltage_dV >> 8);
