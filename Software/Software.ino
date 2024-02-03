/* Do not change any code below this line unless you are sure what you are doing */
/* Only change battery specific settings in "USER_SETTINGS.h" */

#include <Arduino.h>
#include <Preferences.h>
#include "HardwareSerial.h"
#include "USER_SETTINGS.h"
#include "src/battery/BATTERIES.h"
#include "src/charger/CHARGERS.h"
#include "src/devboard/config.h"
#include "src/inverter/INVERTERS.h"
#include "src/lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include "src/lib/eModbus-eModbus/Logging.h"
#include "src/lib/eModbus-eModbus/ModbusServerRTU.h"
#include "src/lib/eModbus-eModbus/scripts/mbServerFCs.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#ifdef WEBSERVER
#include "src/devboard/webserver/webserver.h"
#endif

Preferences settings;  // Store user settings

// Interval settings
int intervalUpdateValues = 4800;  // Interval at which to update inverter values / Modbus registers
const int interval10 = 10;        // Interval for 10ms tasks
unsigned long previousMillis10ms = 50;
unsigned long previousMillisUpdateVal = 0;

// CAN parameters
CAN_device_t CAN_cfg;          // CAN Config
const int rx_queue_size = 10;  // Receive Queue size

#ifdef DUAL_CAN
#include "src/lib/pierremolinaro-acan2515/ACAN2515.h"
static const uint32_t QUARTZ_FREQUENCY = 8UL * 1000UL * 1000UL;  // 8 MHz
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
static ACAN2515_Buffer16 gBuffer;
#endif

// ModbusRTU parameters
#if defined(BYD_MODBUS)
#define MB_RTU_NUM_VALUES 30000
#endif
#if defined(LUNA2000_MODBUS)
#define MB_RTU_NUM_VALUES 30000
#endif
#if defined(BYD_MODBUS) || defined(LUNA2000_MODBUS)
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
#endif

// Common inverter parameters. Batteries map their values to these variables
uint16_t max_voltage = ABSOLUTE_MAX_VOLTAGE;  // If higher charging is not possible (goes into forced discharge)
uint16_t min_voltage = ABSOLUTE_MIN_VOLTAGE;  // If lower disables discharging battery
uint16_t battery_voltage = 3700;              //V+1,  0-500.0 (0-5000)
uint16_t battery_current = 0;
uint16_t SOC = 5000;                              //SOC%, 0-100.00 (0-10000)
uint16_t StateOfHealth = 9900;                    //SOH%, 0-100.00 (0-10000)
uint16_t capacity_Wh = BATTERY_WH_MAX;            //Wh,   0-60000
uint16_t remaining_capacity_Wh = BATTERY_WH_MAX;  //Wh,   0-60000
uint16_t max_target_discharge_power = 0;          // 0W (0W > restricts to no discharge), Updates later on from CAN
uint16_t max_target_charge_power = 4312;          // Init to 4.3kW, Updates later on from CAN
uint16_t temperature_max = 50;          //C+1,  Goes thru convert2unsignedint16 function (15.0C = 150, -15.0C =  65385)
uint16_t temperature_min = 60;          // Reads from battery later
uint8_t bms_char_dis_status = STANDBY;  // 0 standby, 1 discharging, 2, charging
uint8_t bms_status = ACTIVE;            // ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
uint16_t stat_batt_power = 0;           // Power going in/out of battery
uint16_t cell_max_voltage = 3700;       // Stores the highest cell voltage value in the system
uint16_t cell_min_voltage = 3700;       // Stores the minimum cell voltage value in the system
uint16_t cellvoltages[120];             // Stores all cell voltages
uint8_t nof_cellvoltages = 0;           // Total number of cell voltages, set by each battery.
bool LFP_Chemistry = false;

// Common charger parameters
volatile float charger_setpoint_HV_VDC = 0.0f;
volatile float charger_setpoint_HV_IDC = 0.0f;
volatile float charger_setpoint_HV_IDC_END = 0.0f;
bool charger_HV_enabled = false;
bool charger_aux12V_enabled = false;

// Common charger statistics, instantaneous values
float charger_stat_HVcur = 0;
float charger_stat_HVvol = 0;
float charger_stat_ACcur = 0;
float charger_stat_ACvol = 0;
float charger_stat_LVcur = 0;
float charger_stat_LVvol = 0;

// LED parameters
Adafruit_NeoPixel pixels(1, WS2812_PIN, NEO_GRB + NEO_KHZ800);
static uint8_t brightness = 0;
static bool rampUp = true;
const uint8_t maxBrightness = 100;
uint8_t LEDcolor = GREEN;

// Contactor parameters
#ifdef CONTACTOR_CONTROL
enum State { DISCONNECTED, PRECHARGE, NEGATIVE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

#define MAX_ALLOWED_FAULT_TICKS 500
#define PRECHARGE_TIME_MS 160
#define NEGATIVE_CONTACTOR_TIME_MS 1000
#define POSITIVE_CONTACTOR_TIME_MS 2000
#ifdef PWM_CONTACTOR_CONTROL
#define PWM_Freq 20000  // 20 kHz frequency, beyond audible range
#define PWM_Res 10      // 10 Bit resolution 0 to 1023, maps 'nicely' to 0% 100%
#define PWM_Hold_Duty 250
#define POSITIVE_PWM_Ch 0
#define NEGATIVE_PWM_Ch 1
#endif
unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long timeSpentInFaultedMode = 0;
#endif
bool batteryAllowsContactorClosing = false;
bool inverterAllowsContactorClosing = true;

// Initialization
void setup() {
  init_serial();

  init_stored_settings();

#ifdef WEBSERVER
  init_webserver();
#endif

  init_CAN();

  init_LED();

  init_contactors();

  init_modbus();

  init_serialDataLink();

  inform_user_on_inverter();

  inform_user_on_battery();

#ifdef BATTERY_HAS_INIT
  init_battery();
#endif
}

// Perform main program functions
void loop() {

#ifdef WEBSERVER
  // Over-the-air updates by ElegantOTA
  ElegantOTA.loop();
  WiFi_monitor_loop();
#ifdef MQTT
  mqtt_loop();
#endif
#endif

  // Input
  receive_can();  // Receive CAN messages. Runs as fast as possible
#ifdef DUAL_CAN
  receive_can2();
#endif
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
  runSerialDataLink();
#endif

  // Process
  if (millis() - previousMillis10ms >= interval10)  // Every 10ms
  {
    previousMillis10ms = millis();
    handle_LED_state();  // Set the LED color according to state
#ifdef CONTACTOR_CONTROL
    handle_contactors();  // Take care of startup precharge/contactor closing
#endif
  }

  if (millis() - previousMillisUpdateVal >= intervalUpdateValues)  // Every 4.8s
  {
    previousMillisUpdateVal = millis();
    update_values();  // Update values heading towards inverter. Prepare for sending on CAN, or write directly to Modbus.
  }

  // Output
  send_can();  // Send CAN messages
#ifdef DUAL_CAN
  send_can2();
#endif
}

// Initialization functions
void init_serial() {
  // Init Serial monitor
  Serial.begin(115200);
  while (!Serial) {}
  Serial.println("__ OK __");
}

void init_stored_settings() {
  settings.begin("batterySettings", false);

#ifndef LOAD_SAVED_SETTINGS_ON_BOOT
  settings.clear();  // If this clear function is executed, no settings will be read from storage
#endif

  static uint16_t temp = 0;
  temp = settings.getUInt("BATTERY_WH_MAX", false);
  if (temp != 0) {
    BATTERY_WH_MAX = temp;
  }
  temp = settings.getUInt("MAXPERCENTAGE", false);
  if (temp != 0) {
    MAXPERCENTAGE = temp;
  }
  temp = settings.getUInt("MINPERCENTAGE", false);
  if (temp != 0) {
    MINPERCENTAGE = temp;
  }
  temp = settings.getUInt("MAXCHARGEAMP", false);
  if (temp != 0) {
    MAXCHARGEAMP = temp;
  }
  temp = settings.getUInt("MAXDISCHARGEAMP", false);
  if (temp != 0) {
    MAXDISCHARGEAMP = temp;
  }
  settings.end();
}

void init_CAN() {
  // CAN pins
  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);
  CAN_cfg.speed = CAN_SPEED_500KBPS;
  CAN_cfg.tx_pin_id = GPIO_NUM_27;
  CAN_cfg.rx_pin_id = GPIO_NUM_26;
  CAN_cfg.rx_queue = xQueueCreate(rx_queue_size, sizeof(CAN_frame_t));
  // Init CAN Module
  ESP32Can.CANInit();
  Serial.println(CAN_cfg.speed);

#ifdef DUAL_CAN
  Serial.println("Dual CAN Bus (ESP32+MCP2515) selected");
  gBuffer.initWithSize(25);
  SPI.begin(MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI);
  Serial.println("Configure ACAN2515");
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 500UL * 1000UL);  // CAN bit rate 500 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;       // Select loopback mode
  can.begin(settings, [] { can.isr(); });
#endif
}

void init_LED() {
  // Init LED control
  pixels.begin();
}

void init_contactors() {
  // Init contactor pins
#ifdef CONTACTOR_CONTROL
  pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
  digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
  pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
  digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
#ifdef PWM_CONTACTOR_CONTROL
  ledcSetup(POSITIVE_PWM_Ch, PWM_Freq, PWM_Res);           // Setup PWM Channel Frequency and Resolution
  ledcSetup(NEGATIVE_PWM_Ch, PWM_Freq, PWM_Res);           // Setup PWM Channel Frequency and Resolution
  ledcAttachPin(POSITIVE_CONTACTOR_PIN, POSITIVE_PWM_Ch);  // Attach Positive Contactor Pin to Hardware PWM Channel
  ledcAttachPin(NEGATIVE_CONTACTOR_PIN, NEGATIVE_PWM_Ch);  // Attach Positive Contactor Pin to Hardware PWM Channel
  ledcWrite(POSITIVE_PWM_Ch, 0);                           // Set Positive PWM to 0%
  ledcWrite(NEGATIVE_PWM_Ch, 0);                           // Set Negative PWM to 0%
#endif
  pinMode(PRECHARGE_PIN, OUTPUT);
  digitalWrite(PRECHARGE_PIN, LOW);
#endif
}

void init_modbus() {
  // Set up Modbus RTU Server
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);
  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);

#ifdef BYD_MODBUS
  // Init Static data to the RTU Modbus
  handle_static_data_modbus_byd();
#endif
#if defined(BYD_MODBUS) || defined(LUNA2000_MODBUS)
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
// Check that Dual LilyGo via RS485 option isn't enabled, this collides with Modbus!
#error MODBUS CANNOT BE USED IN DOUBLE LILYGO SETUPS! CHECK USER SETTINGS!
#endif

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
}

void inform_user_on_inverter() {
  // Inform user what Inverter is used
#ifdef BYD_CAN
  Serial.println("BYD CAN protocol selected");
#endif
#ifdef BYD_MODBUS
  Serial.println("BYD Modbus RTU protocol selected");
#endif
#ifdef LUNA2000_MODBUS
  Serial.println("Luna2000 Modbus RTU protocol selected");
#endif
#ifdef PYLON_CAN
  Serial.println("PYLON CAN protocol selected");
#endif
#ifdef SMA_CAN
  Serial.println("SMA CAN protocol selected");
#endif
#ifdef SOFAR_CAN
  Serial.println("SOFAR CAN protocol selected");
#endif
#ifdef SOLAX_CAN
  inverterAllowsContactorClosing = false;  // The inverter needs to allow first on this protocol
  intervalUpdateValues = 800;              // This protocol also requires the values to be updated faster
  Serial.println("SOLAX CAN protocol selected");
#endif
}

void inform_user_on_battery() {
  // Inform user what battery is used
#ifdef BMW_I3_BATTERY
  Serial.println("BMW i3 battery selected");
#endif
#ifdef CHADEMO_BATTERY
  Serial.println("Chademo battery selected");
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
  Serial.println("Mitsubishi i-MiEV / Citroen C-Zero / Peugeot Ion battery selected");
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
  Serial.println("Kia Niro / Hyundai Kona 64kWh battery selected");
#endif
#ifdef NISSAN_LEAF_BATTERY
  Serial.println("Nissan LEAF battery selected");
#endif
#ifdef RENAULT_KANGOO_BATTERY
  Serial.println("Renault Kangoo battery selected");
#endif
#ifdef SANTA_FE_PHEV_BATTERY
  Serial.println("Hyundai Santa Fe PHEV battery selected");
#endif
#ifdef RENAULT_ZOE_BATTERY
  Serial.println("Renault Zoe battery selected");
#endif
#ifdef TESLA_MODEL_3_BATTERY
  Serial.println("Tesla Model 3 battery selected");
#endif
#ifdef TEST_FAKE_BATTERY
  Serial.println("Test mode with fake battery selected");
#endif
#ifdef SERIAL_LINK_RECEIVER
  Serial.println("SERIAL_DATA_LINK_RECEIVER selected");
#endif
#if !defined(ABSOLUTE_MAX_VOLTAGE)
#error No battery selected! Choose one from the USER_SETTINGS.h file
#endif
}

// Functions
void receive_can() {  // This section checks if we have a complete CAN message incoming
  // Depending on which battery/inverter is selected, we forward this to their respective CAN routines
  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    if (rx_frame.FIR.B.FF == CAN_frame_std) {
      //printf("New standard frame");
      // Battery
#ifdef BMW_I3_BATTERY
      receive_can_i3_battery(rx_frame);
#endif
#ifdef CHADEMO_BATTERY
      receive_can_chademo_battery(rx_frame);
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
      receive_can_imiev_battery(rx_frame);
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
      receive_can_kiaHyundai_64_battery(rx_frame);
#endif
#ifdef NISSAN_LEAF_BATTERY
      receive_can_leaf_battery(rx_frame);
#endif
#ifdef RENAULT_KANGOO_BATTERY
      receive_can_kangoo_battery(rx_frame);
#endif
#ifdef SANTA_FE_PHEV_BATTERY
      receive_can_santafe_phev_battery(rx_frame);
#endif
#ifdef RENAULT_ZOE_BATTERY
      receive_can_zoe_battery(rx_frame);
#endif
#ifdef TESLA_MODEL_3_BATTERY
      receive_can_tesla_model_3_battery(rx_frame);
#endif
#ifdef TEST_FAKE_BATTERY
      receive_can_test_battery(rx_frame);
#endif
      // Inverter
#ifdef BYD_CAN
      receive_can_byd(rx_frame);
#endif
#ifdef SMA_CAN
      receive_can_sma(rx_frame);
#endif
      // Charger
#ifdef CHEVYVOLT_CHARGER
      receive_can_chevyvolt_charger(rx_frame);
#endif
#ifdef NISSANLEAF_CHARGER
      receive_can_nissanleaf_charger(rx_frame);
#endif
    } else {
      //printf("New extended frame");
#ifdef PYLON_CAN
      receive_can_pylon(rx_frame);
#endif
#ifdef SOFAR_CAN
      receive_can_sofar(rx_frame);
#endif
#ifdef SOLAX_CAN
      receive_can_solax(rx_frame);
#endif
    }
  }
}

void send_can() {
  // Send CAN messages
  // Inverter
#ifdef BYD_CAN
  send_can_byd();
#endif
#ifdef SMA_CAN
  send_can_sma();
#endif
#ifdef SOFAR_CAN
  send_can_sofar();
#endif
  // Battery
#ifdef BMW_I3_BATTERY
  send_can_i3_battery();
#endif
#ifdef CHADEMO_BATTERY
  send_can_chademo_battery();
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
  send_can_imiev_battery();
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
  send_can_kiaHyundai_64_battery();
#endif
#ifdef NISSAN_LEAF_BATTERY
  send_can_leaf_battery();
#endif
#ifdef RENAULT_KANGOO_BATTERY
  send_can_kangoo_battery();
#endif
#ifdef SANTA_FE_PHEV_BATTERY
  send_can_santafe_phev_battery();
#endif
#ifdef RENAULT_ZOE_BATTERY
  send_can_zoe_battery();
#endif
#ifdef TESLA_MODEL_3_BATTERY
  send_can_tesla_model_3_battery();
#endif
#ifdef TEST_FAKE_BATTERY
  send_can_test_battery();
#endif
#ifdef CHEVYVOLT_CHARGER
  send_can_chevyvolt_charger();
#endif
#ifdef NISSANLEAF_CHARGER
  send_can_nissanleaf_charger();
#endif
}

#ifdef DUAL_CAN
void receive_can2() {  // This function is similar to receive_can, but just takes care of inverters in the 2nd bus.
  // Depending on which inverter is selected, we forward this to their respective CAN routines
  CAN_frame_t rx_frame2;    // Struct with ESP32Can library format, compatible with the rest of the program
  CANMessage MCP2515Frame;  // Struct with ACAN2515 library format, needed to use thw MCP2515 library

  if (can.available()) {
    can.receive(MCP2515Frame);

    rx_frame2.MsgID = MCP2515Frame.id;
    rx_frame2.FIR.B.FF = MCP2515Frame.ext ? CAN_frame_ext : CAN_frame_std;
    rx_frame2.FIR.B.RTR = MCP2515Frame.rtr ? CAN_RTR : CAN_no_RTR;
    rx_frame2.FIR.B.DLC = MCP2515Frame.len;
    for (uint8_t i = 0; i < MCP2515Frame.len; i++) {
      rx_frame2.data.u8[i] = MCP2515Frame.data[i];
    }

    if (rx_frame2.FIR.B.FF == CAN_frame_std) {
//Serial.println("New standard frame");
#ifdef BYD_CAN
      receive_can_byd(rx_frame2);
#endif
    } else {
      //Serial.println("New extended frame");
#ifdef PYLON_CAN
      receive_can_pylon(rx_frame2);
#endif
#ifdef SOLAX_CAN
      receive_can_solax(rx_frame2);
#endif
    }
  }
}

void send_can2() {
  // Send CAN
  // Inverter
#ifdef BYD_CAN
  send_can_byd();
#endif
}
#endif

void handle_LED_state() {
  // Determine how bright the LED should be
  if (rampUp && brightness < maxBrightness) {
    brightness++;
  } else if (rampUp && brightness == maxBrightness) {
    rampUp = false;
  } else if (!rampUp && brightness > 0) {
    brightness--;
  } else if (!rampUp && brightness == 0) {
    rampUp = true;
  }
  switch (LEDcolor) {
    case GREEN:
      pixels.setPixelColor(0, pixels.Color(0, brightness, 0));  // Green pulsing LED
      break;
    case YELLOW:
      pixels.setPixelColor(0, pixels.Color(brightness, brightness, 0));  // Yellow pulsing LED
      break;
    case BLUE:
      pixels.setPixelColor(0, pixels.Color(0, 0, brightness));  // Blue pulsing LED
      break;
    case RED:
      pixels.setPixelColor(0, pixels.Color(150, 0, 0));  // Red LED full brightness
      break;
    case TEST_ALL_COLORS:
      pixels.setPixelColor(0, pixels.Color(brightness, abs((100 - brightness)), abs((50 - brightness))));  // RGB
      break;
    default:
      break;
  }

  // BMS in fault state overrides everything
  if (bms_status == FAULT) {
    LEDcolor = RED;
    pixels.setPixelColor(0, pixels.Color(255, 0, 0));  // Red LED full brightness
  }

  pixels.show();  // This sends the updated pixel color to the hardware.
}

#ifdef CONTACTOR_CONTROL
void handle_contactors() {
  // First check if we have any active errors, incase we do, turn off the battery
  if (bms_status == FAULT) {
    timeSpentInFaultedMode++;
  } else {
    timeSpentInFaultedMode = 0;
  }

  if (timeSpentInFaultedMode > MAX_ALLOWED_FAULT_TICKS) {
    contactorStatus = SHUTDOWN_REQUESTED;
  }
  if (contactorStatus == SHUTDOWN_REQUESTED) {
    digitalWrite(PRECHARGE_PIN, LOW);
    digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
    digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
    return;  // A fault scenario latches the contactor control. It is not possible to recover without a powercycle (and investigation why fault occured)
  }

  // After that, check if we are OK to start turning on the battery
  if (contactorStatus == DISCONNECTED) {
    digitalWrite(PRECHARGE_PIN, LOW);
#ifdef PWM_CONTACTOR_CONTROL
    ledcWrite(POSITIVE_PWM_Ch, 0);
    ledcWrite(NEGATIVE_PWM_Ch, 0);
#endif

    if (batteryAllowsContactorClosing && inverterAllowsContactorClosing) {
      contactorStatus = PRECHARGE;
    }
  }

  // In case the inverter requests contactors to open, set the state accordingly
  if (contactorStatus == COMPLETED) {
    if (!inverterAllowsContactorClosing)
      contactorStatus = DISCONNECTED;
    // Skip running the state machine below if it has already completed
    return;
  }

  unsigned long currentTime = millis();
  // Handle actual state machine. This first turns on Precharge, then Negative, then Positive, and finally turns OFF precharge
  switch (contactorStatus) {
    case PRECHARGE:
      digitalWrite(PRECHARGE_PIN, HIGH);
      prechargeStartTime = currentTime;
      contactorStatus = NEGATIVE;
      break;

    case NEGATIVE:
      if (currentTime - prechargeStartTime >= PRECHARGE_TIME_MS) {
        digitalWrite(NEGATIVE_CONTACTOR_PIN, HIGH);
#ifdef PWM_CONTACTOR_CONTROL
        ledcWrite(NEGATIVE_PWM_Ch, 1023);
#endif
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= NEGATIVE_CONTACTOR_TIME_MS) {
        digitalWrite(POSITIVE_CONTACTOR_PIN, HIGH);
#ifdef PWM_CONTACTOR_CONTROL
        ledcWrite(POSITIVE_PWM_Ch, 1023);
#endif
        contactorStatus = PRECHARGE_OFF;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - negativeStartTime >= POSITIVE_CONTACTOR_TIME_MS) {
        digitalWrite(PRECHARGE_PIN, LOW);
#ifdef PWM_CONTACTOR_CONTROL
        ledcWrite(NEGATIVE_PWM_Ch, PWM_Hold_Duty);
        ledcWrite(POSITIVE_PWM_Ch, PWM_Hold_Duty);
#endif
        contactorStatus = COMPLETED;
      }
      break;
    default:
      break;
  }
}
#endif

void update_values() {
  // Battery
#ifdef BMW_I3_BATTERY
  update_values_i3_battery();  // Map the values to the correct registers
#endif
#ifdef CHADEMO_BATTERY
  update_values_chademo_battery();  // Map the values to the correct registers
#endif
#ifdef IMIEV_CZERO_ION_BATTERY
  update_values_imiev_battery();  // Map the values to the correct registers
#endif
#ifdef KIA_HYUNDAI_64_BATTERY
  update_values_kiaHyundai_64_battery();  // Map the values to the correct registers
#endif
#ifdef NISSAN_LEAF_BATTERY
  update_values_leaf_battery();  // Map the values to the correct registers
#endif
#ifdef RENAULT_KANGOO_BATTERY
  update_values_kangoo_battery();  // Map the values to the correct registers
#endif
#ifdef SANTA_FE_PHEV_BATTERY
  update_values_santafe_phev_battery();  // Map the values to the correct registers
#endif
#ifdef RENAULT_ZOE_BATTERY
  update_values_zoe_battery();  // Map the values to the correct registers
#endif
#ifdef TESLA_MODEL_3_BATTERY
  update_values_tesla_model_3_battery();  // Map the values to the correct registers
#endif
#ifdef TEST_FAKE_BATTERY
  update_values_test_battery();  // Map the fake values to the correct registers
#endif
  // Inverter
#ifdef BYD_CAN
  update_values_can_byd();
#endif
#ifdef BYD_MODBUS
  update_modbus_registers_byd();
#endif
#ifdef LUNA2000_MODBUS
  update_modbus_registers_luna2000();
#endif
#ifdef PYLON_CAN
  update_values_can_pylon();
#endif
#ifdef SMA_CAN
  update_values_can_sma();
#endif
#ifdef SOFAR_CAN
  update_values_can_sofar();
#endif
#ifdef SOLAX_CAN
  update_values_can_solax();
#endif
}

void runSerialDataLink() {
  static unsigned long updateTime = 0;
  unsigned long currentMillis = millis();
#ifdef SERIAL_LINK_RECEIVER
  if ((currentMillis - updateTime) > 1) {  //Every 2ms
    updateTime = currentMillis;
    manageSerialLinkReceiver();
  }
#endif

#ifdef SERIAL_LINK_TRANSMITTER
  if ((currentMillis - updateTime) > 1) {  //Every 2ms
    updateTime = currentMillis;
    manageSerialLinkTransmitter();
  }
#endif
}

void init_serialDataLink() {
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
#endif
}

void storeSettings() {
  settings.begin("batterySettings", false);
  settings.putUInt("BATTERY_WH_MAX", BATTERY_WH_MAX);
  settings.putUInt("MAXPERCENTAGE", MAXPERCENTAGE);
  settings.putUInt("MINPERCENTAGE", MINPERCENTAGE);
  settings.putUInt("MAXCHARGEAMP", MAXCHARGEAMP);
  settings.putUInt("MAXDISCHARGEAMP", MAXDISCHARGEAMP);
  settings.end();
}
