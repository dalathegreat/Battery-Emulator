#include <Arduino.h>
#include "HardwareSerial.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "src/battery/BATTERIES.h"
#include "src/charger/CHARGERS.h"
#include "src/communication/Transmitter.h"
#include "src/communication/can/comm_can.h"
#include "src/communication/contactorcontrol/comm_contactorcontrol.h"
#include "src/communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "src/communication/nvm/comm_nvm.h"
#include "src/communication/precharge_control/precharge_control.h"
#include "src/communication/rs485/comm_rs485.h"
#include "src/datalayer/datalayer.h"
#include "src/devboard/mqtt/mqtt.h"
#include "src/devboard/sdcard/sdcard.h"
#include "src/devboard/utils/events.h"
#include "src/devboard/utils/led_handler.h"
#include "src/devboard/utils/logging.h"
#include "src/devboard/utils/time_meas.h"
#include "src/devboard/utils/timer.h"
#include "src/devboard/utils/types.h"
#include "src/devboard/utils/value_mapping.h"
#include "src/devboard/webserver/webserver.h"
#include "src/devboard/wifi/wifi.h"
#include "src/inverter/INVERTERS.h"

#if !defined(HW_LILYGO) && !defined(HW_LILYGO2CAN) && !defined(HW_STARK) && !defined(HW_3LB) && !defined(HW_DEVKIT)
#error You must select a target hardware!
#endif

// The current software version, shown on webserver
const char* version_number = "9.2.dev";

// Interval timers
volatile unsigned long currentMillis = 0;
unsigned long previousMillis10ms = 0;
unsigned long previousMillisUpdateVal = 0;
// Task time measurement for debugging
MyTimer core_task_timer_10s(INTERVAL_10_S);
uint64_t start_time_10ms = 0;
uint64_t start_time_values = 0;
uint64_t start_time_cantx = 0;
TaskHandle_t main_loop_task;
TaskHandle_t connectivity_loop_task;
TaskHandle_t logging_loop_task;
TaskHandle_t mqtt_loop_task;

Logging logging;

std::string mqtt_user;      //TODO, move?
std::string mqtt_password;  //TODO, move?
std::string http_username;  //TODO, move?
std::string http_password;  //TODO, move?

static std::list<Transmitter*> transmitters;
void register_transmitter(Transmitter* transmitter) {
  transmitters.push_back(transmitter);
  DEBUG_PRINTF("transmitter registered, total: %d\n", transmitters.size());
}

// Initialization functions
void init_serial() {
  // Init Serial monitor
  Serial.begin(115200);
#if HW_LILYGO2CAN
  // Wait up to 100ms for Serial to be available. On the ESP32S3 Serial is
  // provided by the USB controller, so will only work if the board is connected
  // to a computer.
  for (int i = 0; i < 10; i++) {
    if (Serial)
      break;
    delay(10);
  }
#else
  while (!Serial) {}
#endif
}

void connectivity_loop(void*) {
  esp_task_wdt_add(NULL);  // Register this task with WDT
  // Init wifi
  init_WiFi();

  init_webserver();

  if (mdns_enabled) {
    init_mDNS();
  }

  while (true) {
    START_TIME_MEASUREMENT(wifi);
    wifi_monitor();

    ota_monitor();

    END_TIME_MEASUREMENT_MAX(wifi, datalayer.system.status.wifi_task_10s_max_us);

    esp_task_wdt_reset();  // Reset watchdog
    delay(1);
  }
}

void logging_loop(void*) {

  init_logging_buffers();
  init_sdcard();

  while (true) {
    if (datalayer.system.info.SD_logging_active) {
      write_log_to_sdcard();
    }

    if (datalayer.system.info.CAN_SD_logging_active) {
      write_can_frame_to_sdcard();
    }
  }
}

void check_interconnect_available() {
  if (datalayer.battery.status.voltage_dV == 0 || datalayer.battery2.status.voltage_dV == 0) {
    return;  // Both voltage values need to be available to start check
  }

  uint16_t voltage_diff = abs(datalayer.battery.status.voltage_dV - datalayer.battery2.status.voltage_dV);

  if (voltage_diff <= 30) {  // If we are within 3.0V between the batteries
    clear_event(EVENT_VOLTAGE_DIFFERENCE);
    if (datalayer.battery.status.bms_status == FAULT) {
      // If main battery is in fault state, disengage the second battery
      datalayer.system.status.battery2_allowed_contactor_closing = false;
    } else {  // If main battery is OK, allow second battery to join
      datalayer.system.status.battery2_allowed_contactor_closing = true;
    }
  } else {  //Voltage between the two packs is too large
    set_event(EVENT_VOLTAGE_DIFFERENCE, (uint8_t)(voltage_diff / 10));
  }
}

void update_calculated_values(unsigned long currentMillis) {
  /* Update CPU temperature*/
  union {
    float temp;
    uint32_t hex;
  } temp = {.temp = temperatureRead()};
  if (temp.hex != 0x42555555) {
    // Ignoring erroneous temperature value that ESP32 sometimes returns
    datalayer.system.info.CPU_temperature = temp.temp;
  }

  /* Check is remote set limits have timed out */
  if (currentMillis > datalayer.battery.settings.remote_set_timestamp + datalayer.battery.settings.remote_set_timeout) {
    datalayer.battery.settings.remote_settings_limit_charge = false;
    datalayer.battery.settings.remote_settings_limit_discharge = false;
    datalayer.battery.settings.max_remote_set_charge_dA = 0;
    datalayer.battery.settings.max_remote_set_discharge_dA = 0;
  }

  /* Calculate allowed charge/discharge currents*/
  if (datalayer.battery.status.voltage_dV > 10) {
    // Only update value when we have voltage available to avoid div0. TODO: This should be based on nominal voltage
    datalayer.battery.status.max_charge_current_dA =
        ((datalayer.battery.status.max_charge_power_W * 100) / datalayer.battery.status.voltage_dV);
    datalayer.battery.status.max_discharge_current_dA =
        ((datalayer.battery.status.max_discharge_power_W * 100) / datalayer.battery.status.voltage_dV);
  }

  /* Apply remote restrictions if set*/
  if (datalayer.battery.settings.remote_settings_limit_charge) {
    if (datalayer.battery.status.max_charge_current_dA > datalayer.battery.settings.max_remote_set_charge_dA) {
      datalayer.battery.status.max_charge_current_dA = datalayer.battery.settings.max_remote_set_charge_dA;
    }
  } else {
    /* Restrict values from user settings if needed*/
    if (datalayer.battery.status.max_charge_current_dA > datalayer.battery.settings.max_user_set_charge_dA) {
      datalayer.battery.status.max_charge_current_dA = datalayer.battery.settings.max_user_set_charge_dA;
      datalayer.battery.settings.user_settings_limit_charge = true;
    } else {
      datalayer.battery.settings.user_settings_limit_charge = false;
    }
  }

  /* Apply remote restrictions if set*/
  if (datalayer.battery.settings.remote_settings_limit_discharge) {
    if (datalayer.battery.status.max_discharge_current_dA > datalayer.battery.settings.max_remote_set_charge_dA) {
      datalayer.battery.status.max_discharge_current_dA = datalayer.battery.settings.max_remote_set_discharge_dA;
    }
  } else {
    /* Restrict values from user settings if needed*/
    if (datalayer.battery.status.max_discharge_current_dA > datalayer.battery.settings.max_user_set_discharge_dA) {
      datalayer.battery.status.max_discharge_current_dA = datalayer.battery.settings.max_user_set_discharge_dA;
      datalayer.battery.settings.user_settings_limit_discharge = true;
    } else {
      datalayer.battery.settings.user_settings_limit_discharge = false;
    }
  }

  /* Calculate active power based on voltage and current*/
  datalayer.battery.status.active_power_W =
      (datalayer.battery.status.current_dA * (datalayer.battery.status.voltage_dV / 100));
  /* Calculate if battery or inverter is limiting factor*/

  if (datalayer.battery.status.current_dA == 0) {  //Battery idle
    if (datalayer.battery.status.max_discharge_current_dA > 0) {
      //We allow discharge, but inverter does nothing. Inverter is limiting
      datalayer.battery.settings.inverter_limits_discharge = true;
    } else {
      datalayer.battery.settings.inverter_limits_discharge = false;
    }
    if (datalayer.battery.status.max_charge_current_dA > 0) {
      //We allow charge, but inverter does nothing. Inverter is limiting
      datalayer.battery.settings.inverter_limits_charge = true;
    } else {
      datalayer.battery.settings.inverter_limits_charge = false;
    }
  } else if (datalayer.battery.status.current_dA < 0) {  //Battery discharging
    if (-datalayer.battery.status.current_dA < datalayer.battery.status.max_discharge_current_dA) {
      datalayer.battery.settings.inverter_limits_discharge = true;
    } else {
      datalayer.battery.settings.inverter_limits_discharge = false;
    }
  } else {  // > 0 Battery charging
    //If actual current is smaller than max we allow, inverter is limiting factor
    if (datalayer.battery.status.current_dA < datalayer.battery.status.max_charge_current_dA) {
      datalayer.battery.settings.inverter_limits_charge = true;
    } else {
      datalayer.battery.settings.inverter_limits_charge = false;
    }
  }

  if (battery2) {
    /* Calculate active power based on voltage and current for battery 2*/
    datalayer.battery2.status.active_power_W =
        (datalayer.battery2.status.current_dA * (datalayer.battery2.status.voltage_dV / 100));
  }

  if (datalayer.battery.settings.soc_scaling_active) {
    /** SOC Scaling
   * A static version of a stochastic oscillator. The scaled SoC is calculated as:
   * 
   *     10000 * (real_soc - min_percentage)
   * ---------------------------------------
   *     (max_percentage - min_percentage)
   * 
   * And scaled capacity is:
   * 
   *     reported_total_capacity_Wh = total_capacity_Wh * (max - min) / 10000
   *     reported_remaining_capacity_Wh = reported_total_capacity_Wh * scaled_soc / 10000
   */
    // Compute delta_pct and clamped_soc
    int32_t delta_pct = datalayer.battery.settings.max_percentage - datalayer.battery.settings.min_percentage;
    int32_t clamped_soc = CONSTRAIN(datalayer.battery.status.real_soc, datalayer.battery.settings.min_percentage,
                                    datalayer.battery.settings.max_percentage);
    int32_t scaled_soc = 0;
    int32_t scaled_total_capacity = 0;
    if (delta_pct != 0) {  //Safeguard against division by 0
      scaled_soc = 10000 * (clamped_soc - datalayer.battery.settings.min_percentage) / delta_pct;
    }

    datalayer.battery.status.reported_soc = scaled_soc;

    // If battery info is valid
    if (datalayer.battery.info.total_capacity_Wh > 0 && datalayer.battery.status.real_soc > 0) {
      // Scale total usable capacity
      scaled_total_capacity = (datalayer.battery.info.total_capacity_Wh * delta_pct) / 10000;
      datalayer.battery.info.reported_total_capacity_Wh = scaled_total_capacity;

      // Scale remaining capacity based on scaled SOC
      datalayer.battery.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;

    } else {
      // Fallback if scaling cannot be performed
      datalayer.battery.info.reported_total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    }

    if (battery2) {
      // If battery info is valid
      if (datalayer.battery2.info.total_capacity_Wh > 0 && datalayer.battery.status.real_soc > 0) {

        datalayer.battery2.info.reported_total_capacity_Wh = scaled_total_capacity;
        // Scale remaining capacity based on scaled SOC
        datalayer.battery2.status.reported_remaining_capacity_Wh = (scaled_total_capacity * scaled_soc) / 10000;

      } else {
        // Fallback if scaling cannot be performed
        datalayer.battery2.info.reported_total_capacity_Wh = datalayer.battery2.info.total_capacity_Wh;
        datalayer.battery2.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
      }

      //Since we are running double battery, the scaled value of battery1 becomes the sum of battery1+battery2
      //This way the inverter connected to the system sees both batteries as one large battery
      datalayer.battery.info.reported_total_capacity_Wh += datalayer.battery2.info.reported_total_capacity_Wh;
      datalayer.battery.status.reported_remaining_capacity_Wh +=
          datalayer.battery2.status.reported_remaining_capacity_Wh;
    }

  } else {  // soc_scaling_active == false. No SOC window wanted. Set scaled to same as real.
    datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
    datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    datalayer.battery.info.reported_total_capacity_Wh = datalayer.battery.info.total_capacity_Wh;

    if (battery2) {
      datalayer.battery2.status.reported_soc = datalayer.battery2.status.real_soc;
      datalayer.battery2.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
      datalayer.battery2.info.reported_total_capacity_Wh = datalayer.battery2.info.total_capacity_Wh;
    }
  }

  if (battery2) {
    // Perform extra SOC sanity checks on double battery setups
    if (datalayer.battery.status.real_soc < 100) {  //If this battery is under 1.00%, use this as SOC instead of average
      datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    }
    if (datalayer.battery2.status.real_soc <
        100) {  //If this battery is under 1.00%, use this as SOC instead of average
      datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
    }

    if (datalayer.battery.status.real_soc >
        9900) {  //If this battery is over 99.00%, use this as SOC instead of average
      datalayer.battery.status.reported_soc = datalayer.battery.status.real_soc;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery.status.remaining_capacity_Wh;
    }
    if (datalayer.battery2.status.real_soc >
        9900) {  //If this battery is over 99.00%, use this as SOC instead of average
      datalayer.battery.status.reported_soc = datalayer.battery2.status.real_soc;
      datalayer.battery.status.reported_remaining_capacity_Wh = datalayer.battery2.status.remaining_capacity_Wh;
    }
  }
}

void check_reset_reason() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_UNKNOWN:  //Reset reason can not be determined
      set_event(EVENT_RESET_UNKNOWN, reason);
      break;
    case ESP_RST_POWERON:  //OK Reset due to power-on event
      set_event(EVENT_RESET_POWERON, reason);
      break;
    case ESP_RST_EXT:  //Reset by external pin (not applicable for ESP32)
      set_event(EVENT_RESET_EXT, reason);
      break;
    case ESP_RST_SW:  //OK Software reset via esp_restart
      set_event(EVENT_RESET_SW, reason);
      break;
    case ESP_RST_PANIC:  //Software reset due to exception/panic
      set_event(EVENT_RESET_PANIC, reason);
      break;
    case ESP_RST_INT_WDT:  //Reset (software or hardware) due to interrupt watchdog
      set_event(EVENT_RESET_INT_WDT, reason);
      break;
    case ESP_RST_TASK_WDT:  //Reset due to task watchdog
      set_event(EVENT_RESET_TASK_WDT, reason);
      break;
    case ESP_RST_WDT:  //Reset due to other watchdogs
      set_event(EVENT_RESET_WDT, reason);
      break;
    case ESP_RST_DEEPSLEEP:  //Reset after exiting deep sleep mode
      set_event(EVENT_RESET_DEEPSLEEP, reason);
      break;
    case ESP_RST_BROWNOUT:  //Brownout reset (software or hardware)
      set_event(EVENT_RESET_BROWNOUT, reason);
      break;
    case ESP_RST_SDIO:  //Reset over SDIO
      set_event(EVENT_RESET_SDIO, reason);
      break;
    case ESP_RST_USB:  //Reset by USB peripheral
      set_event(EVENT_RESET_USB, reason);
      break;
    case ESP_RST_JTAG:  //Reset by JTAG
      set_event(EVENT_RESET_JTAG, reason);
      break;
    case ESP_RST_EFUSE:  //Reset due to efuse error
      set_event(EVENT_RESET_EFUSE, reason);
      break;
    case ESP_RST_PWR_GLITCH:  //Reset due to power glitch detected
      set_event(EVENT_RESET_PWR_GLITCH, reason);
      break;
    case ESP_RST_CPU_LOCKUP:  //Reset due to CPU lock up
      set_event(EVENT_RESET_CPU_LOCKUP, reason);
      break;
    default:
      break;
  }
}

void core_loop(void*) {
  esp_task_wdt_add(NULL);  // Register this task with WDT
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(1);  // Convert 1ms to ticks

  while (true) {

    START_TIME_MEASUREMENT(all);
    START_TIME_MEASUREMENT(comm);

    monitor_equipment_stop_button();

    // Input, Runs as fast as possible
    receive_can();    // Receive CAN messages
    receive_rs485();  // Process serial2 RS485 interface

    END_TIME_MEASUREMENT_MAX(comm, datalayer.system.status.time_comm_us);

    START_TIME_MEASUREMENT(ota);
    ElegantOTA.loop();
    END_TIME_MEASUREMENT_MAX(ota, datalayer.system.status.time_ota_us);

    // Process
    currentMillis = millis();
    if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
      if ((currentMillis - previousMillis10ms >= INTERVAL_10_MS_DELAYED) &&
          (milliseconds(currentMillis) > esp32hal->BOOTUP_TIME())) {
        set_event(EVENT_TASK_OVERRUN, (currentMillis - previousMillis10ms));
      }
      previousMillis10ms = currentMillis;
      if (datalayer.system.info.performance_measurement_active) {
        START_TIME_MEASUREMENT(10ms);
      }
      led_exe();
      handle_contactors();  // Take care of startup precharge/contactor closing
      if (precharge_control_enabled) {
        handle_precharge_control(currentMillis);  //Drive the hia4v1 via PWM
      }

      if (datalayer.system.info.performance_measurement_active) {
        END_TIME_MEASUREMENT_MAX(10ms, datalayer.system.status.time_10ms_us);
      }
    }

    if (currentMillis - previousMillisUpdateVal >= INTERVAL_1_S) {
      previousMillisUpdateVal = currentMillis;  // Order matters on the update_loop!
      if (datalayer.system.info.performance_measurement_active) {
        START_TIME_MEASUREMENT(values);
      }
      update_pause_state();  // Check if we are OK to send CAN or need to pause

      // Fetch battery values
      if (battery) {
        battery->update_values();
      }

      if (battery2) {
        battery2->update_values();
        check_interconnect_available();
      }
      update_calculated_values(currentMillis);
      update_machineryprotection();  // Check safeties

      // Update values heading towards inverter
      if (inverter) {
        inverter->update_values();
      }

      if (datalayer.system.info.performance_measurement_active) {
        END_TIME_MEASUREMENT_MAX(values, datalayer.system.status.time_values_us);
      }
    }
    if (datalayer.system.info.performance_measurement_active) {
      START_TIME_MEASUREMENT(cantx);
    }

    // Let all transmitter objects send their messages
    for (auto& transmitter : transmitters) {
      transmitter->transmit(currentMillis);
    }

    if (datalayer.system.info.performance_measurement_active) {
      END_TIME_MEASUREMENT_MAX(cantx, datalayer.system.status.time_cantx_us);
      END_TIME_MEASUREMENT_MAX(all, datalayer.system.status.core_task_10s_max_us);
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
        datalayer.system.status.wifi_task_10s_max_us = 0;
        datalayer.system.status.mqtt_task_10s_max_us = 0;
      }
    }
    esp_task_wdt_reset();  // Reset watchdog to prevent reset
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void mqtt_loop(void*) {
  esp_task_wdt_add(NULL);  // Register this task with WDT

  while (true) {
    START_TIME_MEASUREMENT(mqtt);
    mqtt_client_loop();
    END_TIME_MEASUREMENT_MAX(mqtt, datalayer.system.status.mqtt_task_10s_max_us);
    esp_task_wdt_reset();  // Reset watchdog
    delay(1);
  }
}

// Initialization
void setup() {
  init_hal();

  init_serial();

  // We print this after setting up serial, so that is also printed if configured to do so
  DEBUG_PRINTF("Battery emulator %s build " __DATE__ " " __TIME__ "\n", version_number);

  init_events();

  init_stored_settings();

  if (wifi_enabled) {
    xTaskCreatePinnedToCore((TaskFunction_t)&connectivity_loop, "connectivity_loop", 4096, NULL, TASK_CONNECTIVITY_PRIO,
                            &connectivity_loop_task, esp32hal->WIFICORE());
  }

  led_init();

  if (datalayer.system.info.CAN_SD_logging_active || datalayer.system.info.SD_logging_active) {
    xTaskCreatePinnedToCore((TaskFunction_t)&logging_loop, "logging_loop", 4096, NULL, TASK_CONNECTIVITY_PRIO,
                            &logging_loop_task, esp32hal->WIFICORE());
  }

  init_contactors();

  init_precharge_control();

  setup_charger();
  setup_inverter();
  setup_battery();
  setup_can_shunt();

  // Init CAN only after any CAN receivers have had a chance to register.
  init_CAN();

  init_rs485();

  init_equipment_stop_button();

  // BOOT button at runtime is used as an input for various things
  pinMode(0, INPUT_PULLUP);

  check_reset_reason();

  // Initialize Task Watchdog for subscribed tasks
  esp_task_wdt_config_t wdt_config = {// 5s should be enough for the connectivity tasks (which are all contending
                                      // for the same core) to yield to each other and reset their watchdogs.
                                      .timeout_ms = INTERVAL_5_S,
                                      // We don't benefit from idle task watchdogs, our critical loops have their
                                      // own. The idle watchdogs can cause nuisance reboots under heavy load.
                                      .idle_core_mask = 0,
                                      // Panic (and reboot) on timeout
                                      .trigger_panic = true};
#ifdef CONFIG_ESP_TASK_WDT
  // ESP-IDF will have already initialized it, so reconfigure.
  // Arduino and PlatformIO have different watchdog defaults, so we reconfigure
  // for consistency.
  esp_task_wdt_reconfigure(&wdt_config);
#else
  // Otherwise initialize it for the first time.
  esp_task_wdt_init(&wdt_config);
#endif

  // Start tasks

  if (mqtt_enabled) {
    init_mqtt();

    xTaskCreatePinnedToCore((TaskFunction_t)&mqtt_loop, "mqtt_loop", 4096, NULL, TASK_MQTT_PRIO, &mqtt_loop_task,
                            esp32hal->WIFICORE());
  }

  xTaskCreatePinnedToCore((TaskFunction_t)&core_loop, "core_loop", 4096, NULL, TASK_CORE_PRIO, &main_loop_task,
                          esp32hal->CORE_FUNCTION_CORE());

  DEBUG_PRINTF("Setup complete!\n");
}

// Loop empty, all functionality runs in tasks
void loop() {}
