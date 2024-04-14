#ifndef __EVENTS_H__
#define __EVENTS_H__
#ifndef UNIT_TEST
#include "../../include.h"
#endif

// #define INCLUDE_EVENTS_TEST  // Enable to run an event test loop, see events_test_on_target.cpp

#define EE_MAGIC_HEADER_VALUE 0x0001  // 0x0000 to 0xFFFF

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

/** EVENT ENUMERATION
 *
 * Try not to change the order!
 * When adding events, add them RIGHT BEFORE the EVENT_NOF_EVENTS enum.
 * In addition, the event name must start with "EVENT_".
 * If you don't follow this instruction, the EEPROM log will become corrupt.
 * To handle this, follow the instruction for EE_MAGIC_HEADER_VALUE as
 * described below.
 * 
 * After adding an event:
 * - Assign the proper event level in events.cpp:init_events()
 * - Increment EE_MAGIC_HEADER_VALUE in case you've changed the order
 */

#define EVENTS_ENUM_TYPE(XX)            \
  XX(EVENT_CANFD_INIT_FAILURE)          \
  XX(EVENT_CAN_OVERRUN)                 \
  XX(EVENT_CAN_RX_FAILURE)              \
  XX(EVENT_CANFD_RX_FAILURE)            \
  XX(EVENT_CAN_RX_WARNING)              \
  XX(EVENT_CAN_TX_FAILURE)              \
  XX(EVENT_WATER_INGRESS)               \
  XX(EVENT_12V_LOW)                     \
  XX(EVENT_SOC_PLAUSIBILITY_ERROR)      \
  XX(EVENT_KWH_PLAUSIBILITY_ERROR)      \
  XX(EVENT_BATTERY_EMPTY)               \
  XX(EVENT_BATTERY_FULL)                \
  XX(EVENT_BATTERY_CHG_STOP_REQ)        \
  XX(EVENT_BATTERY_DISCHG_STOP_REQ)     \
  XX(EVENT_BATTERY_CHG_DISCHG_STOP_REQ) \
  XX(EVENT_LOW_SOH)                     \
  XX(EVENT_HVIL_FAILURE)                \
  XX(EVENT_INTERNAL_OPEN_FAULT)         \
  XX(EVENT_INVERTER_OPEN_CONTACTOR)     \
  XX(EVENT_CELL_UNDER_VOLTAGE)          \
  XX(EVENT_CELL_OVER_VOLTAGE)           \
  XX(EVENT_CELL_DEVIATION_HIGH)         \
  XX(EVENT_UNKNOWN_EVENT_SET)           \
  XX(EVENT_OTA_UPDATE)                  \
  XX(EVENT_OTA_UPDATE_TIMEOUT)          \
  XX(EVENT_DUMMY_INFO)                  \
  XX(EVENT_DUMMY_DEBUG)                 \
  XX(EVENT_DUMMY_WARNING)               \
  XX(EVENT_DUMMY_ERROR)                 \
  XX(EVENT_SERIAL_RX_WARNING)           \
  XX(EVENT_SERIAL_RX_FAILURE)           \
  XX(EVENT_SERIAL_TX_FAILURE)           \
  XX(EVENT_SERIAL_TRANSMITTER_FAILURE)  \
  XX(EVENT_EEPROM_WRITE)                \
  XX(EVENT_NOF_EVENTS)

typedef enum { EVENTS_ENUM_TYPE(GENERATE_ENUM) } EVENTS_ENUM_TYPE;

/* Event type enumeration, keep in order of priority! */
#define EVENTS_LEVEL_TYPE(XX) \
  XX(EVENT_LEVEL_INFO)        \
  XX(EVENT_LEVEL_DEBUG)       \
  XX(EVENT_LEVEL_WARNING)     \
  XX(EVENT_LEVEL_ERROR)       \
  XX(EVENT_LEVEL_UPDATE)

typedef enum { EVENTS_LEVEL_TYPE(GENERATE_ENUM) } EVENTS_LEVEL_TYPE;

typedef enum {
  EVENT_STATE_PENDING = 0,
  EVENT_STATE_INACTIVE,
  EVENT_STATE_ACTIVE,
  EVENT_STATE_ACTIVE_LATCHED
} EVENTS_STATE_TYPE;

typedef struct {
  uint32_t timestamp;       // Time in seconds since startup when the event occurred
  uint8_t data;             // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;       // Number of occurrences since startup
  EVENTS_LEVEL_TYPE level;  // Event level, i.e. ERROR/WARNING...
  EVENTS_STATE_TYPE state;  // Event state, i.e. ACTIVE/INACTIVE...
  bool log;
} EVENTS_STRUCT_TYPE;

const char* get_event_enum_string(EVENTS_ENUM_TYPE event);
const char* get_event_message_string(EVENTS_ENUM_TYPE event);
const char* get_event_level_string(EVENTS_ENUM_TYPE event);
const char* get_event_type(EVENTS_ENUM_TYPE event);

EVENTS_LEVEL_TYPE get_event_level(void);

void init_events(void);
void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void clear_event(EVENTS_ENUM_TYPE event);

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event);

void run_event_handling(void);

void run_sequence_on_target(void);

#endif  // __MYTIMER_H__
