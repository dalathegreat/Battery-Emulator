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

#include "src/datalayer/datalayer.h"

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

Preferences settings;  // Store user settings
// The current software version, shown on webserver
const char* version_number = "7.1.0";

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
#else
typedef char CANFDMessage;
#endif

// ModbusRTU parameters
#ifdef MODBUS_INVERTER_SELECTED
#define MB_RTU_NUM_VALUES 30000
uint16_t mbPV[MB_RTU_NUM_VALUES];  // Process variable memory
// Create a ModbusRTU server instance listening on Serial2 with 2000ms timeout
ModbusServerRTU MBserver(Serial2, 2000);
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

// Contactor parameters
#ifdef CONTACTOR_CONTROL
enum State { DISCONNECTED, PRECHARGE, NEGATIVE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

#define MAX_ALLOWED_FAULT_TICKS 1000
/* NOTE: modify the precharge time constant below to account for the resistance and capacitance of the target system.
 *	t=3RC at minimum, t=5RC ideally 
 */
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

TaskHandle_t main_loop_task;
TaskHandle_t connectivity_loop_task;

// Initialization
void setup() {
  init_serial();

  init_stored_settings();

#ifdef WEBSERVER
  xTaskCreatePinnedToCore((TaskFunction_t)&connectivity_loop, "connectivity_loop", 4096, &connectivity_task_time_us,
                          TASK_CONNECTIVITY_PRIO, &connectivity_loop_task, WIFI_CORE);
#endif

  init_events();

  init_CAN();

  init_contactors();

  init_rs485();

  init_serialDataLink();

  init_inverter();

  init_battery();

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

#ifdef WEBSERVER
void connectivity_loop(void* task_time_us) {
  // Init
  init_webserver();
#ifdef MDNSRESPONDER
  init_mDNS();
#endif
#ifdef MQTT
  init_mqtt();
#endif

  while (true) {
    START_TIME_MEASUREMENT(wifi);
    wifi_monitor();
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
    // Input, Runs as fast as possible
    receive_can_native();  // Receive CAN messages from native CAN port
#ifdef CAN_FD
    receive_canfd();  // Receive CAN-FD messages.
#endif
#ifdef DUAL_CAN
    receive_can_addonMCP2515();  // Receive CAN messages on add-on MCP2515 chip
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
#ifdef DOUBLE_BATTERY
      check_interconnect_available();
#endif
    }
    END_TIME_MEASUREMENT_MAX(time_10ms, datalayer.system.status.time_10ms_us);

    START_TIME_MEASUREMENT(time_5s);
    if (millis() - previousMillisUpdateVal >= intervalUpdateValues)  // Every 5s normally
    {
      previousMillisUpdateVal = millis();  // Order matters on the update_loop!
      update_values_battery();             // Fetch battery values
#ifdef DOUBLE_BATTERY
      update_values_battery2();
#endif
      update_SOC();  // Check if real or calculated SOC% value should be sent
#ifndef SERIAL_LINK_RECEIVER
      update_machineryprotection();  // Check safeties (Not on serial link reciever board)
#endif
      update_values_inverter();  // Update values heading towards inverter
      if (DUMMY_EVENT_ENABLED) {
        set_event(EVENT_DUMMY_ERROR, (uint8_t)millis());
      }
    }
    END_TIME_MEASUREMENT_MAX(time_5s, datalayer.system.status.time_5s_us);

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
      datalayer.system.status.time_snap_5s_us = datalayer.system.status.time_5s_us;
      datalayer.system.status.time_snap_cantx_us = datalayer.system.status.time_cantx_us;
      datalayer.system.status.time_snap_ota_us = datalayer.system.status.time_ota_us;
    }

    datalayer.system.status.core_task_max_us =
        MAX(datalayer.system.status.core_task_10s_max_us, datalayer.system.status.core_task_max_us);
    if (core_task_timer_10s.elapsed()) {
      datalayer.system.status.time_ota_us = 0;
      datalayer.system.status.time_comm_us = 0;
      datalayer.system.status.time_10ms_us = 0;
      datalayer.system.status.time_5s_us = 0;
      datalayer.system.status.time_cantx_us = 0;
      datalayer.system.status.core_task_10s_max_us = 0;
    }
#endif
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

#ifdef MDNSRESPONDER
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
#endif  // MDNSRESPONDER

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

  static uint32_t temp = 0;
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
    datalayer.battery.info.max_charge_amp_dA = temp;
  }
  temp = settings.getUInt("MAXDISCHARGEAMP", false);
  if (temp != 0) {
    datalayer.battery.info.max_discharge_amp_dA = temp;
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
  can.begin(settings, [] { can.isr(); });
#endif

#ifdef CAN_FD
#ifdef DEBUG_VIA_USB
  Serial.println("CAN FD add-on (ESP32+MCP2517) selected");
#endif
  SPI.begin(MCP2517_SCK, MCP2517_SDO, MCP2517_SDI);
  ACAN2517FDSettings settings(ACAN2517FDSettings::OSC_40MHz, 500 * 1000,
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
  pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
  digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
  pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
  digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
#ifdef PWM_CONTACTOR_CONTROL
  ledcAttachChannel(POSITIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res,
                    POSITIVE_PWM_Ch);  // Setup PWM Channel Frequency and Resolution
  ledcAttachChannel(NEGATIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res,
                    NEGATIVE_PWM_Ch);  // Setup PWM Channel Frequency and Resolution
  ledcWrite(POSITIVE_PWM_Ch, 0);       // Set Positive PWM to 0%
  ledcWrite(NEGATIVE_PWM_Ch, 0);       // Set Negative PWM to 0%
#endif
  pinMode(PRECHARGE_PIN, OUTPUT);
  digitalWrite(PRECHARGE_PIN, LOW);
#endif
// Init BMS contactor
#ifdef HW_STARK  // TODO: Rewrite this so LilyGo can aslo handle this BMS contactor
  pinMode(BMS_POWER, OUTPUT);
  digitalWrite(BMS_POWER, HIGH);
#endif
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

void init_inverter() {
#ifdef SOLAX_CAN
  datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  intervalUpdateValues = 800;  // This protocol also requires the values to be updated faster
#endif
#ifdef BYD_SMA
  datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  pinMode(INVERTER_CONTACTOR_ENABLE_PIN, INPUT);
#endif
}

void init_battery() {
  // Inform user what battery is used and perform setup
  setup_battery();

#ifdef CHADEMO_BATTERY
  intervalUpdateValues = 800;  // This mode requires the values to be updated faster
#endif
}

#ifdef CAN_FD
// Functions
#ifdef DEBUG_CANFD_DATA
void print_canfd_frame(CANFDMessage rx_frame) {
  int i = 0;
  Serial.print(rx_frame.id, HEX);
  Serial.print(" ");
  for (i = 0; i < rx_frame.len; i++) {
    Serial.print(rx_frame.data[i] < 16 ? "0" : "");
    Serial.print(rx_frame.data[i], HEX);
    Serial.print(" ");
  }
  Serial.println(" ");
}
#endif
void receive_canfd() {  // This section checks if we have a complete CAN-FD message incoming
  CANFDMessage frame;
  if (canfd.available()) {
    canfd.receive(frame);
#ifdef DEBUG_CANFD_DATA
    print_canfd_frame(frame);
#endif
    CAN_frame rx_frame;
    rx_frame.ID = frame.id;
    rx_frame.ext_ID = frame.ext;
    rx_frame.DLC = frame.len;
    for (uint8_t i = 0; i < rx_frame.DLC && i < 64; i++) {
      rx_frame.data.u8[i] = frame.data[i];
    }
    //message incoming, pass it on to the handler
    receive_can(&rx_frame, CAN_ADDON_FD_MCP2518);
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

  if (abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV) < 30) {  // If we are within 3.0V
    clear_event(EVENT_VOLTAGE_DIFFERENCE);
    if (datalayer.battery.status.bms_status != FAULT) {  // Only proceed if we are not in faulted state
      datalayer.system.status.battery2_allows_contactor_closing = true;
    }
  } else {  //We are over 3.0V diff
    set_event(EVENT_VOLTAGE_DIFFERENCE,
              (uint8_t)(abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV) / 10));
  }
}
#endif  //DOUBLE_BATTERY

void handle_contactors() {

#ifdef BYD_SMA
  datalayer.system.status.inverter_allows_contactor_closing = digitalRead(INVERTER_CONTACTOR_ENABLE_PIN);
#endif

#ifdef CONTACTOR_CONTROL

  // First check if we have any active errors, incase we do, turn off the battery
  if (datalayer.battery.status.bms_status == FAULT) {
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
    set_event(EVENT_ERROR_OPEN_CONTACTOR, 0);
    return;  // A fault scenario latches the contactor control. It is not possible to recover without a powercycle (and investigation why fault occured)
  }

  // After that, check if we are OK to start turning on the battery
  if (contactorStatus == DISCONNECTED) {
    digitalWrite(PRECHARGE_PIN, LOW);
    digitalWrite(NEGATIVE_CONTACTOR_PIN, LOW);
    digitalWrite(POSITIVE_CONTACTOR_PIN, LOW);
#ifdef PWM_CONTACTOR_CONTROL
    ledcWrite(POSITIVE_PWM_Ch, 0);
    ledcWrite(NEGATIVE_PWM_Ch, 0);
#endif

    if (datalayer.system.status.battery_allows_contactor_closing &&
        datalayer.system.status.inverter_allows_contactor_closing) {
      contactorStatus = PRECHARGE;
    }
  }

  // In case the inverter requests contactors to open, set the state accordingly
  if (contactorStatus == COMPLETED) {
    if (!datalayer.system.status.inverter_allows_contactor_closing)
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
#endif  // CONTACTOR_CONTROL
}

void update_SOC() {
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
    // Make sure that the SOC starts out between min and max percentages
    calc_soc = CONSTRAIN(datalayer.battery.status.real_soc, datalayer.battery.settings.min_percentage,
                         datalayer.battery.settings.max_percentage);
    // Perform scaling
    calc_soc = 10000 * (calc_soc - datalayer.battery.settings.min_percentage);
    calc_soc = calc_soc / (datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage);
    datalayer.battery.status.reported_soc = calc_soc;
  } else {  // No SOC window wanted. Set scaled to same as real.
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
  }
#ifdef DOUBLE_BATTERY
  // Perform extra SOC sanity checks on double battery setups
  if (datalayer.battery.status.real_soc < 100) {  //If this battery is under 1.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
  }
  if (datalayer.battery2.status.real_soc < 100) {  //If this battery is under 1.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
  }

  if (datalayer.battery.status.real_soc > 9900) {  //If this battery is over 99.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
  }
  if (datalayer.battery2.status.real_soc > 9900) {  //If this battery is over 99.00%, use this as SOC instead of average
    datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
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
  settings.putString("SSID", String(ssid.c_str()));
  settings.putString("PASSWORD", String(password.c_str()));
  settings.putUInt("BATTERY_WH_MAX", datalayer.battery.info.total_capacity_Wh);
  settings.putUInt("MAXPERCENTAGE",
                   datalayer.battery.settings.max_percentage / 10);  // Divide by 10 for backwards compatibility
  settings.putUInt("MINPERCENTAGE",
                   datalayer.battery.settings.min_percentage / 10);  // Divide by 10 for backwards compatibility
  settings.putUInt("MAXCHARGEAMP", datalayer.battery.info.max_charge_amp_dA);
  settings.putUInt("MAXDISCHARGEAMP", datalayer.battery.info.max_discharge_amp_dA);
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
      MCP2518Frame.id = tx_frame->ID;
      MCP2518Frame.ext = tx_frame->ext_ID ? CAN_frame_ext : CAN_frame_std;
      MCP2518Frame.len = tx_frame->DLC;
      for (uint8_t i = 0; i < MCP2518Frame.len; i++) {
        MCP2518Frame.data[i] = tx_frame->data.u8[i];
      }
      canfd.tryToSend(MCP2518Frame);
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

  if (interface == can_config.battery) {
    receive_can_battery(*rx_frame);
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
