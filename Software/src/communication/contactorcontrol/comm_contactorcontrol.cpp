#include "comm_contactorcontrol.h"
#include "../../include.h"

// Parameters
#ifndef CONTACTOR_CONTROL
#ifdef PWM_CONTACTOR_CONTROL
#error CONTACTOR_CONTROL needs to be enabled for PWM_CONTACTOR_CONTROL
#endif
#endif

#ifdef CONTACTOR_CONTROL
enum State { DISCONNECTED, START_PRECHARGE, PRECHARGE, POSITIVE, PRECHARGE_OFF, COMPLETED, SHUTDOWN_REQUESTED };
State contactorStatus = DISCONNECTED;

#define ON 1
#define OFF 0

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
unsigned long prechargeStartTime = 0;
unsigned long negativeStartTime = 0;
unsigned long prechargeCompletedTime = 0;
unsigned long timeSpentInFaultedMode = 0;
#endif
unsigned long currentTime = 0;
unsigned long lastPowerRemovalTime = 0;
const unsigned long powerRemovalInterval = 24 * 60 * 60 * 1000;  // 24 hours in milliseconds
const unsigned long powerRemovalDuration = 30000;                // 30 seconds in milliseconds

void set(uint8_t pin, bool direction, uint32_t pwm_freq = 0xFFFF) {
#ifdef PWM_CONTACTOR_CONTROL
  if (pwm_freq != 0xFFFF) {
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

// Initialization functions

void init_contactors() {
  // Init contactor pins
#ifdef CONTACTOR_CONTROL
#ifdef PWM_CONTACTOR_CONTROL
  // Setup PWM Channel Frequency and Resolution
  ledcAttachChannel(POSITIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res, PWM_Positive_Channel);
  ledcAttachChannel(NEGATIVE_CONTACTOR_PIN, PWM_Freq, PWM_Res, PWM_Negative_Channel);
  // Set all pins OFF (0% PWM)
  ledcWrite(POSITIVE_CONTACTOR_PIN, PWM_OFF_DUTY);
  ledcWrite(NEGATIVE_CONTACTOR_PIN, PWM_OFF_DUTY);
#else   //Normal CONTACTOR_CONTROL
  pinMode(POSITIVE_CONTACTOR_PIN, OUTPUT);
  set(POSITIVE_CONTACTOR_PIN, OFF);
  pinMode(NEGATIVE_CONTACTOR_PIN, OUTPUT);
  set(NEGATIVE_CONTACTOR_PIN, OFF);
#endif  // Precharge never has PWM regardless of setting
  pinMode(PRECHARGE_PIN, OUTPUT);
  set(PRECHARGE_PIN, OFF);
#endif  // CONTACTOR_CONTROL
#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
  pinMode(SECOND_POSITIVE_CONTACTOR_PIN, OUTPUT);
  set(SECOND_POSITIVE_CONTACTOR_PIN, OFF);
  pinMode(SECOND_NEGATIVE_CONTACTOR_PIN, OUTPUT);
  set(SECOND_NEGATIVE_CONTACTOR_PIN, OFF);
#endif  // CONTACTOR_CONTROL_DOUBLE_BATTERY
// Init BMS contactor
#if defined HW_STARK || defined HW_3LB  // This hardware has dedicated pin, always enable on start
  pinMode(BMS_POWER, OUTPUT);           //LilyGo is omitted from this, only enabled if user selects PERIODIC_BMS_RESET
  digitalWrite(BMS_POWER, HIGH);
#ifdef BMS_2_POWER  //Hardware supports 2x BMS
  pinMode(BMS_2_POWER, OUTPUT);
  digitalWrite(BMS_2_POWER, HIGH);
#endif BMS_2_POWER
#endif                                                        // HW with dedicated BMS pins
#if defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)  // User has enabled BMS reset, turn on output on start
  pinMode(BMS_POWER, OUTPUT);
  digitalWrite(BMS_POWER, HIGH);
#ifdef BMS_2_POWER  //Hardware supports 2x BMS
  pinMode(BMS_2_POWER, OUTPUT);
  digitalWrite(BMS_2_POWER, HIGH);
#endif  //BMS_2_POWER
#endif  //PERIODIC_BMS_RESET
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
#if defined(SMA_BYD_H_CAN) || defined(SMA_BYD_HVS_CAN) || defined(SMA_TRIPOWER_CAN)
  datalayer.system.status.inverter_allows_contactor_closing = digitalRead(INVERTER_CONTACTOR_ENABLE_PIN);
#endif

  handle_BMSpower();  // Some batteries need to be periodically power cycled

#ifdef CONTACTOR_CONTROL_DOUBLE_BATTERY
  handle_contactors_battery2();
#endif  // CONTACTOR_CONTROL_DOUBLE_BATTERY

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
    datalayer.system.status.contactors_engaged = false;

    if (datalayer.system.status.battery_allows_contactor_closing &&
        datalayer.system.status.inverter_allows_contactor_closing && !datalayer.system.settings.equipment_stop_active) {
      contactorStatus = START_PRECHARGE;
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
#endif  // CONTACTOR_CONTROL_DOUBLE_BATTERY

/* PERIODIC_BMS_RESET - Once every 24 hours we remove power from the BMS_power pin for 30 seconds.
REMOTE_BMS_RESET - Allows the user to remotely powercycle the BMS by sending a command to the emulator via MQTT.

This makes the BMS recalculate all SOC% and avoid memory leaks
During that time we also set the emulator state to paused in order to not try and send CAN messages towards the battery
Feature is only used if user has enabled PERIODIC_BMS_RESET in the USER_SETTINGS */

void handle_BMSpower() {
#if defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)
  // Get current time
  currentTime = millis();

#ifdef PERIODIC_BMS_RESET
  // Check if 24 hours have passed since the last power removal
  if (currentTime - lastPowerRemovalTime >= powerRemovalInterval) {
    start_bms_reset();
  }
#endif  //PERIODIC_BMS_RESET

  // If power has been removed for 30 seconds, restore the power and resume the emulator
  if (datalayer.system.status.BMS_reset_in_progress && currentTime - lastPowerRemovalTime >= powerRemovalDuration) {
    // Reapply power to the BMS
    digitalWrite(BMS_POWER, HIGH);
#ifdef BMS_2_POWER
    digitalWrite(BMS_2_POWER, HIGH);  // Same for battery 2
#endif

    //Resume from the power pause
    setBatteryPause(false, false, false, false);

    datalayer.system.status.BMS_reset_in_progress = false;  // Reset the power removal flag
  }
#endif  //defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)
}

void start_bms_reset() {
#if defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)
  if (!datalayer.system.status.BMS_reset_in_progress) {
    lastPowerRemovalTime = currentTime;  // Record the time when BMS reset was started

    // Set a flag to let the rest of the system know we are cutting power to the BMS.
    // The battery CAN sending routine will then know not to try to send anything towards battery while active
    datalayer.system.status.BMS_reset_in_progress = true;

    // Set emulator state to paused (Max Charge/Discharge = 0 & CAN = stop)
    // We try to keep contactors engaged during this pause, and just ramp power down to 0.
    setBatteryPause(true, false, false, false);

    digitalWrite(BMS_POWER, LOW);  // Remove power by setting the BMS power pin to LOW
#ifdef BMS_2_POWER
    digitalWrite(BMS_2_POWER, LOW);  // Same for battery 2
#endif
  }
#endif  //defined(PERIODIC_BMS_RESET) || defined(REMOTE_BMS_RESET)
}
