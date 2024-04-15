#include "../include.h"
#ifdef RENAULT_KANGOO_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "RENAULT-KANGOO-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static uint32_t LB_Battery_Voltage = 3700;
static uint32_t LB_Charge_Power_Limit_Watts = 0;
static uint32_t LB_Discharge_Power_Limit_Watts = 0;
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
static uint16_t cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static uint8_t CANstillAlive = 12;      //counter for checking if CAN is still alive
static uint8_t LB_Discharge_Power_Limit_Byte1 = 0;
static uint8_t GVI_Pollcounter = 0;
static bool GVB_79B_Continue = false;

CAN_frame_t KANGOO_423 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_std,
                                      }},
                          .MsgID = 0x423,
                          .data = {0x33, 0x00, 0xFF, 0xFF, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame_t KANGOO_79B = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_std,
                                      }},
                          .MsgID = 0x79B,
                          .data = {0x02, 0x21, 0x01, 0x00, 0x00, 0xE0, 0x00, 0x00}};
CAN_frame_t KANGOO_79B_Continue = {.FIR = {.B =
                                               {
                                                   .DLC = 8,
                                                   .FF = CAN_frame_std,
                                               }},
                                   .MsgID = 0x79B,
                                   .data = {0x030, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static unsigned long previousMillis10 = 0;    // will store last time a 10ms CAN Message was sent
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis1000 = 0;  // will store last time a 1000ms CAN Message was sent
static unsigned long GVL_pause = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = (LB_SOC * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.soh_pptt = (LB_SOH * 100);  //Increase range from 99% -> 99.00%

  datalayer.battery.status.voltage_dV = LB_Battery_Voltage;

  datalayer.battery.status.current_dA = LB_Current;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  LB_Discharge_Power_Limit_Watts = (LB_Discharge_Power_Limit * 500);  //Convert value fetched from battery to watts
  /* Define power able to be discharged from battery */
  if (LB_Discharge_Power_Limit_Watts > 60000)  //if >60kW can be pulled from battery
  {
    datalayer.battery.status.max_discharge_power_W = 60000;  //cap value so we don't go over the uint16 limit
  } else {
    datalayer.battery.status.max_discharge_power_W = LB_Discharge_Power_Limit_Watts;
  }
  if (datalayer.battery.status.reported_soc == 0)  //Scaled SOC% value is 0.00%, we should not discharge battery further
  {
    datalayer.battery.status.max_discharge_power_W = 0;
  }

  LB_Charge_Power_Limit_Watts = (LB_Charge_Power_Limit * 500);  //Convert value fetched from battery to watts
  /* Define power able to be put into the battery */
  if (LB_Charge_Power_Limit_Watts > 60000)  //if >60kW can be put into the battery
  {
    datalayer.battery.status.max_charge_power_W = 60000;  //cap value so we don't go over the uint16 limit
  } else {
    datalayer.battery.status.max_charge_power_W = LB_Charge_Power_Limit_Watts;
  }
  if (datalayer.battery.status.reported_soc == 10000)  //Scaled SOC% value is 100.00%
  {
    datalayer.battery.status.max_charge_power_W = 0;  //No need to charge further, set max power to 0
  }

  datalayer.battery.status.active_power_W =
      (datalayer.battery.status.voltage_dV * LB_Current);  //TODO: check if scaling is OK

  datalayer.battery.status.temperature_min_dC = (LB_MIN_TEMPERATURE * 10);

  datalayer.battery.status.temperature_max_dC = (LB_MAX_TEMPERATURE * 10);

  datalayer.battery.status.cell_min_voltage_mV = LB_Cell_Min_Voltage;

  datalayer.battery.status.cell_max_voltage_mV = LB_Cell_Max_Voltage;

  cell_deviation_mV = (datalayer.battery.status.temperature_max_dC - datalayer.battery.status.temperature_min_dC);

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  if (LB_Cell_Max_Voltage >= ABSOLUTE_CELL_MAX_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (LB_Cell_Min_Voltage <= ABSOLUTE_CELL_MIN_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    set_event(EVENT_CELL_DEVIATION_HIGH, 0);
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

#ifdef DEBUG_VIA_USB
  Serial.println("Values going to inverter:");
  Serial.print("SOH%: ");
  Serial.print(datalayer.battery.status.soh_pptt);
  Serial.print(", SOC% scaled: ");
  Serial.print(datalayer.battery.status.reported_soc);
  Serial.print(", Voltage: ");
  Serial.print(datalayer.battery.status.voltage_dV);
  Serial.print(", Max discharge power: ");
  Serial.print(datalayer.battery.status.max_discharge_power_W);
  Serial.print(", Max charge power: ");
  Serial.print(datalayer.battery.status.max_charge_power_W);
  Serial.print(", Max temp: ");
  Serial.print(datalayer.battery.status.temperature_max_dC);
  Serial.print(", Min temp: ");
  Serial.print(datalayer.battery.status.temperature_min_dC);
  Serial.print(", BMS Status (3=OK): ");
  Serial.print(datalayer.battery.status.bms_status);

  Serial.println("Battery values: ");
  Serial.print("Real SOC: ");
  Serial.print(LB_SOC);
  Serial.print(", Current: ");
  Serial.print(LB_Current);
  Serial.print(", kWh remain: ");
  Serial.print(LB_kWh_Remaining);
  Serial.print(", max mV: ");
  Serial.print(LB_Cell_Max_Voltage);
  Serial.print(", min mV: ");
  Serial.print(LB_Cell_Min_Voltage);

#endif
}

void receive_can_battery(CAN_frame_t rx_frame)  //GKOE reworked
{

  switch (rx_frame.MsgID) {
    case 0x155:            //BMS1
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_Current = word((rx_frame.data.u8[1] & 0xF), rx_frame.data.u8[2]) * 0.25 - 500;  //OK!

      LB_SOC = ((rx_frame.data.u8[4] << 8) | (rx_frame.data.u8[5])) * 0.0025;  //OK!
      break;

    case 0x424:            //BMS2
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_SOH = (rx_frame.data.u8[5]);
      LB_MIN_TEMPERATURE = ((rx_frame.data.u8[4]) - 40);  //OK!
      LB_MAX_TEMPERATURE = ((rx_frame.data.u8[7]) - 40);  //OK!
      break;

    case 0x425:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_kWh_Remaining = word((rx_frame.data.u8[0] & 0x1), rx_frame.data.u8[1]) / 10;  //OK!
      break;

    case 0x445:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS
      LB_Cell_Max_Voltage = 1000 + word((rx_frame.data.u8[3] & 0x1), rx_frame.data.u8[4]) * 10;  //OK!
      LB_Cell_Min_Voltage = 1000 + (word(rx_frame.data.u8[5], rx_frame.data.u8[6]) >> 7) * 10;   //OK!

      if ((LB_Cell_Max_Voltage == 6110) or (LB_Cell_Min_Voltage == 6110)) {  //Read Error
        LB_Cell_Max_Voltage = 3880;
        LB_Cell_Min_Voltage = 3880;
        break;
      }
      break;
    case 0x7BB:
      CANstillAlive = 12;  //Indicate that we are still getting CAN messages from the BMS

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

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message (for 2.4s, then pause 10s)
  if ((currentMillis - previousMillis100) >= (INTERVAL_100_MS + GVL_pause)) {
    previousMillis100 = currentMillis;
    ESP32Can.CANWriteFrame(&KANGOO_423);
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
      ESP32Can.CANWriteFrame(&KANGOO_79B_Continue);
  } else {
    ESP32Can.CANWriteFrame(&KANGOO_79B);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Renault Kangoo battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV =
      4040;  // 404.0V, over this, charging is not possible (goes into forced discharge)
  datalayer.battery.info.min_design_voltage_dV = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
