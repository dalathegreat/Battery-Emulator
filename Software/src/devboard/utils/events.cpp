#include "events.h"

typedef struct {
  uint32_t timestamp;  // Time in seconds since startup when the event occurred
  uint8_t data;        // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;  // Number of occurrences since startup
} EVENTS_STRUCT_TYPE;

static EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
static unsigned long previous_millis = 0;
static uint32_t time_seconds = 0;

void init_events(void) {
  for (uint8_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    entries[i].timestamp = 0;
    entries[i].data = 0;
    entries[i].occurences = 0;
  }
}

void set_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  entries[event].timestamp = time_seconds;
  entries[event].data = data;
  entries[event].occurences++;
}

void update_event_timestamps(void) {
  unsigned long new_millis = millis();
  if (new_millis - previous_millis >= 1000) {
    time_seconds++;
    previous_millis = new_millis;
  }
}
