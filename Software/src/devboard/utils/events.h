#ifndef __EVENTS_H__
#define __EVENTS_H__

#ifndef UNIT_TEST
#include <Arduino.h>
#endif

#include <stdint.h>

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

void init_events(void);
void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void clear_event(EVENTS_ENUM_TYPE event);

void run_event_handling(void);

extern uint8_t bms_status;  //Enum, 0-5
extern uint8_t LEDcolor;

#endif  // __MYTIMER_H__
