#include "events.h"

#include "../../../USER_SETTINGS.h"
#include "../config.h"

typedef struct {
  uint32_t timestamp;  // Time in seconds since startup when the event occurred
  uint8_t data;        // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;  // Number of occurrences since startup
  uint8_t led_color;   // Weirdly indented comment
} EVENTS_STRUCT_TYPE;

static EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
static unsigned long previous_millis = 0;
static uint32_t time_seconds = 0;
static uint8_t total_led_color = GREEN;
static char event_message[256];

/* Local function prototypes */
static void update_event_time(void);
static void set_event_message(EVENTS_ENUM_TYPE event);
static void update_led_color(EVENTS_ENUM_TYPE event);

/* Exported functions */
void run_event_handling(void) {
  update_event_time();
}

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

  // BLUE...
  entries[EVENT_OTA_UPDATE].led_color = BLUE;
}

void set_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }
  entries[event].timestamp = time_seconds;
  entries[event].data = data;
  entries[event].occurences++;

  update_led_color(event);

  if (total_led_color == RED) {
    bms_status = FAULT;
  } else if (total_led_color) {
    bms_status = UPDATING;
  }

  set_event_message(event);
#ifdef DEBUG_VIA_USB
  Serial.println(event_message);
#endif
}

uint8_t get_event_ledcolor(void) {
  return total_led_color;
}

/* Local functions */
static void update_event_time(void) {
  unsigned long new_millis = millis();
  if (new_millis - previous_millis >= 1000) {
    time_seconds++;
    previous_millis = new_millis;
  }
}

static void update_led_color(EVENTS_ENUM_TYPE event) {
  total_led_color = max(total_led_color, entries[event].led_color);
}

static void set_event_message(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CAN_FAILURE:
      snprintf(event_message, sizeof(event_message),
               "No CAN communication detected for 60s. Shutting down battery control.");
      break;
    case EVENT_CAN_WARNING:
      snprintf(event_message, sizeof(event_message),
               "ERROR: High amount of corrupted CAN messages detected. Check CAN wire shielding!");
      break;
    case EVENT_WATER_INGRESS:
      snprintf(event_message, sizeof(event_message),
               "Water leakage inside battery detected. Operation halted. Inspect battery!");
      break;
    case EVENT_12V_LOW:
      snprintf(event_message, sizeof(event_message),
               "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!");
      break;
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      snprintf(event_message, sizeof(event_message), "ERROR: SOC reported by battery not plausible. Restart battery!");
      break;
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      snprintf(event_message, sizeof(event_message),
               "Warning: kWh remaining reported by battery not plausible. Battery needs cycling.");
      break;
    case EVENT_BATTERY_CHG_STOP_REQ:
      snprintf(event_message, sizeof(event_message),
               "ERROR: Battery raised caution indicator AND requested charge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      snprintf(event_message, sizeof(event_message),
               "ERROR: Battery raised caution indicator AND requested discharge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      snprintf(event_message, sizeof(event_message),
               "ERROR: Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!");
      break;
    case EVENT_LOW_SOH:
      snprintf(
          event_message, sizeof(event_message),
          "ERROR: State of health critically low. Battery internal resistance too high to continue. Recycle battery.");
      break;
    case EVENT_HVIL_FAILURE:
      snprintf(event_message, sizeof(event_message),
               "ERROR: Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be "
               "disabled!");
      break;
    case EVENT_INTERNAL_OPEN_FAULT:
      snprintf(event_message, sizeof(event_message),
               "ERROR: High voltage cable removed while battery running. Opening contactors!");
      break;
    case EVENT_CELL_UNDER_VOLTAGE:
      snprintf(event_message, sizeof(event_message),
               "ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_OVER_VOLTAGE:
      snprintf(event_message, sizeof(event_message),
               "ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_DEVIATION_HIGH:
      snprintf(event_message, sizeof(event_message), "ERROR: HIGH CELL DEVIATION!!! Inspect battery!");
      break;
    case EVENT_UNKNOWN_EVENT_SET:
      snprintf(event_message, sizeof(event_message), "An unknown event was set! Review your code!");
      break;
    case EVENT_OTA_UPDATE:
      snprintf(event_message, sizeof(event_message), "OTA update started!");
      break;
    case EVENT_DUMMY:
      snprintf(event_message, sizeof(event_message), "The dummy event was set!");  // Don't change this event message!
      break;
    default:
      break;
  }
}
