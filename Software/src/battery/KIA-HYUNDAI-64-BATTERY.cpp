#include "KIA-HYUNDAI-64-BATTERY.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10ms = 0;  // will store last time a 10s CAN Message was send
static const int interval100 = 100;           // interval (ms) at which send CAN Messages
static const int interval10ms = 10;           // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;            //counter for checking if CAN is still alive

#define MAX_SOC 1000            //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define MIN_SOC 0               //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter
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

void update_values_kiaHyundai_64_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //Calculate the SOC% value to send to inverter
  soc_calculated = SOC_Display;
  soc_calculated = MIN_SOC + (MAX_SOC - MIN_SOC) * (soc_calculated - MINPERCENTAGE) / (MAXPERCENTAGE - MINPERCENTAGE);
  if (soc_calculated < 0) {  //We are in the real SOC% range of 0-20%, always set SOC sent to Inverter as 0%
    soc_calculated = 0;
  }
  if (soc_calculated > 1000) {  //We are in the real SOC% range of 80-100%, always set SOC sent to Inverter as 100%
    soc_calculated = 1000;
  }
  SOC = (soc_calculated * 10);  //increase SOC range from 0-100.0 -> 100.00

  StateOfHealth = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  battery_voltage = batteryVoltage;  //value is *10 (3700 = 370.0)

  battery_current = convertToUnsignedInt16(batteryAmps);  //value is *10 (150 = 15.0)

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh = static_cast<int>((static_cast<double>(SOC) / 10000) * BATTERY_WH_MAX);

  //max_target_charge_power = (uint16_t)allowedChargePower * 10;  //From kW*100 to Watts
  //The allowed charge power is not available. We estimate this value
  if (SOC == 10000) {  // When scaled SOC is 100%, set allowed charge power to 0
    max_target_charge_power = 0;
  } else {  // No limits, max charging power allowed
    max_target_charge_power = MAXCHARGEPOWERALLOWED;
  }
  //max_target_discharge_power = (uint16_t)allowedDischargePower * 10;  //From kW*100 to Watts
  if (SOC < 100) {  // When scaled SOC is <1%, set allowed charge power to 0
    max_target_discharge_power = 0;
  } else {  // No limits, max charging power allowed
    max_target_discharge_power = MAXDISCHARGEPOWERALLOWED;
  }

  powerWatt = ((batteryVoltage * batteryAmps) / 100);

  stat_batt_power = convertToUnsignedInt16(powerWatt);  //Power in watts, Negative = charging batt

  temperature_min = convertToUnsignedInt16((int8_t)temperatureMin * 10);  //Increase decimals, 17C -> 17.0C

  temperature_max = convertToUnsignedInt16((int8_t)temperatureMax * 10);  //Increase decimals, 18C -> 18.0C

  cell_max_voltage = CellVoltMax_mV;

  cell_min_voltage = CellVoltMin_mV;

  bms_status = ACTIVE;  //Startout in active mode. Then check safeties

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    set_event(EVENT_CAN_FAILURE, 0);
  } else {
    CANstillAlive--;
  }

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }

  // Check if cell voltages are within allowed range
  cell_deviation_mV = (cell_max_voltage - cell_min_voltage);

  if (cell_max_voltage >= MAX_CELL_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (cell_min_voltage <= MIN_CELL_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }
  if (cell_deviation_mV > MAX_CELL_DEVIATION) {
    set_event(EVENT_CELL_DEVIATION_HIGH, 0);
  }

  if (bms_status == FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    max_target_charge_power = 0;
    max_target_discharge_power = 0;
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
  Serial.print((int16_t)stat_batt_power);
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

void receive_can_kiaHyundai_64_battery(CAN_frame_t rx_frame) {
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
      } else if (poll_data_pid == 5) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id5);
      } else if (poll_data_pid == 6) {
        ESP32Can.CANWriteFrame(&KIA64_7E4_id6);
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
          }
          break;
        case 0x22:  //Second datarow in PID group
          if (poll_data_pid == 6) {
            batteryManagementMode = rx_frame.data.u8[5];
          }
          break;
        case 0x23:  //Third datarow in PID group
          if (poll_data_pid == 1) {
            temperature_water_inlet = rx_frame.data.u8[6];
            CellVoltMax_mV = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
          }
          if (poll_data_pid == 5) {
            heatertemp = rx_frame.data.u8[7];
          }
          break;
        case 0x24:  //Fourth datarow in PID group
          if (poll_data_pid == 1) {
            CellVmaxNo = rx_frame.data.u8[1];
            CellVminNo = rx_frame.data.u8[3];
            CellVoltMin_mV = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
          } else if (poll_data_pid == 5) {
            batterySOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
          }
          break;
        case 0x25:  //Fifth datarow in PID group
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

void send_can_kiaHyundai_64_battery() {
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

uint16_t convertToUnsignedInt16(int16_t signed_value) {
  if (signed_value < 0) {
    return (65535 + signed_value);
  } else {
    return (uint16_t)signed_value;
  }
}
