#include "../include.h"
#ifdef CMFA_EV_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "CMFA-EV-BATTERY.h"

/*
TODO:
- Find the following data points in the transmitted CAN data:
   - SOC%
   - SOH% (optional)
   - Current
   - Pack voltage
   - Max discharge power
   - Max charge power
   - Temperature min
   - Temperature max
   - Cellvoltage min
   - Cellvoltage max
    - Alternatively all these values can be taken from OBD2 PID polling
- Figure out which messages need to be sent towards the battery to keep it satisfied and staying alive
  - TODO: Test that the current amount of messages sent is enough to keep it alive
/*

/* Do not change code below unless you are sure what you are doing */

CAN_frame CMFA_1EA = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x1EA, .data = {0x00}};
CAN_frame CMFA_125 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 7,
                      .ID = 0x125,
                      .data = {0x7D, 0x7D, 0x7D, 0x07, 0x82, 0x6A, 0x8A}};
CAN_frame CMFA_134 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x134,
                      .data = {0x90, 0x8A, 0x7E, 0x3E, 0xB2, 0x4C, 0x80, 0x00}};
CAN_frame CMFA_135 = {.FD = false, .ext_ID = false, .DLC = 5, .ID = 0x135, .data = {0xD5, 0x85, 0x38, 0x80, 0x01}};
CAN_frame CMFA_3D3 = {.FD = false,
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x3D3,
                      .data = {0x47, 0x30, 0x00, 0x02, 0x5D, 0x80, 0x5D, 0xE7}};
CAN_frame CMFA_59B = {.FD = false, .ext_ID = false, .DLC = 3, .ID = 0x59B, .data = {0x00, 0x02, 0x00}};

static uint8_t counter_10ms = 0;
static uint8_t content_125[16] = {0x07, 0x0C, 0x01, 0x06, 0x0B, 0x00, 0x05, 0x0A,
                                  0x0F, 0x04, 0x09, 0x0E, 0x03, 0x08, 0x0D, 0x02};
static uint8_t content_135[16] = {0x85, 0xD5, 0x25, 0x75, 0xC5, 0x15, 0x65, 0xB5,
                                  0x05, 0x55, 0xA5, 0xF5, 0x45, 0x95, 0xE5, 0x35};
static unsigned long previousMillis100ms = 0;
static unsigned long previousMillis10ms = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.real_soc;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.voltage_dV;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.cell_max_voltage_mV;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x127:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D6:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x43C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x431:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5A9:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C8:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10ms >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10ms));
    }
    previousMillis10ms = currentMillis;
    transmit_can_frame(&CMFA_1EA, can_config.battery);
    transmit_can_frame(&CMFA_135, can_config.battery);
    transmit_can_frame(&CMFA_134, can_config.battery);
    transmit_can_frame(&CMFA_125, can_config.battery);

    CMFA_135.data.u8[1] = content_135[counter_10ms];
    CMFA_125.data.u8[3] = content_125[counter_10ms];
    counter_10ms = (counter_10ms + 1) % 16;  // counter_10ms cycles between 0-1-2-3..15-0-1...
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&CMFA_59B, can_config.battery);
    transmit_can_frame(&CMFA_3D3, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "CMFA platform 26.8/27.4kWh", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif  //CMFA_EV_BATTERY
