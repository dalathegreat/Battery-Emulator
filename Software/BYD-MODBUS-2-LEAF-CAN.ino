//This software converts the LEAF CAN into Modbus RTU registers understood by solar inverters that take the BYD 11kWh HVM battery
//This code runs on the LilyGo ESP32 T-CAN485 devboard

#include <Arduino.h>
#include "config.h"
#include <HardwareSerial.h>
#include "ESP32CAN.h"
#include "CAN_config.h"

//CAN sending parameters
CAN_device_t CAN_cfg;             // CAN Config
unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
const int interval10 = 10;      // interval (ms) at which send CAN Messages
const int interval100 = 100;    // interval (ms) at which send CAN Messages
const int rx_queue_size = 10;     // Receive Queue size
byte mprun10 = 0;                 //counter 0-3
byte mprun100 = 0;                 //counter 0-3

//Nissan LEAF battery parameters
#define WH_PER_GID 77 //One GID is this amount of Watt hours
int LB_Discharge_Power_Limit = 0; //Limit in kW
int LB_MAX_POWER_FOR_CHARGER = 0; //Limit in kW
int LB_SOC = 0; //0 - 100.0 % (0-1000)
int LB_kWh = 0; //Amount of energy in battery, in kWh
long LB_Wh = 0; //Amount of energy in battery, in Wh
int LB_GIDS = 0;
int LB_max_gids = 0;
int LB_Current = 0; //Current in kW going in/out of battery
int LB_Total_Voltage = 0; //Battery voltage (0-450V)


void setup()
{
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);

  Serial.begin(115200);

  //SD_test(); //SD card
  Serial.println("Startup in progress...");
  CAN_cfg.speed = CAN_SPEED_500KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_27;
  CAN_cfg.rx_pin_id = GPIO_NUM_26;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();

  Serial.print("CAN SPEED :");
  Serial.println(CAN_cfg.speed);

  // put your setup code here, to run once:
}

void loop()
{
  CAN_frame_t rx_frame;

  unsigned long currentMillis = millis();

  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {

    if (rx_frame.FIR.B.FF == CAN_frame_std)
    {
      //printf("New standard frame");
      switch (rx_frame.MsgID) {
        case 0x1DB:
          LB_Current = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
          LB_Total_Voltage = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6) / 2;
        break;
        case 0x1DC:
          LB_Discharge_Power_Limit = ( ( rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6 ) / 4.0 );
          LB_MAX_POWER_FOR_CHARGER = ( ( ( (rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2 ) / 10.0 ) - 10); //check if -10 is correct offset
        break;
        case 0x55B:
          LB_SOC = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
        break;
        case 0x5BC:
          LB_max_gids = ((rx_frame.data.u8[5] & 0x10) >> 4);
          if(LB_max_gids)
          {
            //Max gids active, do nothing
            //Only the 30/40/62kWh packs have this mux
          }
          else
          { //Normal current GIDS value is transmitted
            LB_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
            LB_Wh = (LB_GIDS * WH_PER_GID);
            LB_kWh = ((LB_Wh) / 1000);
          }
          break;
        case 0x59E: //This message is only present on 2013+ AZE0 and upwards
          break;
        case 0x5C0:
          break;
        default:
          break;
      }
    }
    else
    {
      //printf("New extended frame");
    }

    // if (rx_frame.FIR.B.RTR == CAN_RTR)
    // {
    //   printf(" RTR from 0x%08X, DLC %d\r\n", rx_frame.MsgID, rx_frame.FIR.B.DLC);
    // }
    // else
    // {
    //   printf(" from 0x%08X, DLC %d, Data ", rx_frame.MsgID, rx_frame.FIR.B.DLC);
    //   for (int i = 0; i < rx_frame.FIR.B.DLC; i++)
    //   {
    //     printf("0x%02X ", rx_frame.data.u8[i]);
    //   }
    //   printf("\n");
    // }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= interval100)
  {
    previousMillis100 = currentMillis;
    mprun100++;
    if(mprun100 > 3)
    {
      mprun100 = 0;
    }

    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = 0x50B;
    tx_frame.FIR.B.DLC = 8;
    tx_frame.data.u8[0] = 0x00;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x06;
    tx_frame.data.u8[3] = 0xC0; //HCM_WakeUpSleepCmd = Wakeup
    tx_frame.data.u8[4] = 0x00;
    tx_frame.data.u8[5] = 0x00;
    tx_frame.data.u8[6] = 0x00;
    tx_frame.data.u8[7] = 0x00;

    ESP32Can.CANWriteFrame(&tx_frame);
    Serial.println("CAN send 50B done");

    tx_frame.MsgID = 0x50C;
    tx_frame.FIR.B.DLC = 8;
    tx_frame.data.u8[0] = 0x00;
    tx_frame.data.u8[1] = 0x00;
    tx_frame.data.u8[2] = 0x00;
    tx_frame.data.u8[3] = 0x00;
    tx_frame.data.u8[4] = 0x00;
    if(mprun100 == 0)
    {
      tx_frame.data.u8[5] = 0x00;
      tx_frame.data.u8[6] = 0x5D;
      tx_frame.data.u8[7] = 0xC8;
    }
    if(mprun100 == 1)
    {
      tx_frame.data.u8[5] = 0x01;
      tx_frame.data.u8[6] = 0x5D;
      tx_frame.data.u8[7] = 0x5F;
    }
    if(mprun100 == 2)
    {
      tx_frame.data.u8[5] = 0x02;
      tx_frame.data.u8[6] = 0x5D;
      tx_frame.data.u8[7] = 0x63;
    }
    if(mprun100 == 3)
    {
      tx_frame.data.u8[5] = 0x03;
      tx_frame.data.u8[6] = 0x5D;
      tx_frame.data.u8[7] = 0xF4;
    }
    ESP32Can.CANWriteFrame(&tx_frame);
    Serial.println("CAN send 50C done");
  }
  
  if (currentMillis - previousMillis10 >= interval10)
  {
    previousMillis10 = currentMillis;
    mprun10++;
    if(mprun10 > 3)
    {
      mprun10 = 0;
    }

    CAN_frame_t tx_frame;
    tx_frame.FIR.B.FF = CAN_frame_std;
    tx_frame.MsgID = 0x1F2;
    tx_frame.FIR.B.DLC = 8;
    tx_frame.data.u8[0] = 0x64;
    tx_frame.data.u8[1] = 0x64;
    tx_frame.data.u8[2] = 0x32;
    tx_frame.data.u8[3] = 0xA0; 
    tx_frame.data.u8[4] = 0x00;
    tx_frame.data.u8[5] = 0x0A;
    if(mprun10 == 0)
    {
    tx_frame.data.u8[6] = 0x00;
    tx_frame.data.u8[7] = 0x8F;
    }
    if(mprun10 == 1)
    {
    tx_frame.data.u8[6] = 0x01;
    tx_frame.data.u8[7] = 0x80;
    }
    if(mprun10 == 2)
    {
    tx_frame.data.u8[6] = 0x02;
    tx_frame.data.u8[7] = 0x81;
    }
    if(mprun10 == 3)
    {
    tx_frame.data.u8[6] = 0x03;
    tx_frame.data.u8[7] = 0x82;
    }
    ESP32Can.CANWriteFrame(&tx_frame);
    Serial.println("CAN send 1F2 done");
  }
}
