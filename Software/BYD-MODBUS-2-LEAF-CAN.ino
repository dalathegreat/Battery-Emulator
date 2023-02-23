// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to ModbusClient
//               MIT license - see license.md for details
// =================================================================================================
// Includes: <Arduino.h> for Serial etc., WiFi.h for WiFi support
#include <Arduino.h>
#include "HardwareSerial.h"
#include "config.h"
#include "logging.h"
#include "mbServerFCs.h"
#include "ModbusServerRTU.h"
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

// global Modbus memory registers
uint16_t mbPV[30000]; // process variable memory: produced by sensors, etc., cyclic read by PLC via modbus TCP
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
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
uint16_t TemperatureMax = 50; //Todo, read from LEAF pack
uint16_t TemperatureMin = 60; //Todo, read from LEAF pack

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
// loop() - nothing done here today!
void loop() {
  delay(10000);
    //Print value of holting register 40001
    Serial.println(mbPV[0]);
    //Set value of holting register 40002
    
  for (i = 0; i < (sizeof(p101_data) / sizeof(p101_data[0])); i++) {
    mbPV[i] = p101_data[i];
  }
}
