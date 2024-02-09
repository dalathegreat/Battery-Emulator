#ifndef __EVENTS_H__
#define __EVENTS_H__

#ifndef UNIT_TEST
#include <Arduino.h>
extern unsigned long previous_millis;
extern uint32_t time_seconds;
#endif

#include <stdint.h>

#define EVENTS_ENUM_TYPE(XX) \
    XX(EVENT_CAN_FAILURE) \
    XX(EVENT_CAN_WARNING) \
    XX(EVENT_WATER_INGRESS) \
    XX(EVENT_12V_LOW) \
    XX(EVENT_SOC_PLAUSIBILITY_ERROR) \
    XX(EVENT_KWH_PLAUSIBILITY_ERROR) \
    XX(EVENT_BATTERY_CHG_STOP_REQ) \
    XX(EVENT_BATTERY_DISCHG_STOP_REQ) \
    XX(EVENT_BATTERY_CHG_DISCHG_STOP_REQ) \
    XX(EVENT_LOW_SOH) \
    XX(EVENT_HVIL_FAILURE) \
    XX(EVENT_INTERNAL_OPEN_FAULT) \
    XX(EVENT_CELL_UNDER_VOLTAGE) \
    XX(EVENT_CELL_OVER_VOLTAGE) \
    XX(EVENT_CELL_DEVIATION_HIGH) \
    XX(EVENT_UNKNOWN_EVENT_SET) \
    XX(EVENT_DUMMY) \
    XX(EVENT_NOF_EVENTS)

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum {
    EVENTS_ENUM_TYPE(GENERATE_ENUM)
} EVENTS_ENUM_TYPE;

static const char *EVENTS_ENUM_TYPE_STRING[] = {
    EVENTS_ENUM_TYPE(GENERATE_STRING)
};

const char* get_event_enum_string(EVENTS_ENUM_TYPE event);

const char* get_event_message(EVENTS_ENUM_TYPE event);

const char* get_led_color_display_text(u_int8_t led_color);

void init_events(void);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void update_event_timestamps(void);
typedef struct {
  uint32_t timestamp;  // Time in seconds since startup when the event occurred
  uint8_t data;        // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;  // Number of occurrences since startup
  uint8_t led_color;   // Weirdly indented comment
} EVENTS_STRUCT_TYPE;
extern EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];

#endif  // __MYTIMER_H__
