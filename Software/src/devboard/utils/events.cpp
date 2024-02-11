#include "events.h"

#include "../../../USER_SETTINGS.h"
#include "../config.h"
#include "timer.h"

typedef struct {
  EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
  uint32_t time_seconds;
  MyTimer second_timer;
  EVENTS_LEVEL_TYPE level;
} EVENT_TYPE;

/* Local variables */
static EVENT_TYPE events;
static const char* EVENTS_ENUM_TYPE_STRING[] = {EVENTS_ENUM_TYPE(GENERATE_STRING)};
static const char* EVENTS_LEVEL_TYPE_STRING[] = {EVENTS_LEVEL_TYPE(GENERATE_STRING)};

/* Local function prototypes */
static void update_event_time(void);
static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched);
static void update_event_level(void);
static void update_bms_status(void);

/* Exported functions */

/* Main execution function, should handle various continuous functionality */
void run_event_handling(void) {
  update_event_time();
  run_sequence_on_target();
}

/* Initialization function */
void init_events(void) {

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[EVENT_CAN_FAILURE].data = 0;
    events.entries[EVENT_CAN_FAILURE].timestamp = 0;
    events.entries[EVENT_CAN_FAILURE].occurences = 0;
  }

  events.entries[EVENT_CAN_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_WATER_INGRESS].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_12V_LOW].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOC_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_KWH_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_LOW_SOH].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_HVIL_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INTERNAL_OPEN_FAULT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_UNDER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_OVER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_UNKNOWN_EVENT_SET].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_OTA_UPDATE].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_DEBUG].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_DUMMY_ERROR].level = EVENT_LEVEL_ERROR;

  events.second_timer.set_interval(1000);
}

void set_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  set_event(event, data, false);
}

void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data) {
  set_event(event, data, true);
}

void clear_event(EVENTS_ENUM_TYPE event) {
  if (events.entries[event].state == EVENT_STATE_ACTIVE) {
    events.entries[event].state = EVENT_STATE_INACTIVE;
    update_event_level();
    update_bms_status();
  }
}

const char* get_event_message_string(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CAN_FAILURE:
      return "No CAN communication detected for 60s. Shutting down battery control.";
    case EVENT_CAN_WARNING:
      return "ERROR: High amount of corrupted CAN messages detected. Check CAN wire shielding!";
    case EVENT_WATER_INGRESS:
      return "Water leakage inside battery detected. Operation halted. Inspect battery!";
    case EVENT_12V_LOW:
      return "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!";
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      return "ERROR: SOC% reported by battery not plausible. Restart battery!";
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      return "Warning: kWh remaining reported by battery not plausible. Battery needs cycling.";
    case EVENT_BATTERY_CHG_STOP_REQ:
      return "ERROR: Battery raised caution indicator AND requested charge stop. Inspect battery status!";
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      return "ERROR: Battery raised caution indicator AND requested discharge stop. Inspect battery status!";
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      return "ERROR: Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!";
    case EVENT_LOW_SOH:
      return "ERROR: State of health critically low. Battery internal resistance too high to continue. Recycle "
             "battery.";
    case EVENT_HVIL_FAILURE:
      return "ERROR: Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be "
             "disabled!";
    case EVENT_INTERNAL_OPEN_FAULT:
      return "ERROR: High voltage cable removed while battery running. Opening contactors!";
    case EVENT_CELL_UNDER_VOLTAGE:
      return "ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!";
    case EVENT_CELL_OVER_VOLTAGE:
      return "ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!";
    case EVENT_CELL_DEVIATION_HIGH:
      return "ERROR: HIGH CELL DEVIATION!!! Inspect battery!";
    case EVENT_UNKNOWN_EVENT_SET:
      return "An unknown event was set! Review your code!";
    case EVENT_DUMMY_INFO:
      return "The dummy info event was set!";  // Don't change this event message!
    case EVENT_DUMMY_DEBUG:
      return "The dummy debug event was set!";  // Don't change this event message!
    case EVENT_DUMMY_WARNING:
      return "The dummy warning event was set!";  // Don't change this event message!
    case EVENT_DUMMY_ERROR:
      return "The dummy error event was set!";  // Don't change this event message!
    case EVENT_SERIAL_RX_WARNING:
      return "Error in serial function: No data received for some time, see data for minutes";
    case EVENT_SERIAL_RX_FAILURE:
      return "Error in serial function: No data for a long time!";
    case EVENT_SERIAL_TX_FAILURE:
      return "Error in serial function: No ACK from receiver!";
    case EVENT_SERIAL_TRANSMITTER_FAILURE:
      return "Error in serial function: Some ERROR level fault in transmitter, received by receiver";
    default:
      return "";
  }
}

const char* get_event_enum_string(EVENTS_ENUM_TYPE event) {
  // Return the event name but skip "EVENT_" that should always be first
  return EVENTS_ENUM_TYPE_STRING[event] + 6;
}

const char* get_event_level_string(EVENTS_ENUM_TYPE event) {
  // Return the event level but skip "EVENT_LEVEL_" that should always be first
  return EVENTS_LEVEL_TYPE_STRING[events.entries[event].level] + 12;
}

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event) {
  return &events.entries[event];
}

EVENTS_LEVEL_TYPE get_event_level(void) {
  return events.level;
}

/* Local functions */

static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched) {
  // Just some defensive stuff if someone sets an unknown event
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }

  // If the event is already set, no reason to continue
  if ((events.entries[event].state != EVENT_STATE_ACTIVE) &&
      (events.entries[event].state != EVENT_STATE_ACTIVE_LATCHED)) {
    events.entries[event].occurences++;
  }

  // We should set the event, update event info
  events.entries[event].timestamp = events.time_seconds;
  events.entries[event].data = data;
  // Check if the event is latching
  events.entries[event].state = latched ? EVENT_STATE_ACTIVE_LATCHED : EVENT_STATE_ACTIVE;

  update_event_level();
  update_bms_status();

#ifdef DEBUG_VIA_USB
  Serial.println(get_event_message_string(event));
#endif
}

static void update_bms_status(void) {
  switch (events.level) {
    case EVENT_LEVEL_INFO:
    case EVENT_LEVEL_WARNING:
      bms_status = ACTIVE;
      break;
    case EVENT_LEVEL_DEBUG:
      bms_status = UPDATING;
      break;
    case EVENT_LEVEL_ERROR:
      bms_status = FAULT;
      break;
    default:
      break;
  }
}

static void update_event_level(void) {
  events.level = EVENT_LEVEL_INFO;
  for (uint8_t i = 0u; i < EVENT_NOF_EVENTS; i++) {
    if ((events.entries[i].state == EVENT_STATE_ACTIVE) || (events.entries[i].state == EVENT_STATE_ACTIVE_LATCHED)) {
      events.level = max(events.entries[i].level, events.level);
    }
  }
}

static void update_event_time(void) {
  if (events.second_timer.elapsed() == true) {
    events.time_seconds++;
  }
}
