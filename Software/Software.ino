/* Do not change any code below this line unless you are sure what you are doing */
/* Only change battery specific settings in "USER_SETTINGS.h" */
#include "HardwareSerial.h"
#include "USER_SECRETS.h"
#include "USER_SETTINGS.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/communication/can/comm_can.h"
#include "src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "src/communication/nvm/comm_nvm.h"
#include "src/communication/precharge_control/precharge_control.h"
#include "src/communication/rs485/comm_rs485.h"
#include "src/communication/seriallink/comm_seriallink.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/led_handler.h"
#include "src/devboard/utils/logging.h"
#include "src/devboard/utils/timer.h"
#include "src/devboard/utils/value_mapping.h"
#include "src/include.h"
#ifndef AP_PASSWORD
#error \
    "Initial setup not completed, USER_SECRETS.h is missing. Please rename the file USER_SECRETS.TEMPLATE.h to USER_SECRETS.h and fill in the required credentials. This file is ignored by version control to keep sensitive information private."
#endif
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

// The current software version, shown on webserver
const char* version_number = "8.6.dev";

// Interval settings
uint16_t intervalUpdateValues = INTERVAL_1_S;  // Interval at which to update inverter values / Modbus registers
unsigned long previousMillis10ms = 0;
unsigned long previousMillisUpdateVal = 0;

// Task time measurement for debugging and for setting CPU load events
int64_t core_task_time_us;
MyTimer core_task_timer_10s(INTERVAL_10_S);

int64_t connectivity_task_time_us;
MyTimer connectivity_task_timer_10s(INTERVAL_10_S);

int64_t logging_task_time_us;
MyTimer logging_task_timer_10s(INTERVAL_10_S);

MyTimer loop_task_timer_10s(INTERVAL_10_S);

MyTimer check_pause_2s(INTERVAL_2_S);

TaskHandle_t main_loop_task;
TaskHandle_t connectivity_loop_task;
TaskHandle_t logging_loop_task;

Logging logging;

// Initialization
void setup() {
  init_serial();

  // We print this after setting up serial, such that is also printed to serial with DEBUG_VIA_USB set.
  logging.printf("Battery emulator %s build " __DATE__ " " __TIME__ "\n", version_number);

  init_stored_settings();

#ifdef WIFI
  xTaskCreatePinnedToCore((TaskFunction_t)&connectivity_loop, "connectivity_loop", 4096, &connectivity_task_time_us,
                          TASK_CONNECTIVITY_PRIO, &connectivity_loop_task, WIFI_CORE);
#endif

#if defined(LOG_CAN_TO_SD) || defined(LOG_TO_SD)
  xTaskCreatePinnedToCore((TaskFunction_t)&logging_loop, "logging_loop", 4096, &logging_task_time_us,
                          TASK_CONNECTIVITY_PRIO, &logging_loop_task, WIFI_CORE);
#endif

  init_events();

  init_CAN();

  init_contactors();

#ifdef PRECHARGE_CONTROL
  init_precharge_control();
#endif  // PRECHARGE_CONTROL

  init_rs485();

  init_serialDataLink();
#if defined(CAN_INVERTER_SELECTED) || defined(MODBUS_INVERTER_SELECTED) || defined(RS485_INVERTER_SELECTED)
  setup_inverter();
#endif
  setup_battery();
#ifdef EQUIPMENT_STOP_BUTTON
  init_equipment_stop_button();
#endif
#ifdef CAN_SHUNT_SELECTED
  setup_can_shunt();
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

#if defined(LOG_CAN_TO_SD) || defined(LOG_TO_SD)
void logging_loop(void* task_time_us) {

  init_logging_buffers();
  init_sdcard();

  while (true) {
#ifdef LOG_TO_SD
    write_log_to_sdcard();
#endif
#ifdef LOG_CAN_TO_SD
    write_can_frame_to_sdcard();
#endif
  }
}
#endif

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
    receive_can();  // Receive CAN messages
#ifdef RS485_INVERTER_SELECTED
    receive_RS485();  // Process serial2 RS485 interface
#endif                // RS485_INVERTER_SELECTED
#if defined(SERIAL_LINK_RECEIVER) || defined(SERIAL_LINK_TRANSMITTER)
    run_serialDataLink();
#endif  // SERIAL_LINK_RECEIVER || SERIAL_LINK_TRANSMITTER
    END_TIME_MEASUREMENT_MAX(comm, datalayer.system.status.time_comm_us);
#ifdef WEBSERVER
    START_TIME_MEASUREMENT(ota);
    ElegantOTA.loop();
    END_TIME_MEASUREMENT_MAX(ota, datalayer.system.status.time_ota_us);
#endif  // WEBSERVER

    START_TIME_MEASUREMENT(time_10ms);
    // Process
    if (millis() - previousMillis10ms >= INTERVAL_10_MS) {
      previousMillis10ms = millis();
      led_exe();
      handle_contactors();  // Take care of startup precharge/contactor closing
#ifdef PRECHARGE_CONTROL
      handle_precharge_control();
#endif  // PRECHARGE_CONTROL
    }
    END_TIME_MEASUREMENT_MAX(time_10ms, datalayer.system.status.time_10ms_us);

    START_TIME_MEASUREMENT(time_values);
    if (millis() - previousMillisUpdateVal >= intervalUpdateValues) {
      previousMillisUpdateVal = millis();  // Order matters on the update_loop!
      update_values_battery();             // Fetch battery values
#ifdef DOUBLE_BATTERY
      update_values_battery2();
      check_interconnect_available();
#endif  // DOUBLE_BATTERY
      update_calculated_values();
#ifndef SERIAL_LINK_RECEIVER
      update_machineryprotection();  // Check safeties (Not on serial link reciever board)
#endif                               // SERIAL_LINK_RECEIVER
      update_values_inverter();      // Update values heading towards inverter
    }
    END_TIME_MEASUREMENT_MAX(time_values, datalayer.system.status.time_values_us);

    START_TIME_MEASUREMENT(cantx);
    // Output
    transmit_can();  // Send CAN messages to all components

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
#endif  // FUNCTION_TIME_MEASUREMENT
    if (check_pause_2s.elapsed()) {
      emulator_pause_state_transmit_can_battery();
    }
#ifdef DEBUG_LOG
    logging.log_bms_status(datalayer.battery.status.real_bms_status, 1);
#endif

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
#endif  // DEBUG_VIA_USB
}

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
#endif  // DOUBLE_BATTERY

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

#ifdef DOUBLE_BATTERY
  /* Calculate active power based on voltage and current for battery 2*/
  datalayer.battery2.status.active_power_W =
      (datalayer.battery2.status.current_dA * (datalayer.battery2.status.voltage_dV / 100));
#endif  // DOUBLE_BATTERY

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
#endif  // DOUBLE_BATTERY

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
#endif  // DOUBLE_BATTERY
}

void update_values_inverter() {
#ifdef CAN_INVERTER_SELECTED
  update_values_can_inverter();
#endif  // CAN_INVERTER_SELECTED
#ifdef MODBUS_INVERTER_SELECTED
  update_modbus_registers_inverter();
#endif  // CAN_INVERTER_SELECTED
#ifdef RS485_INVERTER_SELECTED
  update_RS485_registers_inverter();
#endif  // CAN_INVERTER_SELECTED
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
