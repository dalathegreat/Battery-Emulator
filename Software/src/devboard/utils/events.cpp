#include "events.h"

#include "../../../USER_SETTINGS.h"
#include "../config.h"
#include "timer.h"

typedef enum {
  EVENT_STATE_INIT = 0,
  EVENT_STATE_INACTIVE,
  EVENT_STATE_ACTIVE,
  EVENT_STATE_ACTIVE_LATCHED
} EVENT_STATE_TYPE;

typedef struct {
  uint32_t timestamp;      // Time in seconds since startup when the event occurred
  uint8_t data;            // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;      // Number of occurrences since startup
  uint8_t led_color;       // LED indication
  EVENT_STATE_TYPE state;  // Event state
} EVENTS_STRUCT_TYPE;

typedef struct {
  EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
  uint32_t time_seconds;
  char message[256];
  MyTimer second_timer;
  uint8_t nof_yellow_events;
  uint8_t nof_blue_events;
  uint8_t nof_red_events;
} EVENT_TYPE;

/* Local variables */
static EVENT_TYPE events;

/* Local function prototypes */
static void update_event_time(void);
static void set_message(EVENTS_ENUM_TYPE event);
static void update_led_color(void);
static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched);
static void update_event_numbers(void);
static void update_bms_status(void);

/* Exported functions */

/* Main execution function, should handle various continuous functionality */
void run_event_handling(void) {
  update_event_time();
}

/* Initialization function */
void init_events(void) {

  events.entries[EVENT_CAN_FAILURE] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_CAN_WARNING] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = YELLOW, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_WATER_INGRESS] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_12V_LOW] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = YELLOW, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_SOC_PLAUSIBILITY_ERROR] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_KWH_PLAUSIBILITY_ERROR] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = YELLOW, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_BATTERY_CHG_STOP_REQ] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_LOW_SOH] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_HVIL_FAILURE] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_INTERNAL_OPEN_FAULT] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_CELL_UNDER_VOLTAGE] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_CELL_OVER_VOLTAGE] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_CELL_DEVIATION_HIGH] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = YELLOW, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_UNKNOWN_EVENT_SET] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_OTA_UPDATE] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = BLUE, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_DUMMY] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};
  events.entries[EVENT_NOF_EVENTS] = {
      .timestamp = 0, .data = 0, .occurences = 0, .led_color = RED, .state = EVENT_STATE_INACTIVE};

  // BLUE...
  events.entries[EVENT_OTA_UPDATE].led_color = BLUE;

  events.second_timer.interval = 1000;

  events.nof_blue_events = 0;
  events.nof_red_events = 0;
  events.nof_yellow_events = 0;
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
    update_event_numbers();
    update_led_color();
    update_bms_status();
  }
}

/* Local functions */

static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched) {
  // Just some defensive stuff if someone sets an unknown event
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }

  // If the event is already set, no reason to continue
  if ((events.entries[event].state == EVENT_STATE_ACTIVE) ||
      (events.entries[event].state == EVENT_STATE_ACTIVE_LATCHED)) {
    return;
  }

  // We should set the event, update event info
  events.entries[event].timestamp = events.time_seconds;
  events.entries[event].data = data;
  events.entries[event].occurences++;
  // Check if the event is latching
  events.entries[event].state = latched ? EVENT_STATE_ACTIVE_LATCHED : EVENT_STATE_ACTIVE;

  update_event_numbers();
  update_led_color();
  update_bms_status();

  // Set the associated event message, even if debug is disabled
  set_message(event);
#ifdef DEBUG_VIA_USB
  Serial.println(events.message);
#endif
}

static void update_bms_status(void) {
  if (events.nof_red_events > 0) {
    bms_status = FAULT;
  } else if (events.nof_blue_events > 0) {
    bms_status = UPDATING;
  } else if (events.nof_yellow_events > 0) {
    // No bms_status update
  }
}

static void update_event_numbers(void) {
  events.nof_red_events = 0;
  events.nof_blue_events = 0;
  events.nof_yellow_events = 0;
  for (uint8_t i = 0u; i < EVENT_NOF_EVENTS; i++) {
    if ((events.entries[i].state == EVENT_STATE_ACTIVE) || (events.entries[i].state == EVENT_STATE_ACTIVE_LATCHED)) {
      switch (events.entries[i].led_color) {
        case GREEN:
          // Just informative
          break;
        case YELLOW:
          events.nof_yellow_events++;
          break;
        case BLUE:
          events.nof_blue_events++;
          break;
        case RED:
          events.nof_red_events++;
          break;
        default:
          break;
      }
    }
  }
}

static void update_event_time(void) {
  unsigned long new_millis = millis();
  if (events.second_timer.elapsed() == true) {
    events.time_seconds++;
  }
}

static void update_led_color(void) {
  if (events.nof_red_events > 0) {
    LEDcolor = RED;
  } else if (events.nof_blue_events > 0) {
    LEDcolor = BLUE;
  } else if (events.nof_yellow_events > 0) {
    LEDcolor = YELLOW;
  } else {
    LEDcolor = GREEN;
  }

  // events.total_led_color = max(events.total_led_color, events.entries[event].led_color);
}

static void set_message(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CAN_FAILURE:
      snprintf(events.message, sizeof(events.message),
               "No CAN communication detected for 60s. Shutting down battery control.");
      break;
    case EVENT_CAN_WARNING:
      snprintf(events.message, sizeof(events.message),
               "ERROR: High amount of corrupted CAN messages detected. Check CAN wire shielding!");
      break;
    case EVENT_WATER_INGRESS:
      snprintf(events.message, sizeof(events.message),
               "Water leakage inside battery detected. Operation halted. Inspect battery!");
      break;
    case EVENT_12V_LOW:
      snprintf(events.message, sizeof(events.message),
               "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!");
      break;
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      snprintf(events.message, sizeof(events.message),
               "ERROR: SOC reported by battery not plausible. Restart battery!");
      break;
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      snprintf(events.message, sizeof(events.message),
               "Warning: kWh remaining reported by battery not plausible. Battery needs cycling.");
      break;
    case EVENT_BATTERY_CHG_STOP_REQ:
      snprintf(events.message, sizeof(events.message),
               "ERROR: Battery raised caution indicator AND requested charge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      snprintf(events.message, sizeof(events.message),
               "ERROR: Battery raised caution indicator AND requested discharge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      snprintf(events.message, sizeof(events.message),
               "ERROR: Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!");
      break;
    case EVENT_LOW_SOH:
      snprintf(
          events.message, sizeof(events.message),
          "ERROR: State of health critically low. Battery internal resistance too high to continue. Recycle battery.");
      break;
    case EVENT_HVIL_FAILURE:
      snprintf(events.message, sizeof(events.message),
               "ERROR: Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be "
               "disabled!");
      break;
    case EVENT_INTERNAL_OPEN_FAULT:
      snprintf(events.message, sizeof(events.message),
               "ERROR: High voltage cable removed while battery running. Opening contactors!");
      break;
    case EVENT_CELL_UNDER_VOLTAGE:
      snprintf(events.message, sizeof(events.message),
               "ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_OVER_VOLTAGE:
      snprintf(events.message, sizeof(events.message),
               "ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_DEVIATION_HIGH:
      snprintf(events.message, sizeof(events.message), "ERROR: HIGH CELL DEVIATION!!! Inspect battery!");
      break;
    case EVENT_UNKNOWN_EVENT_SET:
      snprintf(events.message, sizeof(events.message), "An unknown event was set! Review your code!");
      break;
    case EVENT_OTA_UPDATE:
      snprintf(events.message, sizeof(events.message), "OTA update started!");
      break;
    case EVENT_DUMMY:
      snprintf(events.message, sizeof(events.message), "The dummy event was set!");  // Don't change this event message!
      break;
    default:
      return "UNKNOWN";
  }
}

const char* get_event_message(EVENTS_ENUM_TYPE event) {
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
    case EVENT_DUMMY:
      return "The dummy event was set!";  // Don't change this event message!
    default:
      return "";
  }
}

const char* get_event_enum_string(EVENTS_ENUM_TYPE event) {
  const char* fullString = EVENTS_ENUM_TYPE_STRING[event];
  if (strncmp(fullString, "EVENT_", 6) == 0) {
    return fullString + 6;  // Skip the first 6 characters
  }
  return fullString;
}

static void set_event_message(EVENTS_ENUM_TYPE event) {
  const char* message = get_event_message(event);
  snprintf(event_message, sizeof(event_message), "%s", message);
}
