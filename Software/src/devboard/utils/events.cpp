#include "events.h"
#include "../../datalayer/datalayer.h"
#ifndef UNIT_TEST
#include <EEPROM.h>
#endif

#include "../../../USER_SETTINGS.h"
#include "timer.h"

#define EE_NOF_EVENT_ENTRIES 30
#define EE_EVENT_ENTRY_SIZE sizeof(EVENT_LOG_ENTRY_TYPE)
#define EE_WRITE_PERIOD_MINUTES 10

/** EVENT LOG STRUCTURE
 * 
 * The event log is stored in a simple header-block structure. The
 * header contains a magic number to identify it as an event log,
 * a head index and a tail index. The head index points to the last
 * recorded event, the tail index points to the "oldest" event in the
 * log. The event log is set up like a circular buffer, so we only
 * store the set amount of events. The head continuously overwrites
 * the oldest events, and both the head and tail indices wrap around
 * to 0 at the end of the event log:
 * 
 * [ HEADER ]
 * [ MAGIC NUMBER ][ HEAD INDEX ][ TAIL INDEX ][ EVENT BLOCK 0 ][ EVENT BLOCK 1]...
 * [ 2 bytes      ][ 2 bytes    ][ 2 bytes    ][ 6 bytes       ][ 6 bytes      ]
 * 
 * 1024 bytes are allocated to the event log in flash emulated EEPROM,
 * giving room for (1024 - (2 + 2 + 2)) / 6 ~= 169 events
 * 
 * For now, we store 30 to make it easier to handle initial debugging.
*/
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
  MyTimer update_timer;
  EVENTS_LEVEL_TYPE level;
  uint16_t event_log_head_index;
  uint16_t event_log_tail_index;
  uint8_t nof_logged_events;
  uint16_t nof_eeprom_writes;
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
  update_event_level();
}

/* Initialization function */
void init_events(void) {

  EEPROM.begin(1024);
  events.nof_logged_events = 0;

  uint16_t header = EEPROM.readUShort(EE_EVENT_LOG_START_ADDRESS);
  if (header != EE_MAGIC_HEADER_VALUE) {
    // The header doesn't appear to be a compatible event log, clear it and initialize
    EEPROM.writeUShort(EE_EVENT_LOG_START_ADDRESS, EE_MAGIC_HEADER_VALUE);
    EEPROM.writeUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS, 0);
    EEPROM.writeUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS, 0);

    // Prepare an empty event block to write
    EVENT_LOG_ENTRY_TYPE entry = {.event = EVENT_NOF_EVENTS, .timestamp = 0, .data = 0};

    // Put the event in (what I guess is) the RAM EEPROM mirror, or write buffer

    for (int i = 0; i < EE_NOF_EVENT_ENTRIES; i++) {
      // Start at the oldest event, work through the log all the way the the head
      int address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * i;
      EEPROM.put(address, entry);
    }

    // Push changes to eeprom
    EEPROM.commit();
#ifdef DEBUG_VIA_USB
    Serial.println("EEPROM wasn't ready");
#endif
  } else {
    events.event_log_head_index = EEPROM.readUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS);
    events.event_log_tail_index = EEPROM.readUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS);
#ifdef DEBUG_VIA_USB
    Serial.println("EEPROM was initialized for event logging");
    Serial.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));
#endif
    print_event_log();
  }

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[i].data = 0;
    events.entries[i].timestamp = 0;
    events.entries[i].occurences = 0;
    events.entries[i].log = true;
  }

  events.entries[EVENT_CANFD_INIT_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_OVERRUN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_RX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CANFD_RX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_RX_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_TX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_WATER_INGRESS].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_12V_LOW].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOC_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_KWH_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_EMPTY].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_FULL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_LOW_SOH].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_HVIL_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INTERNAL_OPEN_FAULT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INVERTER_OPEN_CONTACTOR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CELL_UNDER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_OVER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_UNKNOWN_EVENT_SET].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_OTA_UPDATE].level = EVENT_LEVEL_UPDATE;
  events.entries[EVENT_OTA_UPDATE_TIMEOUT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_DEBUG].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_DUMMY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_RX_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SERIAL_RX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TRANSMITTER_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_EEPROM_WRITE].level = EVENT_LEVEL_INFO;

  events.entries[EVENT_EEPROM_WRITE].log = false;  // Don't log the logger...

  events.second_timer.set_interval(1000);
  // Write to EEPROM every X minutes (if an event has been set)
  events.ee_timer.set_interval(EE_WRITE_PERIOD_MINUTES * 60 * 1000);
  events.update_timer.set_interval(2000);
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
    case EVENT_CANFD_INIT_FAILURE:
      return "CAN-FD initialization failed. Check hardware or bitrate settings";
    case EVENT_CAN_OVERRUN:
      return "CAN message failed to send within defined time. Contact developers, CPU load might be too high.";
    case EVENT_CAN_RX_FAILURE:
      return "No CAN communication detected for 60s. Shutting down battery control.";
    case EVENT_CANFD_RX_FAILURE:
      return "No CANFD communication detected for 60s. Shutting down battery control.";
    case EVENT_CAN_RX_WARNING:
      return "ERROR: High amount of corrupted CAN messages detected. Check CAN wire shielding!";
    case EVENT_CAN_TX_FAILURE:
      return "ERROR: CAN messages failed to transmit, or no one on the bus to ACK the message!";
    case EVENT_WATER_INGRESS:
      return "Water leakage inside battery detected. Operation halted. Inspect battery!";
    case EVENT_12V_LOW:
      return "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!";
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      return "ERROR: SOC% reported by battery not plausible. Restart battery!";
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      return "Info: kWh remaining reported by battery not plausible. Battery needs cycling.";
    case EVENT_BATTERY_EMPTY:
      return "Info: Battery is completely discharged";
    case EVENT_BATTERY_FULL:
      return "Info: Battery is fully charged";
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
    case EVENT_INVERTER_OPEN_CONTACTOR:
      return "Info: Inverter side opened contactors. Normal operation.";
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
    case EVENT_OTA_UPDATE:
      return "OTA update started!";
    case EVENT_OTA_UPDATE_TIMEOUT:
      return "OTA update timed out!";
    case EVENT_EEPROM_WRITE:
      return "Info: The EEPROM was written";
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

  // Update event level, only upwards. Downward changes are done in Software.ino:loop()
  events.level = max(events.level, events.entries[event].level);

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
      datalayer.battery.status.bms_status = ACTIVE;
      break;
    case EVENT_LEVEL_UPDATE:
      datalayer.battery.status.bms_status = UPDATING;
      break;
    case EVENT_LEVEL_ERROR:
      datalayer.battery.status.bms_status = FAULT;
      break;
    default:
      break;
  }
}

static void update_event_level(void) {
  EVENTS_LEVEL_TYPE temporary_level = EVENT_LEVEL_INFO;
  for (uint8_t i = 0u; i < EVENT_NOF_EVENTS; i++) {
    if ((events.entries[i].state == EVENT_STATE_ACTIVE) || (events.entries[i].state == EVENT_STATE_ACTIVE_LATCHED)) {
      temporary_level = max(events.entries[i].level, temporary_level);
    }
  }
  events.level = temporary_level;
}

static void update_event_time(void) {
  if (events.second_timer.elapsed() == true) {
    events.time_seconds++;
  }
}

static void log_event(EVENTS_ENUM_TYPE event, uint8_t data) {
  // Update head with wrap to 0
  if (++events.event_log_head_index == EE_NOF_EVENT_ENTRIES) {
    events.event_log_head_index = 0;
  }

  // If the head now points to the tail, move the tail, with wrap to 0
  if (events.event_log_head_index == events.event_log_tail_index) {
    if (++events.event_log_tail_index == EE_NOF_EVENT_ENTRIES) {
      events.event_log_tail_index = 0;
    }
  }

  // The head now holds the index to the oldest event, the one we want to overwrite,
  // so calculate the absolute address
  int entry_address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * events.event_log_head_index;

  // Prepare an event block to write
  EVENT_LOG_ENTRY_TYPE entry = {.event = event, .timestamp = events.time_seconds, .data = data};

  // Put the event in (what I guess is) the RAM EEPROM mirror, or write buffer
  EEPROM.put(entry_address, entry);

  // Store the new indices
  EEPROM.writeUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS, events.event_log_head_index);
  EEPROM.writeUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS, events.event_log_tail_index);
  //Serial.println("Wrote event " + String(event) + " to " + String(entry_address));
  //Serial.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));

  // We don't need the exact number, it's just for deciding to store or not
  events.nof_logged_events += (events.nof_logged_events < 255) ? 1 : 0;
}

static void print_event_log(void) {
  // If the head actually points to the tail, the log is probably blank
  if (events.event_log_head_index == events.event_log_tail_index) {
#ifdef DEBUG_VIA_USB
    Serial.println("No events in log");
#endif
    return;
  }
  EVENT_LOG_ENTRY_TYPE entry;

  for (int i = 0; i < EE_NOF_EVENT_ENTRIES; i++) {
    // Start at the oldest event, work through the log all the way the the head
    int index = ((events.event_log_tail_index + i) % EE_NOF_EVENT_ENTRIES);
    int address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * index;

    EEPROM.get(address, entry);
    if (entry.event == EVENT_NOF_EVENTS) {
      // The entry is a blank that has been left behind somehow
      continue;
    }
#ifdef DEBUG_VIA_USB
    Serial.println("Event: " + String(get_event_enum_string(entry.event)) + ", data: " + String(entry.data) +
                   ", time: " + String(entry.timestamp));
#endif
    if (index == events.event_log_head_index) {
      break;
    }
  }
}

static void check_ee_write(void) {
  // Only actually write to flash emulated EEPROM every EE_WRITE_PERIOD_MINUTES minutes,
  // and only if we've logged any events
  if (events.ee_timer.elapsed() && (events.nof_logged_events > 0)) {
    EEPROM.commit();
    events.nof_eeprom_writes += (events.nof_eeprom_writes < 65535) ? 1 : 0;
    events.nof_logged_events = 0;

    // We want to know how many writes we have, and to increment the occurrence counter
    // we need to clear it first. It's just the way life is. Using events is a smooth
    // way to visualize it in the web UI
    clear_event(EVENT_EEPROM_WRITE);
    set_event(EVENT_EEPROM_WRITE, events.nof_eeprom_writes);
  }
}
