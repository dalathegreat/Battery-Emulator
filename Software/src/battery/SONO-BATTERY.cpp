#include "../include.h"
#ifdef SONO_BATTERY
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "SONO-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was send

static uint8_t seconds = 0;
static uint8_t functionalsafetybitmask = 0;
static uint16_t batteryVoltage = 3700;
static uint16_t allowedDischargePower = 0;
static uint16_t allowedChargePower = 0;
static uint16_t CellVoltMax_mV = 0;
static uint16_t CellVoltMin_mV = 0;
static int16_t batteryAmps = 0;
static int16_t temperatureMin = 0;
static int16_t temperatureMax = 0;
static uint8_t batterySOH = 99;
static uint8_t realSOC = 99;

CAN_frame SONO_400 = {.FD = false,  //Message of Vehicle Command, 100ms
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x400,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame SONO_401 = {.FD = false,  //Message of Vehicle Date, 1000ms
                      .ext_ID = false,
                      .DLC = 8,
                      .ID = 0x400,
                      .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

void SonoBattery::update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (realSOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = (batterySOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery.status.voltage_dV = batteryVoltage;

  datalayer.battery.status.current_dA = batteryAmps;

  datalayer.battery.info.total_capacity_Wh = 54000;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = allowedDischargePower * 100;

  datalayer.battery.status.max_charge_power_W = allowedChargePower * 100;

  datalayer.battery.status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = CellVoltMin_mV;

  datalayer.battery.status.temperature_min_dC = temperatureMin;

  datalayer.battery.status.temperature_max_dC = temperatureMax;
}

void SonoBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x100:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x101:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x102:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      functionalsafetybitmask = rx_frame.data.u8[0];  //If any bits are high here, battery has a HSD fault active.
      break;
    case 0x200:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x220:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      allowedChargePower = (rx_frame.data.u8[0] << 8) + rx_frame.data.u8[1];
      allowedDischargePower = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      break;
    case 0x221:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x300:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryVoltage = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5];
      break;
    case 0x301:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      CellVoltMax_mV = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2];
      CellVoltMin_mV = (rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5];
      break;
    case 0x310:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x311:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batteryAmps = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]) - 1000;
      break;
    case 0x320:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperatureMax = ((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2]) - 400;
      temperatureMin = ((rx_frame.data.u8[4] << 8) + rx_frame.data.u8[5]) - 400;
      break;
    case 0x321:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      batterySOH = rx_frame.data.u8[4];
      break;
    case 0x330:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x331:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x601:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      realSOC = rx_frame.data.u8[0];
      break;
    case 0x610:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x611:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x613:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x614:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x615:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}
void SonoBattery::transmit_can() {
  unsigned long currentMillis = millis();

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //VCU Command message
    SONO_400.data.u8[0] = 0x15;  //Charging enabled bit01, dischargign enabled bit23, dc charging bit45

    if (datalayer.battery.status.bms_status == FAULT) {
      SONO_400.data.u8[0] = 0x14;  //Charging DISABLED
    }
    transmit_can_frame(&SONO_400, can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    //Time and date
    //Let's see if the battery is happy with just getting seconds incrementing
    SONO_401.data.u8[0] = 2025;     //Year
    SONO_401.data.u8[1] = 1;        //Month
    SONO_401.data.u8[2] = 1;        //Day
    SONO_401.data.u8[3] = 12;       //Hour
    SONO_401.data.u8[4] = 15;       //Minute
    SONO_401.data.u8[5] = seconds;  //Second
    seconds = (seconds + 1) % 61;
    transmit_can_frame(&SONO_401, can_config.battery);
  }
}

void SonoBattery::setup(void) {  // Performs one time setup at startup
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = max_pack_voltage_dv();
  datalayer.battery.info.min_design_voltage_dV = min_pack_voltage_dv();
  datalayer.battery.info.max_cell_voltage_mV = max_cell_voltage_mv();
  datalayer.battery.info.min_cell_voltage_mV = min_cell_voltage_mv();
  datalayer.battery.info.max_cell_voltage_deviation_mV = max_cell_deviation_mv();
  datalayer.battery.info.chemistry = battery_chemistry_enum::LFP;
}

#endif
