#ifndef __EVENTS_H__
#define __EVENTS_H__
#ifndef UNIT_TEST
#include "../../include.h"
#endif

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

#define EVENTS_ENUM_TYPE(XX)                   \
  XX(EVENT_CANMCP2517FD_INIT_FAILURE)          \
  XX(EVENT_CANMCP2515_INIT_FAILURE)            \
  XX(EVENT_CANFD_BUFFER_FULL)                  \
  XX(EVENT_CAN_BUFFER_FULL)                    \
  XX(EVENT_CAN_CORRUPTED_WARNING)              \
  XX(EVENT_CAN_BATTERY_MISSING)                \
  XX(EVENT_CAN_BATTERY2_MISSING)               \
  XX(EVENT_CAN_CHARGER_MISSING)                \
  XX(EVENT_CAN_INVERTER_MISSING)               \
  XX(EVENT_CAN_NATIVE_TX_FAILURE)              \
  XX(EVENT_CHARGE_LIMIT_EXCEEDED)              \
  XX(EVENT_CONTACTOR_WELDED)                   \
  XX(EVENT_CPU_OVERHEATING)                    \
  XX(EVENT_CPU_OVERHEATED)                     \
  XX(EVENT_DISCHARGE_LIMIT_EXCEEDED)           \
  XX(EVENT_WATER_INGRESS)                      \
  XX(EVENT_12V_LOW)                            \
  XX(EVENT_SOC_PLAUSIBILITY_ERROR)             \
  XX(EVENT_SOC_UNAVAILABLE)                    \
  XX(EVENT_STALE_VALUE)                        \
  XX(EVENT_KWH_PLAUSIBILITY_ERROR)             \
  XX(EVENT_BALANCING_START)                    \
  XX(EVENT_BALANCING_END)                      \
  XX(EVENT_BATTERY_EMPTY)                      \
  XX(EVENT_BATTERY_FULL)                       \
  XX(EVENT_BATTERY_FUSE)                       \
  XX(EVENT_BATTERY_FROZEN)                     \
  XX(EVENT_BATTERY_CAUTION)                    \
  XX(EVENT_BATTERY_CHG_STOP_REQ)               \
  XX(EVENT_BATTERY_DISCHG_STOP_REQ)            \
  XX(EVENT_BATTERY_CHG_DISCHG_STOP_REQ)        \
  XX(EVENT_BATTERY_OVERHEAT)                   \
  XX(EVENT_BATTERY_OVERVOLTAGE)                \
  XX(EVENT_BATTERY_UNDERVOLTAGE)               \
  XX(EVENT_BATTERY_VALUE_UNAVAILABLE)          \
  XX(EVENT_BATTERY_ISOLATION)                  \
  XX(EVENT_BATTERY_REQUESTS_HEAT)              \
  XX(EVENT_BATTERY_WARMED_UP)                  \
  XX(EVENT_VOLTAGE_DIFFERENCE)                 \
  XX(EVENT_SOH_DIFFERENCE)                     \
  XX(EVENT_SOH_LOW)                            \
  XX(EVENT_HVIL_FAILURE)                       \
  XX(EVENT_PRECHARGE_FAILURE)                  \
  XX(EVENT_INTERNAL_OPEN_FAULT)                \
  XX(EVENT_INVERTER_OPEN_CONTACTOR)            \
  XX(EVENT_INTERFACE_MISSING)                  \
  XX(EVENT_MODBUS_INVERTER_MISSING)            \
  XX(EVENT_NO_ENABLE_DETECTED)                 \
  XX(EVENT_ERROR_OPEN_CONTACTOR)               \
  XX(EVENT_CELL_CRITICAL_UNDER_VOLTAGE)        \
  XX(EVENT_CELL_CRITICAL_OVER_VOLTAGE)         \
  XX(EVENT_CELL_UNDER_VOLTAGE)                 \
  XX(EVENT_CELL_OVER_VOLTAGE)                  \
  XX(EVENT_CELL_DEVIATION_HIGH)                \
  XX(EVENT_UNKNOWN_EVENT_SET)                  \
  XX(EVENT_OTA_UPDATE)                         \
  XX(EVENT_OTA_UPDATE_TIMEOUT)                 \
  XX(EVENT_DUMMY_INFO)                         \
  XX(EVENT_DUMMY_DEBUG)                        \
  XX(EVENT_DUMMY_WARNING)                      \
  XX(EVENT_DUMMY_ERROR)                        \
  XX(EVENT_PERSISTENT_SAVE_INFO)               \
  XX(EVENT_SERIAL_RX_WARNING)                  \
  XX(EVENT_SERIAL_RX_FAILURE)                  \
  XX(EVENT_SERIAL_TX_FAILURE)                  \
  XX(EVENT_SERIAL_TRANSMITTER_FAILURE)         \
  XX(EVENT_SMA_PAIRING)                        \
  XX(EVENT_TASK_OVERRUN)                       \
  XX(EVENT_RESET_UNKNOWN)                      \
  XX(EVENT_RESET_POWERON)                      \
  XX(EVENT_RESET_EXT)                          \
  XX(EVENT_RESET_SW)                           \
  XX(EVENT_RESET_PANIC)                        \
  XX(EVENT_RESET_INT_WDT)                      \
  XX(EVENT_RESET_TASK_WDT)                     \
  XX(EVENT_RESET_WDT)                          \
  XX(EVENT_RESET_DEEPSLEEP)                    \
  XX(EVENT_RESET_BROWNOUT)                     \
  XX(EVENT_RESET_SDIO)                         \
  XX(EVENT_RESET_USB)                          \
  XX(EVENT_RESET_JTAG)                         \
  XX(EVENT_RESET_EFUSE)                        \
  XX(EVENT_RESET_PWR_GLITCH)                   \
  XX(EVENT_RESET_CPU_LOCKUP)                   \
  XX(EVENT_RJXZS_LOG)                          \
  XX(EVENT_PAUSE_BEGIN)                        \
  XX(EVENT_PAUSE_END)                          \
  XX(EVENT_PID_FAILED)                         \
  XX(EVENT_WIFI_CONNECT)                       \
  XX(EVENT_WIFI_DISCONNECT)                    \
  XX(EVENT_MQTT_CONNECT)                       \
  XX(EVENT_MQTT_DISCONNECT)                    \
  XX(EVENT_EQUIPMENT_STOP)                     \
  XX(EVENT_AUTOMATIC_PRECHARGE_FAILURE)        \
  XX(EVENT_SD_INIT_FAILED)                     \
  XX(EVENT_PERIODIC_BMS_RESET)                 \
  XX(EVENT_PERIODIC_BMS_RESET_AT_INIT_SUCCESS) \
  XX(EVENT_PERIODIC_BMS_RESET_AT_INIT_FAILED)  \
  XX(EVENT_BATTERY_TEMP_DEVIATION_HIGH)        \
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
  uint32_t timestamp;           // Time in seconds since startup when the event occurred
  uint8_t millisrolloverCount;  // number of times millis rollovers before timestamp
  uint8_t data;                 // Custom data passed when setting the event, for example cell number for under voltage
  uint8_t occurences;           // Number of occurrences since startup
  EVENTS_LEVEL_TYPE level;      // Event level, i.e. ERROR/WARNING...
  EVENTS_STATE_TYPE state;      // Event state, i.e. ACTIVE/INACTIVE...
  bool MQTTpublished;
} EVENTS_STRUCT_TYPE;

// Define a struct to hold event data
struct EventData {
  EVENTS_ENUM_TYPE event_handle;
  const EVENTS_STRUCT_TYPE* event_pointer;
};

const char* get_event_enum_string(EVENTS_ENUM_TYPE event);
const char* get_event_message_string(EVENTS_ENUM_TYPE event);
const char* get_event_level_string(EVENTS_ENUM_TYPE event);

EVENTS_LEVEL_TYPE get_event_level(void);

void init_events(void);
void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data);
void set_event(EVENTS_ENUM_TYPE event, uint8_t data);
void clear_event(EVENTS_ENUM_TYPE event);
void reset_all_events();
void set_event_MQTTpublished(EVENTS_ENUM_TYPE event);

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event);

bool compareEventsByTimestampAsc(const EventData& a, const EventData& b);
bool compareEventsByTimestampDesc(const EventData& a, const EventData& b);

#endif  // __MYTIMER_H__
