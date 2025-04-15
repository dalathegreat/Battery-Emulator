#include "precharge_control.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../include.h"

// Parameters

#ifdef PRECHARGE_CONTROL

#define MAX_PRECHARGE_TIME_MS 15000  // Maximum time precharge may be enabled

#define Precharge_default_PWM_Freq 11000
#define Precharge_min_PWM_Freq 5000
#define Precharge_max_PWM_Freq 34000
#define Precharge_PWM_Res 8

#define PWM_Freq 20000  // 20 kHz frequency, beyond audible range
#define PWM_Res 10      // 10 Bit resolution 0 to 1023, maps 'nicely' to 0% 100%
#define PWM_HOLD_DUTY 250
#define PWM_OFF_DUTY 0
#define PWM_ON_DUTY 1023

#define ON 0  //Normally closed contactors use inverted logic
#define OFF 1 //Normally closed contactors use inverted logic

#ifdef INVERTER_DISCONNECT_CONTACTOR_IS_NO  
#undef ON
#define ON 1
#undef OFF
#define OFF 0
#endif  //INVERTER_DISCONNECT_CONTACTOR_IS_NO


#define PWM_Precharge_Channel 0
unsigned long prechargeStartTime = 0;
static uint32_t freq = Precharge_default_PWM_Freq;
uint16_t delta_freq = 1;
static int32_t prev_external_voltage = 20000;

// Initialization functions

void init_precharge_control() {
  // Setup PWM Channel Frequency and Resolution
#ifdef DEBUG_LOG
  logging.printf("Precharge control initialised\n");
#endif
  pinMode(HIA4V1_PIN, OUTPUT);
  digitalWrite(HIA4V1_PIN, LOW);
  pinMode(INVERTER_DISCONNECT_CONTACTOR_PIN, OUTPUT);
  digitalWrite(INVERTER_DISCONNECT_CONTACTOR_PIN, ON);
}

// Main functions
void handle_precharge_control() {
  unsigned long currentTime = millis();
#ifdef MEB_BATTERY
  int32_t target_voltage = datalayer.battery.status.voltage_dV;
#ifdef MEB_BATTERY_DC_CHARGEPORT  
  int32_t external_voltage = datalayer_extended.meb.BMS_voltage_HV_charge_port_dV;
#else
  int32_t external_voltage = datalayer_extended.meb.BMS_voltage_intermediate_dV;
#endif
#endif

  switch (datalayer.system.status.precharge_status) {
    case AUTO_PRECHARGE_IDLE:

      if (datalayer.system.settings.start_precharging) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_START;
      }
      break;

    case AUTO_PRECHARGE_START:
      freq = Precharge_default_PWM_Freq;
      ledcAttachChannel(HIA4V1_PIN, freq, Precharge_PWM_Res, PWM_Precharge_Channel);
      ledcWriteTone(HIA4V1_PIN, freq);  // Set frequency and set dutycycle to 50%
      prechargeStartTime = currentTime;
      datalayer.system.status.precharge_status = AUTO_PRECHARGE_PRECHARGING;
#ifdef DEBUG_LOG
      logging.printf("Precharge: Starting sequence\n");
#endif
      digitalWrite(INVERTER_DISCONNECT_CONTACTOR_PIN, OFF);

      break;

    case AUTO_PRECHARGE_PRECHARGING:
    case AUTO_PRECHARGE_PRECHARGING_FINAL:
      //  Check if external voltage measurement changed, for instance with the MEB batteries, the external voltage is only updated every 100ms.
      if (prev_external_voltage != external_voltage && external_voltage != 0) {
        prev_external_voltage = external_voltage;

        /*if (labs(target_voltage - external_voltage) > 150) {
          delta_freq = 2000;
        } else*/ if (labs(target_voltage - external_voltage) > 80) {
          delta_freq = labs(target_voltage - external_voltage) * 6;
        } else {
          datalayer.system.status.precharge_status = AUTO_PRECHARGE_PRECHARGING_FINAL;
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
#ifdef DEBUG_LOG
        logging.printf("Precharge: Target: %d V  Extern: %d V  New frequency: %u Hz\n", target_voltage / 10,
                       external_voltage / 10, freq);
#endif
        ledcWriteTone(HIA4V1_PIN, freq);
      }

      if ((datalayer.battery.status.real_bms_status != BMS_STANDBY &&
           datalayer.battery.status.real_bms_status != BMS_ACTIVE) ||
          datalayer.battery.status.bms_status != ACTIVE || datalayer.system.settings.equipment_stop_active) {
        pinMode(HIA4V1_PIN, OUTPUT);
        digitalWrite(HIA4V1_PIN, LOW);
        digitalWrite(INVERTER_DISCONNECT_CONTACTOR_PIN, ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
#ifdef DEBUG_LOG
        logging.printf("Precharge: Disabling Precharge bms not standby/active or equipment stop\n");
#endif
      } else if (currentTime - prechargeStartTime >= MAX_PRECHARGE_TIME_MS ||
                 datalayer.battery.status.real_bms_status == BMS_FAULT) {
        pinMode(HIA4V1_PIN, OUTPUT);
        digitalWrite(HIA4V1_PIN, LOW);
        digitalWrite(INVERTER_DISCONNECT_CONTACTOR_PIN, ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_OFF;
#ifdef DEBUG_LOG
        logging.printf("Precharge: Disabled (timeout reached / BMS fault) -> AUTO_PRECHARGE_OFF\n");
#endif
        set_event(EVENT_AUTOMATIC_PRECHARGE_FAILURE, 0);

        // Add event
      } else if (datalayer.system.status.battery_allows_contactor_closing) {
        pinMode(HIA4V1_PIN, OUTPUT);
        digitalWrite(HIA4V1_PIN, LOW);
        digitalWrite(INVERTER_DISCONNECT_CONTACTOR_PIN, ON);
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_COMPLETED;
#ifdef DEBUG_LOG
        logging.printf("Precharge: Disabled (contacts closed) -> COMPLETED\n");
#endif
      }
      break;

    case AUTO_PRECHARGE_COMPLETED:
      if (datalayer.system.settings.equipment_stop_active || datalayer.battery.status.bms_status != ACTIVE) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
#ifdef DEBUG_LOG
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
#endif
      }
      break;

    case AUTO_PRECHARGE_OFF:
      if (!datalayer.system.status.battery_allows_contactor_closing ||
          !datalayer.system.status.inverter_allows_contactor_closing ||
          datalayer.system.settings.equipment_stop_active || datalayer.battery.status.bms_status != FAULT) {
        datalayer.system.status.precharge_status = AUTO_PRECHARGE_IDLE;
        pinMode(HIA4V1_PIN, OUTPUT);
        digitalWrite(HIA4V1_PIN, LOW);
#ifdef DEBUG_LOG
        logging.printf("Precharge: equipment stop activated -> IDLE\n");
#endif
      }
      break;

    default:
      break;
  }
}
#endif  // AUTO_PRECHARGE_CONTROL
