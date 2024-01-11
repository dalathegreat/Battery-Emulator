#include "KIA-HYUNDAI-64-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10ms = 0;  // will store last time a 10s CAN Message was send
static const int interval100 = 100;            // interval (ms) at which send CAN Messages
static const int interval10ms = 10;          // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive

#define MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter
#define MAX_CELL_VOLTAGE 4250   //Battery is put into emergency stop if one cell goes over this value
#define MIN_CELL_VOLTAGE 2950   //Battery is put into emergency stop if one cell goes below this value
#define MAX_CELL_DEVIATION 150  //LED turns yellow on the board if mv delta exceeds this value

static uint16_t soc_calculated = 0;
static uint16_t SOC_BMS = 0;
static uint16_t SOC_Display = 0;
static uint16_t battSOH = 0;
static uint8_t waterleakageSensor = 164;
static int16_t aux12volt = 0;
static uint16_t CellVoltMax = 3700;
static uint8_t CellVmaxNo = 0;
static uint16_t CellVoltMin = 3700;
static uint8_t CellVminNo = 0;
static uint16_t cell_deviation_mV = 0;  //contains the deviation between highest and lowest cell in mV
static int16_t allowedDischargePower = 0;
static int16_t allowedChargePower = 0;
static int16_t battVolts = 0;
static int16_t battAmps = 0;
static int8_t temperatureMax = 0;
static int8_t temperatureMin = 0;
static int8_t temp1 = 0;
static int8_t tempinlet = 0;
static uint8_t batteryManagementMode = 0;
static uint8_t BMS_ign = 0;
static int16_t poll_data_pid = 0;
static int16_t poll109 = 0;
static int16_t pollA2A = 0;
static int16_t poll200 = 0;
static int16_t poll201 = 0;
static int16_t poll2B0 = 0;
static int16_t poll450 = 0;
static int16_t poll471 = 0;
static int16_t poll523 = 0;
static int8_t heatertemp = 0;
//static int airbag = 0; Not needed
static uint8_t batteryRelay = 0;
static int8_t PRAtemp = 0;
static int16_t Inv_VoltsTmp = 0;
static int16_t Inv_Volts = 0;
static uint8_t mode491 = 0;
static int8_t radiatorTemperature = 0;
static int8_t chargeTemperature = 0;
static int16_t frame524volts = 0;
static int8_t fanMode = 0;
static int8_t fanSpeed = 0;
static int8_t state3F6 = 0;
static uint counter = 0;
static int8_t sendDiagnostic = 0;

CAN_frame_t KIA_HYUNDAI_200 = {.FIR = {.B =
                                    {
                                        .DLC = 8,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x200,
                        //.data = {0x00, 0x00, 0x00, 0x04, 0x00, 0x50, 0xD0, 0x00}}; //Initial value
                        .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}}; //Mid log value
CAN_frame_t KIA_HYUNDAI_523 = {.FIR = {.B =
                                    {
                                        .DLC = 8,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x523,
                        //.data = {0x00, 0x38, 0x28, 0x28, 0x28, 0x28, 0x00, 0x01}}; //Initial value
                        .data = {0x08, 0x38, 0x36, 0x36, 0x33, 0x34, 0x00, 0x01}}; //Mid log value
CAN_frame_t KIA_HYUNDAI_524 = {.FIR = {.B =
                                    {
                                        .DLC = 8,
                                        .FF = CAN_frame_std,
                                    }},
                        .MsgID = 0x524,
                        //.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; //Initial value
                        .data = {0x88, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; //Mid log value

//553 Needed frame 200ms
CAN_frame_t KIA64_553 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x553,.data = {0x04, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80, 0x00}};
//57F Needed frame 100ms
CAN_frame_t KIA64_57F = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x57F,.data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};
//Needed frame 100ms
CAN_frame_t KIA64_2A1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x2A1,.data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};


CAN_frame_t KIA64_7E4_id1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 01
CAN_frame_t KIA64_7E4_id2 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 02
CAN_frame_t KIA64_7E4_id3 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 03
CAN_frame_t KIA64_7E4_id4 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 04
CAN_frame_t KIA64_7E4_id5 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 05
CAN_frame_t KIA64_7E4_id6 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 06

CAN_frame_t KIA64_7E4_ack = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; //Ack frame, correct PID is returned



static uint8_t counter_200 = 0;

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
  
  StateOfHealth = (battSOH * 10); //Increase decimals from 100.0% -> 100.00%

  battery_voltage = (uint16_t)battVolts; //value is *10

  battery_current = (int16_t)battAmps; //value is *10 //Todo, the signed part breaks something here for sure

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh = static_cast<int>((static_cast<double>(SOC) / 10000) * BATTERY_WH_MAX);

  max_target_discharge_power = (uint16_t)allowedDischargePower * 10; //From kW*100 to Watts

  max_target_charge_power = (uint16_t)allowedChargePower * 10; //From kW*100 to Watts

  stat_batt_power = ((uint16_t)battVolts * (int16_t)battAmps) / 100; //Power in watts, Negative = charging batt. //Todo, the signed part will break something here

  temperature_min = ((int8_t)temperatureMin * 10); //Increase decimals, 17C -> 17.0C
  
  temperature_max = ((int8_t)temperatureMax * 10); //Increase decimals, 18C -> 18.0C
  
  cell_max_voltage = CellVoltMax; //in millivolt
  
  cell_min_voltage = CellVoltMin; //in millivolt

  bms_status = ACTIVE; //Startout in active mode. Then check safeties

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

  if(waterleakageSensor == 0){
    Serial.println("Water leakage inside battery detected. Operation halted. Inspect battery!");
    bms_status = FAULT;
  }

  // Check if cell voltages are within allowed range
  cell_deviation_mV = (cell_max_voltage - cell_min_voltage);

  if (cell_max_voltage >= MAX_CELL_VOLTAGE) {
      bms_status = FAULT;
      Serial.println("ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
    }
    if (cell_min_voltage <= MIN_CELL_VOLTAGE) {
      bms_status = FAULT;
      Serial.println("ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
    }
    if (cell_deviation_mV > MAX_CELL_DEVIATION) {
      LEDcolor = YELLOW;
      Serial.println("ERROR: HIGH CELL DEVIATION!!! Inspect battery!");
    }

  if (bms_status == FAULT) {  //Incase we enter a critical fault state, zero out the allowed limits
    max_target_charge_power = 0;
    max_target_discharge_power = 0;
  }

  /* Safeties verified. Perform USB serial printout if configured to do so */

#ifdef DEBUG_VIA_USB
  Serial.println(); //sepatator
  Serial.println("Values from battery: ");
  Serial.print("SOC BMS: ");
  Serial.print(SOC_BMS / 10.0, 1);
  Serial.print("%  |  SOC Display: ");
  Serial.print(SOC_Display / 10.0, 1);
  Serial.print("%  |  SOH ");
  Serial.print(battSOH / 10.0, 1);
  Serial.println("%");
  Serial.print((int16_t)battAmps / 10.0, 1);
  Serial.print(" Amps  |  ");
  Serial.print((int16_t)battVolts / 10.0, 1);
  Serial.print(" Volts  |  ");
  Serial.print((int16_t)stat_batt_power);
  Serial.println(" Watts");
  Serial.print("Allowed Charge ");
  Serial.print((uint16_t)allowedChargePower * 10);
  Serial.print(" W  |  Allowed Discharge ");
  Serial.print((uint16_t)allowedDischargePower * 10);
  Serial.println(" W");
  Serial.print("MaxCellVolt ");
  Serial.print(CellVoltMax);
  Serial.print(" mV  No  ");
  Serial.print(CellVmaxNo);
  Serial.print("  |  MinCellVolt ");
  Serial.print(CellVoltMin);
  Serial.print(" mV  No  ");
  Serial.println(CellVminNo);
  Serial.print("TempHi ");
  Serial.print((int8_t)temperatureMax);
  Serial.print("°C  TempLo ");
  Serial.print((int8_t)temperatureMin);
  Serial.print("°C  WaterInlet ");
  Serial.print((int8_t)tempinlet);
  Serial.print("°C  PowerRelay ");
  Serial.print((int8_t)PRAtemp * 2);
  Serial.println("°C");
  Serial.print("Aux12volt: ");
  Serial.print((int16_t)aux12volt / 10.0, 1);
  Serial.println("V  |  ");
  Serial.print("BmsMagementMode ");
  Serial.print((uint8_t)batteryManagementMode, BIN);
  if (bitRead((uint8_t)BMS_ign, 2) == 1){
    Serial.print("  |  BmsIgitionON");
  }
  else {
    Serial.print("  |  BmsIgitionOFF");
  }
  
  if (bitRead((uint8_t)batteryRelay, 0) == 1){
    Serial.print("  |  BMS Output ON");
  }
  else {
    Serial.print("  |  BMS Output OFF");
  }
    Serial.print("  |  Inverter ");
    Serial.print((int16_t)Inv_Volts);
    Serial.println(" Volts");
    //Serial.print("Diagnostic state(12=outputON): ");
    //Serial.println(sendDiagnostic);
  }
#endif

void receive_can_kiaHyundai_64_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;
  switch (rx_frame.MsgID) {
    case 0x491:
      mode491 = rx_frame.data.u8[0];
      break;
    case 0x4DE:
      radiatorTemperature = rx_frame.data.u8[4]; //-15
      break;
    case 0x542: //BMS SOC
      SOC_Display = rx_frame.data.u8[0] * 5; //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      SOC_BMS = rx_frame.data.u8[5] * 5; //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      battVolts = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      battAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      frame524volts = (int16_t)battVolts / 10;
      //if (counter > 51) {KIA64_524_1.data = {(uint8_t)frame524volts, (uint8_t)(frame524volts >> 8), 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; } //vcu volts to bms
      break;
    case 0x596:
      aux12volt = rx_frame.data.u8[1]; //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6]; //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7]; //Highest temp in battery
      break;
    case 0x598:
      chargeTemperature = rx_frame.data.u8[7]; //ChargeportTemperature
      break;
    case 0x5D5:
      waterleakageSensor = rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      PRAtemp = rx_frame.data.u8[7]; //PowerRelayTemp
      break;
    case 0x5D8:
      counter++;
      //ESP32Can.CANWriteFrame(&KIA64_524_1);
      //ESP32Can.CANWriteFrame(&KIA64_291_1); //motor speed to 32767 if 100ms sent

      //PID data is polled after last message sent from battery:
      if (poll_data_pid >= 10){  //polling one of ten PIDs at 100ms, resolution = 1s
        poll_data_pid = 0;
        }
      poll_data_pid++;
      if (poll_data_pid == 1){
        ESP32Can.CANWriteFrame(&KIA64_7E4_id1);
      }
      else if (poll_data_pid == 2){
        ESP32Can.CANWriteFrame(&KIA64_7E4_id2);
      }
      else if (poll_data_pid == 5){
        ESP32Can.CANWriteFrame(&KIA64_7E4_id5);
      }
      else if (poll_data_pid == 6){
        ESP32Can.CANWriteFrame(&KIA64_7E4_id6);
      }
      else if (poll_data_pid == 8){
      }
      else if (poll_data_pid == 9){
      }
      else if (poll_data_pid == 10){
      }
      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      switch (rx_frame.data.u8[0])
      {
        case 0x02:  //"Diagnostic"
         if (rx_frame.data.u8[1] == 0x7E){
           sendDiagnostic = 2;
         }
         case 0x03:  //"Diagnostic"
         if (rx_frame.data.u8[1] == 0x7F){
           sendDiagnostic = 4;
         }
        case 0x04:  //"Diagnostic"
        if (rx_frame.data.u8[1] == 0x6F){
           sendDiagnostic = 3;
         }
        case 0x10:  //"PID Header"
         if (rx_frame.data.u8[4] == poll_data_pid){
           ESP32Can.CANWriteFrame(&KIA64_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
         }
      break;
        case 0x21:  //First frame in PID group
         if (poll_data_pid == 1){
           allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
           allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
           batteryRelay = rx_frame.data.u8[7];
         }
      break;
        case 0x22:  //Second datarow in PID group
         if (poll_data_pid == 1){
           //battAmps = (rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2]; //moved to passive data
           //battVolts = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]; //moved to passive data
           //tempMax = rx_frame.data.u8[5]; //moved to passive data
           //tempMin = rx_frame.data.u8[6]; //moved to passive data
           temp1 = rx_frame.data.u8[7];
         }
         if (poll_data_pid == 6){
           batteryManagementMode = rx_frame.data.u8[5];
         }
      break;
        case 0x23:  //Third datarow in PID group
         if (poll_data_pid == 1){
           tempinlet = rx_frame.data.u8[6];
           CellVoltMax = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
         }
         if (poll_data_pid == 5){
           //airbag = rx_frame.data.u8[6]; Not needed
           heatertemp = rx_frame.data.u8[7];
         }
      break;
        case 0x24:  //Fourth datarow in PID group
         if (poll_data_pid == 1){
           CellVmaxNo = rx_frame.data.u8[1];
           CellVminNo = rx_frame.data.u8[3];
           CellVoltMin = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
           fanMode = rx_frame.data.u8[4];
           fanSpeed = rx_frame.data.u8[5];
         }
         else if (poll_data_pid == 5){
           battSOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
         }
      break;
        case 0x25:  //Fifth datarow in PID group
      break;
        case 0x26:  //Sixth datarow in PID group
      break;
        case 0x27:  //Seventh datarow in PID group
         if (poll_data_pid == 1){
         BMS_ign = rx_frame.data.u8[6];
         Inv_VoltsTmp = rx_frame.data.u8[7];
         }
      break;
        case 0x28:  //Eighth datarow in PID group
         if (poll_data_pid == 1){
         Inv_Volts = (Inv_VoltsTmp << 8) + rx_frame.data.u8[1];
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
        ++counter_200;
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
