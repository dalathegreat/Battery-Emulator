#include "comm_contactorcontrol.h"
#include "../../include.h"

#ifdef CONTACTOR_CONTROL
const bool contactor_control_enabled_default = true;
#else
const bool contactor_control_enabled_default = false;
#endif
bool contactor_control_enabled = contactor_control_enabled_default;

#ifdef PWM_CONTACTOR_CONTROL
const bool pwn_contactor_control_default = true;
#else
const bool pwn_contactor_control_default = false;
#endif
bool pwm_contactor_control = pwn_contactor_control_default;

#ifdef PERIODIC_BMS_RESET
const bool periodic_bms_reset_default = true;
#else
const bool periodic_bms_reset_default = false;
#endif
bool periodic_bms_reset = periodic_bms_reset_default;

#ifdef REMOTE_BMS_RESET
const bool remote_bms_reset_default = true;
#else
const bool remote_bms_reset_default = true;
#endif
bool remote_bms_reset = remote_bms_reset_default;

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
const bool contactor_control_enabled_double_battery_default = true;
#else
const bool contactor_control_enabled_double_battery_default = false;
#endif
bool contactor_control_enabled_double_battery = contactor_control_enabled_double_battery_default;

// TODO: Ensure valid values at run-time

// Parameters

enum State { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

const int ON = 1;
const int OFF = 0;

#ifdef NC_CONTACTORS  //Normally closed contactors use inverted logic
#undef ON
#define ON 0
#undef OFF
#define OFF 1
#endif  //NC_CONTACTORS

#define MAX_ALLOWED_FAULT_TICKS 1000
#define NEGATIVE_CONTACTOR_TIME_MS \
  500  // Time after negative contactor is turned on, to start precharge (not actual precharge time!)
#define PRECHARGE_COMPLETED_TIME_MS \
  1000  // After successful precharge, resistor is turned off after this delay (and contactors are economized if PWM enabled)
#define PWM_Freq 20000  // 20 kHz frequency, beyond audible range
#define PWM_Res 10      // 10 Bit resolution 0 to 1023, maps 'nicely' to 0% 100%
#define PWM_HOLD_DUTY 250
#define PWM_OFF_DUTY 0
#define PWM_ON_DUTY 1023
#define PWM_Positive_Channel 0
#define PWM_Negative_Channel 1
static unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long prechargeCompletedTime = 0;
unsigned long timeSpentInFaultedMode = 0;
unsigned long currentTime = 0;
unsigned long lastPowerRemovalTime = 0;
unsigned long bmsPowerOnTime = 0;
const unsigned long powerRemovalInterval = 24 * 60 * 60 * 1000;  // 24 hours in milliseconds
const unsigned long powerRemovalDuration = 30000;                // 30 seconds in milliseconds
const unsigned long bmsWarmupDuration = 3000;

void set(uint8_t pin, bool direction, uint32_t pwm_freq = 0xFFFF) {
  if (pwm_contactor_control) {
    if (pwm_freq != 0xFFFF) {
      ledcWrite(pin, pwm_freq);
      return;
    }
  }
  if (direction == 1) {
    digitalWrite(pin, HIGH);
  } else {  // 0
    digitalWrite(pin, LOW);
  }
}

// Initialization functions

void init_contactors() {
  // Init contactor pins
  if (contactor_control_enabled) {
    if (pwm_contactor_control) {
      // Setup PWM Channel Frequency and Resolution
      ledcAttachChannel(POSITIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res, PWM_Positive_Channel);
      ledcAttachChannel(NEGATIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res, PWM_Negative_Channel);
      // Set all pins OFF (0% PWM)
      ledcWrite(POSITIVE_CONTACTOR_PIN, PWM_OFF_DUTY);
      ledcWrite(NEGATIVE_CONTACTOR_PIN, PWM_OFF_DUTY);
    } else {  //Normal CONTACTOR_CONTROL
      pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
      set(POSITIVE_CONTACTOR_PIN, OFF);
      pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
      set(NEGATIVE_CONTACTOR_PIN, OFF);
    }  // Precharge never has PWM regardless of setting
    pinMode(PRECHARGE_PIN, OUTPUT);
    set(PRECHARGE_PIN, OFF);
  }
  if (contactor_control_enabled_double_battery) {
    pinMode(SECOND_BATTERY_CONTACTORS_PIN, OUTPUT);
    set(SECOND_BATTERY_CONTACTORS_PIN, OFF);
  }
// Init BMS contactor
#if defined HW_STARK || defined HW_3LB  // This hardware has dedicated pin, always enable on start
  pinMode(BMS_POWER, OUTPUT);           //LilyGo is omitted from this, only enabled if user selects PERIODIC_BMS_RESET
  digitalWrite(BMS_POWER, HIGH);
#endif  // HW with dedicated BMS pins

#ifdef BMS_POWER
  if (periodic_bms_reset || remote_bms_reset) {
    pinMode(BMS_POWER, OUTPUT);
    digitalWrite(BMS_POWER, HIGH);
  }
#endif
}

static void dbg_contactors(const char* state) {
#ifdef DEBUG_LOG
  logging.print("[");
  logging.print(millis());
  logging.print(" ms] contactors control: ");
  logging.println(state);
#endif
}

// Main functions of the handle_contactors include checking if inverter allows for closing, checking battery 2, checking BMS power output, and actual contactor closing/precharge via GPIO
void handle_contactors() {
  if (inverter && inverter->controls_contactor()) {
    datalayer.system.status.inverter_allows_contactor_closing = inverter->allows_contactor_closing();
  }

#ifdef BMS_POWER
  handle_BMSpower();  // Some batteries need to be periodically power cycled
#endif

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
  handle_contactors_battery2();
#endif  // CONTACTOR_CONTROL_DOUBLE_BATTERY

  if (contactor_control_enabled) {
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
      datalayer.system.status.contactors_engaged = false;

      if (datalayer.system.status.battery_allows_contactor_closing &&
          datalayer.system.status.inverter_allows_contactor_closing &&
          !datalayer.system.settings.equipment_stop_active) {
        contactorStatus = START_PRECHARGE;
      }
    }

    // In case the inverter requests contactors to open, set the state accordingly
    if (contactorStatus == COMPLETED) {
      //Incase inverter (or estop) requests contactors to open, make state machine jump to Disconnected state (recoverable)
      if (!datalayer.system.status.inverter_allows_contactor_closing ||
          datalayer.system.settings.equipment_stop_active) {
        contactorStatus = DISCONNECTED;
      }
      // Skip running the state machine below if it has already completed
      return;
    }

    currentTime = millis();

    if (currentTime < INTERVAL_10_S) {
      // Skip running the state machine before system has started up.
      // Gives the system some time to detect any faults from battery before blindly just engaging the contactors
      return;
    }

    // Handle actual state machine. This first turns on Negative, then Precharge, then Positive, and finally turns OFF precharge
    switch (contactorStatus) {
      case START_PRECHARGE:
        set(NEGATIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
        dbg_contactors("NEGATIVE");
        prechargeStartTime = currentTime;
        contactorStatus = PRECHARGE;
        break;

      case PRECHARGE:
        if (currentTime - prechargeStartTime >= NEGATIVE_CONTACTOR_TIME_MS) {
          set(PRECHARGE_PIN, ON);
          dbg_contactors("PRECHARGE");
          negativeStartTime = currentTime;
          contactorStatus = POSITIVE;
        }
        break;

      case POSITIVE:
        if (currentTime - negativeStartTime >= PRECHARGE_TIME_MS) {
          set(POSITIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
          dbg_contactors("POSITIVE");
          prechargeCompletedTime = currentTime;
          contactorStatus = PRECHARGE_OFF;
        }
        break;

      case PRECHARGE_OFF:
        if (currentTime - prechargeCompletedTime >= PRECHARGE_COMPLETED_TIME_MS) {
          set(PRECHARGE_PIN, OFF);
          set(NEGATIVE_CONTACTOR_PIN, ON, PWM_HOLD_DUTY);
          set(POSITIVE_CONTACTOR_PIN, ON, PWM_HOLD_DUTY);
          dbg_contactors("PRECHARGE_OFF");
          contactorStatus = COMPLETED;
          datalayer.system.status.contactors_engaged = true;
        }
        break;
      default:
        break;
    }
  }
}

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
void handle_contactors_battery2() {
  if ((contactorStatus == COMPLETED) && datalayer.system.status.battery2_allowed_contactor_closing) {
    set(SECOND_BATTERY_CONTACTORS_PIN, ON);
    datalayer.system.status.contactors_battery2_engaged = true;
  } else {  // Closing contactors on secondary battery not allowed
    set(SECOND_BATTERY_CONTACTORS_PIN, OFF);
    datalayer.system.status.contactors_battery2_engaged = false;
  }
}
#endif

/* PERIODIC_BMS_RESET - Once every 24 hours we remove power from the BMS_power pin for 30 seconds.
REMOTE_BMS_RESET - Allows the user to remotely powercycle the BMS by sending a command to the emulator via MQTT.

This makes the BMS recalculate all SOC% and avoid memory leaks
During that time we also set the emulator state to paused in order to not try and send CAN messages towards the battery
Feature is only used if user has enabled PERIODIC_BMS_RESET in the USER_SETTINGS */

#ifdef BMS_POWER
void handle_BMSpower() {
  if (periodic_bms_reset || remote_bms_reset) {
    // Get current time
    currentTime = millis();

    if (periodic_bms_reset) {
      // Check if 24 hours have passed since the last power removal
      if ((currentTime + bmsResetTimeOffset) - lastPowerRemovalTime >= powerRemovalInterval) {
        start_bms_reset();
      }
    }

    // If power has been removed for 30 seconds, restore the power
    if (datalayer.system.status.BMS_reset_in_progress && currentTime - lastPowerRemovalTime >= powerRemovalDuration) {
      // Reapply power to the BMS
      digitalWrite(BMS_POWER, HIGH);
#ifdef BMS_2_POWER
      digitalWrite(BMS_2_POWER, HIGH);  // Same for battery 2
#endif
      bmsPowerOnTime = currentTime;
      datalayer.system.status.BMS_reset_in_progress = false;   // Reset the power removal flag
      datalayer.system.status.BMS_startup_in_progress = true;  // Set the BMS warmup flag
    }
    //if power has been restored we need to wait a couple of seconds to unpause the battery
    if (datalayer.system.status.BMS_startup_in_progress && currentTime - bmsPowerOnTime >= bmsWarmupDuration) {

      setBatteryPause(false, false, false, false);

      datalayer.system.status.BMS_startup_in_progress = false;  // Reset the BMS warmup removal flag
      set_event(EVENT_PERIODIC_BMS_RESET, 0);
    }
  }
}
#endif

void start_bms_reset() {
  if (periodic_bms_reset || remote_bms_reset) {
    if (!datalayer.system.status.BMS_reset_in_progress) {
      lastPowerRemovalTime = currentTime;  // Record the time when BMS reset was started
                                           // we are now resetting at the correct time. We don't need to offset anymore
      bmsResetTimeOffset = 0;
      // Set a flag to let the rest of the system know we are cutting power to the BMS.
      // The battery CAN sending routine will then know not to try guto send anything towards battery while active
      datalayer.system.status.BMS_reset_in_progress = true;

      // Set emulator state to paused (Max Charge/Discharge = 0 & CAN = stop)
      // We try to keep contactors engaged during this pause, and just ramp power down to 0.
      setBatteryPause(true, false, false, false);

#ifdef BMS_POWER
      digitalWrite(BMS_POWER, LOW);  // Remove power by setting the BMS power pin to LOW
#endif
#ifdef BMS_2_POWER
      digitalWrite(BMS_2_POWER, LOW);  // Same for battery 2
#endif
    }
  }
}
