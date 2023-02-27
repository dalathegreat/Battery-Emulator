#include <Arduino.h>
#include "HardwareSerial.h"
#include "config.h"
#include "logging.h"
#include "mbServerFCs.h"
#include "ModbusServerRTU.h"
#include "ESP32CAN.h"
#include "CAN_config.h"

//Battery size
#define BATTERY_WH_MAX 30000

//CAN parameters
CAN_device_t CAN_cfg; // CAN Config
unsigned long previousMillis10 = 0; // will store last time a 10ms CAN Message was send
unsigned long previousMillis100 = 0; // will store last time a 100ms CAN Message was send
const int interval10 = 10; // interval (ms) at which send CAN Messages
const int interval100 = 100; // interval (ms) at which send CAN Messages
const int rx_queue_size = 10; // Receive Queue size
byte mprun10 = 0; //counter 0-3
byte mprun100 = 0; //counter 0-3

CAN_frame_t LEAF_1F2 = {.MsgID = 0x1F2, LEAF_1F2.FIR.B.DLC = 8, LEAF_1F2.FIR.B.FF = CAN_frame_std, LEAF_1F2.data.u8[0] = 0x64, LEAF_1F2.data.u8[1] = 0x64,LEAF_1F2.data.u8[2] = 0x32, LEAF_1F2.data.u8[3] = 0xA0,LEAF_1F2.data.u8[4] = 0x00,LEAF_1F2.data.u8[5] = 0x0A};
CAN_frame_t LEAF_50B = {.MsgID = 0x50B, LEAF_50B.FIR.B.DLC = 8, LEAF_50B.FIR.B.FF = CAN_frame_std, LEAF_50B.data.u8[0] = 0x00, LEAF_50B.data.u8[1] = 0x00,LEAF_50B.data.u8[2] = 0x06, LEAF_50B.data.u8[3] = 0xC0,LEAF_50B.data.u8[4] = 0x00,LEAF_50B.data.u8[5] = 0x00};
CAN_frame_t LEAF_50C = {.MsgID = 0x50C, LEAF_50C.FIR.B.DLC = 8, LEAF_50C.FIR.B.FF = CAN_frame_std, LEAF_50C.data.u8[0] = 0x00, LEAF_50C.data.u8[1] = 0x00,LEAF_50C.data.u8[2] = 0x00, LEAF_50C.data.u8[3] = 0x00,LEAF_50C.data.u8[4] = 0x00,LEAF_50C.data.u8[5] = 0x00};

//Nissan LEAF battery parameters from CAN
#define WH_PER_GID 77 //One GID is this amount of Watt hours
uint16_t LB_Discharge_Power_Limit = 0; //Limit in kW
uint16_t LB_MAX_POWER_FOR_CHARGER = 0; //Limit in kW
uint16_t LB_SOC = 0; //0 - 100.0 % (0-1000)
uint16_t LB_Wh_Remaining = 0; //Amount of energy in battery, in Wh
uint16_t LB_GIDS = 0;
uint16_t LB_MAX = 0;
uint16_t LB_Max_GIDS = 273; //Startup in 24kWh mode
int16_t LB_Current = 0; //Current in A going in/out of battery
uint16_t LB_Total_Voltage = 370; //Battery voltage (0-450V)

// global Modbus memory registers
const int intervalModbusTask = 4800; //Interval at which to refresh modbus registers
unsigned long previousMillisModbus = 0; //will store last time a modbus register refresh
// ModbusRTU Server
#define MB_RTU_NUM_VALUES 30000
//#define MB_RTU_DIVICE_ID 21
uint16_t mbPV[MB_RTU_NUM_VALUES];          // process variable memory: produced by sensors, etc., cyclic read by PLC via modbus TCP

uint16_t capacity_Wh_startup = BATTERY_WH_MAX;
uint16_t MaxPower = 40960; //41kW TODO, fetch from LEAF battery (or does it matter, polled after startup?)
uint16_t MaxVoltage = 4672; //(467.2V), if higher charging is not possible (goes into forced discharge)
uint16_t MinVoltage = 3200; //Min Voltage (320.0V), if lower Gen24 disables battery

uint16_t SOC = 5000; //SOC 0-100.00% //Updates later on from CAN
uint16_t capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t remaining_capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t max_target_discharge_power = 0; //0W (0W > restricts to no discharge) //Updates later on from CAN
uint16_t max_target_charge_power = 4312;
//4.3kW (during charge), both 307&308 can be set (>0) at the same time //Updates later on from CAN
uint16_t TemperatureMax = 50; //Todo, read from LEAF pack, uint not ok
uint16_t TemperatureMin = 60; //Todo, read from LEAF pack, uint not ok
uint16_t bms_char_dis_status; //0 idle, 1 discharging, 2, charging
uint16_t bms_status = 0; //ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]

// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

// Setup() - initialization happens here
void setup()
{
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

	// Init Serial monitor
	Serial.begin(9600);
	while (!Serial)
	{
	}
	Serial.println("__ OK __");

  //Set up Modbus RTU Server
  Serial.println("Set ModbusRtu PIN");
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);

  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);

  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

  // Init Static data to the RTU Modbus
  handle_static_data_modbus();

  // Init Serial2 connected to the RTU Modbus
  RTUutils::prepareHardwareSerial(Serial2);
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);

  // Register served function code worker for server
  MBserver.registerWorker(MBTCP_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(MBTCP_ID, WRITE_HOLD_REGISTER, &FC06);
  MBserver.registerWorker(MBTCP_ID, WRITE_MULT_REGISTERS, &FC16);
  MBserver.registerWorker(MBTCP_ID, R_W_MULT_REGISTERS, &FC23);

  // Start ModbusRTU background task
  MBserver.begin(Serial2);
}

// perform main program functions
void loop()
{
	handle_can();
  //every 10s
	if (millis() - previousMillisModbus >= intervalModbusTask)
	{
		previousMillisModbus = millis();
    update_values();
    handle_update_data_modbusp201();  //Updata for ModbusRTU Server for GEN24
    handle_update_data_modbusp301();  //Updata for ModbusRTU Server for GEN24
	}
}

void update_values()
{
	MaxPower = (LB_Discharge_Power_Limit * 1000); //kW to W
	SOC = (LB_SOC * 10); //increase range from 0-100.0 -> 100.00
	capacity_Wh = (LB_Max_GIDS * WH_PER_GID);
	remaining_capacity_Wh = LB_Wh_Remaining;
  if(LB_Discharge_Power_Limit > 60) //if >60kW can be pulled from battery
  {
    max_target_discharge_power = 60000; //cap value so we don't go over 16bits
  }
  else
  {
    max_target_discharge_power = (LB_Discharge_Power_Limit * 1000); //kW to W
  }
  if(LB_MAX_POWER_FOR_CHARGER > 60)
  {
    max_target_charge_power = 60000; //cap value so we don't go over 16bits
  }
  else
  {
    max_target_charge_power = (LB_MAX_POWER_FOR_CHARGER * 1000); //kW to W
  }
	TemperatureMin = 50; //hardcoded, todo, read from 5C0
	TemperatureMax = 60; //hardcoded, todo, read from 5C0
}

void handle_static_data_modbus() {
  // Store the data into the array
  static uint16_t si_data[] = { 21321, 1 };
  static uint16_t byd_data[] = { 16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint16_t battery_data[] = { 16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752, 20594, 25965, 26997, 27936, 18518, 0, 0, 0 };
  static uint16_t volt_data[] = { 13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12598, 0, 0, 0, 0, 0, 0 };
  static uint16_t serial_data[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  static uint16_t static_data[] = { 1, 0 };
  static uint16_t* data_array_pointers[] = { si_data, byd_data, battery_data, volt_data, serial_data, static_data };
  static uint16_t data_sizes[] = { sizeof(si_data), sizeof(byd_data), sizeof(battery_data), sizeof(volt_data), sizeof(serial_data), sizeof(static_data) };
  static uint16_t i = 100;
  for (uint8_t arr_idx = 0; arr_idx < sizeof(data_array_pointers) / sizeof(uint16_t*); arr_idx++) {
    uint16_t data_size = data_sizes[arr_idx];
    memcpy(&mbPV[i], data_array_pointers[arr_idx], data_size);
    i += data_size / sizeof(uint16_t);
  }
}

void handle_update_data_modbusp201() {
  // Store the data into the array
  static uint16_t system_data[13];
  system_data[0] = 0;                                                          // Id.: p201 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[1] = 0;                                                          // Id.: p202 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[2] = (capacity_Wh_startup);                                      // Id.: p203 Value.: 32000 Scaled value.: 32kWh Comment.: Capacity rated
  system_data[3] = (capacity_Wh_startup);                                      // Id.: p204 Value.: 32000 Scaled value.: 32kWh Comment.: Nominal capacity
  system_data[4] = (MaxPower);                                                 // Id.: p205 Value.: 14500 Scaled value.: 30,42kW  Comment.: Max Charge/Discharge Power=10.24kW
  system_data[5] = (MaxVoltage);                                               // Id.: p206 Value.: 3667 Scaled value.: 362,7VDC Comment.: Max Voltage, if higher charging is not possible (goes into forced discharge)
  system_data[6] = (MinVoltage);                                               // Id.: p207 Value.: 2776 Scaled value.: 277,6VDC Comment.: Min Voltage, if lower Gen24 disables battery
  system_data[7] = 53248;                                                      // Id.: p208 Value.: 53248 Scaled value.: 53248 Comment.: Always 53248 for this BYD, Peak Charge power?
  system_data[8] = 10;                                                         // Id.: p209 Value.: 10 Scaled value.: 10 Comment.: Always 10
  system_data[9] = 53248;                                                      // Id.: p210 Value.: 53248 Scaled value.: 53248 Comment.: Always 53248 for this BYD, Peak Discharge power?
  system_data[10] = 10;                                                        // Id.: p211 Value.: 10 Scaled value.: 10 Comment.: Always 10
  system_data[11] = 0;                                                         // Id.: p212 Value.: 0 Scaled value.: 0 Comment.: Always 0
  system_data[12] = 0;                                                         // Id.: p213 Value.: 0 Scaled value.: 0 Comment.: Always 0
  static uint16_t i = 200;
  memcpy(&mbPV[i], system_data, sizeof(system_data));
}

void handle_update_data_modbusp301() {
  // Store the data into the array
  static uint16_t battery_data[23];
  if (LB_Current > 0) { //Positive value = Charging on LEAF 
    bms_char_dis_status = 2; //Charging
  } else if (LB_Current < 0) { //Negative value = Discharging on LEAF
    bms_char_dis_status = 1; //Discharging
  } else { //LB_Current == 0
    bms_char_dis_status = 0; //idle
  }

  bms_status = 3; //Todo add logic here

  if (bms_status == 3) {
    battery_data[8] = LB_Total_Voltage;  // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  } else {
    battery_data[8] = 0;
  }
  battery_data[0] = bms_status;                          // Id.: p301 Value.: 3 Scaled value.: 3 Comment.: status(*): ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
  battery_data[1] = 0;                                   // Id.: p302 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[2] = 128 + bms_char_dis_status;           // Id.: p303 Value.: 130 Scaled value.: 130 Comment.: mode(*): normal
  battery_data[3] = SOC;                                 // Id.: p304 Value.: 1700 Scaled value.: 17% Comment.: soc: 32%
  battery_data[4] = capacity_Wh;                         // Id.: p305 Value.: 32000 Scaled value.: 32kWh Comment.: tot cap:
  battery_data[5] = remaining_capacity_Wh;               // Id.: p306 Value.: 13260 Scaled value.: 13,26kWh Comment.: remaining cap: 7.68kWh
  battery_data[6] = max_target_discharge_power;          // Id.: p307 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target discharge power: 0W (0W > restricts to no discharge)
  battery_data[7] = max_target_charge_power;             // Id.: p308 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target charge power: 4.3kW (during charge), both 307&308 can be set (>0) at the same time
  //Battery_data[8] set previously in function           // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  battery_data[9] = 2000;                                // Id.: p310 Value.: 64121 Scaled value.: 6412,1W Comment.: Current Power to API: if>32768... -(65535-61760)=3775W
  battery_data[10] = LB_Total_Voltage;                   // Id.: p311 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage inner: 173.2V
  battery_data[11] = 2000;                               // Id.: p312 Value.: 64121 Scaled value.: 6412,1W Comment.: p310
  battery_data[12] = TemperatureMin;                     // Id.: p313 Value.: 75 Scaled value.: 7,5 Comment.: temp min: 14 degrees (if below 0....65535-t)
  battery_data[13] = TemperatureMax;                     // Id.: p314 Value.: 95 Scaled value.: 9,5 Comment.: temp max: 15 degrees (if below 0....65535-t)
  battery_data[14] = 0;                                  // Id.: p315 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[15] = 0;                                  // Id.: p316 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[16] = 16;                                 // Id.: p317 Value.: 0 Scaled value.: 0 Comment.: counter charge hi
  battery_data[17] = 22741;                              // Id.: p318 Value.: 0 Scaled value.: 0 Comment.: counter charge lo....65536*101+9912 = 6629048 Wh?
  battery_data[18] = 0;                                  // Id.: p319 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[19] = 0;                                  // Id.: p320 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[20] = 13;                                 // Id.: p321 Value.: 0 Scaled value.: 0 Comment.: counter discharge hi
  battery_data[21] = 52064;                              // Id.: p322 Value.: 0 Scaled value.: 0 Comment.: counter discharge lo....65536*92+7448 = 6036760 Wh?
  battery_data[22] = 80;                                 // Id.: p323 Value.: 0 Scaled value.: 0 Comment.: temp? (23 degrees)
  battery_data[23] = 9900;                               // Id.: p324 Value.: 0 Scaled value.: 0 Comment.: unknown
  static uint16_t i = 300;
  memcpy(&mbPV[i], battery_data, sizeof(battery_data));
}

void handle_can()
{
  CAN_frame_t rx_frame;
  unsigned long currentMillis = millis();

  // Receive next CAN frame from queue
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    if (rx_frame.FIR.B.FF == CAN_frame_std)
    {
      //printf("New standard frame");
      switch (rx_frame.MsgID)
			{
      case 0x1DB:
				LB_Current = (rx_frame.data.u8[0] << 3) | (rx_frame.data.u8[1] & 0xe0) >> 5;
        if (LB_Current & 0x0400)
        {
          // negative so extend the sign bit
          LB_Current |= 0xf800;
        }
				LB_Total_Voltage = ((rx_frame.data.u8[2] << 2) | (rx_frame.data.u8[3] & 0xc0) >> 6) / 2;
				break;
			case 0x1DC:
				LB_Discharge_Power_Limit = ((rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6) / 4.0);
				LB_MAX_POWER_FOR_CHARGER = ((((rx_frame.data.u8[2] & 0x0F) << 6 | rx_frame.data.u8[3] >> 2) / 10.0) - 10);
				break;
			case 0x55B:
				LB_SOC = (rx_frame.data.u8[0] << 2 | rx_frame.data.u8[1] >> 6);
				break;
			case 0x5BC:
				LB_MAX = ((rx_frame.data.u8[5] & 0x10) >> 4);
				if (LB_MAX)
				{
					LB_Max_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
					//Max gids active, do nothing
					//Only the 30/40/62kWh packs have this mux
				}
				else
				{
					//Normal current GIDS value is transmitted
					LB_GIDS = (rx_frame.data.u8[0] << 2) | ((rx_frame.data.u8[1] & 0xC0) >> 6);
					LB_Wh_Remaining = (LB_GIDS * WH_PER_GID);
				}
				break;
      default:
				break;
      }      
    }
    else
    {
      //printf("New extended frame");
    }
  }
	// Send 100ms CAN Message
	if (currentMillis - previousMillis100 >= interval100)
	{
		previousMillis100 = currentMillis;

    ESP32Can.CANWriteFrame(&LEAF_50B); //Always send 50B as a static message

		mprun100++;
		if (mprun100 > 3)
		{
			mprun100 = 0;
		}

		if (mprun100 == 0)
		{
			LEAF_50C.data.u8[5] = 0x00;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0xC8;
		}
		else if(mprun100 == 1)
		{
			LEAF_50C.data.u8[5] = 0x01;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0x5F;
		}
		else if(mprun100 == 2)
		{
			LEAF_50C.data.u8[5] = 0x02;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0x63;
		}
		else if(mprun100 == 3)
		{
			LEAF_50C.data.u8[5] = 0x03;
			LEAF_50C.data.u8[6] = 0x5D;
			LEAF_50C.data.u8[7] = 0xF4;
		}
		ESP32Can.CANWriteFrame(&LEAF_50C);
	}
  //Send 10ms message
	if (currentMillis - previousMillis10 >= interval10)
	{ 
		previousMillis10 = currentMillis;

    if(mprun10 == 0)
    {
      LEAF_1F2.data.u8[6] = 0x00;
      LEAF_1F2.data.u8[7] = 0x8F;
    }
    else if(mprun10 == 1)
    {
      LEAF_1F2.data.u8[6] = 0x01;
      LEAF_1F2.data.u8[7] = 0x80;
    }
    else if(mprun10 == 2)
    {
      LEAF_1F2.data.u8[6] = 0x02;
      LEAF_1F2.data.u8[7] = 0x81;
    }
    else if(mprun10 == 3)
    {
      LEAF_1F2.data.u8[6] = 0x03;
      LEAF_1F2.data.u8[7] = 0x82;
    }
    ESP32Can.CANWriteFrame(&LEAF_1F2);

		mprun10++;
		if (mprun10 > 3)
		{
			mprun10 = 0;
		}
		//Serial.println("CAN 10ms done");
	}
}