#include "KIA-HYUNDAI-64-BATTERY.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis10s = 0;  // will store last time a 10s CAN Message was send
static const int interval100 = 100;            // interval (ms) at which send CAN Messages
static const int interval10s = 10000;          // interval (ms) at which send CAN Messages
static uint8_t CANstillAlive = 12;           //counter for checking if CAN is still alive

#define LB_MAX_SOC 1000  //BMS never goes over this value. We use this info to rescale SOC% sent to Inverter
#define LB_MIN_SOC 0     //BMS never goes below this value. We use this info to rescale SOC% sent to Inverter

static double SOC_BMS = 0;
static double SOC_Display = 0;
static int currentSensor = 0;
static int RES_WS0 = 0;
static int tempSensH = 0;
static double aux12volt = 0;
static int testdata0 = 0x0;
static int testdata1 = 0x0;
static int testdata2 = 0x0;
static int testdata3 = 0x0;
static int testdata4 = 0x0;
static int testdata5 = 0x0;
static int testdata6 = 0x0;
static int CoolWaterSensor = 0;
static int poll_data_pid = 0;
static int battSOH = 0;
static int CellVoltMax = 0;
static int CellVmaxNo = 0;
static int CellVoltMin = 0;
static int CellVminNo = 0;
static int allowedDischargePower = 0;
static int allowedChargePower = 0;
static double battVolts = 0;
static double battAmps = 0;

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

CAN_frame_t KIA64_7E4_id1 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 01
CAN_frame_t KIA64_7E4_id2 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 02
CAN_frame_t KIA64_7E4_id3 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x03, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 03
CAN_frame_t KIA64_7E4_id4 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x04, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 04
CAN_frame_t KIA64_7E4_id5 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x05, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 05
CAN_frame_t KIA64_7E4_id6 = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x03, 0x22, 0x01, 0x06, 0x00, 0x00, 0x00, 0x00}}; //Poll PID 03 22 01 06

CAN_frame_t KIA64_7E4_ack = {.FIR = {.B = {.DLC = 8,.FF = CAN_frame_std,}},.MsgID = 0x7E4,.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}; //Ack frame, correct PID is returned



static uint8_t counter_200 = 0;

void update_values_kiaHyundai_64_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  bms_status = ACTIVE;                        //Startout in active mode

  SOC = (SOC_Display * 100); //Increase decimals from 50% -> 50.00%

  battery_voltage;

  battery_current;

  capacity_Wh = BATTERY_WH_MAX;

  remaining_capacity_Wh;

  max_target_discharge_power;

  max_target_charge_power;

  stat_batt_power;

  temperature_min;

  temperature_max;

  /* Check if the BMS is still sending CAN messages. If we go 60s without messages we raise an error*/
  if (!CANstillAlive) {
    bms_status = FAULT;
    Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
  } else {
    CANstillAlive--;
  }

#ifdef DEBUG_VIA_USB
    Serial.println("Values from battery: ");
    Serial.print("SOC% BMS: ");
    Serial.print(SOC_BMS);
    Serial.print("  |  SOC% Display: ");
    Serial.println(SOC_Display);
    Serial.print("SOC%*100: ");
    Serial.println(SOC);
    Serial.print("BatteryCurrent: ");
    Serial.println(currentSensor);  
    Serial.print("Vattensensor: ");
    Serial.println(RES_WS0);
    Serial.print("HighestTemp: ");
    Serial.print(tempSensH);
    Serial.println("°C");
    Serial.print("Aux12volt: ");
    Serial.print(aux12volt);
    Serial.println("V");
    Serial.print("SOH*10 ");
    Serial.println(battSOH);
    Serial.print("MaxCellVolt ");
    Serial.print(CellVoltMax);
    Serial.print(" mV  No  ");
    Serial.print(CellVmaxNo);
    Serial.print("  |  MinCellVolt ");
    Serial.print(CellVoltMin);
    Serial.print(" mV  No  ");
    Serial.println(CellVminNo);
    Serial.print("CoolingWaterSensor ");
    Serial.print(CoolWaterSensor);
    Serial.println("°C");
    Serial.print("Allowed Chargepower ");
    Serial.print(allowedChargePower);
    Serial.print(" W  |  Allowed Dischargepower ");
    Serial.print(allowedDischargePower);
    Serial.println(" W");
    Serial.print(battAmps);
    Serial.print(" Amps  |  ");
    Serial.print(battVolts);
    Serial.println(" Volts");
#endif
}

void receive_can_kiaHyundai_64_battery(CAN_frame_t rx_frame) {
  CANstillAlive = 12;
  switch (rx_frame.MsgID) {
    case 0x3F6:
      break;
    case 0x491:
      break;
    case 0x493:
      break;
    case 0x497:
      break;
    case 0x498:
      break;
    case 0x4DD:
      break;
    case 0x4DE:
      break;
    case 0x4E2:
      break;
    case 0x542: //BMS SOC
      SOC_Display = (rx_frame.data.u8[0] / 2.00);
      break;
    case 0x594:
      SOC_BMS = (rx_frame.data.u8[5] / 2.00);
      break;
    case 0x595:
      break;
    case 0x596:
      aux12volt = (rx_frame.data.u8[1] / 10.00);
      tempSensH = (rx_frame.data.u8[7]);
      break;
    case 0x597:
      break;
    case 0x598:
      break;
    case 0x599:
      break;
    case 0x59C:
      break;
    case 0x59E:
      break;
    case 0x5A3:
      break;
    case 0x5D5:
      RES_WS0 = rx_frame.data.u8[3]; // water sensor
      break;
    case 0x5D6:
      currentSensor = (((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[2]) / 10.0); //strömsensor
      break;
    case 0x5D7:
      break;
    case 0x5D8:
      break;
case 0x7EC:  //Data From polled PID group
      switch (rx_frame.data.u8[0])
      {
        case 0x10:  //"PID Header"
         if (rx_frame.data.u8[4] == poll_data_pid){
           ESP32Can.CANWriteFrame(&KIA64_7E4_ack);  //Send ack to BMS if the same frame is sent as polled
         }
      break;
        case 0x21:  //First datarow in PID group
         if (poll_data_pid == 1){
           allowedChargePower = ((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]);
           allowedDischargePower = ((rx_frame.data.u8[5] << 8) + rx_frame.data.u8[6]);
         }
         if (poll_data_pid == 6){
           CoolWaterSensor = rx_frame.data.u8[2];
         }
      break;
        case 0x22:  //Second datarow in PID group
         if (poll_data_pid == 1){
           battAmps = (((rx_frame.data.u8[1] << 8) + rx_frame.data.u8[2]) / 10.0);
           battVolts = (((rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4]) / 10.0);
         }
      break;
        case 0x23:  //Third datarow in PID group
         if (poll_data_pid == 1){
         CellVoltMax = (rx_frame.data.u8[7] * 20);  //(volts *50) *20 =mV
         }
      break;
        case 0x24:  //Fourth datarow in PID group
         if (poll_data_pid == 1){
         CellVmaxNo = rx_frame.data.u8[1];
         CellVminNo = rx_frame.data.u8[3];
         CellVoltMin = (rx_frame.data.u8[2] * 20);  //(volts *50) *20 =mV
         }
         if (poll_data_pid == 5){
         battSOH = ((rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3]);
         }
      break;
        case 0x25:  //Fifth datarow in PID group
      break;
        case 0x26:  //Sixth datarow in PID group
      break;
        case 0x27:  //Seventh datarow in PID group
      break;
        case 0x28:  //Eighth datarow in PID group
      break;
      }
  default:
  break;
  }
}
void send_can_kiaHyundai_64_battery() {
  unsigned long currentMillis = millis();
  //Send 100ms message
  if (currentMillis - previousMillis100 >= interval100) {
    previousMillis100 = currentMillis;
    if (poll_data_pid >= 6){
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
  }
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= interval10s) {
    previousMillis10s = currentMillis;

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
