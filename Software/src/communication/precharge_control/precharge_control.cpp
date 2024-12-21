#include "precharge_control.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../include.h"

// Parameters

#ifdef PRECHARGE_CONTROL
enum State { PRECHARGE_IDLE, START_PRECHARGE, PRECHARGE, PRECHARGE_OFF, COMPLETED };
State prechargeStatus = PRECHARGE_IDLE;

#define MAX_ALLOWED_FAULT_TICKS 1000
#define MAX_PRECHARGE_TIME_MS 15000  // Maximum time precharge may be enabled

#define Precharge_default_PWM_Freq 22000
#define Precharge_min_PWM_Freq 18000
#define Precharge_max_PWM_Freq 34000
#define PWM_Res 8
#define PWM_OFF_DUTY 0

#define PWM_Precharge_Channel 0
unsigned long prechargeStartTime = 0;
static uint32_t freq = Precharge_default_PWM_Freq;
uint16_t delta_freq = 1;
static int32_t prev_external_voltage = 20000;

// Initialization functions

void init_precharge_control() {
  // Setup PWM Channel Frequency and Resolution
  logging.printf("Precharge control initialised\n");
  pinMode(PRECHARGE_PIN, OUTPUT);
  digitalWrite(PRECHARGE_PIN, LOW);
}

// Main functions
void handle_precharge_control() {
  unsigned long currentTime = millis();
#ifdef MEB_BATTERY
  int32_t target_voltage = datalayer.battery.status.voltage_dV;
  int32_t external_voltage = datalayer_extended.meb.BMS_voltage_intermediate_dV;
#endif

  // Handle actual state machine. This first turns on Negative, then Precharge, then Positive, and finally turns OFF precharge
  switch (prechargeStatus) {
    case PRECHARGE_IDLE:

      if (datalayer.battery.status.bms_status == STANDBY && datalayer.system.status.inverter_allows_contactor_closing &&
          !datalayer.system.settings.equipment_stop_active) {
        prechargeStatus = START_PRECHARGE;
      }
      break;

    case START_PRECHARGE:
      freq = Precharge_default_PWM_Freq;
      ledcAttachChannel(PRECHARGE_PIN, freq, PWM_Res, PWM_Precharge_Channel);
      ledcWriteTone(PRECHARGE_PIN, freq);  // Set frequency and set dutycycle to 50%
      prechargeStartTime = currentTime;
      prechargeStatus = PRECHARGE;
      logging.printf("Precharge: Starting sequence\n");
      break;

    case PRECHARGE:
      //  Check if external voltage measurement changed, for instance with the MEB batteries, the external voltage is only updated every 100ms.
      if (prev_external_voltage != external_voltage) {
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
        ledcWriteTone(PRECHARGE_PIN, freq);
      }

      if ((datalayer.battery.status.bms_status != STANDBY && datalayer.battery.status.bms_status != ACTIVE) ||
          datalayer.system.settings.equipment_stop_active) {
        pinMode(PRECHARGE_PIN, OUTPUT);
        digitalWrite(PRECHARGE_PIN, LOW);
        prechargeStatus = PRECHARGE_IDLE;
        logging.printf("Precharge: Disabling Precharge bms not standby/active or equipment stop\n");
      } else if (currentTime - prechargeStartTime >= MAX_PRECHARGE_TIME_MS) {
        pinMode(PRECHARGE_PIN, OUTPUT);
        digitalWrite(PRECHARGE_PIN, LOW);
        prechargeStatus = PRECHARGE_OFF;
        datalayer.battery.status.bms_status = FAULT;
        logging.printf("Precharge: Disabled (timeout reached) -> PRECHARGE_OFF\n");
        // Add event
      } else if (datalayer.system.status.battery_allows_contactor_closing) {
        pinMode(PRECHARGE_PIN, OUTPUT);
        digitalWrite(PRECHARGE_PIN, LOW);
        prechargeStatus = COMPLETED;
        logging.printf("Precharge: Disabled (contacts closed) -> COMPLETED\n");
      }
      break;

    case COMPLETED:
      if (datalayer.system.settings.equipment_stop_active || datalayer.battery.status.bms_status != ACTIVE) {
        prechargeStatus = PRECHARGE_IDLE;
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
      }
      break;

    case PRECHARGE_OFF:
      if (!datalayer.system.status.battery_allows_contactor_closing ||
          !datalayer.system.status.inverter_allows_contactor_closing ||
          datalayer.system.settings.equipment_stop_active || datalayer.battery.status.bms_status != FAULT) {
        prechargeStatus = PRECHARGE_IDLE;
        pinMode(PRECHARGE_PIN, OUTPUT);
        digitalWrite(PRECHARGE_PIN, LOW);
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
      }
      break;

    default:
      break;
  }
}
#endif  // PRECHARGE_CONTROL
