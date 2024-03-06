#include "BATTERIES.h"
#ifdef KIA_HYUNDAI_64_BATTERY
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "KIA-HYUNDAI-64-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10ms = 0;  // will store last time a 10s CAN Message was send
static const int interval100 = 100;           // interval (ms) at which send CAN Messages
static const int interval10ms = 10;           // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;            //counter for checking if CAN is still alive

#define MAX_CELL_VOLTAGE 4250   //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950   //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION 150  //LED turns yellow on the board if mv delta exceeds this value

static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t batterySOH = 1000;
static uint8_t waterleakageSensor = 164;
static int16_t leadAcidBatteryVoltage = 0;
static uint16_t CellVoltMax_mV = 3700;
static uint8_t CellVmaxNo = 0;
static uint16_t CellVoltMin_mV = 3700;
static uint8_t CellVminNo = 0;
static uint16_t cell_deviation_mV = 0;
static int16_t allowedDischargePower = 0;
static int16_t allowedChargePower = 0;
static uint16_t batteryVoltage = 0;
static int16_t batteryAmps = 0;
static int16_t powerWatt = 0;
static int16_t temperatureMax = 0;
static int16_t temperatureMin = 0;
static int8_t temperature_water_inlet = 0;
static uint8_t batteryManagementMode = 0;
static uint8_t BMS_ign = 0;
static int16_t poll_data_pid = 0;
static int8_t heatertemp = 0;
static uint8_t batteryRelay = 0;
static int8_t powerRelayTemperature = 0;
static uint16_t inverterVoltageFrameHigh = 0;
static uint16_t inverterVoltage = 0;
static uint8_t startedUp = false;
static uint8_t counter_200 = 0;

CAN_frame_t KIA_HYUNDAI_200 = {.FIR = {.B =
                                           {
                                               .DLC = 8,
                                               .FF = CAN_frame_std,
                                           }},
                               .MsgID = 0x200,
                               //.data = {0x00, 0x00, 0x00, 0x04, 0x00, 0x50, 0xD0, 0x00}}; //Initial value
                               .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};  //Mid log value
CAN_frame_t KIA_HYUNDAI_523 = {.FIR = {.B =
                                           {
                                               .DLC = 8,
                                               .FF = CAN_frame_std,
                                           }},
                               .MsgID = 0x523,
                               //.data = {0x00, 0x38, 0x28, 0x28, 0x28, 0x28, 0x00, 0x01}}; //Initial value
                               .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}};  //Mid log value
CAN_frame_t KIA_HYUNDAI_524 = {.FIR = {.B =
                                           {
                                               .DLC = 8,
                                               .FF = CAN_frame_std,
                                           }},
                               .MsgID = 0x524,
                               .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Initial value

//553 Needed frame 200ms
CAN_frame_t KIA64_553 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x553,
                         .data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
//57F Needed frame 100ms
CAN_frame_t KIA64_57F = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x57F,
                         .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
//Needed frame 100ms
CAN_frame_t KIA64_2A1 = {.FIR = {.B =
                                     {
                                         .DLC = 8,
                                         .FF = CAN_frame_std,
                                     }},
                         .MsgID = 0x2A1,
                         .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

CAN_frame_t KIA64_7E4_id1 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 01
CAN_frame_t KIA64_7E4_id2 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 02
CAN_frame_t KIA64_7E4_id3 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 03
CAN_frame_t KIA64_7E4_id4 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 04
CAN_frame_t KIA64_7E4_id5 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 05
CAN_frame_t KIA64_7E4_id6 = {.FIR = {.B =
                                         {
                                             .DLC = 8,
                                             .FF = CAN_frame_std,
                                         }},
                             .MsgID = 0x7E4,
                             .data = {0x03, 0x22, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00}};  //Poll PID 03 22 01 06
CAN_frame_t KIA64_7E4_ack = {
    .FIR = {.B =
                {
                    .DLC = 8,
                    .FF = CAN_frame_std,
                }},
    .MsgID = 0x7E4,
    .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};  //Ack frame, correct PID is returned

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  system_real_SOC_pptt = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  system_SOH_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  system_battery_voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  system_battery_current_dA = batteryAmps;  //value is *10 (150 = 15.0)

  system_capacity_Wh = BATTERY_WH_MAX;

  system_remaining_capacity_Wh = static_cast<int>((static_cast<double>(system_real_SOC_pptt) / 10000) * BATTERY_WH_MAX);

  //system_max_charge_power_W = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We estimate this value
  if (system_scaled_SOC_pptt == 10000) {  // When scaled SOC is 100%, set allowed charge power to 0
    system_max_charge_power_W = 0;
  } else {  // No limits, max charging power allowed
    system_max_charge_power_W = MAXCHARGEPOWERALLOWED;
  }
  //system_max_discharge_power_W = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  if (system_scaled_SOC_pptt < 100) {  // When scaled SOC is <1%, set allowed charge power to 0
    system_max_discharge_power_W = 0;
  } else {  // No limits, max charging power allowed
    system_max_discharge_power_W = MAXDISCHARGEPOWERALLOWED;
  }

  powerWatt = ((batteryVoltage * batteryAmps) / 100);

  system_active_power_W = powerWatt;  //Power in watts, Negative = charging batt

  system_temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  system_temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  system_cell_max_voltage_mV = CellVoltMax_mV;

  system_cell_min_voltage_mV = CellVoltMin_mV;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_RX_FAILURE, 0);
  } else {
    CANstillAlive--;
    clear_event(EVENT_CAN_RX_FAILURE);
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Check if cell voltages are within allowed range
  cell_deviation_mV = (system_cell_max_voltage_mV - system_cell_min_voltage_mV);

  if (CellVoltMax_mV >= MAX_CELL_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (CellVoltMin_mV <= MIN_CELL_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION) {
    set_event(EVENT_CELL_DEVIATION_HIGH, 0);
  } else {
    clear_event(EVENT_CELL_DEVIATION_HIGH);
  }

  if (system_bms_status == FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    system_max_charge_power_W = 0;
    system_max_discharge_power_W = 0;
  }

  /* Safeties verified. Perform USB serial printout if configured to do so */

#ifdef DEBUG_VIA_USB
  Serial.println();  //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print((uint16_t)SOC_BMS / 10.0, 1);
  Serial.print("%  |  SOC Display: ");
  Serial.print((uint16_t)SOC_Display / 10.0, 1);
  Serial.print("%  |  SOH ");
  Serial.print((uint16_t)batterySOH / 10.0, 1);
  Serial.println("%");
  Serial.print((int16_t)batteryAmps / 10.0, 1);
  Serial.print(" Amps  |  ");
  Serial.print((uint16_t)batteryVoltage / 10.0, 1);
  Serial.print(" Volts  |  ");
  Serial.print((int16_t)system_active_power_W);
  Serial.println(" Watts");
  Serial.print("Allowed Charge ");
  Serial.print((uint16_t)allowedChargePower * 10);
  Serial.print(" W  |  Allowed Discharge ");
  Serial.print((uint16_t)allowedDischargePower * 10);
  Serial.println(" W");
  Serial.print("MaxCellVolt ");
  Serial.print(CellVoltMax_mV);
  Serial.print(" mV  No  ");
  Serial.print(CellVmaxNo);
  Serial.print("  |  MinCellVolt ");
  Serial.print(CellVoltMin_mV);
  Serial.print(" mV  No  ");
  Serial.println(CellVminNo);
  Serial.print("TempHi ");
  Serial.print((int16_t)temperatureMax);
  Serial.print("°C  TempLo ");
  Serial.print((int16_t)temperatureMin);
  Serial.print("°C  WaterInlet ");
  Serial.print((int8_t)temperature_water_inlet);
  Serial.print("°C  PowerRelay ");
  Serial.print((int8_t)powerRelayTemperature * 2);
  Serial.println("°C");
  Serial.print("Aux12volt: ");
  Serial.print((int16_t)leadAcidBatteryVoltage / 10.0, 1);
  Serial.println("V  |  ");
  Serial.print("BmsManagementMode ");
  Serial.print((uint8_t)batteryManagementMode, BIN);
  if (bitRead((uint8_t)BMS_ign, 2) == 1) {
    Serial.print("  |  BmsIgnition ON");
  } else {
    Serial.print("  |  BmsIgnition OFF");
  }

  if (bitRead((uint8_t)batteryRelay, 0) == 1) {
    Serial.print("  |  PowerRelay ON");
  } else {
    Serial.print("  |  PowerRelay OFF");
  }
  Serial.print("  |  Inverter ");
  Serial.print(inverterVoltage);
  Serial.println(" Volts");
#endif
}

void receive_can_battery(CAN_frame_t rx_frame) {
  switch (rx_frame.MsgID) {
    case 0x4DE:
      break;
    case 0x542:                               //BMS SOC
      CANstillAlive = 12;                     //We use this message to verify that BMS is still alive
      SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (counter_200 > 3) {
        KIA_HYUNDAI_524.data.u8[0] = (uint8_t)(batteryVoltage / 10);
        KIA_HYUNDAI_524.data.u8[1] = (uint8_t)((batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms
      break;
    case 0x596:
      leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      break;
    case 0x5D5:
      waterleakageSensor = rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      startedUp = 1;

      //PID data is polled after last message sent from battery:
      if (poll_data_pid >= 10) {  //polling one of ten PIDs at 100ms, resolution = 1s
        poll_data_pid = 0;
      }
      poll_data_pid++;
      if (poll_data_pid == 1) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id1);
      } else if (poll_data_pid == 2) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id2);
      } else if (poll_data_pid == 3) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id3);
      } else if (poll_data_pid == 4) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id4);
      } else if (poll_data_pid == 5) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id5);
      } else if (poll_data_pid == 6) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id6);
      } else if (poll_data_pid == 7) {
      } else if (poll_data_pid == 8) {
      } else if (poll_data_pid == 9) {
      } else if (poll_data_pid == 10) {
      }
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0]) {
        case 0x10:  //"PID Header"
          if (rx_frame.data.u8[4] == poll_data_pid) {
            ESP32Can.CANWriteFrame(&KIA64_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
          }
          break;
        case 0x21:  //First frame in PID group
          if (poll_data_pid == 1) {
            allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
            allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
            batteryRelay = rx_frame.data.u8[7];
          } else if (poll_data_pid == 2) {
            system_cellvoltages_mV[0] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[1] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[2] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[3] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[4] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[5] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            system_cellvoltages_mV[32] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[33] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[34] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[35] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[36] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[37] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            system_cellvoltages_mV[64] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[65] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[66] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[67] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[68] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[69] = (rx_frame.data.u8[7] * 20);
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 2) {
            system_cellvoltages_mV[6] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[7] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[8] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[9] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[10] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[11] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[12] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            system_cellvoltages_mV[38] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[39] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[40] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[41] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[42] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[43] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[44] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            system_cellvoltages_mV[70] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[71] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[72] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[73] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[74] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[75] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[76] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            system_cellvoltages_mV[13] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[14] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[15] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[16] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[17] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[18] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[19] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            system_cellvoltages_mV[45] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[46] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[47] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[48] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[49] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[50] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[51] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            system_cellvoltages_mV[77] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[78] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[79] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[80] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[81] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[82] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[83] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 5) {
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 2) {
            system_cellvoltages_mV[20] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[21] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[22] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[23] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[24] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[25] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[26] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 3) {
            system_cellvoltages_mV[52] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[53] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[54] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[55] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[56] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[57] = (rx_frame.data.u8[6] * 20);
            system_cellvoltages_mV[58] = (rx_frame.data.u8[7] * 20);
          } else if (poll_data_pid == 4) {
            system_cellvoltages_mV[84] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[85] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[86] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[87] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[88] = (rx_frame.data.u8[5] * 20);
            system_cellvoltages_mV[89] = (rx_frame.data.u8[6] * 20);
            if (rx_frame.data.u8[7] > 50) {
              system_cellvoltages_mV[90] = (rx_frame.data.u8[7] * 20);
            } else {  //90S battery
              system_cellvoltages_mV[90] = 0;
              system_number_of_cells = 90;
            }
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
          if (poll_data_pid == 2) {
            system_cellvoltages_mV[27] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[28] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[29] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[30] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[31] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 3) {
            system_cellvoltages_mV[59] = (rx_frame.data.u8[1] * 20);
            system_cellvoltages_mV[60] = (rx_frame.data.u8[2] * 20);
            system_cellvoltages_mV[61] = (rx_frame.data.u8[3] * 20);
            system_cellvoltages_mV[62] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[63] = (rx_frame.data.u8[5] * 20);
          } else if (poll_data_pid == 4) {
            if (rx_frame.data.u8[1] > 50) {  //We have cellvoltages
              system_cellvoltages_mV[91] = (rx_frame.data.u8[1] * 20);
              system_cellvoltages_mV[92] = (rx_frame.data.u8[2] * 20);
              system_cellvoltages_mV[93] = (rx_frame.data.u8[3] * 20);
              system_cellvoltages_mV[94] = (rx_frame.data.u8[4] * 20);
              system_cellvoltages_mV[95] = (rx_frame.data.u8[5] * 20);
            } else {  //90S battery
              system_number_of_cells = 90;
              system_cellvoltages_mV[91] = 0;
              system_cellvoltages_mV[92] = 0;
              system_cellvoltages_mV[93] = 0;
              system_cellvoltages_mV[94] = 0;
              system_cellvoltages_mV[95] = 0;
              system_max_design_voltage_dV = 3870;
              system_min_design_voltage_dV = 2250;
            }

          } else if (poll_data_pid == 5) {
            system_cellvoltages_mV[96] = (rx_frame.data.u8[4] * 20);
            system_cellvoltages_mV[97] = (rx_frame.data.u8[5] * 20);
            system_number_of_cells = 98;
          }
          break;
        case 0x26:  //Sixth datarow in PID group
          break;
        case 0x27:  //Seventh datarow in PID group
          if (poll_data_pid == 1) {
            BMS_ign = rx_frame.data.u8[6];
            inverterVoltageFrameHigh = rx_frame.data.u8[7];
          }
          break;
        case 0x28:  //Eighth datarow in PID group
          if (poll_data_pid == 1) {
            inverterVoltage = (inverterVoltageFrameHigh << 8) + rx_frame.data.u8[1];
          }
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  //Send 100ms message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&KIA64_553);
    ESP32Can.CANWriteFrame(&KIA64_57F);
    ESP32Can.CANWriteFrame(&KIA64_2A1);
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= interval10ms) {
    previousMillis10ms = currentMillis;

    switch (counter_200) {
      case 0:
        KIA_HYUNDAI_200.data.u8[5] = 0x17;
        ++counter_200;
        break;
      case 1:
        KIA_HYUNDAI_200.data.u8[5] = 0x57;
        ++counter_200;
        break;
      case 2:
        KIA_HYUNDAI_200.data.u8[5] = 0x97;
        ++counter_200;
        break;
      case 3:
        KIA_HYUNDAI_200.data.u8[5] = 0xD7;
        if (startedUp) {
          ++counter_200;
        } else {
          counter_200 = 0;
        }
        break;
      case 4:
        KIA_HYUNDAI_200.data.u8[3] = 0x10;
        KIA_HYUNDAI_200.data.u8[5] = 0xFF;
        ++counter_200;
        break;
      case 5:
        KIA_HYUNDAI_200.data.u8[5] = 0x3B;
        ++counter_200;
        break;
      case 6:
        KIA_HYUNDAI_200.data.u8[5] = 0x7B;
        ++counter_200;
        break;
      case 7:
        KIA_HYUNDAI_200.data.u8[5] = 0xBB;
        ++counter_200;
        break;
      case 8:
        KIA_HYUNDAI_200.data.u8[5] = 0xFB;
        counter_200 = 5;
        break;
    }

    ESP32Can.CANWriteFrame(&KIA_HYUNDAI_200);

    ESP32Can.CANWriteFrame(&KIA_HYUNDAI_523);

    ESP32Can.CANWriteFrame(&KIA_HYUNDAI_524);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  Serial.println("Kia Niro / Hyundai Kona 64kWh battery selected");

  system_max_design_voltage_dV = 4040;  // 404.0V, over this, charging is not possible (goes into forced discharge)
  system_min_design_voltage_dV = 3100;  // 310.0V under this, discharging further is disabled
}

#endif
