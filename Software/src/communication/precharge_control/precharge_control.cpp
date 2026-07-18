#include "precharge_control.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/hal/hal.h"
#include "precharge_control_i2c_g05.h"
#include "src/battery/BATTERIES.h"

// Parameters adjustable by user in Settings page
PrechargeControlMode precharge_control_mode = PrechargeControlMode::Disabled;

bool is_precharge_control_enabled() {
  return precharge_control_mode != PrechargeControlMode::Disabled;
}

bool is_precharge_control_i2c_g05_enabled() {
  return precharge_control_mode == PrechargeControlMode::I2cG05;
}
bool precharge_inverter_normally_open_contactor = false;
uint16_t precharge_max_precharge_time_before_fault = 15000;
uint16_t Precharge_max_PWM_Freq = 34000;

// Hardcoded parameters
#define Precharge_default_PWM_Freq 11000
#define Precharge_min_PWM_Freq 5000
#define Precharge_PWM_Res 8
#define PWM_Freq 20000  // 20 kHz frequency, beyond audible range
#define PWM_Precharge_Channel 0
#define CONTACTOR_ON (precharge_inverter_normally_open_contactor ? 1 : 0)
#define CONTACTOR_OFF (precharge_inverter_normally_open_contactor ? 0 : 1)

static unsigned long prechargeStartTime = 0;
static uint32_t freq = Precharge_default_PWM_Freq;
static uint16_t delta_freq = 1;
static int32_t prev_external_voltage = 20000;
static unsigned long last_i2c_update = 0;
static float g05_tps_voltage = 4.0f;

static float g05_target_to_tps_voltage(int32_t target_dV) {
  float target_V = target_dV / 10.0f;

  // Measured transfer curve:
  // TPS 4.0 V  -> approx. 175 V external
  // TPS 11.0 V -> approx. 500 V external
  float tps = 4.0f + ((target_V - 175.0f) * 7.0f / 325.0f);

  if (tps < 4.0f)
    tps = 4.0f;
  if (tps > 11.0f)
    tps = 11.0f;

  return tps;
}

static void disable_precharge_output(gpio_num_t hia4v1_pin) {
  if (is_precharge_control_i2c_g05_enabled()) {
    g05_disable_output();
  } else {
    pinMode(hia4v1_pin, OUTPUT);
    digitalWrite(hia4v1_pin, LOW);
  }
}

// Initialization functions

bool init_precharge_control() {
  if (!is_precharge_control_enabled()) {
    return true;
  }

  auto hia4v1_pin = esp32hal->HIA4V1_PIN();
  auto inverter_disconnect_contactor_pin = esp32hal->INVERTER_DISCONNECT_CONTACTOR_PIN();

  if (is_precharge_control_i2c_g05_enabled()) {
    if (!esp32hal->alloc_pins("Precharge control", inverter_disconnect_contactor_pin)) {
      DEBUG_PRINTF("Precharge control setup failed\n");
      return false;
    }
  } else {
    if (!esp32hal->alloc_pins("Precharge control", hia4v1_pin, inverter_disconnect_contactor_pin)) {
      DEBUG_PRINTF("Precharge control setup failed\n");
      return false;
    }

    pinMode(hia4v1_pin, OUTPUT);
    digitalWrite(hia4v1_pin, LOW);
  }

  pinMode(inverter_disconnect_contactor_pin, OUTPUT);
  digitalWrite(inverter_disconnect_contactor_pin, LOW);

  DEBUG_PRINTF("Precharge control setup successful\n");
  return true;
}

// Main functions
void handle_precharge_control(unsigned long currentMillis) {
  auto hia4v1_pin = esp32hal->HIA4V1_PIN();
  auto inverter_disconnect_contactor_pin = esp32hal->INVERTER_DISCONNECT_CONTACTOR_PIN();

  // If we're in FAILURE state, completely disable any further precharge attempts
  if (datalayer.system.status.precharge_status == AUTO_PRECHARGE_FAILURE) {
    disable_precharge_output(hia4v1_pin);
    digitalWrite(inverter_disconnect_contactor_pin, CONTACTOR_ON);
    return;  // Exit immediately - no further processing allowed. Reboot required to recover
  }

  // If we are running in test mode (No battery configured, enable precharge sequence so user can test HW)
  if (battery == NULL) {
    datalayer.system.info.start_precharging = true;
    datalayer.battery.status.real_bms_status = BMS_STANDBY;
  }

  int32_t target_voltage = datalayer.battery.status.voltage_dV;
  int32_t external_voltage = datalayer_extended.meb.BMS_voltage_intermediate_dV;

  switch (datalayer.system.status.precharge_status) {
    case AUTO_PRECHARGE_IDLE:
      if (datalayer.system.info.start_precharging && datalayer.system.status.inverter_allows_contactor_closing) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_START;
      }
      break;
    case AUTO_PRECHARGE_START:
      if (is_precharge_control_i2c_g05_enabled()) {
        if (g05_i2c_init_if_needed()) {
          g05_set_voltage(4.0f);
          g05_enable_output();
          logging.printf("Precharge G05: Output enabled\n");
        } else {
          logging.printf("Precharge G05: waiting for TPS55288 power-up\n");
        }
      } else {
        freq = Precharge_default_PWM_Freq;
        ledcAttachChannel(hia4v1_pin, freq, Precharge_PWM_Res, PWM_Precharge_Channel);
        ledcWriteTone(hia4v1_pin, freq);  // Set frequency and set dutycycle to 50%
      }
      prechargeStartTime = currentMillis;
      datalayer.system.status.precharge_status = AUTO_PRECHARGE_PRECHARGING;
      logging.printf("Precharge: Starting sequence\n");
      digitalWrite(inverter_disconnect_contactor_pin, CONTACTOR_OFF);
      break;

    case AUTO_PRECHARGE_PRECHARGING:
      if (is_precharge_control_i2c_g05_enabled()) {
        if (!g05_is_initialized()) {
          if (currentMillis - last_i2c_update >= 50) {
            last_i2c_update = currentMillis;

            if (g05_i2c_init_if_needed()) {
              g05_tps_voltage = g05_target_to_tps_voltage(target_voltage);
              g05_set_voltage(g05_tps_voltage);
              g05_enable_output();
              logging.printf("Precharge G05: TPS55288 found, output enabled\n");
            } else {
              logging.printf("Precharge G05: waiting for TPS55288\n");
            }
          }
        } else if (currentMillis - last_i2c_update >= 500) {
          last_i2c_update = currentMillis;

          if (external_voltage == 0) {
            logging.printf("Precharge G05: waiting for external voltage feedback\n");
          } else {
            int32_t error_dV = target_voltage - external_voltage;

            float step = 0.02f;  // 20mV minimum TPS step
            if (labs(error_dV) > 200) {
              step = 0.50f;
            } else if (labs(error_dV) > 100) {
              step = 0.25f;
            } else if (labs(error_dV) > 50) {
              step = 0.10f;
            } else if (labs(error_dV) > 20) {
              step = 0.04f;
            }

            if (error_dV > 10) {
              g05_tps_voltage += step;
            } else if (error_dV < -10) {
              g05_tps_voltage -= step;
            }

            if (g05_tps_voltage < 4.0f)
              g05_tps_voltage = 4.0f;
            if (g05_tps_voltage > 11.0f)
              g05_tps_voltage = 11.0f;

            g05_set_voltage(g05_tps_voltage);
            logging.printf("Precharge G05: Target: %d V  Extern: %d V  Error: %d dV  TPS: %.2f V\n",
                           target_voltage / 10, external_voltage / 10, error_dV, g05_tps_voltage);
          }
        }
      } else {
        //  Check if external voltage measurement changed, for instance with the MEB batteries, the external voltage is only updated every 100ms.
        if (prev_external_voltage != external_voltage && external_voltage != 0) {
          prev_external_voltage = external_voltage;

          if (labs(target_voltage - external_voltage) > 150) {
            delta_freq = 2000;
          } else if (labs(target_voltage - external_voltage) > 80) {
            delta_freq = labs(target_voltage - external_voltage) * 6;
          } else {
            delta_freq = labs(target_voltage - external_voltage) * 3;
          }
          if (target_voltage > external_voltage) {
            freq += delta_freq;
          } else {
            freq -= delta_freq;
          }
          if (freq > Precharge_max_PWM_Freq)
            freq = Precharge_max_PWM_Freq;
          if (freq < Precharge_min_PWM_Freq)
            freq = Precharge_min_PWM_Freq;
          logging.printf("Precharge: Target: %d V  Extern: %d V  Frequency: %u\n", target_voltage / 10,
                         external_voltage / 10, freq);
          ledcWriteTone(hia4v1_pin, freq);
        }
      }

      if (currentMillis - prechargeStartTime >= precharge_max_precharge_time_before_fault ||
          datalayer.battery.status.real_bms_status == BMS_FAULT) {
        pinMode(hia4v1_pin, OUTPUT);
        digitalWrite(hia4v1_pin, LOW);
        if (is_precharge_control_i2c_g05_enabled())
          g05_disable_output();
        digitalWrite(inverter_disconnect_contactor_pin, CONTACTOR_ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_FAILURE;
        logging.printf("Precharge: CRITICAL FAILURE (timeout/BMS fault) -> REQUIRES REBOOT\n");
        set_event(EVENT_AUTOMATIC_PRECHARGE_FAILURE, 0);
        // Force stop any further precharge attempts
        datalayer.system.info.start_precharging = false;
      } else if ((datalayer.battery.status.real_bms_status != BMS_STANDBY &&
                  datalayer.battery.status.real_bms_status != BMS_ACTIVE) ||
                 datalayer.system.status.system_status != ACTIVE || datalayer.system.info.equipment_stop_active ||
                 !datalayer.system.status.inverter_allows_contactor_closing) {
        disable_precharge_output(hia4v1_pin);
        digitalWrite(inverter_disconnect_contactor_pin, CONTACTOR_ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
        logging.printf("Precharge: Disabling Precharge bms not standby/active or equipment stop\n");
      } else if (datalayer.system.status.battery_allows_contactor_closing) {
        disable_precharge_output(hia4v1_pin);
        digitalWrite(inverter_disconnect_contactor_pin, CONTACTOR_ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_COMPLETED;
        logging.printf("Precharge: Disabled (contacts closed) -> COMPLETED\n");
      }
      break;

    case AUTO_PRECHARGE_COMPLETED:
      // If equipment stop is activated, or BE is in a non-active state, or the
      // BMS has gone back to standby (eg, after a BMS reset), then we'll allow
      // the precharge to be restarted.
      if (datalayer.system.info.equipment_stop_active || datalayer.system.status.system_status != ACTIVE ||
          datalayer.battery.status.real_bms_status == BMS_STANDBY ||
          !datalayer.system.status.inverter_allows_contactor_closing) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
      }
      break;

    case AUTO_PRECHARGE_OFF:
      // This is not used anymore?
      if (!datalayer.system.status.battery_allows_contactor_closing ||
          !datalayer.system.status.inverter_allows_contactor_closing || datalayer.system.info.equipment_stop_active ||
          datalayer.system.status.system_status != FAULT) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
        disable_precharge_output(hia4v1_pin);
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
      }
      break;

    default:
      break;
  }
}
