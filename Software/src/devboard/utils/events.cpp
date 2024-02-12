#include "events.h"

#ifndef UNIT_TEST
#include <EEPROM.h>
#endif

#include "../../../USER_SETTINGS.h"
#include "../config.h"
#include "timer.h"

#define EE_MAGIC_HEADER_VALUE 0xAA55
#define EE_NOF_EVENT_ENTRIES 3
#define EE_EVENT_ENTRY_SIZE sizeof(EVENT_LOG_ENTRY_TYPE)

#define EE_EVENT_LOG_START_ADDRESS 0
#define EE_EVENT_LOG_HEAD_INDEX_ADDRESS EE_EVENT_LOG_START_ADDRESS + 2
#define EE_EVENT_LOG_TAIL_INDEX_ADDRESS EE_EVENT_LOG_HEAD_INDEX_ADDRESS + 2
#define EE_EVENT_ENTRY_START_ADDRESS EE_EVENT_LOG_TAIL_INDEX_ADDRESS + 2

typedef struct {
  EVENTS_ENUM_TYPE event;
  uint32_t timestamp;
  uint8_t data;
} EVENT_LOG_ENTRY_TYPE;

typedef struct {
  EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
  uint32_t time_seconds;
  MyTimer second_timer;
  MyTimer ee_timer;
  EVENTS_LEVEL_TYPE level;
  uint16_t event_log_head_index;
  uint16_t event_log_tail_index;
} EVENT_TYPE;

/* Local variables */
static EVENT_TYPE events;
static const char* EVENTS_ENUM_TYPE_STRING[] = {EVENTS_ENUM_TYPE(GENERATE_STRING)};
static const char* EVENTS_LEVEL_TYPE_STRING[] = {EVENTS_LEVEL_TYPE(GENERATE_STRING)};

/* Local function prototypes */
static void update_event_time(void);
static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched);
static void update_event_level(void);
static void update_bms_status(void);
static void log_event(EVENTS_ENUM_TYPE event, uint8_t data);
static void print_event_log(void);
static void check_ee_write(void);

/* Exported functions */

/* Main execution function, should handle various continuous functionality */
void run_event_handling(void) {
  update_event_time();
  run_sequence_on_target();
  check_ee_write();
}

/* Initialization function */
void init_events(void) {

  EEPROM.begin(1024);

  uint16_t header = EEPROM.readUShort(EE_EVENT_LOG_START_ADDRESS);
  if (header != EE_MAGIC_HEADER_VALUE) {
    EEPROM.writeUShort(EE_EVENT_LOG_START_ADDRESS, EE_MAGIC_HEADER_VALUE);
    EEPROM.writeUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS, 0);
    EEPROM.writeUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS, 0);
    EEPROM.commit();
    Serial.println("EEPROM wasn't ready");
  } else {
    events.event_log_head_index = EEPROM.readUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS);
    events.event_log_tail_index = EEPROM.readUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS);
    Serial.println("EEPROM was initialized for event logging");
    Serial.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));
    print_event_log();
  }

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[EVENT_CAN_FAILURE].data = 0;
    events.entries[EVENT_CAN_FAILURE].timestamp = 0;
    events.entries[EVENT_CAN_FAILURE].occurences = 0;
    events.entries[EVENT_CAN_FAILURE].log = false;
  }

  events.entries[EVENT_CAN_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_WATER_INGRESS].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_12V_LOW].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOC_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_KWH_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_LOW_SOH].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_HVIL_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INTERNAL_OPEN_FAULT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_UNDER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_OVER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_UNKNOWN_EVENT_SET].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_OTA_UPDATE].level = EVENT_LEVEL_UPDATE;
  events.entries[EVENT_DUMMY_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_DEBUG].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_DUMMY_ERROR].level = EVENT_LEVEL_ERROR;

  events.entries[EVENT_DUMMY_INFO].log = true;
  events.entries[EVENT_DUMMY_DEBUG].log = true;
  events.entries[EVENT_DUMMY_WARNING].log = true;
  events.entries[EVENT_DUMMY_ERROR].log = true;

  events.second_timer.set_interval(1000);
  events.ee_timer.set_interval(15 * 60 * 1000);  // Write to EEPROM every 15 minutes
}

void set_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  set_event(event, data, false);
}

void set_event_latched(EVENTS_ENUM_TYPE event, uint8_t data) {
  set_event(event, data, true);
}

void clear_event(EVENTS_ENUM_TYPE event) {
  if (events.entries[event].state == EVENT_STATE_ACTIVE) {
    events.entries[event].state = EVENT_STATE_INACTIVE;
    update_event_level();
    update_bms_status();
  }
}

const char* get_event_message_string(EVENTS_ENUM_TYPE event) {
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
    case EVENT_DUMMY_INFO:
      return "The dummy info event was set!";  // Don't change this event message!
    case EVENT_DUMMY_DEBUG:
      return "The dummy debug event was set!";  // Don't change this event message!
    case EVENT_DUMMY_WARNING:
      return "The dummy warning event was set!";  // Don't change this event message!
    case EVENT_DUMMY_ERROR:
      return "The dummy error event was set!";  // Don't change this event message!
    case EVENT_SERIAL_RX_WARNING:
      return "Error in serial function: No data received for some time, see data for minutes";
    case EVENT_SERIAL_RX_FAILURE:
      return "Error in serial function: No data for a long time!";
    case EVENT_SERIAL_TX_FAILURE:
      return "Error in serial function: No ACK from receiver!";
    case EVENT_SERIAL_TRANSMITTER_FAILURE:
      return "Error in serial function: Some ERROR level fault in transmitter, received by receiver";
    default:
      return "";
  }
}

const char* get_event_enum_string(EVENTS_ENUM_TYPE event) {
  // Return the event name but skip "EVENT_" that should always be first
  return EVENTS_ENUM_TYPE_STRING[event] + 6;
}

const char* get_event_level_string(EVENTS_ENUM_TYPE event) {
  // Return the event level but skip "EVENT_LEVEL_" that should always be first
  return EVENTS_LEVEL_TYPE_STRING[events.entries[event].level] + 12;
}

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event) {
  return &events.entries[event];
}

EVENTS_LEVEL_TYPE get_event_level(void) {
  return events.level;
}

/* Local functions */

static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched) {
  // Just some defensive stuff if someone sets an unknown event
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }

  // If the event is already set, no reason to continue
  if ((events.entries[event].state != EVENT_STATE_ACTIVE) &&
      (events.entries[event].state != EVENT_STATE_ACTIVE_LATCHED)) {
    events.entries[event].occurences++;
    if (events.entries[event].log) {
      log_event(event, data);
    }
  }

  // We should set the event, update event info
  events.entries[event].timestamp = events.time_seconds;
  events.entries[event].data = data;
  // Check if the event is latching
  events.entries[event].state = latched ? EVENT_STATE_ACTIVE_LATCHED : EVENT_STATE_ACTIVE;

  update_event_level();
  update_bms_status();

#ifdef DEBUG_VIA_USB
  Serial.println(get_event_message_string(event));
#endif
}

static void update_bms_status(void) {
  switch (events.level) {
    case EVENT_LEVEL_INFO:
    case EVENT_LEVEL_WARNING:
    case EVENT_LEVEL_DEBUG:
      bms_status = ACTIVE;
      break;
    case EVENT_LEVEL_UPDATE:
      bms_status = UPDATING;
      break;
    case EVENT_LEVEL_ERROR:
      bms_status = FAULT;
      break;
    default:
      break;
  }
}

static void update_event_level(void) {
  events.level = EVENT_LEVEL_INFO;
  for (uint8_t i = 0u; i < EVENT_NOF_EVENTS; i++) {
    if ((events.entries[i].state == EVENT_STATE_ACTIVE) || (events.entries[i].state == EVENT_STATE_ACTIVE_LATCHED)) {
      events.level = max(events.entries[i].level, events.level);
    }
  }
}

static void update_event_time(void) {
  if (events.second_timer.elapsed() == true) {
    events.time_seconds++;
  }
}

static void log_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  // Update head, move tail
  if (++events.event_log_head_index == EE_NOF_EVENT_ENTRIES) {
    events.event_log_head_index = 0;
  }

  if (events.event_log_head_index == events.event_log_tail_index) {
    if (++events.event_log_tail_index == EE_NOF_EVENT_ENTRIES) {
      events.event_log_tail_index = 0;
    }
  }

  int entry_address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * events.event_log_head_index;
  EVENT_LOG_ENTRY_TYPE entry = {.event = event, .timestamp = events.time_seconds, .data = data};

  EEPROM.put(entry_address, entry);
  EEPROM.writeUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS, events.event_log_head_index);
  EEPROM.writeUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS, events.event_log_tail_index);
  //Serial.println("Wrote event " + String(event) + " to " + String(entry_address));
  //Serial.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));
}

static void print_event_log(void) {
  if (events.event_log_head_index == events.event_log_tail_index) {
    Serial.println("No events in log");
    return;
  }
  EVENT_LOG_ENTRY_TYPE entry;

  for (int i = 0; i < EE_NOF_EVENT_ENTRIES; i++) {
    int index = ((events.event_log_tail_index + i) % EE_NOF_EVENT_ENTRIES);
    int address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * index;

    EEPROM.get(address, entry);
    Serial.println("Event: " + String(get_event_enum_string(entry.event)) + ", data: " + String(entry.data) +
                   ", time: " + String(entry.timestamp));

    if (index == events.event_log_head_index) {
      break;
    }
  }
}

static void check_ee_write(void) {
  if (events.ee_timer.elapsed()) {
    EEPROM.commit();
  }
}
