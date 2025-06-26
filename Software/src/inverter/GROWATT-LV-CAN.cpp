#include "GROWATT-LV-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* Growatt BMS CAN-Bus-protocol Low Voltage Rev_04
CAN 2.0A
500kBit/sec
Big-endian

The inverter replies data every second (standard frame/decimal)0x301:*/

void GrowattLvInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  cell_delta_mV = datalayer.battery.status.cell_max_voltage_mV - datalayer.battery.status.cell_min_voltage_mV;

  if (datalayer.battery.status.voltage_dV > 10) {  // Only update value when we have voltage available to avoid div0
    ampere_hours_remaining =
        ((datalayer.battery.status.reported_remaining_capacity_Wh / datalayer.battery.status.voltage_dV) *
         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
    ampere_hours_full = ((datalayer.battery.info.total_capacity_Wh / datalayer.battery.status.voltage_dV) *
                         100);  //(WH[10000] * V+1[3600])*100 = 270 (27.0Ah)
  }
  //Map values to CAN messages

  //Battery charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 41V, MAX 63V, default 54V)
  GROWATT_311.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) >> 8);
  GROWATT_311.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) & 0x00FF);
  //Charge limited current, 125 =12.5A (0.1, A)
  GROWATT_311.data.u8[2] = (datalayer.battery.status.max_charge_current_dA >> 8);
  GROWATT_311.data.u8[3] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  //Discharge limited current, 500 = 50A, (0.1, A)
  GROWATT_311.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  GROWATT_311.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Status bits (see documentation for all bits, most important are bit0-1 (Status), and bit 10-11 (SP status))
  if (datalayer.battery.status.active_power_W < -1) {        // Discharging
    GROWATT_311.data.u8[6] = 0x0C;                           //0b11 discharging on bit10-11
    GROWATT_311.data.u8[7] = 0x03;                           //0b11 discharging on bit0-1
  } else if (datalayer.battery.status.active_power_W > 1) {  // Charging
    GROWATT_311.data.u8[6] = 0x08;                           //0b10 charging on bit10-11
    GROWATT_311.data.u8[7] = 0x02;                           //0b10 charging on bit0-1
  } else {                                                   //Idle
    GROWATT_311.data.u8[6] = 0x04;                           //0b01 charging on bit10-11
    GROWATT_311.data.u8[7] = 0x01;                           //0b01 charging on bit0-1
  }

  //Fault status bits. TODO, map these according to docmentation.
  //GROWATT_312.data.u8[0] =
  //GROWATT_312.data.u8[1] =
  //GROWATT_312.data.u8[2] =
  //GROWATT_312.data.u8[3] =
  GROWATT_312.data.u8[4] = 0x01;                                    // Pack number
  GROWATT_312.data.u8[5] = 0xAA;                                    // Manufacturer code
  GROWATT_312.data.u8[6] = 0xBB;                                    // Manufacturer code
  GROWATT_312.data.u8[7] = datalayer.battery.info.number_of_cells;  // Total cell number (1-254)

  //Voltage of single module or Average module voltage of system (0.01V)
  GROWATT_313.data.u8[0] = ((datalayer.battery.status.voltage_dV * 10) >> 8);
  GROWATT_313.data.u8[1] = ((datalayer.battery.status.voltage_dV * 10) & 0x00FF);
  //Module or system total current (0.1A Sint16)
  GROWATT_313.data.u8[2] = (datalayer.battery.status.current_dA >> 8);
  GROWATT_313.data.u8[3] = (datalayer.battery.status.current_dA & 0x00FF);
  //Cell max temperature (0.1C)
  GROWATT_313.data.u8[4] = (datalayer.battery.status.temperature_max_dC >> 8);
  GROWATT_313.data.u8[5] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //SOC of single module or average value of system (%)
  GROWATT_313.data.u8[6] = (datalayer.battery.status.reported_soc / 100);
  //SOH (%) (Bit 0~ Bit6 SOH Counters) Bit7 SOH flag (Indicates that battery is in unsafe use)
  GROWATT_313.data.u8[7] = (datalayer.battery.status.soh_pptt / 100);

  //Remaining capacity (10 mAh)
  GROWATT_314.data.u8[0] = ((ampere_hours_remaining * 100) >> 8);
  GROWATT_314.data.u8[1] = ((ampere_hours_remaining * 100) & 0x00FF);
  //Fully charged capacity (10 mAh)
  GROWATT_314.data.u8[2] = ((ampere_hours_full * 100) >> 8);
  GROWATT_314.data.u8[3] = ((ampere_hours_full * 100) & 0x00FF);
  //Delta V (mV)
  GROWATT_314.data.u8[4] = (cell_delta_mV >> 8);
  GROWATT_314.data.u8[5] = (cell_delta_mV & 0x00FF);
  //Cycle count (h)
  GROWATT_314.data.u8[6] = 0;
  GROWATT_314.data.u8[7] = 0;

  //Request charge/discharge
  if (datalayer.battery.status.bms_status == ACTIVE) {
    GROWATT_319.data.u8[0] =
        0xC0;  //Bit7 charge enabled, Bit 6 discharge enabled (bit5 req force charge, bit 4 req force charge 2)
  } else {
    GROWATT_319.data.u8[0] = 0x00;
  }
  //TODO: if battery falls below SOC 5% during long idle time, we should set bit 5

  //Maximum cell voltage (mV)
  GROWATT_319.data.u8[1] = (datalayer.battery.status.cell_max_voltage_mV >> 8);
  GROWATT_319.data.u8[2] = (datalayer.battery.status.cell_max_voltage_mV & 0x00FF);
  // Min cell voltage (mV)
  GROWATT_319.data.u8[3] = (datalayer.battery.status.cell_min_voltage_mV >> 8);
  GROWATT_319.data.u8[4] = (datalayer.battery.status.cell_min_voltage_mV & 0x00FF);
  //Maximum cell voltage number
  GROWATT_319.data.u8[5] = 1;  //Fake
  // Min cell voltage number
  GROWATT_319.data.u8[6] = 2;  //Fake
  //Protect pack ID
  GROWATT_319.data.u8[7] = 0;  //?

  // Manufacturer name (ASCII) Battery manufacturer abbreviation in capital letters
  GROWATT_320.data.u8[0] = 0x42;  //B
  GROWATT_320.data.u8[1] = 0x45;  //E
  // Hardware revision (1-9)
  GROWATT_320.data.u8[2] = 0x01;
  // Software version (1-9)
  GROWATT_320.data.u8[3] = 0x01;
  //Date and Time
  //Bit 0~5 Second0~59
  //Bit 6~11 Minute0~59
  //Bit 12~16 Hour0~23
  //Bit 17~21Day1~31
  //Bit 22~25 Month 1-12
  //Bit 26~31 Year (2000-2063)
  GROWATT_320.data.u8[4] = 0;  //TODO
  GROWATT_320.data.u8[5] = 0;
  GROWATT_320.data.u8[6] = 0;
  GROWATT_320.data.u8[7] = 0;

  //Message 0x321 is update status. All blank is OK

  //Cellvoltage #1
  GROWATT_315.data.u8[0] = (datalayer.battery.status.cell_voltages_mV[0] >> 8);
  GROWATT_315.data.u8[1] = (datalayer.battery.status.cell_voltages_mV[0] & 0x00FF);
  //Cellvoltage #2
  GROWATT_315.data.u8[2] = (datalayer.battery.status.cell_voltages_mV[1] >> 8);
  GROWATT_315.data.u8[3] = (datalayer.battery.status.cell_voltages_mV[1] & 0x00FF);
  //Cellvoltage #3
  GROWATT_315.data.u8[4] = (datalayer.battery.status.cell_voltages_mV[2] >> 8);
  GROWATT_315.data.u8[5] = (datalayer.battery.status.cell_voltages_mV[2] & 0x00FF);
  //Cellvoltage #4
  GROWATT_315.data.u8[6] = (datalayer.battery.status.cell_voltages_mV[3] >> 8);
  GROWATT_315.data.u8[7] = (datalayer.battery.status.cell_voltages_mV[3] & 0x00FF);

  //Cellvoltage #5
  GROWATT_316.data.u8[0] = (datalayer.battery.status.cell_voltages_mV[4] >> 8);
  GROWATT_316.data.u8[1] = (datalayer.battery.status.cell_voltages_mV[4] & 0x00FF);
  //Cellvoltage #6
  GROWATT_316.data.u8[2] = (datalayer.battery.status.cell_voltages_mV[5] >> 8);
  GROWATT_316.data.u8[3] = (datalayer.battery.status.cell_voltages_mV[5] & 0x00FF);
  //Cellvoltage #7
  GROWATT_316.data.u8[4] = (datalayer.battery.status.cell_voltages_mV[6] >> 8);
  GROWATT_316.data.u8[5] = (datalayer.battery.status.cell_voltages_mV[6] & 0x00FF);
  //Cellvoltage #8
  GROWATT_316.data.u8[6] = (datalayer.battery.status.cell_voltages_mV[7] >> 8);
  GROWATT_316.data.u8[7] = (datalayer.battery.status.cell_voltages_mV[7] & 0x00FF);

  //Cellvoltage #9
  GROWATT_317.data.u8[0] = (datalayer.battery.status.cell_voltages_mV[8] >> 8);
  GROWATT_317.data.u8[1] = (datalayer.battery.status.cell_voltages_mV[8] & 0x00FF);
  //Cellvoltage #10
  GROWATT_317.data.u8[2] = (datalayer.battery.status.cell_voltages_mV[9] >> 8);
  GROWATT_317.data.u8[3] = (datalayer.battery.status.cell_voltages_mV[9] & 0x00FF);
  //Cellvoltage #11
  GROWATT_317.data.u8[4] = (datalayer.battery.status.cell_voltages_mV[10] >> 8);
  GROWATT_317.data.u8[5] = (datalayer.battery.status.cell_voltages_mV[10] & 0x00FF);
  //Cellvoltage #12
  GROWATT_317.data.u8[6] = (datalayer.battery.status.cell_voltages_mV[11] >> 8);
  GROWATT_317.data.u8[7] = (datalayer.battery.status.cell_voltages_mV[11] & 0x00FF);

  //Cellvoltage #13
  GROWATT_318.data.u8[0] = (datalayer.battery.status.cell_voltages_mV[12] >> 8);
  GROWATT_318.data.u8[1] = (datalayer.battery.status.cell_voltages_mV[12] & 0x00FF);
  //Cellvoltage #14
  GROWATT_318.data.u8[2] = (datalayer.battery.status.cell_voltages_mV[13] >> 8);
  GROWATT_318.data.u8[3] = (datalayer.battery.status.cell_voltages_mV[13] & 0x00FF);
  //Cellvoltage #15
  GROWATT_318.data.u8[4] = (datalayer.battery.status.cell_voltages_mV[14] >> 8);
  GROWATT_318.data.u8[5] = (datalayer.battery.status.cell_voltages_mV[14] & 0x00FF);
  //Cellvoltage #16
  GROWATT_318.data.u8[6] = (datalayer.battery.status.cell_voltages_mV[15] >> 8);
  GROWATT_318.data.u8[7] = (datalayer.battery.status.cell_voltages_mV[15] & 0x00FF);
}

void GrowattLvInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x301:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_frame(&GROWATT_311, can_config.inverter);
      transmit_can_frame(&GROWATT_312, can_config.inverter);
      transmit_can_frame(&GROWATT_313, can_config.inverter);
      transmit_can_frame(&GROWATT_314, can_config.inverter);
      transmit_can_frame(&GROWATT_315, can_config.inverter);
      transmit_can_frame(&GROWATT_316, can_config.inverter);
      transmit_can_frame(&GROWATT_317, can_config.inverter);
      transmit_can_frame(&GROWATT_318, can_config.inverter);
      transmit_can_frame(&GROWATT_319, can_config.inverter);
      transmit_can_frame(&GROWATT_320, can_config.inverter);
      transmit_can_frame(&GROWATT_321, can_config.inverter);
      break;
    default:
      break;
  }
}

void GrowattLvInverter::transmit_can(unsigned long currentMillis) {
  // No periodic sending for this battery type. Data is sent when inverter requests it
}

void GrowattLvInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
