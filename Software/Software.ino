/* Do not change any code below this line unless you are sure what you are doing */
/* Only change battery specific settings in "USER_SETTINGS.h" */

#include "src/include.h"

#include "HardwareSerial.h"
#include "USER_SETTINGS.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "src/charger/CHARGERS.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/led_handler.h"
#include "src/devboard/utils/value_mapping.h"
#include "src/lib/YiannisBourkelis-Uptime-Library/src/uptime.h"
#include "src/lib/YiannisBourkelis-Uptime-Library/src/uptime_formatter.h"
#include "src/lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "src/lib/eModbus-eModbus/Logging.h"
#include "src/lib/eModbus-eModbus/ModbusServerRTU.h"
#include "src/lib/eModbus-eModbus/scripts/mbServerFCs.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "src/lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"

#ifdef WIFI
#include "src/devboard/wifi/wifi.h"
#ifdef WEBSERVER
#include "src/devboard/webserver/webserver.h"
#ifdef MDNSRESPONDER
#include <ESPmDNS.h>
#endif  // MDNSRESONDER
#else   // WEBSERVER
#ifdef MDNSRESPONDER
#error WEBSERVER needs to be enabled for MDNSRESPONDER!
#endif  // MDNSRSPONDER
#endif  // WEBSERVER
#ifdef MQTT
#include "src/devboard/mqtt/mqtt.h"
#endif  // MQTT
#endif  // WIFI

#ifndef CONTACTOR_CONTROL
#ifdef PWM_CONTACTOR_CONTROL
#error CONTACTOR_CONTROL needs to be enabled for PWM_CONTACTOR_CONTROL
#endif
#endif

#ifdef EQUIPMENT_STOP_BUTTON
#include "src/devboard/utils/debounce_button.h"
#endif

Preferences settings;  // Store user settings
// The current software version, shown on webserver
const char* version_number = "7.9.0";

// Interval settings
uint16_t intervalUpdateValues = INTERVAL_1_S;  // Interval at which to update inverter values / Modbus registers
unsigned long previousMillis10ms = 0;
unsigned long previousMillisUpdateVal = 0;

// CAN parameters
CAN_device_t CAN_cfg;          // CAN Config
const int rx_queue_size = 10;  // Receive Queue size
volatile bool send_ok = 0;

#ifdef DUAL_CAN
#include "src/lib/pierremolinaro-acan2515/ACAN2515.h"
static const uint32_t QUARTZ_FREQUENCY = CRYSTAL_FREQUENCY_MHZ * 1000000UL;  //MHZ configured in USER_SETTINGS.h
ACAN2515 can(MCP2515_CS, SPI, MCP2515_INT);
static ACAN2515_Buffer16 gBuffer;
#endif  //DUAL_CAN
#ifdef CAN_FD
#include "src/lib/pierremolinaro-ACAN2517FD/ACAN2517FD.h"
ACAN2517FD canfd(MCP2517_CS, SPI, MCP2517_INT);
#endif  //CAN_FD

// ModbusRTU parameters
#ifdef MODBUS_INVERTER_SELECTED
#define MB_RTU_NUM_VALUES 13100
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
#endif
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
#define SERIAL_LINK_BAUDRATE 112500
#endif

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

// Task time measurement for debugging and for setting CPU load events
int64_t core_task_time_us;
MyTimer core_task_timer_10s(INTERVAL_10_S);

int64_t connectivity_task_time_us;
MyTimer connectivity_task_timer_10s(INTERVAL_10_S);

MyTimer loop_task_timer_10s(INTERVAL_10_S);

MyTimer check_pause_2s(INTERVAL_2_S);

// Contactor parameters
#ifdef CONTACTOR_CONTROL
enum State { DISCONNECTED, PRECHARGE, NEGATIVE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

#define ON 1
#define OFF 0

#ifdef NC_CONTACTORS  //Normally closed contactors use inverted logic
#undef ON
#define ON 0
#undef OFF
#define OFF 1
#endif

#define MAX_ALLOWED_FAULT_TICKS 1000
/* NOTE: modify the precharge time constant below to account for the resistance and capacitance of the target system.
 *	t=3RC at minimum, t=5RC ideally 
 */
#define PRECHARGE_TIME_MS 160
#define NEGATIVE_CONTACTOR_TIME_MS 1000
#define POSITIVE_CONTACTOR_TIME_MS 2000
#define PWM_Freq 20000  // 20 kHz frequency, beyond audible range
#define PWM_Res 10      // 10 Bit resolution 0 to 1023, maps 'nicely' to 0% 100%
#define PWM_HOLD_DUTY 250
#define PWM_OFF_DUTY 0
#define PWM_ON_DUTY 1023
#define POSITIVE_PWM_Ch 0
#define NEGATIVE_PWM_Ch 1
unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long timeSpentInFaultedMode = 0;
#endif

void set(uint8_t pin, bool direction, uint32_t pwm_freq = 0xFFFFFFFFFF) {
#ifdef PWM_CONTACTOR_CONTROL
  if (pwm_freq != 0xFFFFFFFFFF) {
    ledcWrite(pin, pwm_freq);
    return;
  }
#endif
  if (direction == 1) {
    digitalWrite(pin, HIGH);
  } else {  // 0
    digitalWrite(pin, LOW);
  }
}

#ifdef EQUIPMENT_STOP_BUTTON
const unsigned long equipment_button_long_press_duration =
    15000;                                                     // 15 seconds for long press in case of MOMENTARY_SWITCH
const unsigned long equipment_button_debounce_duration = 200;  // 250ms for debouncing the button
unsigned long timeSincePress = 0;                              // Variable to store the time since the last press
DebouncedButton equipment_stop_button;                         // Debounced button object
#endif

TaskHandle_t main_loop_task;
TaskHandle_t connectivity_loop_task;

// Initialization
void setup() {
  init_serial();

  init_stored_settings();

#ifdef WIFI
  xTaskCreatePinnedToCore((TaskFunction_t)&connectivity_loop, "connectivity_loop", 4096, &connectivity_task_time_us,
                          TASK_CONNECTIVITY_PRIO, &connectivity_loop_task, WIFI_CORE);
#endif

  init_events();

  init_CAN();

  init_contactors();

  init_rs485();

  init_serialDataLink();
#if defined(CAN_INVERTER_SELECTED) || defined(MODBUS_INVERTER_SELECTED) || defined(RS485_INVERTER_SELECTED)
  setup_inverter();
#endif
  setup_battery();
#ifdef EQUIPMENT_STOP_BUTTON
  init_equipment_stop_button();
#endif
  // BOOT button at runtime is used as an input for various things
  pinMode(0, INPUT_PULLUP);

  esp_task_wdt_deinit();  // Disable watchdog

  check_reset_reason();

  xTaskCreatePinnedToCore((TaskFunction_t)&core_loop, "core_loop", 4096, &core_task_time_us, TASK_CORE_PRIO,
                          &main_loop_task, CORE_FUNCTION_CORE);
}

// Perform main program functions
void loop() {
  START_TIME_MEASUREMENT(loop_func);
  run_event_handling();
  END_TIME_MEASUREMENT_MAX(loop_func, datalayer.system.status.loop_task_10s_max_us);
#ifdef FUNCTION_TIME_MEASUREMENT
  if (loop_task_timer_10s.elapsed()) {
    datalayer.system.status.loop_task_10s_max_us = 0;
  }
#endif
}

#ifdef WIFI
void connectivity_loop(void* task_time_us) {

  // Init wifi
  init_WiFi();

#ifdef WEBSERVER
  // Init webserver
  init_webserver();
#endif
#ifdef MDNSRESPONDER
  init_mDNS();
#endif
#ifdef MQTT
  init_mqtt();
#endif

  while (true) {
    START_TIME_MEASUREMENT(wifi);
    wifi_monitor();
#ifdef WEBSERVER
    ota_monitor();
#endif
    END_TIME_MEASUREMENT_MAX(wifi, datalayer.system.status.wifi_task_10s_max_us);
#ifdef MQTT
    START_TIME_MEASUREMENT(mqtt);
    mqtt_loop();
    END_TIME_MEASUREMENT_MAX(mqtt, datalayer.system.status.mqtt_task_10s_max_us);
#endif

#ifdef FUNCTION_TIME_MEASUREMENT
    if (connectivity_task_timer_10s.elapsed()) {
      datalayer.system.status.mqtt_task_10s_max_us = 0;
      datalayer.system.status.wifi_task_10s_max_us = 0;
    }
#endif
    delay(1);
  }
}
#endif

void core_loop(void* task_time_us) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1);  // Convert 1ms to ticks
  led_init();

  while (true) {
    START_TIME_MEASUREMENT(all);
    START_TIME_MEASUREMENT(comm);
#ifdef EQUIPMENT_STOP_BUTTON
    monitor_equipment_stop_button();
#endif

    // Input, Runs as fast as possible
    receive_can_native();  // Receive CAN messages from native CAN port
#ifdef CAN_FD
    receive_canfd();  // Receive CAN-FD messages.
#endif
#ifdef DUAL_CAN
    receive_can_addonMCP2515();  // Receive CAN messages on add-on MCP2515 chip
#endif
#ifdef RS485_INVERTER_SELECTED
    receive_RS485();  // Process serial2 RS485 interface
#endif
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
    runSerialDataLink();
#endif
    END_TIME_MEASUREMENT_MAX(comm, datalayer.system.status.time_comm_us);
#ifdef WEBSERVER
    START_TIME_MEASUREMENT(ota);
    ElegantOTA.loop();
    END_TIME_MEASUREMENT_MAX(ota, datalayer.system.status.time_ota_us);
#endif

    START_TIME_MEASUREMENT(time_10ms);
    // Process
    if (millis() - previousMillis10ms >= INTERVAL_10_MS) {
      previousMillis10ms = millis();
      led_exe();
      handle_contactors();  // Take care of startup precharge/contactor closing
    }
    END_TIME_MEASUREMENT_MAX(time_10ms, datalayer.system.status.time_10ms_us);

    START_TIME_MEASUREMENT(time_values);
    if (millis() - previousMillisUpdateVal >= intervalUpdateValues) {
      previousMillisUpdateVal = millis();  // Order matters on the update_loop!
      update_values_battery();             // Fetch battery values
#ifdef DOUBLE_BATTERY
      update_values_battery2();
      check_interconnect_available();
#endif
      update_calculated_values();
#ifndef SERIAL_LINK_RECEIVER
      update_machineryprotection();  // Check safeties (Not on serial link reciever board)
#endif
      update_values_inverter();  // Update values heading towards inverter
      if (DUMMY_EVENT_ENABLED) {
        set_event(EVENT_DUMMY_ERROR, (uint8_t)millis());
      }
    }
    END_TIME_MEASUREMENT_MAX(time_values, datalayer.system.status.time_values_us);

    START_TIME_MEASUREMENT(cantx);
    // Output
    send_can();  // Send CAN messages to all components

    END_TIME_MEASUREMENT_MAX(cantx, datalayer.system.status.time_cantx_us);
    END_TIME_MEASUREMENT_MAX(all, datalayer.system.status.core_task_10s_max_us);
#ifdef FUNCTION_TIME_MEASUREMENT

    if (datalayer.system.status.core_task_10s_max_us > datalayer.system.status.core_task_max_us) {
      // Update worst case total time
      datalayer.system.status.core_task_max_us = datalayer.system.status.core_task_10s_max_us;
      // Record snapshots of task times
      datalayer.system.status.time_snap_comm_us = datalayer.system.status.time_comm_us;
      datalayer.system.status.time_snap_10ms_us = datalayer.system.status.time_10ms_us;
      datalayer.system.status.time_snap_values_us = datalayer.system.status.time_values_us;
      datalayer.system.status.time_snap_cantx_us = datalayer.system.status.time_cantx_us;
      datalayer.system.status.time_snap_ota_us = datalayer.system.status.time_ota_us;
    }

    datalayer.system.status.core_task_max_us =
        MAX(datalayer.system.status.core_task_10s_max_us, datalayer.system.status.core_task_max_us);
    if (core_task_timer_10s.elapsed()) {
      datalayer.system.status.time_ota_us = 0;
      datalayer.system.status.time_comm_us = 0;
      datalayer.system.status.time_10ms_us = 0;
      datalayer.system.status.time_values_us = 0;
      datalayer.system.status.time_cantx_us = 0;
      datalayer.system.status.core_task_10s_max_us = 0;
    }

#endif
    if (check_pause_2s.elapsed()) {
      emulator_pause_state_send_CAN_battery();
    }

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

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
  static uint32_t temp = 0;
  settings.begin("batterySettings", false);

  // Always get the equipment stop status
  datalayer.system.settings.equipment_stop_active = settings.getBool("EQUIPMENT_STOP", false);
  if (datalayer.system.settings.equipment_stop_active) {
    set_event(EVENT_EQUIPMENT_STOP, 1);
  }

#ifndef LOAD_SAVED_SETTINGS_ON_BOOT
  settings.clear();  // If this clear function is executed, no settings will be read from storage

  //always save the equipment stop status
  settings.putBool("EQUIPMENT_STOP", datalayer.system.settings.equipment_stop_active);

#endif

#ifdef WIFI

  char tempSSIDstring[63];  // Allocate buffer with sufficient size
  size_t lengthSSID = settings.getString("SSID", tempSSIDstring, sizeof(tempSSIDstring));
  if (lengthSSID > 0) {  // Successfully read the string from memory. Set it to SSID!
    ssid = tempSSIDstring;
  } else {  // Reading from settings failed. Do nothing with SSID. Raise event?
  }
  char tempPasswordString[63];  // Allocate buffer with sufficient size
  size_t lengthPassword = settings.getString("PASSWORD", tempPasswordString, sizeof(tempPasswordString));
  if (lengthPassword > 7) {  // Successfully read the string from memory. Set it to password!
    password = tempPasswordString;
  } else {  // Reading from settings failed. Do nothing with SSID. Raise event?
  }
#endif

  temp = settings.getUInt("BATTERY_WH_MAX", false);
  if (temp != 0) {
    datalayer.battery.info.total_capacity_Wh = temp;
  }
  temp = settings.getUInt("MAXPERCENTAGE", false);
  if (temp != 0) {
    datalayer.battery.settings.max_percentage = temp * 10;  // Multiply by 10 for backwards compatibility
  }
  temp = settings.getUInt("MINPERCENTAGE", false);
  if (temp != 0) {
    datalayer.battery.settings.min_percentage = temp * 10;  // Multiply by 10 for backwards compatibility
  }
  temp = settings.getUInt("MAXCHARGEAMP", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_charge_dA = temp;
  }
  temp = settings.getUInt("MAXDISCHARGEAMP", false);
  if (temp != 0) {
    datalayer.battery.settings.max_user_set_discharge_dA = temp;
    temp = settings.getBool("USE_SCALED_SOC", false);
    datalayer.battery.settings.soc_scaling_active = temp;  //This bool needs to be checked inside the temp!= block
  }                                                        // No way to know if it wasnt reset otherwise

  settings.end();
}

void init_CAN() {
// CAN pins
#ifdef CAN_SE_PIN
  pinMode(CAN_SE_PIN, OUTPUT);
  digitalWrite(CAN_SE_PIN, LOW);
#endif
  CAN_cfg.speed = CAN_SPEED_500KBPS;
#ifdef NATIVECAN_250KBPS  // Some component is requesting lower CAN speed
  CAN_cfg.speed = CAN_SPEED_250KBPS;
#endif
  CAN_cfg.tx_pin_id = CAN_TX_PIN;
  CAN_cfg.rx_pin_id = CAN_RX_PIN;
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
  const uint16_t errorCodeMCP = can.begin(settings, [] { can.isr(); });
  if (errorCodeMCP == 0) {
#ifdef DEBUG_VIA_USB
    Serial.println("Can ok");
#endif
  } else {
#ifdef DEBUG_VIA_USB
    Serial.print("Error Can: 0x");
    Serial.println(errorCodeMCP, HEX);
#endif
    set_event(EVENT_CANMCP_INIT_FAILURE, (uint8_t)errorCodeMCP);
  }
#endif

#ifdef CAN_FD
#ifdef DEBUG_VIA_USB
  Serial.println("CAN FD add-on (ESP32+MCP2517) selected");
#endif
  SPI.begin(MCP2517_SCK, MCP2517_SDO, MCP2517_SDI);
  ACAN2517FDSettings settings(CAN_FD_CRYSTAL_FREQUENCY_MHZ, 500 * 1000,
                              DataBitRateFactor::x4);  // Arbitration bit rate: 500 kbit/s, data bit rate: 2 Mbit/s
#ifdef USE_CANFD_INTERFACE_AS_CLASSIC_CAN
  settings.mRequestedMode = ACAN2517FDSettings::Normal20B;  // ListenOnly / Normal20B / NormalFD
#else
  settings.mRequestedMode = ACAN2517FDSettings::NormalFD;  // ListenOnly / Normal20B / NormalFD
#endif
  const uint32_t errorCode = canfd.begin(settings, [] { canfd.isr(); });
  canfd.poll();
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
#ifdef PWM_CONTACTOR_CONTROL
  ledcAttachChannel(POSITIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res,
                    POSITIVE_PWM_Ch);  // Setup PWM Channel Frequency and Resolution
  ledcAttachChannel(NEGATIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res,
                    NEGATIVE_PWM_Ch);               // Setup PWM Channel Frequency and Resolution
  ledcWrite(POSITIVE_CONTACTOR_PIN, PWM_OFF_DUTY);  // Set Positive PWM to 0%
  ledcWrite(NEGATIVE_CONTACTOR_PIN, PWM_OFF_DUTY);  // Set Negative PWM to 0%
#else                                               //Normal CONTACTOR_CONTROL
  pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
  set(POSITIVE_CONTACTOR_PIN, OFF);
  pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
  set(NEGATIVE_CONTACTOR_PIN, OFF);
#endif
  pinMode(PRECHARGE_PIN, OUTPUT);
  set(PRECHARGE_PIN, OFF);
#endif  //CONTACTOR_CONTROL
#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY

  pinMode(SECOND_POSITIVE_CONTACTOR_PIN, OUTPUT);
  set(SECOND_POSITIVE_CONTACTOR_PIN, OFF);
  pinMode(SECOND_NEGATIVE_CONTACTOR_PIN, OUTPUT);
  set(SECOND_NEGATIVE_CONTACTOR_PIN, OFF);
#endif  //CONTACTOR_CONTROL_DOUBLE_BATTERY
// Init BMS contactor
#ifdef HW_STARK  // TODO: Rewrite this so LilyGo can also handle this BMS contactor
  pinMode(BMS_POWER, OUTPUT);
  digitalWrite(BMS_POWER, HIGH);
#endif  //HW_STARK
}

void init_rs485() {
// Set up Modbus RTU Server
#ifdef RS485_EN_PIN
  pinMode(RS485_EN_PIN, OUTPUT);
  digitalWrite(RS485_EN_PIN, HIGH);
#endif
#ifdef RS485_SE_PIN
  pinMode(RS485_SE_PIN, OUTPUT);
  digitalWrite(RS485_SE_PIN, HIGH);
#endif
#ifdef PIN_5V_EN
  pinMode(PIN_5V_EN, OUTPUT);
  digitalWrite(PIN_5V_EN, HIGH);
#endif
#ifdef RS485_INVERTER_SELECTED
  Serial2.begin(57600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
#endif
#ifdef MODBUS_INVERTER_SELECTED
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

#ifdef EQUIPMENT_STOP_BUTTON

void monitor_equipment_stop_button() {

  ButtonState changed_state = debounceButton(equipment_stop_button, timeSincePress);

  if (equipment_stop_behavior == LATCHING_SWITCH) {
    if (changed_state == PRESSED) {
      // Changed to ON – initiating equipment stop.
      setBatteryPause(true, false, true);
    } else if (changed_state == RELEASED) {
      // Changed to OFF – ending equipment stop.
      setBatteryPause(false, false, false);
    }
  } else if (equipment_stop_behavior == MOMENTARY_SWITCH) {
    if (changed_state == RELEASED) {  // button is released

      if (timeSincePress < equipment_button_long_press_duration) {
        // Short press detected, trigger equipment stop
        setBatteryPause(true, false, true);
      } else {
        // Long press detected, reset equipment stop state
        setBatteryPause(false, false, false);
      }
    }
  }
}

void init_equipment_stop_button() {
  //using external pullup resistors NC
  pinMode(EQUIPMENT_STOP_PIN, INPUT);
  // Initialize the debounced button with NC switch type and equipment_button_debounce_duration debounce time
  initDebouncedButton(equipment_stop_button, EQUIPMENT_STOP_PIN, NC, equipment_button_debounce_duration);
}

#endif

enum frameDirection { MSG_RX, MSG_TX };  //RX = 0, TX = 1
void print_can_frame(CAN_frame frame, frameDirection msgDir);
void print_can_frame(CAN_frame frame, frameDirection msgDir) {
#ifdef DEBUG_CAN_DATA  // If enabled in user settings, print out the CAN messages via USB
  uint8_t i = 0;
  Serial.print("(");
  Serial.print(millis() / 1000.0);
  (msgDir == MSG_RX) ? Serial.print(") RX0 ") : Serial.print(") TX1 ");
  Serial.print(frame.ID, HEX);
  Serial.print(" [");
  Serial.print(frame.DLC);
  Serial.print("] ");
  for (i = 0; i < frame.DLC; i++) {
    Serial.print(frame.data.u8[i] < 16 ? "0" : "");
    Serial.print(frame.data.u8[i], HEX);
    if (i < frame.DLC - 1)
      Serial.print(" ");
  }
  Serial.println("");
#endif  //#DEBUG_CAN_DATA

  if (datalayer.system.info.can_logging_active) {  // If user clicked on CAN Logging page in webserver, start recording
    char* message_string = datalayer.system.info.logged_can_messages;
    int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
    size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

    if (offset + 128 > sizeof(datalayer.system.info.logged_can_messages)) {
      // Not enough space, reset and start from the beginning
      offset = 0;
    }
    unsigned long currentTime = millis();
    // Add timestamp
    offset += snprintf(message_string + offset, message_string_size - offset, "(%lu.%03lu) ", currentTime / 1000,
                       currentTime % 1000);

    // Add direction. The 0 and 1 after RX and TX ensures that SavvyCAN puts TX and RX in a different bus.
    offset +=
        snprintf(message_string + offset, message_string_size - offset, "%s ", (msgDir == MSG_RX) ? "RX0" : "TX1");

    // Add ID and DLC
    offset += snprintf(message_string + offset, message_string_size - offset, "%X [%u] ", frame.ID, frame.DLC);

    // Add data bytes
    for (uint8_t i = 0; i < frame.DLC; i++) {
      if (i < frame.DLC - 1) {
        offset += snprintf(message_string + offset, message_string_size - offset, "%02X ", frame.data.u8[i]);
      } else {
        offset += snprintf(message_string + offset, message_string_size - offset, "%02X", frame.data.u8[i]);
      }
    }
    // Add linebreak
    offset += snprintf(message_string + offset, message_string_size - offset, "\n");

    datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
  }
}

#ifdef CAN_FD
// Functions
void receive_canfd() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage frame;
  int count = 0;
  while (canfd.available() && count++ < 16) {
    canfd.receive(frame);

    CAN_frame rx_frame;
    rx_frame.ID = frame.id;
    rx_frame.ext_ID = frame.ext;
    rx_frame.DLC = frame.len;
    memcpy(rx_frame.data.u8, frame.data, MIN(rx_frame.DLC, 64));
    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CAN_ADDON_FD_MCP2518);
    receive_can(&rx_frame, CANFD_NATIVE);
  }
}
#endif

void receive_can_native() {  // This section checks if we have a complete CAN message incoming on native CAN port
  CAN_frame_t rx_frame_native;
  if (xQueueReceive(CAN_cfg.rx_queue, &rx_frame_native, 0) == pdTRUE) {
    CAN_frame rx_frame;
    rx_frame.ID = rx_frame_native.MsgID;
    if (rx_frame_native.FIR.B.FF == CAN_frame_std) {
      rx_frame.ext_ID = false;
    } else {  //CAN_frame_ext == 1
      rx_frame.ext_ID = true;
    }
    rx_frame.DLC = rx_frame_native.FIR.B.DLC;
    for (uint8_t i = 0; i < rx_frame.DLC && i < 8; i++) {
      rx_frame.data.u8[i] = rx_frame_native.data.u8[i];
    }
    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CAN_NATIVE);
  }
}

void send_can() {
  if (!allowed_to_send_CAN) {
    return;
  }

  send_can_battery();

#ifdef CAN_INVERTER_SELECTED
  send_can_inverter();
#endif  // CAN_INVERTER_SELECTED

#ifdef CHARGER_SELECTED
  send_can_charger();
#endif  // CHARGER_SELECTED
}

#ifdef DUAL_CAN
void receive_can_addonMCP2515() {  // This section checks if we have a complete CAN message incoming on add-on CAN port
  CAN_frame rx_frame;              // Struct with our CAN format
  CANMessage MCP2515Frame;         // Struct with ACAN2515 library format, needed to use the MCP2515 library

  if (can.available()) {
    can.receive(MCP2515Frame);

    rx_frame.ID = MCP2515Frame.id;
    rx_frame.ext_ID = MCP2515Frame.ext ? CAN_frame_ext : CAN_frame_std;
    rx_frame.DLC = MCP2515Frame.len;
    for (uint8_t i = 0; i < MCP2515Frame.len && i < 8; i++) {
      rx_frame.data.u8[i] = MCP2515Frame.data[i];
    }

    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CAN_ADDON_MCP2515);
  }
}
#endif  // DUAL_CAN

#ifdef DOUBLE_BATTERY
void check_interconnect_available() {
  if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
    return;  // Both voltage values need to be available to start check
  }

  uint16_t voltage_diff = abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV);

  if (voltage_diff <= 30) {  // If we are within 3.0V between the batteries
    clear_event(EVENT_VOLTAGE_DIFFERENCE);
    if (datalayer.battery.status.bms_status == FAULT) {
      // If main battery is in fault state, disengage the second battery
      datalayer.system.status.battery2_allows_contactor_closing = false;
    } else {  // If main battery is OK, allow second battery to join
      datalayer.system.status.battery2_allows_contactor_closing = true;
    }
  } else {  //Voltage between the two packs is too large
    set_event(EVENT_VOLTAGE_DIFFERENCE, (uint8_t)(voltage_diff / 10));
  }
}
#endif  //DOUBLE_BATTERY

void handle_contactors() {
#ifdef BYD_SMA
  datalayer.system.status.inverter_allows_contactor_closing = digitalRead(INVERTER_CONTACTOR_ENABLE_PIN);
#endif

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
  handle_contactors_battery2();
#endif

#ifdef CONTACTOR_CONTROL
  // First check if we have any active errors, incase we do, turn off the battery
  if (datalayer.battery.status.bms_status == FAULT) {
    timeSpentInFaultedMode++;
  } else {
    timeSpentInFaultedMode = 0;
  }

  //handle contactor control SHUTDOWN_REQUESTED
  if (timeSpentInFaultedMode > MAX_ALLOWED_FAULT_TICKS) {
    contactorStatus = SHUTDOWN_REQUESTED;
  }

  if (contactorStatus == SHUTDOWN_REQUESTED) {
    set(PRECHARGE_PIN, OFF);
    set(NEGATIVE_CONTACTOR_PIN, OFF, PWM_OFF_DUTY);
    set(POSITIVE_CONTACTOR_PIN, OFF, PWM_OFF_DUTY);
    set_event(EVENT_ERROR_OPEN_CONTACTOR, 0);
    datalayer.system.status.contactors_engaged = false;
    return;  // A fault scenario latches the contactor control. It is not possible to recover without a powercycle (and investigation why fault occured)
  }

  // After that, check if we are OK to start turning on the battery
  if (contactorStatus == DISCONNECTED) {
    set(PRECHARGE_PIN, OFF);
    set(NEGATIVE_CONTACTOR_PIN, OFF, PWM_OFF_DUTY);
    set(POSITIVE_CONTACTOR_PIN, OFF, PWM_OFF_DUTY);

    if (datalayer.system.status.battery_allows_contactor_closing &&
        datalayer.system.status.inverter_allows_contactor_closing && !datalayer.system.settings.equipment_stop_active) {
      contactorStatus = PRECHARGE;
    }
  }

  // In case the inverter requests contactors to open, set the state accordingly
  if (contactorStatus == COMPLETED) {
    //Incase inverter (or estop) requests contactors to open, make state machine jump to Disconnected state (recoverable)
    if (!datalayer.system.status.inverter_allows_contactor_closing || datalayer.system.settings.equipment_stop_active) {
      contactorStatus = DISCONNECTED;
    }
    // Skip running the state machine below if it has already completed
    return;
  }

  unsigned long currentTime = millis();
  // Handle actual state machine. This first turns on Precharge, then Negative, then Positive, and finally turns OFF precharge
  switch (contactorStatus) {
    case PRECHARGE:
      set(PRECHARGE_PIN, ON);
      prechargeStartTime = currentTime;
      contactorStatus = NEGATIVE;
      break;

    case NEGATIVE:
      if (currentTime - prechargeStartTime >= PRECHARGE_TIME_MS) {
        set(NEGATIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= NEGATIVE_CONTACTOR_TIME_MS) {
        set(POSITIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
        contactorStatus = PRECHARGE_OFF;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - negativeStartTime >= POSITIVE_CONTACTOR_TIME_MS) {
        set(PRECHARGE_PIN, OFF);
        set(NEGATIVE_CONTACTOR_PIN, ON, PWM_HOLD_DUTY);
        set(POSITIVE_CONTACTOR_PIN, ON, PWM_HOLD_DUTY);
        contactorStatus = COMPLETED;
        datalayer.system.status.contactors_engaged = true;
      }
      break;
    default:
      break;
  }
#endif  // CONTACTOR_CONTROL
}

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
void handle_contactors_battery2() {
  if ((contactorStatus == COMPLETED) && datalayer.system.status.battery2_allows_contactor_closing) {
    set(SECOND_NEGATIVE_CONTACTOR_PIN, ON);
    set(SECOND_POSITIVE_CONTACTOR_PIN, ON);
    datalayer.system.status.contactors_battery2_engaged = true;
  } else {  // Closing contactors on secondary battery not allowed
    set(SECOND_NEGATIVE_CONTACTOR_PIN, OFF);
    set(SECOND_POSITIVE_CONTACTOR_PIN, OFF);
    datalayer.system.status.contactors_battery2_engaged = false;
  }
}
#endif  //CONTACTOR_CONTROL_DOUBLE_BATTERY

void update_calculated_values() {
  /* Calculate allowed charge/discharge currents*/
  if (datalayer.battery.status.voltage_dV > 10) {
    // Only update value when we have voltage available to avoid div0. TODO: This should be based on nominal voltage
    datalayer.battery.status.max_charge_current_dA =
        ((datalayer.battery.status.max_charge_power_W * 100) / datalayer.battery.status.voltage_dV);
    datalayer.battery.status.max_discharge_current_dA =
        ((datalayer.battery.status.max_discharge_power_W * 100) / datalayer.battery.status.voltage_dV);
  }
  /* Restrict values from user settings if needed*/
  if (datalayer.battery.status.max_charge_current_dA > datalayer.battery.settings.max_user_set_charge_dA) {
    datalayer.battery.status.max_charge_current_dA = datalayer.battery.settings.max_user_set_charge_dA;
  }
  if (datalayer.battery.status.max_discharge_current_dA > datalayer.battery.settings.max_user_set_discharge_dA) {
    datalayer.battery.status.max_discharge_current_dA = datalayer.battery.settings.max_user_set_discharge_dA;
  }
  /* Calculate active power based on voltage and current*/
  datalayer.battery.status.active_power_W =
      (datalayer.battery.status.current_dA * (datalayer.battery.status.voltage_dV / 100));

  if (datalayer.battery.settings.soc_scaling_active) {
    /** SOC Scaling
     * 
     * This is essentially a more static version of a stochastic oscillator (https://en.wikipedia.org/wiki/Stochastic_oscillator)
     * 
     * The idea is this:
     * 
     *    real_soc - min_percent                   3000 - 1000
     * ------------------------- = scaled_soc, or  ----------- = 0.25
     * max_percent - min-percent                   8000 - 1000
     * 
     * Because we use integers, we want to account for the scaling:
     * 
     * 10000 * (real_soc - min_percent)                   10000 * (3000 - 1000)
     * -------------------------------- = scaled_soc, or  --------------------- = 2500
     *     max_percent - min_percent                           8000 - 1000
     * 
     * Or as a one-liner: (10000 * (real_soc - min_percentage)) / (max_percentage - min_percentage)
     * 
     * Before we use real_soc, we must make sure that it's within the range of min_percentage and max_percentage.
    */
    uint32_t calc_soc;
    uint32_t calc_max_capacity;
    uint32_t calc_reserved_capacity;
    // Make sure that the SOC starts out between min and max percentages
    calc_soc = CONSTRAIN(datalayer.battery.status.real_soc, datalayer.battery.settings.min_percentage,
                         datalayer.battery.settings.max_percentage);
    // Perform scaling
    calc_soc = 10000 * (calc_soc - datalayer.battery.settings.min_percentage);
    calc_soc = calc_soc / (datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage);
    datalayer.battery.status.reported_soc = calc_soc;

    // Calculate the scaled remaining capacity in Wh
    if (datalayer.battery.info.total_capacity_Wh > 0 && datalayer.battery.status.real_soc > 0) {
      calc_max_capacity = (datalayer.battery.status.remaining_capacity_Wh * 10000 / datalayer.battery.status.real_soc);
      calc_reserved_capacity = calc_max_capacity * datalayer.battery.settings.min_percentage / 10000;
      // remove % capacity reserved in min_percentage to total_capacity_Wh
      if (datalayer.battery.status.remaining_capacity_Wh > calc_reserved_capacity) {
        datalayer.battery.status.reported_remaining_capacity_Wh =
            datalayer.battery.status.remaining_capacity_Wh - calc_reserved_capacity;
      } else {
        datalayer.battery.status.reported_remaining_capacity_Wh = 0;
      }

    } else {
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    }

#ifdef DOUBLE_BATTERY
    /* Calculate active power based on voltage and current*/
    datalayer.battery2.status.active_power_W =
        (datalayer.battery2.status.current_dA * (datalayer.battery2.status.voltage_dV / 100));

    // Calculate the scaled remaining capacity in Wh
    if (datalayer.battery2.info.total_capacity_Wh > 0 && datalayer.battery2.status.real_soc > 0) {
      calc_max_capacity =
          (datalayer.battery2.status.remaining_capacity_Wh * 10000 / datalayer.battery2.status.real_soc);
      calc_reserved_capacity = calc_max_capacity * datalayer.battery2.settings.min_percentage / 10000;
      // remove % capacity reserved in min_percentage to total_capacity_Wh
      if (datalayer.battery2.status.remaining_capacity_Wh > calc_reserved_capacity) {
        datalayer.battery2.status.reported_remaining_capacity_Wh =
            datalayer.battery2.status.remaining_capacity_Wh - calc_reserved_capacity;
      } else {
        datalayer.battery2.status.reported_remaining_capacity_Wh = 0;
      }
    } else {
      datalayer.battery2.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
    }
#endif

  } else {  // soc_scaling_active == false. No SOC window wanted. Set scaled to same as real.
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
#ifdef DOUBLE_BATTERY
    datalayer.battery2.status.reported_soc = datalayer.battery2.status.real_soc;
    datalayer.battery2.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
#endif
  }
#ifdef DOUBLE_BATTERY
  // Perform extra SOC sanity checks on double battery setups
  if (datalayer.battery.status.real_soc < 100) {  //If this battery is under 1.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
  }
  if (datalayer.battery2.status.real_soc < 100) {  //If this battery is under 1.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
  }

  if (datalayer.battery.status.real_soc > 9900) {  //If this battery is over 99.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
  }
  if (datalayer.battery2.status.real_soc > 9900) {  //If this battery is over 99.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
  }
#endif  //DOUBLE_BATTERY
}

void update_values_inverter() {
#ifdef CAN_INVERTER_SELECTED
  update_values_can_inverter();
#endif
#ifdef MODBUS_INVERTER_SELECTED
  update_modbus_registers_inverter();
#endif
#ifdef RS485_INVERTER_SELECTED
  update_RS485_registers_inverter();
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
  Serial2.begin(SERIAL_LINK_BAUDRATE, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
#endif
}

void store_settings_equipment_stop() {
  settings.begin("batterySettings", false);
  settings.putBool("EQUIPMENT_STOP", datalayer.system.settings.equipment_stop_active);
  settings.end();
}

void storeSettings() {
  settings.begin("batterySettings", false);
#ifdef WIFI
  settings.putString("SSID", String(ssid.c_str()));
  settings.putString("PASSWORD", String(password.c_str()));
#endif
  settings.putUInt("BATTERY_WH_MAX", datalayer.battery.info.total_capacity_Wh);
  settings.putUInt("MAXPERCENTAGE",
                   datalayer.battery.settings.max_percentage / 10);  // Divide by 10 for backwards compatibility
  settings.putUInt("MINPERCENTAGE",
                   datalayer.battery.settings.min_percentage / 10);  // Divide by 10 for backwards compatibility
  settings.putUInt("MAXCHARGEAMP", datalayer.battery.settings.max_user_set_charge_dA);
  settings.putUInt("MAXDISCHARGEAMP", datalayer.battery.settings.max_user_set_discharge_dA);
  settings.putBool("USE_SCALED_SOC", datalayer.battery.settings.soc_scaling_active);
  settings.end();
}

/** Reset reason numbering and description
 * 
 typedef enum {
  ESP_RST_UNKNOWN,    //!< 0  Reset reason can not be determined
  ESP_RST_POWERON,    //!< 1  OK Reset due to power-on event
  ESP_RST_EXT,        //!< 2  Reset by external pin (not applicable for ESP32)
  ESP_RST_SW,         //!< 3  OK Software reset via esp_restart
  ESP_RST_PANIC,      //!< 4  Software reset due to exception/panic
  ESP_RST_INT_WDT,    //!< 5  Reset (software or hardware) due to interrupt watchdog
  ESP_RST_TASK_WDT,   //!< 6  Reset due to task watchdog
  ESP_RST_WDT,        //!< 7  Reset due to other watchdogs
  ESP_RST_DEEPSLEEP,  //!< 8  Reset after exiting deep sleep mode
  ESP_RST_BROWNOUT,   //!< 9  Brownout reset (software or hardware)
  ESP_RST_SDIO,       //!< 10 Reset over SDIO
  ESP_RST_USB,        //!< 11 Reset by USB peripheral
  ESP_RST_JTAG,       //!< 12 Reset by JTAG
  ESP_RST_EFUSE,      //!< 13 Reset due to efuse error
  ESP_RST_PWR_GLITCH, //!< 14 Reset due to power glitch detected
  ESP_RST_CPU_LOCKUP, //!< 15 Reset due to CPU lock up
} esp_reset_reason_t;
*/
void check_reset_reason() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_UNKNOWN:
      set_event(EVENT_RESET_UNKNOWN, reason);
      break;
    case ESP_RST_POWERON:
      set_event(EVENT_RESET_POWERON, reason);
      break;
    case ESP_RST_EXT:
      set_event(EVENT_RESET_EXT, reason);
      break;
    case ESP_RST_SW:
      set_event(EVENT_RESET_SW, reason);
      break;
    case ESP_RST_PANIC:
      set_event(EVENT_RESET_PANIC, reason);
      break;
    case ESP_RST_INT_WDT:
      set_event(EVENT_RESET_INT_WDT, reason);
      break;
    case ESP_RST_TASK_WDT:
      set_event(EVENT_RESET_TASK_WDT, reason);
      break;
    case ESP_RST_WDT:
      set_event(EVENT_RESET_WDT, reason);
      break;
    case ESP_RST_DEEPSLEEP:
      set_event(EVENT_RESET_DEEPSLEEP, reason);
      break;
    case ESP_RST_BROWNOUT:
      set_event(EVENT_RESET_BROWNOUT, reason);
      break;
    case ESP_RST_SDIO:
      set_event(EVENT_RESET_SDIO, reason);
      break;
    case ESP_RST_USB:
      set_event(EVENT_RESET_USB, reason);
      break;
    case ESP_RST_JTAG:
      set_event(EVENT_RESET_JTAG, reason);
      break;
    case ESP_RST_EFUSE:
      set_event(EVENT_RESET_EFUSE, reason);
      break;
    case ESP_RST_PWR_GLITCH:
      set_event(EVENT_RESET_PWR_GLITCH, reason);
      break;
    case ESP_RST_CPU_LOCKUP:
      set_event(EVENT_RESET_CPU_LOCKUP, reason);
      break;
    default:
      break;
  }
}

void transmit_can(CAN_frame* tx_frame, int interface) {
  if (!allowed_to_send_CAN) {
    return;
  }
  print_can_frame(*tx_frame, frameDirection(MSG_TX));

  switch (interface) {
    case CAN_NATIVE:
      CAN_frame_t frame;
      frame.MsgID = tx_frame->ID;
      frame.FIR.B.FF = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      frame.FIR.B.DLC = tx_frame->DLC;
      frame.FIR.B.RTR = CAN_no_RTR;
      for (uint8_t i = 0; i < tx_frame->DLC; i++) {
        frame.data.u8[i] = tx_frame->data.u8[i];
      }
      ESP32Can.CANWriteFrame(&frame);
      break;
    case CAN_ADDON_MCP2515: {
#ifdef DUAL_CAN
      //Struct with ACAN2515 library format, needed to use the MCP2515 library for CAN2
      CANMessage MCP2515Frame;
      MCP2515Frame.id = tx_frame->ID;
      MCP2515Frame.ext = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      MCP2515Frame.len = tx_frame->DLC;
      MCP2515Frame.rtr = false;
      for (uint8_t i = 0; i < MCP2515Frame.len; i++) {
        MCP2515Frame.data[i] = tx_frame->data.u8[i];
      }
      can.tryToSend(MCP2515Frame);
#else   // Interface not compiled, and settings try to use it
      set_event(EVENT_INTERFACE_MISSING, interface);
#endif  //DUAL_CAN
    } break;
    case CANFD_NATIVE:
    case CAN_ADDON_FD_MCP2518: {
#ifdef CAN_FD
      CANFDMessage MCP2518Frame;
      if (tx_frame->FD) {
        MCP2518Frame.type = CANFDMessage::CANFD_WITH_BIT_RATE_SWITCH;
      } else {  //Classic CAN message
        MCP2518Frame.type = CANFDMessage::CAN_DATA;
      }
      MCP2518Frame.id = tx_frame->ID;
      MCP2518Frame.ext = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      MCP2518Frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < MCP2518Frame.len; i++) {
        MCP2518Frame.data[i] = tx_frame->data.u8[i];
      }
      send_ok = canfd.tryToSend(MCP2518Frame);
      if (!send_ok) {
        set_event(EVENT_CANFD_BUFFER_FULL, interface);
      }
#else   // Interface not compiled, and settings try to use it
      set_event(EVENT_INTERFACE_MISSING, interface);
#endif  //CAN_FD
    } break;
    default:
      // Invalid interface sent with function call. TODO: Raise event that coders messed up
      break;
  }
}
void receive_can(CAN_frame* rx_frame, int interface) {

  print_can_frame(*rx_frame, frameDirection(MSG_RX));

  if (interface == can_config.battery) {
    receive_can_battery(*rx_frame);
#ifdef CHADEMO_BATTERY
    ISA_handleFrame(rx_frame);
#endif
  }
  if (interface == can_config.inverter) {
#ifdef CAN_INVERTER_SELECTED
    receive_can_inverter(*rx_frame);
#endif
  }
  if (interface == can_config.battery_double) {
#ifdef DOUBLE_BATTERY
    receive_can_battery2(*rx_frame);
#endif
  }
  if (interface == can_config.charger) {
#ifdef CHARGER_SELECTED
    receive_can_charger(*rx_frame);
#endif
  }
}
