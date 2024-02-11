#ifndef __EVENTS_H__
#define __EVENTS_H__
#include <stdint.h>

#ifndef UNIT_TEST
#include <Arduino.h>
extern unsigned long previous_millis;
extern uint32_t time_seconds;
#endif

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
  XX(EVENT_DUMMY)                       \
  XX(EVENT_NOF_EVENTS)

typedef enum {
  EVENT_CAN_FAILURE = 0u,             // RED event
  EVENT_CAN_WARNING,                  // YELLOW event
  EVENT_WATER_INGRESS,                // RED event
  EVENT_12V_LOW,                      // YELLOW event
  EVENT_SOC_PLAUSIBILITY_ERROR,       // RED event
  EVENT_KWH_PLAUSIBILITY_ERROR,       // YELLOW event
  EVENT_BATTERY_CHG_STOP_REQ,         // RED event
  EVENT_BATTERY_DISCHG_STOP_REQ,      // RED event
  EVENT_BATTERY_CHG_DISCHG_STOP_REQ,  // RED event
  EVENT_LOW_SOH,                      // RED event
  EVENT_HVIL_FAILURE,                 // RED event
  EVENT_INTERNAL_OPEN_FAULT,          // RED event
  EVENT_CELL_UNDER_VOLTAGE,           // RED event
  EVENT_CELL_OVER_VOLTAGE,            // RED event
  EVENT_CELL_DEVIATION_HIGH,          // YELLOW event
  EVENT_UNKNOWN_EVENT_SET,            // RED event
  EVENT_OTA_UPDATE,                   // BLUE event
  EVENT_DUMMY,                        // RED event
  EVENT_NOF_EVENTS                    // RED event
} EVENTS_ENUM_TYPE;
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum { EVENTS_ENUM_TYPE(GENERATE_ENUM) } EVENTS_ENUM_TYPE;

static const char* EVENTS_ENUM_TYPE_STRING[] = {EVENTS_ENUM_TYPE(GENERATE_STRING)};

const char* get_event_enum_string(EVENTS_ENUM_TYPE event);

const char* get_event_message(EVENTS_ENUM_TYPE event);

const char* get_led_color_display_text(u_int8_t led_color);

void init_events(void);
void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void clear_event(EVENTS_ENUM_TYPE event);

void run_event_handling(void);

extern uint8_t bms_status;  //Enum, 0-5
extern uint8_t LEDcolor;

#endif  // __MYTIMER_H__
