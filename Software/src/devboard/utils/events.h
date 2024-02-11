#ifndef __EVENTS_H__
#define __EVENTS_H__
#include <stdint.h>

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

/* Event enumeration */
#define EVENTS_ENUM_TYPE(XX)            \
  XX(EVENT_CAN_FAILURE)                 \
  XX(EVENT_CAN_WARNING)                 \
  XX(EVENT_WATER_INGRESS)               \
  XX(EVENT_12V_LOW)                     \
  XX(EVENT_SOC_PLAUSIBILITY_ERROR)      \
  XX(EVENT_KWH_PLAUSIBILITY_ERROR)      \
  XX(EVENT_BATTERY_CHG_STOP_REQ)        \
  XX(EVENT_BATTERY_DISCHG_STOP_REQ)     \
  XX(EVENT_BATTERY_CHG_DISCHG_STOP_REQ) \
  XX(EVENT_LOW_SOH)                     \
  XX(EVENT_HVIL_FAILURE)                \
  XX(EVENT_INTERNAL_OPEN_FAULT)         \
  XX(EVENT_CELL_UNDER_VOLTAGE)          \
  XX(EVENT_CELL_OVER_VOLTAGE)           \
  XX(EVENT_CELL_DEVIATION_HIGH)         \
  XX(EVENT_UNKNOWN_EVENT_SET)           \
  XX(EVENT_OTA_UPDATE)                  \
  XX(EVENT_DUMMY)                       \
  XX(EVENT_NOF_EVENTS)

typedef enum { EVENTS_ENUM_TYPE(GENERATE_ENUM) } EVENTS_ENUM_TYPE;

/* Event type enumeration */
#define EVENTS_LEVEL_TYPE(XX) \
  XX(EVENT_LEVEL_ERROR)       \
  XX(EVENT_LEVEL_WARNING)     \
  XX(EVENT_LEVEL_INFO)        \
  XX(EVENT_LEVEL_DEBUG)

typedef enum { EVENTS_LEVEL_TYPE(GENERATE_ENUM) } EVENTS_LEVEL_TYPE;

typedef enum {
  EVENT_STATE_INIT = 0,
  EVENT_STATE_INACTIVE,
  EVENT_STATE_ACTIVE,
  EVENT_STATE_ACTIVE_LATCHED
} EVENTS_STATE_TYPE;

typedef struct {
  uint32_t timestamp;       // Time in seconds since startup when the event occurred
  uint8_t data;             // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;       // Number of occurrences since startup
  uint8_t led_color;        // LED indication
  EVENTS_LEVEL_TYPE level;  // Event level, i.e. ERROR/WARNING...
  EVENTS_STATE_TYPE state;  // Event state, i.e. ACTIVE/INACTIVE...
} EVENTS_STRUCT_TYPE;

const char* get_event_enum_string(EVENTS_ENUM_TYPE event);
const char* get_event_message_string(EVENTS_ENUM_TYPE event);
const char* get_event_level_string(EVENTS_ENUM_TYPE event);
const char* get_event_type(EVENTS_ENUM_TYPE event);

void init_events(void);
void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void clear_event(EVENTS_ENUM_TYPE event);

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event);

void run_event_handling(void);

extern uint8_t bms_status;  //Enum, 0-5
extern uint8_t LEDcolor;

#endif  // __MYTIMER_H__
