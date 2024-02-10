#include "events.h"

#include "../../../USER_SETTINGS.h"
#include "../config.h"

unsigned long previous_millis = 0;
uint32_t time_seconds = 0;
static uint8_t total_led_color = GREEN;
static char event_message[256];
EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];

/* Local function prototypes */
static void set_event_message(EVENTS_ENUM_TYPE event);
static void update_led_color(EVENTS_ENUM_TYPE event);

/* Exported functions */
void init_events(void) {
  for (uint8_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    entries[i].timestamp = 0;
    entries[i].data = 0;
    entries[i].occurences = 0;
    entries[i].led_color = RED;  // Most events are RED
  }

  // YELLOW events below
  entries[EVENT_12V_LOW].led_color = YELLOW;
  entries[EVENT_CAN_WARNING].led_color = YELLOW;
  entries[EVENT_CELL_DEVIATION_HIGH].led_color = YELLOW;
  entries[EVENT_KWH_PLAUSIBILITY_ERROR].led_color = YELLOW;
}

void set_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }
  entries[event].timestamp = time_seconds;
  entries[event].data = data;
  ++entries[event].occurences;
  set_event_message(event);
#ifdef DEBUG_VIA_USB
  Serial.println("Set event: " + String(get_event_enum_string(event)) + ". Has occured " +
                 String(entries[event].occurences) + " times");
#endif
}

void update_event_timestamps(void) {
  unsigned long new_millis = millis();
  if (new_millis - previous_millis >= 1000) {
    time_seconds++;
    previous_millis = new_millis;
  }
}

/* Local functions */
static void update_led_color(EVENTS_ENUM_TYPE event) {
  total_led_color = (total_led_color == RED) ? RED : entries[event].led_color;
}

const char* get_led_color_display_text(u_int8_t led_color) {
  switch (led_color) {
    case RED:
      return "RED";
    case YELLOW:
      return "YELLOW";
    case GREEN:
      return "GREEN";
    case BLUE:
      return "BLUE";
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
