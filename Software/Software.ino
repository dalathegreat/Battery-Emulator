/* Select battery used */
#define BATTERY_TYPE_LEAF         // See NISSAN-LEAF-BATTERY.h for more LEAF battery settings
//#define TESLA_MODEL_3_BATTERY   // See TESLA-MODEL-3-BATTERY.h for more Tesla battery settings
//#define RENAULT_ZOE_BATTERY     // See RENAULT-ZOE-BATTERY.h for more Zoe battery settings

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
//#define MODBUS_BYD      //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define CAN_BYD       //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
#define SOLAX_CAN       //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus

/* Do not change any code below this line unless you are sure what you are doing */
/* Only change battery specific settings and limits in their respective .h files */

#include <Arduino.h>
#include "HardwareSerial.h"
#include "config.h"
#include "Logging.h"
#include "mbServerFCs.h"
#include "ModbusServerRTU.h"
#include "ESP32CAN.h"
#include "CAN_config.h"
#include "Adafruit_NeoPixel.h"
#include "BATTERIES.h"
#include "INVERTERS.h"
//CAN parameters
#define MAX_CAN_FAILURES 5000 //Amount of malformed CAN messages to allow before raising a warning
CAN_device_t CAN_cfg; // CAN Config
const int rx_queue_size = 10; // Receive Queue size

//Interval settings
const int intervalModbusTask = 4800; //Interval at which to refresh modbus registers
const int interval10 = 10;

//ModbusRTU parameters
unsigned long previousMillisModbus = 0; //will store last time a modbus register refresh
#define MB_RTU_NUM_VALUES 30000
uint16_t mbPV[MB_RTU_NUM_VALUES];          // process variable memory: produced by sensors, etc., cyclic read by PLC via modbus TCP

//Gen24 parameters
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5
uint16_t capacity_Wh_startup = BATTERY_WH_MAX;
uint16_t max_power = 40960; //41kW 
const uint16_t max_voltage = ABSOLUTE_MAX_VOLTAGE; //if higher charging is not possible (goes into forced discharge)
const uint16_t min_voltage = ABSOLUTE_MIN_VOLTAGE; //if lower Gen24 disables battery
uint16_t min_volt_byd_can = min_voltage;
uint16_t max_volt_byd_can = max_voltage;
uint16_t min_volt_solax_can = min_voltage;
uint16_t max_volt_solax_can = max_voltage;
uint16_t battery_voltage = 3700;
uint16_t battery_current = 0;
uint16_t SOC = 5000; //SOC 0-100.00% //Updates later on from CAN
uint16_t StateOfHealth = 9900; //SOH 0-100.00% //Updates later on from CAN
uint16_t capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t remaining_capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t max_target_discharge_power = 0; //0W (0W > restricts to no discharge) //Updates later on from CAN
uint16_t max_target_charge_power = 4312; //4.3kW (during charge), both 307&308 can be set (>0) at the same time //Updates later on from CAN
uint16_t temperature_max = 50; //reads from battery later
uint16_t temperature_min = 60; //reads from battery later
uint16_t bms_char_dis_status; //0 idle, 1 discharging, 2, charging
uint16_t bms_status = ACTIVE; //ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
uint16_t stat_batt_power = 0; //power going in/out of battery

// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);

// LED control
Adafruit_NeoPixel pixels(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
unsigned long previousMillis10ms = 0;
static int green = 0;
static bool rampUp = true;
const int maxBrightness = 255;

//Contactor parameters
enum State {
  PRECHARGE,
  NEGATIVE,
  POSITIVE,
  PRECHARGE_OFF,
  COMPLETED,
  SHUTDOWN
};
State contactorStatus = PRECHARGE;
unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;

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

  //Init contactor pins
  pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
	digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
  pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
	digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
  pinMode(PRECHARGE_PIN, OUTPUT);
	digitalWrite(PRECHARGE_PIN, LOW);

	// Init Serial monitor
	Serial.begin(9600);
	while (!Serial)
	{
	}
	Serial.println("__ OK __");

  //Set up Modbus RTU Server
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

  // Init LED control
  pixels.begin();

  //Inform user what setup is used
  #ifdef BATTERY_TYPE_LEAF
  Serial.println("Nissan LEAF battery selected");
  #endif 
  #ifdef TESLA_MODEL_3_BATTERY
  Serial.println("Tesla Model 3 battery selected");
  #endif 
  #ifdef RENAULT_ZOE_BATTERY
  Serial.println("Renault Zoe battery selected");
  #endif 
}

// perform main program functions
void loop()
{
  handle_can(); //runs as fast as possible, handle CAN routines
  
  if (millis() - previousMillis10ms >= interval10) //every 10ms
	{ 
		previousMillis10ms = millis();
    handle_LED_state();   //Set the LED color according to state
    handle_contactors();  //Take care of startup precharge/contactor closing
  }

	if (millis() - previousMillisModbus >= intervalModbusTask) //every 5s
	{
		previousMillisModbus = millis();
    handle_modbus(); //Update values heading towards modbus
	}
}

void handle_can()
{ //This section checks if we have a complete CAN message incoming
  //Depending on which battery/inverter is selected, we forward this to their respective CAN routines
  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE)
  {
    if (rx_frame.FIR.B.FF == CAN_frame_std)
    {
      //printf("New standard frame");
      #ifdef BATTERY_TYPE_LEAF
      receive_can_leaf_battery(rx_frame);
      #endif 
      #ifdef TESLA_MODEL_3_BATTERY
      receive_can_tesla_model_3_battery(rx_frame); 
      #endif
      #ifdef RENAULT_ZOE_BATTERY
      receive_can_zoe_battery(rx_frame);
      #endif
      #ifdef CAN_BYD
      receive_can_byd(rx_frame);
      #endif
    }
    else
    {
      //printf("New extended frame");
      #ifdef SOLAX_CAN
      receive_can_solax(rx_frame);
      #endif
    }
  }
  //When we are done checking if a CAN message has arrived, we can focus on sending CAN messages
  //Inverter sending
  #ifdef CAN_BYD
  send_can_byd();
  #endif
  #ifdef SOLAX_CAN
  send_can_solax();
  #endif
  //Battery sending
  #ifdef BATTERY_TYPE_LEAF
  send_can_leaf_battery();
	#endif 
  #ifdef TESLA_MODEL_3_BATTERY
  send_can_tesla_model_3_battery(); 
  #endif
  #ifdef RENAULT_ZOE_BATTERY
  send_can_zoe_battery();
  #endif
}

void handle_modbus()
{
	  #ifdef BATTERY_TYPE_LEAF
    update_values_leaf_battery(); //Map the values to the correct registers
	  #endif 
    #ifdef TESLA_MODEL_3_BATTERY
    update_values_tesla_model_3_battery(); //Map the values to the correct registers
    #endif
    #ifdef RENAULT_ZOE_BATTERY
    update_values_zoe_battery(); //Map the values to the correct registers
    #endif
    handle_update_data_modbusp201();  //Updata for ModbusRTU Server for GEN24
    handle_update_data_modbusp301();  //Updata for ModbusRTU Server for GEN24
}

void handle_contactors()
{
  if(contactorStatus == SHUTDOWN)
  {
    digitalWrite(PRECHARGE_PIN, LOW);
    digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
    digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
    return;
  }

  if(contactorStatus == COMPLETED)
  {
    return;
  }

  unsigned long currentTime = millis();
  
  switch (contactorStatus) {
    case PRECHARGE:
      digitalWrite(PRECHARGE_PIN, HIGH);
      prechargeStartTime = currentTime;
      contactorStatus = NEGATIVE;
      break;

    case NEGATIVE:
      if (currentTime - prechargeStartTime >= 60) {
        digitalWrite(NEGATIVE_CONTACTOR_PIN, HIGH);
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= 100) {
        digitalWrite(POSITIVE_CONTACTOR_PIN, HIGH);
        contactorStatus = PRECHARGE_OFF;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - negativeStartTime >= 300) {
        digitalWrite(PRECHARGE_PIN, LOW);
        contactorStatus = COMPLETED;
      }
      break;
    default:
    break;
  }
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
  system_data[2] = (capacity_Wh_startup);                                      // Id.: p203 Value.: 32000 Scaled value.: 32kWh Comment.: Capacity rated, maximum value is 60000 (60kWh)
  system_data[3] = (max_power);                                                // Id.: p204 Value.: 32000 Scaled value.: 32kWh Comment.: Nominal capacity
  system_data[4] = (max_power);                                                // Id.: p205 Value.: 14500 Scaled value.: 30,42kW  Comment.: Max Charge/Discharge Power=10.24kW (lowest value of 204 and 205 will be enforced by Gen24)
  system_data[5] = (max_voltage);                                              // Id.: p206 Value.: 3667 Scaled value.: 362,7VDC Comment.: Max Voltage, if higher charging is not possible (goes into forced discharge)
  system_data[6] = (min_voltage);                                              // Id.: p207 Value.: 2776 Scaled value.: 277,6VDC Comment.: Min Voltage, if lower Gen24 disables battery
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
  static uint16_t battery_data[24];
  if (battery_current > 0) { //Positive value = Charging
    bms_char_dis_status = 2; //Charging
  } else if (battery_current < 0) { //Negative value = Discharging
    bms_char_dis_status = 1; //Discharging
  } else { //battery_current == 0
    bms_char_dis_status = 0; //idle
  }

  if (bms_status == ACTIVE) {
    battery_data[8] = battery_voltage;  // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  } else {
    battery_data[8] = 0;
  }
  battery_data[0] = bms_status;                          // Id.: p301 Value.: 3 Scaled value.: 3 Comment.: status(*): ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
  battery_data[1] = 0;                                   // Id.: p302 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[2] = 128 + bms_char_dis_status;           // Id.: p303 Value.: 130 Scaled value.: 130 Comment.: mode(*): normal
  battery_data[3] = SOC;                                 // Id.: p304 Value.: 1700 Scaled value.: 50% Comment.: SOC: (50% would equal 5000)
  battery_data[4] = capacity_Wh;                         // Id.: p305 Value.: 32000 Scaled value.: 32kWh Comment.: tot cap:
  battery_data[5] = remaining_capacity_Wh;               // Id.: p306 Value.: 13260 Scaled value.: 13,26kWh Comment.: remaining cap: 7.68kWh
  battery_data[6] = max_target_discharge_power;          // Id.: p307 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target discharge power: 0W (0W > restricts to no discharge)
  battery_data[7] = max_target_charge_power;             // Id.: p308 Value.: 25604 Scaled value.: 25,604kW Comment.: max/target charge power: 4.3kW (during charge), both 307&308 can be set (>0) at the same time
  //Battery_data[8] set previously in function           // Id.: p309 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage outer (0 if status !=3, maybe a contactor closes when active): 173.4V
  battery_data[9] = 2000;                                // Id.: p310 Value.: 64121 Scaled value.: 6412,1W Comment.: Current Power to API: if>32768... -(65535-61760)=3775W
  battery_data[10] = battery_voltage;                    // Id.: p311 Value.: 3161 Scaled value.: 316,1VDC Comment.: Batt Voltage inner: 173.2V (LEAF voltage is in whole volts, need to add a decimal)
  battery_data[11] = 2000;                               // Id.: p312 Value.: 64121 Scaled value.: 6412,1W Comment.: p310
  battery_data[12] = temperature_min;                    // Id.: p313 Value.: 75 Scaled value.: 7,5 Comment.: temp min: 7 degrees (if below 0....65535-t)
  battery_data[13] = temperature_max;                    // Id.: p314 Value.: 95 Scaled value.: 9,5 Comment.: temp max: 9 degrees (if below 0....65535-t)
  battery_data[14] = 0;                                  // Id.: p315 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[15] = 0;                                  // Id.: p316 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[16] = 16;                                 // Id.: p317 Value.: 0 Scaled value.: 0 Comment.: counter charge hi
  battery_data[17] = 22741;                              // Id.: p318 Value.: 0 Scaled value.: 0 Comment.: counter charge lo....65536*101+9912 = 6629048 Wh?
  battery_data[18] = 0;                                  // Id.: p319 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[19] = 0;                                  // Id.: p320 Value.: 0 Scaled value.: 0 Comment.: always 0
  battery_data[20] = 13;                                 // Id.: p321 Value.: 0 Scaled value.: 0 Comment.: counter discharge hi
  battery_data[21] = 52064;                              // Id.: p322 Value.: 0 Scaled value.: 0 Comment.: counter discharge lo....65536*92+7448 = 6036760 Wh?
  battery_data[22] = 230;                                // Id.: p323 Value.: 0 Scaled value.: 0 Comment.: device temperature (23 degrees)
  battery_data[23] = StateOfHealth;                      // Id.: p324 Value.: 9900 Scaled value.: 99% Comment.: SOH
  static uint16_t i = 300;
  memcpy(&mbPV[i], battery_data, sizeof(battery_data));
}

void handle_LED_state()
{
  // Determine how bright the green LED should be
  if (rampUp && green < maxBrightness)
  {
    green++;
  } 
  else if (rampUp && green == maxBrightness)
  {
    rampUp = false;
  } 
  else if (!rampUp && green > 0)
  {
    green--;
  } else if (!rampUp && green == 0)
  {
    rampUp = true;
  }
  pixels.setPixelColor(0, pixels.Color(0, green, 0)); // Set LED to green according to calculated value

  if(CANerror > MAX_CAN_FAILURES)
  {
    pixels.setPixelColor(0, pixels.Color(255, 255, 0)); // Yellow LED full brightness
  }

  if(bms_status == FAULT)
  {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red LED full brightness
  }

  pixels.show(); // This sends the updated pixel color to the hardware.
}
