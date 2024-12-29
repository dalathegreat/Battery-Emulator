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
#ifdef BMS_POWER
  pinMode(BMS_POWER, OUTPUT);
  digitalWrite(BMS_POWER, HIGH);
#endif  // BMS_POWER
}

// Main functions
void handle_contactors() {
#if defined(BYD_SMA) || defined(SMA_TRIPOWER_CAN)
  datalayer.system.status.inverter_allows_contactor_closing = digitalRead(INVERTER_CONTACTOR_ENABLE_PIN);
#endif

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

  unsigned long currentTime = millis();

  if (currentTime < INTERVAL_10_S) {
    // Skip running the state machine before system has started up.
    // Gives the system some time to detect any faults from battery before blindly just engaging the contactors
    return;
  }

  // Handle actual state machine. This first turns on Negative, then Precharge, then Positive, and finally turns OFF precharge
  switch (contactorStatus) {
    case START_PRECHARGE:
      set(NEGATIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
      prechargeStartTime = currentTime;
      contactorStatus = PRECHARGE;
      break;

    case PRECHARGE:
      if (currentTime - prechargeStartTime >= NEGATIVE_CONTACTOR_TIME_MS) {
        set(PRECHARGE_PIN, ON);
        negativeStartTime = currentTime;
        contactorStatus = POSITIVE;
      }
      break;

    case POSITIVE:
      if (currentTime - negativeStartTime >= PRECHARGE_TIME_MS) {
        set(POSITIVE_CONTACTOR_PIN, ON, PWM_ON_DUTY);
        prechargeCompletedTime = currentTime;
        contactorStatus = PRECHARGE_OFF;
      }
      break;

    case PRECHARGE_OFF:
      if (currentTime - prechargeCompletedTime >= PRECHARGE_COMPLETED_TIME_MS) {
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
#endif  // CONTACTOR_CONTROL_DOUBLE_BATTERY
