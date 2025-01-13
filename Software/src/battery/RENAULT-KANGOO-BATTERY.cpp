#include "../include.h"
#ifdef RENAULT_KANGOO_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RENAULT-KANGOO-BATTERY.h"

/* TODO:
There seems to be some values on the Kangoo that differ between the 22/33 kWh version
- Find some way to autodetect which Kangoo size we are working with
- Fix the mappings of values accordingly
- Values still need fixing
  - SOC% is not valid on all packs
    -Try to use the 7BB value?
  - Max charge power is 0W on some packs
  - SOH% is too high on some packs
  - Add all cellvoltages from https://github.com/jamiejones85/Kangoo36_canDecode/tree/main

This page has info on the larger 33kWh pack: https://openinverter.org/wiki/Renault_Kangoo_36
*/

/* Do not change code below unless you are sure what you are doing */
static uint32_t LB_Battery_Voltage = 3700;
static uint32_t LB_Charge_Power_Limit_Watts = 0;
static int32_t LB_Current = 0;
static int16_t LB_MAX_TEMPERATURE = 0;
static int16_t LB_MIN_TEMPERATURE = 0;
static uint16_t LB_SOC = 0;
static uint16_t LB_SOH = 0;
static uint16_t LB_Discharge_Power_Limit = 0;
static uint16_t LB_Charge_Power_Limit = 0;
static uint16_t LB_kWh_Remaining = 0;
static uint16_t LB_Cell_Max_Voltage = 3700;
static uint16_t LB_Cell_Min_Voltage = 3700;
static uint16_t LB_MaxChargeAllowed_W = 0;
static uint8_t LB_Discharge_Power_Limit_Byte1 = 0;
static uint8_t GVI_Pollcounter = 0;
static uint8_t LB_EOCR = 0;
static uint8_t LB_HVBUV = 0;
static uint8_t LB_HVBIR = 0;
static uint8_t LB_CUV = 0;
static uint8_t LB_COV = 0;
static uint8_t LB_HVBOV = 0;
static uint8_t LB_HVBOT = 0;
static uint8_t LB_HVBOC = 0;
static uint8_t LB_MaxInput_kW = 0;
static uint8_t LB_MaxOutput_kW = 0;
static bool GVB_79B_Continue = false;

CAN_frame KANGOO_423 = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x423,
                        .data = {0x0B, 0x1D, 0x00, 0x02, 0xB2, 0x20, 0xB2, 0xD9}};  // Charging
// Driving: 0x07  0x1D  0x00  0x02  0x5D  0x80  0x5D  0xD8
// Charging: 0x0B   0x1D  0x00  0x02  0xB2  0x20  0xB2  0xD9
// Fastcharging: 0x07   0x1E  0x00  0x01  0x5D  0x20  0xB2  0xC7
// Old hardcoded message: .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame KANGOO_79B = {.FD = false,
                        .ext_ID = false,
                        .DLC = 8,
                        .ID = 0x79B,
                        .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame KANGOO_79B_Continue = {.FD = false,
                                 .ext_ID = false,
                                 .DLC = 8,
                                 .ID = 0x79B,
                                 .data = {0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
static unsigned long GVL_pause = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters

  datalayer.battery.status.real_soc = (LB_SOC * 100);  //increase LB_SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = (LB_SOH * 100);  //Increase range from 99% -> 99.00%
  if (datalayer.battery.status.soh_pptt > 10000) {     // Cap value if glitched out
    datalayer.battery.status.soh_pptt = 10000;
  }

  datalayer.battery.status.voltage_dV = LB_Battery_Voltage;

  datalayer.battery.status.current_dA = LB_Current * 10;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  /* Define power able to be discharged from battery */
  datalayer.battery.status.max_discharge_power_W =
      (LB_Discharge_Power_Limit * 500);  //Convert value fetched from battery to watts

  LB_Charge_Power_Limit_Watts = (LB_Charge_Power_Limit * 500);  //Convert value fetched from battery to watts
  //The above value is 0 on some packs. We instead hardcode this now.
  datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_W;

  datalayer.battery.status.temperature_min_dC = (LB_MIN_TEMPERATURE * 10);

  datalayer.battery.status.temperature_max_dC = (LB_MAX_TEMPERATURE * 10);

  datalayer.battery.status.cell_min_voltage_mV = LB_Cell_Min_Voltage;

  datalayer.battery.status.cell_max_voltage_mV = LB_Cell_Max_Voltage;

#ifdef DEBUG_LOG
  logging.println("Values going to inverter:");
  logging.print("SOH%: ");
  logging.print(datalayer.battery.status.soh_pptt);
  logging.print(", SOC% scaled: ");
  logging.print(datalayer.battery.status.reported_soc);
  logging.print(", Voltage: ");
  logging.print(datalayer.battery.status.voltage_dV);
  logging.print(", Max discharge power: ");
  logging.print(datalayer.battery.status.max_discharge_power_W);
  logging.print(", Max charge power: ");
  logging.print(datalayer.battery.status.max_charge_power_W);
  logging.print(", Max temp: ");
  logging.print(datalayer.battery.status.temperature_max_dC);
  logging.print(", Min temp: ");
  logging.print(datalayer.battery.status.temperature_min_dC);
  logging.print(", BMS Status (3=OK): ");
  logging.print(datalayer.battery.status.bms_status);

  logging.println("Battery values: ");
  logging.print("Real SOC: ");
  logging.print(LB_SOC);
  logging.print(", Current: ");
  logging.print(LB_Current);
  logging.print(", kWh remain: ");
  logging.print(LB_kWh_Remaining);
  logging.print(", max mV: ");
  logging.print(LB_Cell_Max_Voltage);
  logging.print(", min mV: ");
  logging.print(LB_Cell_Min_Voltage);

#endif
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case 0x155:  //BMS1
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //Indicate that we are still getting CAN messages from the BMS
      LB_MaxChargeAllowed_W = (rx_frame.data.u8[0] * 300);
      LB_Current = word((rx_frame.data.u8[1] & 0xF), rx_frame.data.u8[2]) * 0.25 - 500;  //OK!
      LB_SOC = ((rx_frame.data.u8[4] << 8) | (rx_frame.data.u8[5])) * 0.0025;            //OK!
      break;
    case 0x424:  //BMS2
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //Indicate that we are still getting CAN messages from the BMS
      LB_EOCR = (rx_frame.data.u8[0] & 0x03);
      LB_HVBUV = (rx_frame.data.u8[0] & 0x0C) >> 2;
      LB_HVBIR = (rx_frame.data.u8[0] & 0x30) >> 4;
      LB_CUV = (rx_frame.data.u8[0] & 0xC0) >> 6;
      LB_COV = (rx_frame.data.u8[1] & 0x03);
      LB_HVBOV = (rx_frame.data.u8[1] & 0x0C) >> 2;
      LB_HVBOT = (rx_frame.data.u8[1] & 0x30) >> 4;
      LB_HVBOC = (rx_frame.data.u8[1] & 0xC0) >> 6;
      LB_MaxInput_kW = rx_frame.data.u8[2] / 2;
      LB_MaxOutput_kW = rx_frame.data.u8[3] / 2;
      LB_SOH = (rx_frame.data.u8[5]);                     // Only seems valid on Kangoo33?
      LB_MIN_TEMPERATURE = ((rx_frame.data.u8[4]) - 40);  //OK!
      LB_MAX_TEMPERATURE = ((rx_frame.data.u8[7]) - 40);  //OK!
      break;
    case 0x425:
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //Indicate that we are still getting CAN messages from the BMS
      LB_kWh_Remaining = word((rx_frame.data.u8[0] & 0x1), rx_frame.data.u8[1]) / 10;  //OK!
      break;
    case 0x445:
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //Indicate that we are still getting CAN messages from the BMS
      LB_Cell_Max_Voltage = 1000 + word((rx_frame.data.u8[3] & 0x1), rx_frame.data.u8[4]) * 10;  //OK!
      LB_Cell_Min_Voltage = 1000 + (word(rx_frame.data.u8[5], rx_frame.data.u8[6]) >> 7) * 10;   //OK!

      if ((LB_Cell_Max_Voltage == 6110) or (LB_Cell_Min_Voltage == 6110)) {  //Read Error
        LB_Cell_Max_Voltage = 3880;
        LB_Cell_Min_Voltage = 3880;
        break;
      }
      break;
    case 0x7BB:
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //Indicate that we are still getting CAN messages from the BMS

      if (rx_frame.data.u8[0] == 0x10) {  //1st response Bytes 0-7
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x21) {  //2nd response Bytes 8-15
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x22) {  //3rd response Bytes 16-23
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x23) {                                               //4th response Bytes 16-23
        LB_Charge_Power_Limit = word(rx_frame.data.u8[5], rx_frame.data.u8[6]) * 100;  //OK!
        LB_Discharge_Power_Limit_Byte1 = rx_frame.data.u8[7];
        GVB_79B_Continue = true;
      }
      if (rx_frame.data.u8[0] == 0x24) {  //5th response Bytes 24-31
        LB_Discharge_Power_Limit = word(LB_Discharge_Power_Limit_Byte1, rx_frame.data.u8[1]) * 100;  //OK!
        LB_Battery_Voltage = word(rx_frame.data.u8[2], rx_frame.data.u8[3]) / 10;                    //OK!
        GVB_79B_Continue = false;
      }
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message (for 2.4s, then pause 10s)
  if ((currentMillis - previousMillis100) >= (INTERVAL_100_MS + GVL_pause)) {
    previousMillis100 = currentMillis;
    transmit_can_frame(&KANGOO_423, can_config.battery);
    GVI_Pollcounter++;
    GVL_pause = 0;
    if (GVI_Pollcounter >= 24) {
      GVI_Pollcounter = 0;
      GVL_pause = 10000;
    }
  }
  // 1000ms CAN handling
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
    if (GVB_79B_Continue)
      transmit_can_frame(&KANGOO_79B_Continue, can_config.battery);
  } else {
    transmit_can_frame(&KANGOO_79B, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup

  strncpy(datalayer.system.info.battery_protocol, "Renault Kangoo", 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
