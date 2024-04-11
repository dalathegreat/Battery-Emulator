/* Do not change any code below this line unless you are sure what you are doing */
/* Only change battery specific settings in "USER_SETTINGS.h" */

#include <Arduino.h>
#include <Preferences.h>
#include "HardwareSerial.h"
#include "USER_SETTINGS.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "src/battery/BATTERIES.h"
#include "src/charger/CHARGERS.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/led_handler.h"
#include "src/include.h"
#include "src/inverter/INVERTERS.h"
#include "src/lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "src/lib/eModbus-eModbus/Logging.h"
#include "src/lib/eModbus-eModbus/ModbusServerRTU.h"
#include "src/lib/eModbus-eModbus/scripts/mbServerFCs.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#ifdef WEBSERVER
#include <ESPmDNS.h>
#include "src/devboard/webserver/webserver.h"
#endif

Preferences settings;  // Store user settings
// The current software version, shown on webserver
const char* version_number = "5.7.0";

// Interval settings
uint16_t intervalUpdateValues = INTERVAL_5_S;  // Interval at which to update inverter values / Modbus registers
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
#ifdef CAN_FD
#include "src/lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
ACAN2517FD canfd(MCP2517_CS, SPI, MCP2517_INT);
#endif

// ModbusRTU parameters
#if defined(BYD_MODBUS) || defined(LUNA2000_MODBUS)
#define MB_RTU_NUM_VALUES 30000
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
#endif

// Common system parameters. Batteries map their values to these variables
uint32_t system_capacity_Wh = BATTERY_WH_MAX;            //Wh, 0-500000 Wh
uint32_t system_remaining_capacity_Wh = BATTERY_WH_MAX;  //Wh, 0-500000 Wh
int16_t system_temperature_max_dC = 0;                   //C+1, -50.0 - 50.0
int16_t system_temperature_min_dC = 0;                   //C+1, -50.0 - 50.0
int32_t system_active_power_W = 0;                       //Watts, -200000 to 200000 W
int16_t system_battery_current_dA = 0;                   //A+1, -1000 - 1000
uint16_t system_battery_voltage_dV = 3700;               //V+1,  0-1000.0 (0-10000)
uint16_t system_max_design_voltage_dV = 5000;            //V+1,  0-1000.0 (0-10000)
uint16_t system_min_design_voltage_dV = 2500;            //V+1,  0-1000.0 (0-10000)
uint16_t system_scaled_SOC_pptt = 5000;                  //SOC%, 0-100.00 (0-10000)
uint16_t system_real_SOC_pptt = 5000;                    //SOC%, 0-100.00 (0-10000)
uint16_t system_SOH_pptt = 9900;                         //SOH%, 0-100.00 (0-10000)
uint32_t system_max_discharge_power_W = 0;               //Watts, 0 to 200000
uint32_t system_max_charge_power_W = 4312;               //Watts, 0 to 200000
uint16_t system_cell_max_voltage_mV = 3700;              //mV, 0-5000 , Stores the highest cell millivolt value
uint16_t system_cell_min_voltage_mV = 3700;              //mV, 0-5000, Stores the minimum cell millivolt value
uint16_t system_cellvoltages_mV[MAX_AMOUNT_CELLS];  //Array with all cell voltages. Oversized to accomodate all setups
uint8_t system_bms_status = ACTIVE;  //ACTIVE - [0..5]<>[STANDBY,INACTIVE,DARKSTART,ACTIVE,FAULT,UPDATING]
uint8_t system_number_of_cells = 0;  //Total number of cell voltages, set by each battery
bool system_LFP_Chemistry = false;   //Set to true or false depending on cell chemistry

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

TaskHandle_t mainLoopTask;

// Initialization
void setup() {
  init_serial();

  init_stored_settings();

#ifdef WEBSERVER
  init_webserver();

  init_mDNS();
#endif

  init_events();

  init_CAN();

  init_contactors();

  init_modbus();

  init_serialDataLink();

  inform_user_on_inverter();

  init_battery();

  // BOOT button at runtime is used as an input for various things
  pinMode(0, INPUT_PULLUP);

  esp_task_wdt_deinit();  // Disable watchdog

  xTaskCreatePinnedToCore((TaskFunction_t)&mainLoop, "mainLoop", 4096, NULL, 8, &mainLoopTask, MAIN_FUNCTION_CORE);
}

// Perform main program functions
void loop() {
  ;
}

void mainLoop(void* pvParameters) {
  led_init();

  while (true) {

#ifdef WEBSERVER
    // Over-the-air updates by ElegantOTA
    wifi_monitor();
    ElegantOTA.loop();
#ifdef MQTT
    mqtt_loop();
#endif
#endif

    // Input
    receive_can();  // Receive CAN messages. Runs as fast as possible
#ifdef CAN_FD
    receive_canfd();  // Receive CAN-FD messages. Runs as fast as possible
#endif
#ifdef DUAL_CAN
    receive_can2();  // Receive CAN messages on CAN2. Runs as fast as possible
#endif
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
    runSerialDataLink();
#endif

    // Process
    if (millis() - previousMillis10ms >= INTERVAL_10_MS) {
      previousMillis10ms = millis();
      led_exe();
#ifdef CONTACTOR_CONTROL
      handle_contactors();  // Take care of startup precharge/contactor closing
#endif
    }

    if (millis() - previousMillisUpdateVal >= intervalUpdateValues)  // Every 5s normally
    {
      previousMillisUpdateVal = millis();
      update_SOC();     // Check if real or calculated SOC% value should be sent
      update_values();  // Update values heading towards inverter. Prepare for sending on CAN, or write directly to Modbus.
      if (DUMMY_EVENT_ENABLED) {
        set_event(EVENT_DUMMY_ERROR, (uint8_t)millis());
      }
    }

    // Output
    send_can();  // Send CAN messages
#ifdef DUAL_CAN
    send_can2();
#endif
    run_event_handling();

    delay(1);  // Allow the scheduler to start other tasks on other cores
  }
}

#ifdef WEBSERVER
// Initialise mDNS
void init_mDNS() {

  // Calulate the host name using the last two chars from the MAC address so each one is likely unique on a network.
  // e.g batteryemulator8C.local where the mac address is 08:F9:E0:D1:06:8C
  String mac = WiFi.macAddress();
  String mdnsHost = "batteryemulator" + mac.substring(mac.length() - 2);

  // Initialize mDNS .local resolution
  if (!MDNS.begin(mdnsHost)) {
#ifdef DEBUG_VIA_USB
    Serial.println("Error setting up MDNS responder!");
#endif
  } else {
    // Advertise via bonjour the service so we can auto discover these battery emulators on the local network.
    MDNS.addService("battery_emulator", "tcp", 80);
  }
}
#endif

// Initialization functions
void init_serial() {
  // Init Serial monitor
  Serial.begin(115200);
  while (!Serial) {}
#ifdef DEBUG_VIA_USB
  Serial.println("__ OK __");
#endif
}

void init_stored_settings() {
  settings.begin("batterySettings", false);

#ifndef LOAD_SAVED_SETTINGS_ON_BOOT
  settings.clear();  // If this clear function is executed, no settings will be read from storage
#endif

  static uint32_t temp = 0;
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
    temp = settings.getBool("USE_SCALED_SOC", false);
    USE_SCALED_SOC = temp;  //This bool needs to be checked inside the temp!= block
  }                         // No way to know if it wasnt reset otherwise

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

#ifdef DUAL_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("Dual CAN Bus (ESP32+MCP2515) selected");
#endif
  gBuffer.initWithSize(25);
  SPI.begin(MCP2515_SCK, MCP2515_MISO, MCP2515_MOSI);
  ACAN2515Settings settings(QUARTZ_FREQUENCY, 500UL * 1000UL);  // CAN bit rate 500 kb/s
  settings.mRequestedMode = ACAN2515Settings::NormalMode;
  can.begin(settings, [] { can.isr(); });
#endif

#ifdef CAN_FD
#ifdef DEBUG_VIA_USB
  Serial.println("CAN FD add-on (ESP32+MCP2517) selected");
#endif
  SPI.begin(MCP2517_SCK, MCP2517_SDO, MCP2517_SDI);
  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 500 * 1000,
                              DataBitRateFactor::x4);      // Arbitration bit rate: 500 kbit/s, data bit rate: 2 Mbit/s
  settings.mRequestedMode = ACAN2517FDSettings::NormalFD;  // ListenOnly / Normal20B / NormalFD
  const uint32_t errorCode = canfd.begin(settings, [] { canfd.isr(); });
  if (errorCode == 0) {
#ifdef DEBUG_VIA_USB
    Serial.print("Bit Rate prescaler: ");
    Serial.println(settings.mBitRatePrescaler);
    Serial.print("Arbitration Phase segment 1: ");
    Serial.println(settings.mArbitrationPhaseSegment1);
    Serial.print("Arbitration Phase segment 2: ");
    Serial.println(settings.mArbitrationPhaseSegment2);
    Serial.print("Arbitration SJW:");
    Serial.println(settings.mArbitrationSJW);
    Serial.print("Actual Arbitration Bit Rate: ");
    Serial.print(settings.actualArbitrationBitRate());
    Serial.println(" bit/s");
    Serial.print("Exact Arbitration Bit Rate ? ");
    Serial.println(settings.exactArbitrationBitRate() ? "yes" : "no");
    Serial.print("Arbitration Sample point: ");
    Serial.print(settings.arbitrationSamplePointFromBitStart());
    Serial.println("%");
#endif
  } else {
#ifdef DEBUG_VIA_USB
    Serial.print("CAN-FD Configuration error 0x");
    Serial.println(errorCode, HEX);
#endif
    set_event(EVENT_CANFD_INIT_FAILURE, (uint8_t)errorCode);
  }
#endif
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
#if defined(BYD_MODBUS) || defined(LUNA2000_MODBUS)
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

  // Init Serial2 connected to the RTU Modbus
  RTUutils::prepareHardwareSerial(Serial2);
  Serial2.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  // Register served function code worker for server
  MBserver.registerWorker(MBTCP_ID, READ_HOLD_REGISTER, &FC03);
  MBserver.registerWorker(MBTCP_ID, WRITE_HOLD_REGISTER, &FC06);
  MBserver.registerWorker(MBTCP_ID, WRITE_MULT_REGISTERS, &FC16);
  MBserver.registerWorker(MBTCP_ID, R_W_MULT_REGISTERS, &FC23);
  // Start ModbusRTU background task
  MBserver.begin(Serial2, MODBUS_CORE);
#endif
}

void inform_user_on_inverter() {
  // Inform user what Inverter is used
#ifdef BYD_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("BYD CAN protocol selected");
#endif
#endif
#ifdef BYD_MODBUS
#ifdef DEBUG_VIA_USB
  Serial.println("BYD Modbus RTU protocol selected");
#endif
#endif
#ifdef LUNA2000_MODBUS
#ifdef DEBUG_VIA_USB
  Serial.println("Luna2000 Modbus RTU protocol selected");
#endif
#endif
#ifdef PYLON_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("PYLON CAN protocol selected");
#endif
#endif
#ifdef SMA_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("SMA CAN protocol selected");
#endif
#endif
#ifdef SMA_TRIPOWER_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("SMA Tripower CAN protocol selected");
#endif
#endif
#ifdef SOFAR_CAN
#ifdef DEBUG_VIA_USB
  Serial.println("SOFAR CAN protocol selected");
#endif
#endif
#ifdef SOLAX_CAN
  inverterAllowsContactorClosing = false;  // The inverter needs to allow first on this protocol
  intervalUpdateValues = 800;              // This protocol also requires the values to be updated faster
#ifdef DEBUG_VIA_USB
  Serial.println("SOLAX CAN protocol selected");
#endif
#endif
}

void init_battery() {
  // Inform user what battery is used and perform setup
  setup_battery();

#ifndef BATTERY_SELECTED
#error No battery selected! Choose one from the USER_SETTINGS.h file
#endif
}

#ifdef CAN_FD
// Functions
void receive_canfd() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage frame;
  if (canfd.available()) {
    canfd.receive(frame);
    receive_canfd_battery(frame);
  }
}
#endif

void receive_can() {  // This section checks if we have a complete CAN message incoming
  // Depending on which battery/inverter is selected, we forward this to their respective CAN routines
  CAN_frame_t rx_frame;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame, 3 * portTICK_PERIOD_MS) == pdTRUE) {
    if (rx_frame.FIR.B.FF == CAN_frame_std) {  // New standard frame
// Battery
#ifndef SERIAL_LINK_RECEIVER
      receive_can_battery(rx_frame);
#endif
      // Inverter
#ifdef BYD_CAN
      receive_can_byd(rx_frame);
#endif
#ifdef SMA_CAN
      receive_can_sma(rx_frame);
#endif
#ifdef SMA_TRIPOWER_CAN
      receive_can_sma_tripower(rx_frame);
#endif
      // Charger
#ifdef CHEVYVOLT_CHARGER
      receive_can_chevyvolt_charger(rx_frame);
#endif
#ifdef NISSANLEAF_CHARGER
      receive_can_nissanleaf_charger(rx_frame);
#endif
    } else {  // New extended frame
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
#ifdef SMA_TRIPOWER_CAN
  send_can_sma_tripower();
#endif
#ifdef SOFAR_CAN
  send_can_sofar();
#endif
  // Battery
  send_can_battery();
  // Charger
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

    if (rx_frame2.FIR.B.FF == CAN_frame_std) {  // New standard frame
#ifdef BYD_CAN
      receive_can_byd(rx_frame2);
#endif
    } else {  // New extended frame
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

#ifdef CONTACTOR_CONTROL
void handle_contactors() {
  // First check if we have any active errors, incase we do, turn off the battery
  if (system_bms_status == FAULT) {
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

void update_SOC() {
  if (USE_SCALED_SOC) {  //User has configred a SOC window. Calculate a SOC% to send towards inverter
    static int16_t CalculatedSOC = 0;
    CalculatedSOC = system_real_SOC_pptt;
    CalculatedSOC = (10000) * (CalculatedSOC - (MINPERCENTAGE * 10)) / (MAXPERCENTAGE * 10 - MINPERCENTAGE * 10);
    if (CalculatedSOC < 0) {  //We are in the real SOC% range of 0-MINPERCENTAGE%
      CalculatedSOC = 0;
    }
    if (CalculatedSOC > 10000) {  //We are in the real SOC% range of MAXPERCENTAGE-100%
      CalculatedSOC = 10000;
    }
    system_scaled_SOC_pptt = CalculatedSOC;
  } else {  // No SOC window wanted. Set scaled to same as real.
    system_scaled_SOC_pptt = system_real_SOC_pptt;
  }
}

void update_values() {
  // Battery
  update_values_battery();  // Map the fake values to the correct registers
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
#ifdef SMA_TRIPOWER_CAN
  update_values_can_sma_tripower();
#endif
#ifdef SOFAR_CAN
  update_values_can_sofar();
#endif
#ifdef SOLAX_CAN
  update_values_can_solax();
#endif
}

#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
void runSerialDataLink() {
  static unsigned long updateTime = 0;
  unsigned long currentMillis = millis();

  if ((currentMillis - updateTime) > 1) {  //Every 2ms
    updateTime = currentMillis;
#ifdef SERIAL_LINK_RECEIVER
    manageSerialLinkReceiver();
#endif
#ifdef SERIAL_LINK_TRANSMITTER
    manageSerialLinkTransmitter();
#endif
  }
}
#endif

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
  settings.putBool("USE_SCALED_SOC", USE_SCALED_SOC);

  settings.end();
}
