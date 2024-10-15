#include "../include.h"
#ifdef PYLON_LV_CAN
#include "../datalayer/datalayer.h"
#include "PYLON-LV-CAN.h"

/* Do not change code below unless you are sure what you are doing */

static unsigned long previousMillis1000ms = 0;

CAN_frame PYLON_351 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x351,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_355 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 4,
                       .ID = 0x355,
                       .data = {0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_356 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 6,
                       .ID = 0x356,
                       .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame PYLON_359 = {.FD = false,
                       .ext_ID = false,
                       .DLC = 7,
                       .ID = 0x359,
                       .data = {0x00, 0x00, 0x00, 0x00, PACK_NUMBER, 'P', 'N'}};
CAN_frame PYLON_35C = {.FD = false,
                       .ext_ID = false,
                       .DLC = 2,
                       .ID = 0x35C,
                       .data = {0x00, 0x00}};
CAN_frame PYLON_35E = {.FD = false,
                       .ext_ID = false,
                       .DLC = 8,
                       .ID = 0x35E,
                       .data = {
                          MANUFACTURER_NAME[0],
                          MANUFACTURER_NAME[1],
                          MANUFACTURER_NAME[2],
                          MANUFACTURER_NAME[3],
                          MANUFACTURER_NAME[4],
                          MANUFACTURER_NAME[5],
                          MANUFACTURER_NAME[6],
                          MANUFACTURER_NAME[7],
                        }};

void update_values_can_inverter() { 
  // This function maps all the values fetched from battery CAN to the correct CAN messages

  // TODO: officially this value is "battery charge voltage". Do we need to add something here to the actual voltage?
  PYLON_351.data.u8[0] = datalayer.battery.status.voltage_dV & 0xff;
  PYLON_351.data.u8[1] = datalayer.battery.status.voltage_dV >> 8;
  int16_t maxChargeCurrent = datalayer.battery.status.max_charge_power_W * 10 / datalayer.battery.status.voltage_dV;
  PYLON_351.data.u8[2] = maxChargeCurrent & 0xff;
  PYLON_351.data.u8[3] = maxChargeCurrent >> 8;
  int16_t maxDischargeCurrent = datalayer.battery.status.max_discharge_power_W * 10 / datalayer.battery.status.voltage_dV;
  PYLON_351.data.u8[4] = maxDischargeCurrent & 0xff;
  PYLON_351.data.u8[5] = maxDischargeCurrent >> 8;

  PYLON_355.data.u8[0] = (datalayer.battery.status.reported_soc / 10) & 0xff;
  PYLON_355.data.u8[1] = (datalayer.battery.status.reported_soc / 10) >> 8;
  PYLON_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 10) & 0xff;
  PYLON_355.data.u8[3] = (datalayer.battery.status.soh_pptt / 10) >> 8;

  PYLON_356.data.u8[0] = datalayer.battery.status.voltage_dV & 0xff;
  PYLON_356.data.u8[1] = datalayer.battery.status.voltage_dV >> 8;
  PYLON_356.data.u8[2] = datalayer.battery.status.current_dA & 0xff;
  PYLON_356.data.u8[3] = datalayer.battery.status.current_dA >> 8;
  PYLON_356.data.u8[4] = datalayer.battery.status.temperature_max_dC & 0xff;
  PYLON_356.data.u8[5] = datalayer.battery.status.temperature_max_dC >> 8;

  // initialize all errors and warnings to 0
  PYLON_359.data.u8[0] = 0x00;
  PYLON_359.data.u8[1] = 0x00;
  PYLON_359.data.u8[2] = 0x00;
  PYLON_359.data.u8[3] = 0x00;
  PYLON_359.data.u8[4] = PACK_NUMBER;
  PYLON_359.data.u8[5] = 'P';
  PYLON_359.data.u8[6] = 'N';

  // ERRORS
  if(datalayer.battery.status.current_dA >= maxDischargeCurrent)
    PYLON_359.data.u8[0] |= 0x80;
  if(datalayer.battery.status.temperature_min_dC <= MIN_TEMPERATURE)
    PYLON_359.data.u8[0] |= 0x10;
  if(datalayer.battery.status.temperature_max_dC >= 500)
    PYLON_359.data.u8[0] |= 0x0C;
  if(datalayer.battery.status.voltage_dV * 100 <= datalayer.battery.info.min_cell_voltage_mV)
    PYLON_359.data.u8[0] |= 0x04;
  // we never set PYLON_359.data.u8[1] |= 0x80 called "BMS internal"
  if(datalayer.battery.status.current_dA <= -1 * maxChargeCurrent)
    PYLON_359.data.u8[1] |= 0x01;

  // WARNINGS (using same rules as errors but reporting earlier)
  if(datalayer.battery.status.current_dA >= maxDischargeCurrent * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[2] |= 0x80;
  if(datalayer.battery.status.temperature_min_dC <= MIN_TEMPERATURE * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[2] |= 0x10;
  if(datalayer.battery.status.temperature_max_dC >= MAX_TEMPERATURE * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[2] |= 0x0C;
  if(datalayer.battery.status.voltage_dV * 100 <= datalayer.battery.info.min_cell_voltage_mV + 100)
    PYLON_359.data.u8[2] |= 0x04;
  // we never set PYLON_359.data.u8[3] |= 0x80 called "BMS internal"
  if(datalayer.battery.status.current_dA <= -1 * maxChargeCurrent * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[3] |= 0x01;

  PYLON_35C.data.u8[0] = 0xC0; // enable charging and discharging
  PYLON_35C.data.u8[1] = 0x00;
  if(datalayer.battery.status.real_soc <= datalayer.battery.settings.min_percentage)
    PYLON_35C.data.u8[0] = 0xA0; // enable charing, set charge immediately
  if(datalayer.battery.status.real_soc >= datalayer.battery.settings.max_percentage)
    PYLON_35C.data.u8[0] = 0x40; // enable discharging only

  // PYLON_35E is pre-filled with the manufacturer name
}

void receive_can_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:  //Message originating from inverter.
      // TODO: according to the spec, this message includes only 0-bytes
      // we could however use this to check if the inverter is still alive
      break;
    default:
      break;
  }
}

void send_can_inverter() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis1000ms >= 1000) {
    previousMillis1000ms = currentMillis;

    transmit_can(&PYLON_351, can_config.inverter);
    transmit_can(&PYLON_355, can_config.inverter);
    transmit_can(&PYLON_356, can_config.inverter);
    transmit_can(&PYLON_359, can_config.inverter);
    transmit_can(&PYLON_35C, can_config.inverter);
    transmit_can(&PYLON_35E, can_config.inverter);
  }
}
#endif
