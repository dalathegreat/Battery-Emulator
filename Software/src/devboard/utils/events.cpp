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

/* Local function prototypes */
static void print_event_message(EVENTS_ENUM_TYPE event);
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
  entries[event].occurences++;
  print_event_message(event);
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

static void print_event_message(EVENTS_ENUM_TYPE event) {
#ifndef DEBUG_VIA_USB
  return;
#else
  switch (event) {
    case EVENT_CAN_FAILURE:
      Serial.println("No CAN communication detected for 60s. Shutting down battery control.");
      break;
    case EVENT_CAN_WARNING:
      Serial.println("ERROR: High amount of corrupted CAN messages detected. Check CAN wire shielding!");
      break;
    case EVENT_WATER_INGRESS:
      Serial.println("Water leakage inside battery detected. Operation halted. Inspect battery!");
      break;
    case EVENT_12V_LOW:
      Serial.println(
          "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!");
      break;
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      Serial.println("ERROR: SOC% reported by battery not plausible. Restart battery!");
      break;
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      Serial.println("Warning: kWh remaining reported by battery not plausible. Battery needs cycling.");
      break;
    case EVENT_BATTERY_CHG_STOP_REQ:
      Serial.println("ERROR: Battery raised caution indicator AND requested charge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      Serial.println("ERROR: Battery raised caution indicator AND requested discharge stop. Inspect battery status!");
      break;
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      Serial.println(
          "ERROR: Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!");
      break;
    case EVENT_LOW_SOH:
      Serial.println(
          "ERROR: State of health critically low. Battery internal resistance too high to continue. Recycle battery.");
      break;
    case EVENT_HVIL_FAILURE:
      Serial.println(
          "ERROR: Battery interlock loop broken. Check that high voltage connectors are seated. Battery will be "
          "disabled!");
      break;
    case EVENT_INTERNAL_OPEN_FAULT:
      Serial.println("ERROR: High voltage cable removed while battery running. Opening contactors!");
      break;
    case EVENT_CELL_UNDER_VOLTAGE:
      Serial.println("ERROR: CELL UNDERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_OVER_VOLTAGE:
      Serial.println("ERROR: CELL OVERVOLTAGE!!! Stopping battery charging and discharging. Inspect battery!");
      break;
    case EVENT_CELL_DEVIATION_HIGH:
      Serial.println("ERROR: HIGH CELL DEVIATION!!! Inspect battery!");
      break;
    case EVENT_UNKNOWN_EVENT_SET:
      Serial.println("An unknown event was set! Review your code!");
      break;
    case EVENT_DUMMY:
      Serial.println("The dummy event was set!");
      break;
    default:
      break;
  }
#endif
}
