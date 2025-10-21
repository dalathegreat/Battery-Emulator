#include "GROWATT-WIT-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* TODO:
This protocol has not been tested with any inverter. Proceed with extreme caution.
Search the file for "TODO" to see all the places that might require work

Largest TODO: Map all CAN message .ID properly

Example, how should 1AC6XXXX be transmitted? .ID = 0x1AC6, at the time being, but we need to add the targegt address and source address.

GROWATT BATTERY BMS CAN COMMUNICATION PROTOCOL V1.1 2024.7.19
29-bit identifier
500kBit/sec
Big-endian 

Internal CAN: The extended frame CAN_ID is 29 bits, and the 29-bit CAN_ID is divided into five parts: 
PRI, PG, DA, SA, and FSN.
- PRI: indicates priority.
- PG: indicates the page number. If the FSN is not enough, you can turn the page to increase the range of the FSN.
- TA: indicates the target address.
- SA: indicates the source address.
- FSN: represents functional domain.

(See the Wiki for full documentation explained)

Example: 0x1AC0FFF3: PRI is 6, PG is 2, FSN is 0xC0, TA is 0xFF, SA is 0xF3

*/

void GrowattWitInverter::update_values() {

  //Maximum allowable charging current of the battery system, 0-10000 dA
  GROWATT_1AC3XXXX.data.u8[0] = (datalayer.battery.status.max_charge_current_dA >> 8);
  GROWATT_1AC3XXXX.data.u8[1] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  //Maximum allowable discharge current of the battery system, 0-10000 dA
  GROWATT_1AC3XXXX.data.u8[2] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  GROWATT_1AC3XXXX.data.u8[3] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Maximum charging voltage of the battery system 0-15000 dV
  //Stop charging when the current battery output voltage is greater than or equal to this value
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    //User specified charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_1AC3XXXX.data.u8[4] = (datalayer.battery.settings.max_user_set_charge_voltage_dV >> 8);
    GROWATT_1AC3XXXX.data.u8[5] = (datalayer.battery.settings.max_user_set_charge_voltage_dV & 0x00FF);
    GROWATT_1AC3XXXX.data.u8[6] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV >> 8);
    GROWATT_1AC3XXXX.data.u8[7] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV & 0x00FF);
  } else {
    //Battery max voltage used as charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_1AC3XXXX.data.u8[4] = (datalayer.battery.info.max_design_voltage_dV >> 8);
    GROWATT_1AC3XXXX.data.u8[5] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
    GROWATT_1AC3XXXX.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV >> 8);
    GROWATT_1AC3XXXX.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  }

  //CV voltage value
  //Total charge voltage of constant voltage (set to 10.0V below max voltage for now)
  //When the battery output voltage is greater than or equal to this value, enter constant voltage charging state.
  if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
    //User specified charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_1AC4XXXX.data.u8[4] = ((datalayer.battery.settings.max_user_set_charge_voltage_dV - 100) >> 8);
    GROWATT_1AC4XXXX.data.u8[5] = ((datalayer.battery.settings.max_user_set_charge_voltage_dV - 100) & 0x00FF);
  } else {
    //Battery max voltage used as charge voltage (eg 400.0V = 4000 , 16bits long) (MIN 0, MAX 1000V)
    GROWATT_1AC4XXXX.data.u8[4] = ((datalayer.battery.info.max_design_voltage_dV - 100) >> 8);
    GROWATT_1AC4XXXX.data.u8[5] = ((datalayer.battery.info.max_design_voltage_dV - 100) & 0x00FF);
  }

  //System BMS working status
  if (datalayer.battery.status.current_dA == 0) {
    GROWATT_1AC5XXXX.data.u8[0] = 1;                     //Standby
  } else if (datalayer.battery.status.current_dA < 0) {  //Negative value = Discharging
    GROWATT_1AC5XXXX.data.u8[0] = 3;                     //Discharging
  } else {                                               //Positive value = Charging
    GROWATT_1AC5XXXX.data.u8[0] = 2;                     //Charging
  }

  if (datalayer.battery.status.bms_status == FAULT) {
    GROWATT_1AC5XXXX.data.u8[0] = 5;  //FAULT, Stop using battery!
  }

  //SOC 0-100 %
  GROWATT_1AC6XXXX.data.u8[0] = (datalayer.battery.status.reported_soc / 100);
  //SOH (%) (Bit 0~ Bit6 SOH Counters) Bit7 low SOH flag (Indicates that battery is in unsafe use)
  GROWATT_1AC6XXXX.data.u8[1] = (datalayer.battery.status.soh_pptt / 100);
  //Rated battery capacity when new (0-50000) dAH
  //GROWATT_1AC6XXXX.data.u8[2];  //TODO
  //GROWATT_1AC6XXXX.data.u8[3];  //TODO

  //Battery voltage, 0-15000 dV
  GROWATT_1AC7XXXX.data.u8[2] = (datalayer.battery.status.voltage_dV >> 8);
  GROWATT_1AC7XXXX.data.u8[3] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Battery current, 0-20000dA (offset -1000A)
  // Apply the -1000 offset (add 1000 to the dA value)
  GROWATT_1AC7XXXX.data.u8[4] = ((datalayer.battery.status.current_dA + 1000) >> 8);
  GROWATT_1AC7XXXX.data.u8[5] = ((datalayer.battery.status.current_dA + 1000) & 0x00FF);
}

void GrowattWitInverter::map_can_frame_to_variable(CAN_frame rx_frame) {

  uint32_t first4bytes = ((rx_frame.ID & 0xFFFF0000) >> 4);
  //1AB5XXXX becomes 1AB5. Most likely not needed if all PCS messages come from XXXXDFF1

  switch (first4bytes) {
    case 0x1AB5:  // Heartbeat command, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1AB6:  // Time and date, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1AB7:  // PCS status information, 1000ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1AB8:  // Bus voltage setting 1, 50ms
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1ABE:  // PCS product information, Non-periodic
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      transmit_can_frame(&GROWATT_1AC2XXXX);
      transmit_can_frame(&GROWATT_1A80XXXX);
      transmit_can_frame(&GROWATT_1A82XXXX);
      break;
    default:
      break;
  }
}

void GrowattWitInverter::transmit_can(unsigned long currentMillis) {

  //Send 100ms message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&GROWATT_1AC3XXXX);
    transmit_can_frame(&GROWATT_1AC4XXXX);
    transmit_can_frame(&GROWATT_1AC5XXXX);
    transmit_can_frame(&GROWATT_1AC7XXXX);
  }

  //Send 500ms message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&GROWATT_1AC6XXXX);
  }
}
