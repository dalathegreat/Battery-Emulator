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
CAN_frame PYLON_355 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x355, .data = {0x00, 0x00, 0x00, 0x00}};
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
CAN_frame PYLON_35C = {.FD = false, .ext_ID = false, .DLC = 2, .ID = 0x35C, .data = {0x00, 0x00}};
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

// when e.g. the min temperature is 0, max is 100 and the warning percent is 80%
// a warning should be generated at 20 (i.e. at 20% of the value range)
// this function calculates this 20% point for a given min/max
int16_t warning_threshold_of_min(int16_t min_val, int16_t max_val) {
  int16_t diff = max_val - min_val;
  return min_val + (diff * (100 - WARNINGS_PERCENT)) / 100;
}

void update_values_can_inverter() {
  // This function maps all the values fetched from battery CAN to the correct CAN messages

  // Set "battery charge voltage" to volts + 1 or user supplied value
  uint16_t charge_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  if (datalayer.battery.settings.user_set_voltage_limits_active)
    charge_voltage_dV = datalayer.battery.settings.max_user_set_charge_voltage_dV;
  if (charge_voltage_dV > datalayer.battery.info.max_design_voltage_dV)
    charge_voltage_dV = datalayer.battery.info.max_design_voltage_dV;
  PYLON_351.data.u8[0] = charge_voltage_dV & 0xff;
  PYLON_351.data.u8[1] = charge_voltage_dV >> 8;
  PYLON_351.data.u8[2] = datalayer.battery.status.max_charge_current_dA & 0xff;
  PYLON_351.data.u8[3] = datalayer.battery.status.max_charge_current_dA >> 8;
  PYLON_351.data.u8[4] = datalayer.battery.status.max_discharge_current_dA & 0xff;
  PYLON_351.data.u8[5] = datalayer.battery.status.max_discharge_current_dA >> 8;

  PYLON_355.data.u8[0] = (datalayer.battery.status.reported_soc / 100) & 0xff;
  PYLON_355.data.u8[1] = (datalayer.battery.status.reported_soc / 100) >> 8;
  PYLON_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 100) & 0xff;
  PYLON_355.data.u8[3] = (datalayer.battery.status.soh_pptt / 100) >> 8;

  int16_t voltage_cV = datalayer.battery.status.voltage_dV * 10;
  int16_t temperature = (datalayer.battery.status.temperature_min_dC + datalayer.battery.status.temperature_max_dC) / 2;
  PYLON_356.data.u8[0] = voltage_cV & 0xff;
  PYLON_356.data.u8[1] = voltage_cV >> 8;
  PYLON_356.data.u8[2] = datalayer.battery.status.current_dA & 0xff;
  PYLON_356.data.u8[3] = datalayer.battery.status.current_dA >> 8;
  PYLON_356.data.u8[4] = temperature & 0xff;
  PYLON_356.data.u8[5] = temperature >> 8;

  // initialize all errors and warnings to 0
  PYLON_359.data.u8[0] = 0x00;
  PYLON_359.data.u8[1] = 0x00;
  PYLON_359.data.u8[2] = 0x00;
  PYLON_359.data.u8[3] = 0x00;
  PYLON_359.data.u8[4] = PACK_NUMBER;
  PYLON_359.data.u8[5] = 0x50;  //P
  PYLON_359.data.u8[6] = 0x4E;  //N

  // ERRORS
  if (datalayer.battery.status.current_dA >= (datalayer.battery.status.max_discharge_current_dA + 10))
    PYLON_359.data.u8[0] |= 0x80;
  if (datalayer.battery.status.temperature_min_dC <= BATTERY_MINTEMPERATURE)
    PYLON_359.data.u8[0] |= 0x10;
  if (datalayer.battery.status.temperature_max_dC >= BATTERY_MAXTEMPERATURE)
    PYLON_359.data.u8[0] |= 0x0C;
  if (datalayer.battery.status.voltage_dV <= datalayer.battery.info.min_design_voltage_dV)
    PYLON_359.data.u8[0] |= 0x04;
  if (datalayer.battery.status.bms_status == FAULT)
    PYLON_359.data.u8[1] |= 0x80;
  if (datalayer.battery.status.current_dA <= -1 * datalayer.battery.status.max_charge_current_dA)
    PYLON_359.data.u8[1] |= 0x01;

  // WARNINGS (using same rules as errors but reporting earlier)
  if (datalayer.battery.status.current_dA >= datalayer.battery.status.max_discharge_current_dA * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[2] |= 0x80;
  if (datalayer.battery.status.temperature_min_dC <=
      warning_threshold_of_min(BATTERY_MINTEMPERATURE, BATTERY_MAXTEMPERATURE))
    PYLON_359.data.u8[2] |= 0x10;
  if (datalayer.battery.status.temperature_max_dC >= BATTERY_MAXTEMPERATURE * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[2] |= 0x0C;
  if (datalayer.battery.status.voltage_dV <= warning_threshold_of_min(datalayer.battery.info.min_design_voltage_dV,
                                                                      datalayer.battery.info.max_design_voltage_dV))
    PYLON_359.data.u8[2] |= 0x04;
  // we never set PYLON_359.data.u8[3] |= 0x80 called "BMS internal"
  if (datalayer.battery.status.current_dA <=
      -1 * datalayer.battery.status.max_charge_current_dA * WARNINGS_PERCENT / 100)
    PYLON_359.data.u8[3] |= 0x01;

  PYLON_35C.data.u8[0] = 0xC0;  // enable charging and discharging
  if (datalayer.battery.status.bms_status == FAULT)
    PYLON_35C.data.u8[0] = 0x00;  // disable all
  else if (datalayer.battery.settings.user_set_voltage_limits_active &&
           datalayer.battery.status.voltage_dV > datalayer.battery.settings.max_user_set_charge_voltage_dV)
    PYLON_35C.data.u8[0] = 0x40;  // only allow discharging
  else if (datalayer.battery.settings.user_set_voltage_limits_active &&
           datalayer.battery.status.voltage_dV < datalayer.battery.settings.max_user_set_discharge_voltage_dV)
    PYLON_35C.data.u8[0] = 0xA0;  // enable charing, set charge immediately
  else if (datalayer.battery.status.real_soc <= datalayer.battery.settings.min_percentage)
    PYLON_35C.data.u8[0] = 0xA0;  // enable charing, set charge immediately
  else if (datalayer.battery.status.real_soc >= datalayer.battery.settings.max_percentage)
    PYLON_35C.data.u8[0] = 0x40;  // enable discharging only

  // PYLON_35E is pre-filled with the manufacturer name
}

void map_can_frame_to_variable_inverter(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:  //Message originating from inverter.
      // according to the spec, this message includes only 0-bytes
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

#ifdef DEBUG_VIA_USB
void dump_frame(CAN_frame* frame) {
  Serial.print("[PYLON-LV] sending CAN frame ");
  Serial.print(frame->ID, HEX);
  Serial.print(": ");
  for (int i = 0; i < 8; i++) {
    Serial.print(frame->data.u8[i] >> 4, HEX);
    Serial.print(frame->data.u8[i] & 0xf, HEX);
    Serial.print(" ");
  }
  Serial.println();
}
#endif

void transmit_can_inverter() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis1000ms >= 1000) {
    previousMillis1000ms = currentMillis;

#ifdef DEBUG_VIA_USB
    dump_frame(&PYLON_351);
    dump_frame(&PYLON_355);
    dump_frame(&PYLON_356);
    dump_frame(&PYLON_359);
    dump_frame(&PYLON_35C);
    dump_frame(&PYLON_35E);
#endif

    transmit_can_frame(&PYLON_351, can_config.inverter);
    transmit_can_frame(&PYLON_355, can_config.inverter);
    transmit_can_frame(&PYLON_356, can_config.inverter);
    transmit_can_frame(&PYLON_359, can_config.inverter);
    transmit_can_frame(&PYLON_35C, can_config.inverter);
    transmit_can_frame(&PYLON_35E, can_config.inverter);
  }
}
void setup_inverter(void) {  // Performs one time setup at startup over CAN bus
  strncpy(datalayer.system.info.inverter_protocol, "Pylontech LV battery over CAN bus", 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
}
#endif
