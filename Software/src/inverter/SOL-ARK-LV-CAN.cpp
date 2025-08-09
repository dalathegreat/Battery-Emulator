#include "SOL-ARK-LV-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

/* Sol-Ark v1.3 protocol
The Sol-Ark inverters only recognize standard CAN Bus frames containing 8 bytes of data.
CAN FD with 64 data bytes per frame is NOT supported.
Communication Rate: 500 kbps
Data Endianness: Little Endian (least significate byte is at left end of multi or 2-byte values)
Transmission Cycle Rate: BMS full data set here shall be transmitted to the inverter once every second.
BMS sends first message with this full register set. Inverter Heartbeat Response: Each time the inverter
correctly receives data, it will respond with CAN ID 0x305 containing “00 00 00 00 00 00 00 00” as data.*/

void SolArkLvInverter::update_values() {

  // Set "Charge voltage limit" to battery max value OR user supplied value
  uint16_t charge_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  if (datalayer.battery.settings.user_set_voltage_limits_active)
    charge_voltage_dV = datalayer.battery.settings.max_user_set_charge_voltage_dV;
  if (charge_voltage_dV > datalayer.battery.info.max_design_voltage_dV)
    charge_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  SOLARK_351.data.u8[0] = charge_voltage_dV & 0xff;
  SOLARK_351.data.u8[1] = charge_voltage_dV >> 8;
  //Rest of setpoints in deci-units
  SOLARK_351.data.u8[2] = datalayer.battery.status.max_charge_current_dA & 0xff;
  SOLARK_351.data.u8[3] = datalayer.battery.status.max_charge_current_dA >> 8;
  SOLARK_351.data.u8[4] = datalayer.battery.status.max_discharge_current_dA & 0xff;
  SOLARK_351.data.u8[5] = datalayer.battery.status.max_discharge_current_dA >> 8;
  SOLARK_351.data.u8[6] = datalayer.battery.info.min_design_voltage_dV & 0xff;
  SOLARK_351.data.u8[7] = datalayer.battery.info.min_design_voltage_dV >> 8;

  SOLARK_355.data.u8[0] = (datalayer.battery.status.reported_soc / 100) & 0xff;
  SOLARK_355.data.u8[1] = (datalayer.battery.status.reported_soc / 100) >> 8;
  SOLARK_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 100) & 0xff;
  SOLARK_355.data.u8[3] = (datalayer.battery.status.soh_pptt / 100) >> 8;

  int16_t average_temperature =
      (datalayer.battery.status.temperature_min_dC + datalayer.battery.status.temperature_max_dC) / 2;
  SOLARK_356.data.u8[0] = datalayer.battery.status.voltage_dV & 0xff;
  SOLARK_356.data.u8[1] = datalayer.battery.status.voltage_dV >> 8;
  SOLARK_356.data.u8[2] = datalayer.battery.status.current_dA & 0xff;
  SOLARK_356.data.u8[3] = datalayer.battery.status.current_dA >> 8;
  SOLARK_356.data.u8[4] = average_temperature & 0xff;
  SOLARK_356.data.u8[5] = average_temperature >> 8;

  // initialize all errors and warnings to 0
  SOLARK_359.data.u8[0] = 0x00;  //Protection byte 1
  SOLARK_359.data.u8[1] = 0x00;  //Protection byte 2
  SOLARK_359.data.u8[2] = 0x00;  // Alarm byte 1
  SOLARK_359.data.u8[3] = 0x00;  // Alarm byte 2
  SOLARK_359.data.u8[4] = MODULE_NUMBER;
  SOLARK_359.data.u8[5] = 0x50;  //P
  SOLARK_359.data.u8[6] = 0x4E;  //N
  SOLARK_359.data.u8[7] = 0x00;  //Unused, should be 00

  // Protection Byte 1 Bitfield: (If a bit is set, one of these caused batt self-protection mode)
  if (datalayer.battery.status.current_dA >= (datalayer.battery.status.max_discharge_current_dA + 50))
    SOLARK_359.data.u8[0] |= 0x80;
  if (datalayer.battery.status.temperature_min_dC <= BATTERY_MINTEMPERATURE)
    SOLARK_359.data.u8[0] |= 0x10;
  if (datalayer.battery.status.temperature_max_dC >= BATTERY_MAXTEMPERATURE)
    SOLARK_359.data.u8[0] |= 0x0C;
  if (datalayer.battery.status.voltage_dV <= datalayer.battery.info.min_design_voltage_dV)
    SOLARK_359.data.u8[0] |= 0x04;
  if (datalayer.battery.status.bms_status == FAULT)
    SOLARK_359.data.u8[1] |= 0x80;
  if (datalayer.battery.status.current_dA <= -1 * datalayer.battery.status.max_charge_current_dA)
    SOLARK_359.data.u8[1] |= 0x01;

  // WARNINGS (using same rules as errors but reporting earlier)
  // TODO: Not enabled in this integration yet. See Pylon protocol for example integration

  SOLARK_35C.data.u8[0] = 0xC0;  // enable charging and discharging
  if (datalayer.battery.status.bms_status == FAULT)
    SOLARK_35C.data.u8[0] = 0x00;  // disable all
  else if (datalayer.battery.settings.user_set_voltage_limits_active &&
           datalayer.battery.status.voltage_dV > datalayer.battery.settings.max_user_set_charge_voltage_dV)
    SOLARK_35C.data.u8[0] = 0x40;  // only allow discharging
  else if (datalayer.battery.settings.user_set_voltage_limits_active &&
           datalayer.battery.status.voltage_dV < datalayer.battery.settings.max_user_set_discharge_voltage_dV)
    SOLARK_35C.data.u8[0] = 0xA0;  // enable charing, set charge immediately
  else if (datalayer.battery.status.real_soc <= datalayer.battery.settings.min_percentage)
    SOLARK_35C.data.u8[0] = 0xA0;  // enable charing, set charge immediately
  else if (datalayer.battery.status.real_soc >= datalayer.battery.settings.max_percentage)
    SOLARK_35C.data.u8[0] = 0x40;  // enable discharging only

  // SOLARK_35E is pre-filled with the manufacturer name (BAT-EMU)
}

void SolArkLvInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:  //Message originating from inverter, signalling that data rec OK
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void SolArkLvInverter::transmit_can(unsigned long currentMillis) {

  if (currentMillis - previousMillis1000ms >= INTERVAL_1_S) {
    previousMillis1000ms = currentMillis;

    transmit_can_frame(&SOLARK_351);
    transmit_can_frame(&SOLARK_355);
    transmit_can_frame(&SOLARK_356);
    transmit_can_frame(&SOLARK_359);
    transmit_can_frame(&SOLARK_35C);
    transmit_can_frame(&SOLARK_35E);
  }
}
