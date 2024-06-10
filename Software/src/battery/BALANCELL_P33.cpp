#include "../include.h"
#ifdef BALANCELL_P33
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "BALANCELL_P33.h"

/* TODO:
- Test the protocol with a battery
- Check if values are scaled correctly in the Webserver
- Check if CAN sending towards the battery works OK. Tweak values from log file if needed
- If all looks good, try adding an inverter into the mix!
*/

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was sent

//Actual content messages
CAN_frame_t PYLON_3010 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_ext,
                                      }},
                          .MsgID = 0x3010,
                          .data = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_8200 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_ext,
                                      }},
                          .MsgID = 0x8200,
                          .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_8210 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_ext,
                                      }},
                          .MsgID = 0x8210,
                          .data = {0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame_t PYLON_4200 = {.FIR = {.B =
                                      {
                                          .DLC = 8,
                                          .FF = CAN_frame_ext,
                                      }},
                          .MsgID = 0x4200,
                          .data = {0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static int16_t celltemperature_max_dC = 0;
static int16_t celltemperature_min_dC = 0;
static int16_t current_dA = 0;
static uint16_t voltage_dV = 0;
static uint16_t cellvoltage_max_mV = 0;
static uint16_t cellvoltage_min_mV = 0;
static uint16_t charge_cutoff_voltage = 0;
static uint16_t discharge_cutoff_voltage = 0;
static int16_t max_charge_current = 0;
static int16_t max_discharge_current = 0;
static uint8_t ensemble_info_ack = 0;
static uint8_t battery_module_quantity = 0;
static uint8_t battery_modules_in_series = 0;
static uint8_t cell_quantity_in_module = 0;
static uint8_t voltage_level = 0;
static uint8_t ah_number = 0;
static uint8_t SOC = 0;
static uint8_t SOH = 0;
static uint8_t charge_forbidden = 0;
static uint8_t discharge_forbidden = 0;

void update_values_battery() {

  datalayer.battery.status.real_soc = (SOC * 100);  //increase SOC range from 0-100 -> 100.00

  datalayer.battery.status.soh_pptt = (SOH * 100);  //Increase decimals from 100% -> 100.00%

  datalayer.battery.status.voltage_dV = voltage_dV;  //value is *10 (3700 = 370.0)

  datalayer.battery.status.current_dA = current_dA;  //value is *10 (150 = 15.0) , invert the sign

  datalayer.battery.status.active_power_W =  //Power in watts, Negative = charging batt
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.max_charge_power_W = (max_charge_current * (voltage_dV / 10));

  datalayer.battery.status.max_discharge_power_W = (max_discharge_current * (voltage_dV / 10));

  datalayer.battery.status.cell_max_voltage_mV = cellvoltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = cellvoltage_min_mV;

  datalayer.battery.status.temperature_min_dC = celltemperature_min_dC;

  datalayer.battery.status.temperature_max_dC = celltemperature_max_dC;

  datalayer.battery.info.max_design_voltage_dV = charge_cutoff_voltage;

  datalayer.battery.info.min_design_voltage_dV = discharge_cutoff_voltage;

  // for (int i = 0; i < 32; ++i) {
  //   datalayer.battery.status.cell_voltages_mV[i] = 3300 + i;
  // }

// /*Finally print out values to serial if configured to do so*/
// #ifdef DEBUG_VIA_USB
//   Serial.println("Battery Values going to inverter");
//   Serial.print("SOH%: ", (datalayer.battery.status.soh_pptt * 0.01), "% ");
//   // Serial.print(", SOC%: ", (datalayer.battery.status.reported_soc * 0.01), "% ");
//   // Serial.print(", Voltage: ", (datalayer.battery.status.voltage_dV * 0.1), "V ");
//   // Serial.print(", Max discharge power: ", datalayer.battery.status.max_discharge_power_W, "W ");
//   // Serial.print(", Max charge power: ", datalayer.battery.status.max_charge_power_W, "W ");
//   // Serial.print(", Max temp: ", (datalayer.battery.status.temperature_max_dC * 0.1), "°C ");
//   // Serial.print(", Min temp: ", (datalayer.battery.status.temperature_min_dC * 0.1), "°C ");
//   // Serial.print(", Max cell voltage: ", datalayer.battery.status.cell_max_voltage_mV, "mV ");
//   // Serial.print(", Min cell voltage: ", datalayer.battery.status.cell_min_voltage_mV, "mV ");
//   // Serial.println("");
// #endif

}

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0x351:
      //BMS limits
      charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      discharge_cutoff_voltage =  ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);
      max_charge_current = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2])/10;
      max_discharge_current = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4])/10;

      if (max_charge_current > 0){
        charge_forbidden = 0;
      }
      else{
        charge_forbidden = 1;
      }

      if (max_discharge_current > 0){
        discharge_forbidden = 0;
      }
      else{
        discharge_forbidden = 1;
      }

      // Serial.println("0x351 message recieved CV: " + String(charge_cutoff_voltage)  +" DV: " + String(discharge_cutoff_voltage) );
      // Serial.println("0x351 message recieved CCL: " + String(max_charge_current)  +" DCL: " + String(max_discharge_current) );
    break;
    case 0x355:
      // BMS SOC and SOH
      SOC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      SOH = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2])-1;
      // Serial.println("0x355 message recieved SOC: " + String(SOC)  +" SOH: " + String(SOH) );
    break;
    case 0x356:
      // BMS Status Message
      // celltemperature_max_dC = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);
      voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) / 10 - 3;
      current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2])*2;
      Serial.println("0x356 message recieved Temp: " + String(celltemperature_max_dC)  +" Volts: " + String(voltage_dV) +" Current: " + String(current_dA) );
    break;
    case 0x370:
      //High low
      cellvoltage_max_mV = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]);
      cellvoltage_min_mV = ((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]);
      celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0])*10;
      celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2])*10;


      ensemble_info_ack = true;
      battery_module_quantity = 2;
      battery_modules_in_series = 1;
      cell_quantity_in_module = 32;
      // voltage_level = 106;
      ah_number = 206;



    
      break;

      // ensemble_info_ack = true;
      // battery_module_quantity = rx_frame.data.u8[0];
      // battery_modules_in_series = rx_frame.data.u8[2];
      // cell_quantity_in_module = rx_frame.data.u8[3];
      // voltage_level = rx_frame.data.u8[4];
      // ah_number = rx_frame.data.u8[6];
      // voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      // current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      // SOC = rx_frame.data.u8[6];
      // SOH = rx_frame.data.u8[7];
      // charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      // discharge_cutoff_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      // max_charge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.1) - 3000;
      // max_discharge_current = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) * 0.1) - 3000;
      // cellvoltage_max_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      // cellvoltage_min_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
      // celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 1000;
      // celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 1000;
      // charge_forbidden = rx_frame.data.u8[0];
      // discharge_forbidden = rx_frame.data.u8[1];

    // case 0x7310:
    // case 0x7311:
    //   ensemble_info_ack = true;
    //   // This message contains software/hardware version info. No interest to us
    //   break;
    // case 0x7320:
    // case 0x7321:
    //   ensemble_info_ack = true;
    //   battery_module_quantity = rx_frame.data.u8[0];
    //   battery_modules_in_series = rx_frame.data.u8[2];
    //   cell_quantity_in_module = rx_frame.data.u8[3];
    //   voltage_level = rx_frame.data.u8[4];
    //   ah_number = rx_frame.data.u8[6];
    //   break;
    // case 0x4210:
    // case 0x4211:
    //   voltage_dV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
    //   current_dA = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
    //   SOC = rx_frame.data.u8[6];
    //   SOH = rx_frame.data.u8[7];
    //   break;
    // case 0x4220:
    // case 0x4221:
    //   charge_cutoff_voltage = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
    //   discharge_cutoff_voltage = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
    //   max_charge_current = (((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4]) * 0.1) - 3000;
    //   max_discharge_current = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) * 0.1) - 3000;
    //   break;
    // case 0x4230:
    // case 0x4231:
    //   cellvoltage_max_mV = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
    //   cellvoltage_min_mV = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]);
    //   break;
    // case 0x4240:
    // case 0x4241:
    //   celltemperature_max_dC = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) - 1000;
    //   celltemperature_min_dC = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 1000;
    //   break;
    // case 0x4250:
    // case 0x4251:
    //   //Byte0 Basic Status
    //   //Byte1-2 Cycle Period
    //   //Byte3 Error
    //   //Byte4-5 Alarm
    //   //Byte6-7 Protection
    //   break;
    // case 0x4260:
    // case 0x4261:
    //   //Byte0-1 Module Max Voltage
    //   //Byte2-3 Module Min Voltage
    //   //Byte4-5 Module Max. Voltage Number
    //   //Byte6-7 Module Min. Voltage Number
    //   break;
    // case 0x4270:
    // case 0x4271:
    //   //Byte0-1 Module Max. Temperature
    //   //Byte2-3 Module Min. Temperature
    //   //Byte4-5 Module Max. Temperature Number
    //   //Byte6-7 Module Min. Temperature Number
    //   break;
    // case 0x4280:
    // case 0x4281:
    //   charge_forbidden = rx_frame.data.u8[0];
    //   discharge_forbidden = rx_frame.data.u8[1];
    //   break;
    // case 0x4290:
    // case 0x4291:
    //   break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 1s CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {

    previousMillis1000 = currentMillis;

    ESP32Can.CANWriteFrame(&PYLON_3010);  // Heartbeat
    ESP32Can.CANWriteFrame(&PYLON_4200);  // Ensemble OR System equipment info, depends on frame0
    ESP32Can.CANWriteFrame(&PYLON_8200);  // Control device quit sleep status
    ESP32Can.CANWriteFrame(&PYLON_8210);  // Charge command

    if (ensemble_info_ack) {
      PYLON_4200.data.u8[0] = 0x00;  //Request system equipment info
    }
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Balancell battery selected");

      // battery_module_quantity = 2;
      // battery_modules_in_series = 1;
      // cell_quantity_in_module = 32;

#endif

  datalayer.battery.info.max_design_voltage_dV = 1150;  // 115V, charging over this is not possible
  datalayer.battery.info.min_design_voltage_dV = 960;  // 96V, under this, discharging further is disabled
}

#endif
