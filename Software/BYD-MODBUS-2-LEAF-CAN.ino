#include <Arduino.h>
#include "HardwareSerial.h"
#include "config.h"
#include "logging.h"
#include "mbServerFCs.h"
#include "ModbusServerRTU.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

//CAN parameters
CAN_device_t CAN_cfg;             // CAN Config
unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
const int interval10 = 10;      // interval (ms) at which send CAN Messages
const int interval100 = 100;    // interval (ms) at which send CAN Messages
const int rx_queue_size = 10;     // Receive Queue size
byte mprun10 = 0;                 //counter 0-3
byte mprun100 = 0;                 //counter 0-3

//Nissan LEAF battery parameters from CAN
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

// global Modbus memory registers
const int intervalModbusTask = 10000; //Interval at which to refresh modbus registers
unsigned long previousMillisModbus = 0; //will store last time a modbus register refresh
uint16_t mbPV[30000]; // process variable memory: produced by sensors, etc., cyclic read by PLC via modbus TCP
uint16_t capacity_kWh_startup = 30000; //30kWh
uint16_t MaxPower = 40960; //41kW TODO, fetch from LEAF battery (or does it matter, polled after startup?)
uint16_t MaxVoltage = 4672; //(467.2V), if higher charging is not possible (goes into forced discharge)
uint16_t MinVoltage = 3200; //Min Voltage (320.0V), if lower Gen24 disables battery
uint16_t Status = 3; //ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
uint16_t SOC = 5000; //SOC 0-100.00% TODO, fetch from LEAF battery
uint16_t capacity_kWh = 30000; //30kWh TODO, fetch from LEAF battery
uint16_t remaining_capacity_kWh = 30000; //TODO, fetch from LEAF battery 59E
uint16_t max_target_discharge_power = 0; //0W (0W > restricts to no discharge)
uint16_t max_target_charge_power = 4312; //4.3kW (during charge), both 307&308 can be set (>0) at the same time
uint16_t TemperatureMax = 50; //Todo, read from LEAF pack, uint not ok
uint16_t TemperatureMin = 60; //Todo, read from LEAF pack, uint not ok

// Store the data into the array
//uint16_t p101_data[] = {21321, 1, 16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752, 20594, 25965, 26997, 27936, 18518, 0, 0, 0, 13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12598, 0, 0, 0, 0, 0, 0, 20581, 27756, 25856, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0};
uint16_t p101_data[] = {8373, 1}; //SI
uint16_t p103_data[] = {6689, 6832, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; //BY D
uint16_t p119_data[] = {6689, 6832, 6697, 116116, 101114, 12145, 66111, 12032, 80114, 101109, 105117, 10932, 7286, 0, 0, 0}; //BY D  Ba tt er y- Bo x  Pr em iu m  HV
uint16_t p135_data[] = {5346, 48, 0, 0, 0, 0, 0, 0, 5146, 4954, 0, 0, 0, 0, 0, 0}; //5.0 3.16
uint16_t p151_data[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t p167_data[] = {1, 0};
uint16_t p201_data[] = {0, 0, capacity_kWh_startup, MaxPower, MaxPower, MaxVoltage, MinVoltage, 53248, 10, 53248, 10, 0, 0};
uint16_t p301_data[] = {Status, 0, 128, SOC, capacity_kWh, remaining_capacity_kWh, max_target_discharge_power, max_target_charge_power, 0, 0, 2058, 0, TemperatureMin, TemperatureMax, 0, 0, 16, 22741, 0, 0, 13, 52064, 80, 9900};
uint16_t i;
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

// Setup() - initialization happens here
void setup() {
  //CAN pins
  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);
  CAN_cfg.speed = CAN_SPEED_500KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_27;
  CAN_cfg.rx_pin_id = GPIO_NUM_26;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();
  Serial.println(CAN_cfg.speed);

  //Modbus pins
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);

  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);

  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);
  // Init Serial monitor
  Serial.begin(9600);
  while (!Serial) {}
  Serial.println("__ OK __");

  // Init Serial2 connected to the RTU Modbus
  // (Fill in your data here!)
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN); 
  // Register served function code worker for server id 21, FC 0x03
  MBserver.registerWorker(MBTCP_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(MBTCP_ID, WRITE_HOLD_REGISTER, &FC06);
  MBserver.registerWorker(MBTCP_ID, WRITE_MULT_REGISTERS, &FC16);
  MBserver.registerWorker(MBTCP_ID, R_W_MULT_REGISTERS, &FC23);
  // Start ModbusRTU background task
  MBserver.start();
}

// loop() - nothing done here today!
void loop() {
  handle_modbus();
  handle_can();
}

void handle_modbus(){
  static unsigned long currentMillis = millis();
  if (currentMillis - previousMillisModbus >= intervalModbusTask) 
  { //every 10s
    previousMillisModbus = currentMillis;

    //Print value of holting register 40001
    Serial.println(mbPV[0]);
    //Set value of holting register 40002
    
    for (i = 0; i < (sizeof(p101_data) / sizeof(p101_data[0])); i++) {
      mbPV[i] = p101_data[i];
    }
  }
}

void handle_can() {
  CAN_frame_t rx_frame;

  static unsigned long currentMillis = millis();

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