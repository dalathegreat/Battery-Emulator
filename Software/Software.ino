/* Select battery used */
#define BATTERY_TYPE_LEAF         // See NISSAN-LEAF-BATTERY.h for more LEAF battery settings
//#define TESLA_MODEL_3_BATTERY   // See TESLA-MODEL-3-BATTERY.h for more Tesla battery settings
//#define RENAULT_ZOE_BATTERY     // See RENAULT-ZOE-BATTERY.h for more Zoe battery settings
//#define BMW_I3_BATTERY          // See BMW-I3-BATTERY.h for more i3 battery settings
//#define IMIEV_ION_CZERO_BATTERY // See IMIEV-CZERO-ION-BATTERY.h for more triplet battery settings
//#define KIA_HYUNDAI_64_BATTERY  // See KIA-HYUNDAI-64-BATTERY.h for more battery settings
//#define CHADEMO                 // See CHADEMO.h for more Chademo related settings

/* Select inverter communication protocol. See Wiki for which to use with your inverter: https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki */
#define MODBUS_BYD          //Enable this line to emulate a "BYD 11kWh HVM battery" over Modbus RTU
//#define MODBUS_LUNA2000   //Enable this line to emulate a "Luna2000 battery" over Modbus RTU
//#define CAN_BYD           //Enable this line to emulate a "BYD Battery-Box Premium HVS" over CAN Bus
//#define SOLAX_CAN         //Enable this line to emulate a "SolaX Triple Power LFP" over CAN bus
//#define PYLON_CAN		      //Enable this line to emulate a "Pylontech battery" over CAN bus

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
CAN_device_t CAN_cfg; // CAN Config
const int rx_queue_size = 10; // Receive Queue size

//Interval settings
const int intervalInverterTask = 4800; //Interval at which to refresh modbus registers / inverter values
const int interval10 = 10; //Interval for 10ms tasks
unsigned long previousMillis10ms = 50;
unsigned long previousMillisInverter = 0; 

//ModbusRTU parameters
#if defined(MODBUS_BYD) || defined(MODBUS_LUNA2000)
#define MB_RTU_NUM_VALUES 30000
uint16_t mbPV[MB_RTU_NUM_VALUES];  // process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
#endif

//Inverter states
#define STANDBY 0
#define INACTIVE 1
#define DARKSTART 2
#define ACTIVE 3
#define FAULT 4
#define UPDATING 5
//Common inverter parameters
uint16_t capacity_Wh_startup = BATTERY_WH_MAX;
uint16_t max_power = 40960; //41kW 
const uint16_t max_voltage = ABSOLUTE_MAX_VOLTAGE; //if higher charging is not possible (goes into forced discharge)
const uint16_t min_voltage = ABSOLUTE_MIN_VOLTAGE; //if lower Gen24 disables battery
uint16_t min_volt_byd_can = min_voltage;
uint16_t max_volt_byd_can = max_voltage;
uint16_t min_volt_solax_can = min_voltage;
uint16_t max_volt_solax_can = max_voltage;
uint16_t min_volt_pylon_can = min_voltage;
uint16_t max_volt_pylon_can = max_voltage;
uint16_t min_volt_modbus_byd = min_voltage;
uint16_t max_volt_modbus_byd = max_voltage;
uint16_t battery_voltage = 3700;
uint16_t battery_current = 0;
uint16_t SOC = 5000; //SOC 0-100.00% //Updates later on from CAN
uint16_t StateOfHealth = 9900; //SOH 0-100.00% //Updates later on from CAN
uint16_t capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t remaining_capacity_Wh = BATTERY_WH_MAX; //Updates later on from CAN
uint16_t max_target_discharge_power = 0; //0W (0W > restricts to no discharge) //Updates later on from CAN
uint16_t max_target_charge_power = 4312; //4.3kW (during charge), both 307&308 can be set (>0) at the same time //Updates later on from CAN. Max value is 30000W
uint16_t temperature_max = 50; //reads from battery later
uint16_t temperature_min = 60; //reads from battery later
uint16_t bms_char_dis_status; //0 idle, 1 discharging, 2, charging
uint16_t bms_status = ACTIVE; //ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
uint16_t stat_batt_power = 0; //power going in/out of battery

// LED control
#define GREEN 0
#define YELLOW 1
#define RED 2
#define BLUE 3

Adafruit_NeoPixel pixels(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
static uint8_t brightness = 0;
static bool rampUp = true;
const uint8_t maxBrightness = 100;
uint8_t LEDcolor = GREEN;

//Contactor parameters
enum State {
  WAITING_FOR_BATTERY,
  PRECHARGE,
  NEGATIVE,
  POSITIVE,
  PRECHARGE_OFF,
  COMPLETED,
  SHUTDOWN_REQUESTED
};
State contactorStatus = WAITING_FOR_BATTERY;
#define PRECHARGE_TIME_MS 160
#define NEGATIVE_CONTACTOR_TIME_MS 1000
#define POSITIVE_CONTACTOR_TIME_MS 2000
unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long timeSpentInFaultedMode = 0;
uint8_t batteryAllowsContactorClosing = 0;
uint8_t inverterAllowsContactorClosing = 1; //Startup with always allowing closing from inverter side. Only a few inverters disallow it

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

  #ifdef MODBUS_BYD
  // Init Static data to the RTU Modbus
  handle_static_data_modbus_byd();
  #endif
  #if defined(MODBUS_BYD) || defined(MODBUS_LUNA2000)
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
  #endif

  // Init LED control
  pixels.begin();

  //Inverter Setup
  #ifdef SOLAX_CAN
  inverterAllowsContactorClosing = 0; //The inverter needs to allow first!
  Serial.println("SOLAX CAN protocol selected");
  #endif
  #ifdef MODBUS_BYD
  Serial.println("BYD Modbus RTU protocol selected");
  #endif
  #ifdef MODBUS_LUNA2000
  Serial.println("Luna2000 Modbus RTU protocol selected");
  #endif
  #ifdef CAN_BYD
  Serial.println("BYD CAN protocol selected");
  #endif
  //Inform user what setup is used
  #ifdef BATTERY_TYPE_LEAF
  Serial.println("Nissan LEAF battery selected");
  #endif 
  #ifdef TESLA_MODEL_3_BATTERY
  Serial.println("Tesla Model 3 battery selected");
  #endif 
  #ifdef RENAULT_ZOE_BATTERY
  Serial.println("Renault Zoe / Kangoo battery selected");
  #endif
  #ifdef BMW_I3_BATTERY
  Serial.println("BMW i3 battery selected");
  #endif
  #ifdef IMIEV_ION_CZERO_BATTERY
  Serial.println("Mitsubishi i-MiEV / Citroen C-Zero / Peugeot Ion battery selected");
  #endif
  #ifdef KIA_HYUNDAI_64_BATTERY
  Serial.println("Kia Niro / Hyundai Kona 64kWh battery selected");
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

	if (millis() - previousMillisInverter >= intervalInverterTask) //every 5s
	{
		previousMillisInverter = millis();
    handle_inverter(); //Update values heading towards inverter
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
      #ifdef BMW_I3_BATTERY
      receive_can_i3_battery(rx_frame);
      #endif
      #ifdef IMIEV_ION_CZERO_BATTERY
      receive_can_imiev_battery(rx_frame);
      #endif
      #ifdef KIA_HYUNDAI_64_BATTERY
      receive_can_kiaHyundai_64_battery(rx_frame);
      #endif
      #ifdef CAN_BYD
      receive_can_byd(rx_frame);
      #endif
	    #ifdef CHADEMO
      receive_can_chademo(rx_frame);
      #endif

    }
    else
    {
      //printf("New extended frame");
      #ifdef SOLAX_CAN
      receive_can_solax(rx_frame);
      #endif
	  #ifdef PYLON_CAN
	  receive_can_pylon(rx_frame);
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
  #ifdef BMW_I3_BATTERY
  send_can_i3_battery();
  #endif
  #ifdef IMIEV_ION_CZERO_BATTERY
  send_can_imiev_battery();
  #endif
  #ifdef KIA_HYUNDAI_64_BATTERY 
  send_can_kiaHyundai_64_battery();
  #endif
  #ifdef CHADEMO
  send_can_chademo_battery();
  #endif
}

void handle_inverter()
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
    #ifdef BMW_I3_BATTERY
    update_values_i3_battery(); //Map the values to the correct registers
    #endif
    #ifdef IMIEV_ION_CZERO_BATTERY
    update_values_imiev_battery(); //Map the values to the correct registers
    #endif
    #ifdef KIA_HYUNDAI_64_BATTERY
    update_values_kiaHyundai_64_battery(); //Map the values to the correct registers
    #endif
    #ifdef SOLAX_CAN
    update_values_can_solax();
    #endif
    #ifdef CAN_BYD
    update_values_can_byd();
    #endif
    #ifdef PYLON_CAN
    update_values_can_pylon();
    #endif
	  #ifdef CHADEMO
    update_values_can_chademo();
    #endif

    #ifdef MODBUS_BYD
    update_modbus_registers_byd();
    #endif
    #ifdef MODBUS_LUNA2000
    update_modbus_registers_luna2000();
    #endif
}

void handle_contactors()
{
  //First check if we have any active errors, incase we do, turn off the battery after 15 seconds
  if(bms_status == FAULT)
  {
    timeSpentInFaultedMode++;
  }
  if(timeSpentInFaultedMode > 1500)
  {
    contactorStatus = SHUTDOWN_REQUESTED;
  }
  if(contactorStatus == SHUTDOWN_REQUESTED)
  {
    digitalWrite(PRECHARGE_PIN, LOW);
    digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
    digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
    return;
  }

  //After that, check if we are OK to start turning on the battery
  if(contactorStatus == WAITING_FOR_BATTERY)
  {
    if(batteryAllowsContactorClosing && inverterAllowsContactorClosing)
    {
      contactorStatus = PRECHARGE;
    }
  }

  if(contactorStatus == COMPLETED)
  { //Skip running the state machine below if it has already completed
    return;
  }

  unsigned long currentTime = millis();
  //Handle actual state machine. This first turns on Precharge, then Negative, then Positive, and finally turns OFF precharge
  switch (contactorStatus) {
    case PRECHARGE:
      digitalWrite(PRECHARGE_PIN, HIGH);
      prechargeStartTime = currentTime;
      contactorStatus = NEGATIVE;
      break;

    case NEGATIVE:
      if (currentTime - prechargeStartTime >= PRECHARGE_TIME_MS) {
        digitalWrite(NEGATIVE_CONTACTOR_PIN, HIGH);
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= NEGATIVE_CONTACTOR_TIME_MS) {
        digitalWrite(POSITIVE_CONTACTOR_PIN, HIGH);
        contactorStatus = PRECHARGE_OFF;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - negativeStartTime >= POSITIVE_CONTACTOR_TIME_MS) {
        digitalWrite(PRECHARGE_PIN, LOW);
        contactorStatus = COMPLETED;
      }
      break;
    default:
    break;
  }
}

void handle_LED_state()
{
  // Determine how bright the LED should be
  if (rampUp && brightness < maxBrightness){
    brightness++;
  } 
  else if (rampUp && brightness == maxBrightness){
    rampUp = false;
  } 
  else if (!rampUp && brightness > 0){
    brightness--;
  } 
  else if (!rampUp && brightness == 0){
    rampUp = true;
  }
  switch (LEDcolor)
  {
    case GREEN:
    pixels.setPixelColor(0, pixels.Color(0, brightness, 0)); // Green pulsing LED
    break;
    case YELLOW:
    pixels.setPixelColor(0, pixels.Color(brightness, brightness, 0)); // Yellow pulsing LED
    break;
    case BLUE:
    pixels.setPixelColor(0, pixels.Color(0, 0, brightness)); //Blue pulsing LED
    break;
    case RED:
    pixels.setPixelColor(0, pixels.Color(150, 0, 0)); // Red LED full brightness
    break;
    default:
    break;
  }

  //BMS in fault state overrides everything
  if(bms_status == FAULT)
  {
    pixels.setPixelColor(0, pixels.Color(255, 0, 0)); // Red LED full brightness
  }

  pixels.show(); // This sends the updated pixel color to the hardware.
}
