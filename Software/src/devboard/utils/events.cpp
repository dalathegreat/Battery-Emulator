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
  uint8_t millisrolloverCount;
  uint32_t timestamp;
  uint8_t data;
} EVENT_LOG_ENTRY_TYPE;

typedef struct {
  EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
  MyTimer ee_timer;
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
static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched);
static void update_event_level(void);
static void update_bms_status(void);
static void log_event(EVENTS_ENUM_TYPE event, uint8_t millisrolloverCount, uint32_t timestamp, uint8_t data);
static void print_event_log(void);

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
    EVENT_LOG_ENTRY_TYPE entry = {.event = EVENT_NOF_EVENTS, .millisrolloverCount = 0, .timestamp = 0, .data = 0};

    // Put the event in (what I guess is) the RAM EEPROM mirror, or write buffer

    for (int i = 0; i < EE_NOF_EVENT_ENTRIES; i++) {
      // Start at the oldest event, work through the log all the way the the head
      int address = EE_EVENT_ENTRY_START_ADDRESS + EE_EVENT_ENTRY_SIZE * i;
      EEPROM.put(address, entry);
    }

    // Push changes to eeprom
    EEPROM.commit();
#ifdef DEBUG_LOG
    logging.println("EEPROM wasn't ready");
#endif
  } else {
    events.event_log_head_index = EEPROM.readUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS);
    events.event_log_tail_index = EEPROM.readUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS);
#ifdef DEBUG_LOG
    logging.println("EEPROM was initialized for event logging");
    logging.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));
#endif
    print_event_log();
  }

  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[i].data = 0;
    events.entries[i].timestamp = 0;
    events.entries[i].millisrolloverCount = 0;
    events.entries[i].occurences = 0;
    events.entries[i].log = true;
    events.entries[i].MQTTpublished = false;  // Not published by default
  }

  events.entries[EVENT_CANMCP2517FD_INIT_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANMCP2515_INIT_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANFD_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_OVERRUN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_CORRUPTED_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_NATIVE_TX_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_BATTERY_MISSING].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_BATTERY2_MISSING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_CHARGER_MISSING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_INVERTER_MISSING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CONTACTOR_WELDED].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_WATER_INGRESS].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CHARGE_LIMIT_EXCEEDED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DISCHARGE_LIMIT_EXCEEDED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_12V_LOW].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOC_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOC_UNAVAILABLE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_STALE_VALUE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_KWH_PLAUSIBILITY_ERROR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BALANCING_START].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BALANCING_END].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_EMPTY].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_FULL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_FUSE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_FROZEN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_CAUTION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_OVERHEAT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_OVERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_UNDERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_VALUE_UNAVAILABLE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_ISOLATION].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_VOLTAGE_DIFFERENCE].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_SOH_DIFFERENCE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOH_LOW].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_HVIL_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_PRECHARGE_FAILURE].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_AUTOMATIC_PRECHARGE_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INTERNAL_OPEN_FAULT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INVERTER_OPEN_CONTACTOR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_INTERFACE_MISSING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MODBUS_INVERTER_MISSING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_ERROR_OPEN_CONTACTOR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CELL_CRITICAL_UNDER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_CRITICAL_OVER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_UNDER_VOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CELL_OVER_VOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CELL_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_UNKNOWN_EVENT_SET].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_OTA_UPDATE].level = EVENT_LEVEL_UPDATE;
  events.entries[EVENT_OTA_UPDATE_TIMEOUT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_DEBUG].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_DUMMY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_PERSISTENT_SAVE_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_SERIAL_RX_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SERIAL_RX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TRANSMITTER_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_EEPROM_WRITE].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_UNKNOWN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_POWERON].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_EXT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_SW].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_PANIC].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RESET_INT_WDT].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RESET_TASK_WDT].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RESET_WDT].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RESET_DEEPSLEEP].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_BROWNOUT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_SDIO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_USB].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_JTAG].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_EFUSE].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_PWR_GLITCH].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RESET_CPU_LOCKUP].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RJXZS_LOG].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_PAUSE_BEGIN].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_PAUSE_END].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_CONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_DISCONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MQTT_CONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MQTT_DISCONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_EQUIPMENT_STOP].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SD_INIT_FAILED].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_PERIODIC_BMS_RESET].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_PERIODIC_BMS_RESET_AT_INIT_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_PERIODIC_BMS_RESET_AT_INIT_FAILED].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_TEMP_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;

  events.entries[EVENT_EEPROM_WRITE].log = false;  // Don't log the logger...

  // Write to EEPROM every X minutes (if an event has been set)
  events.ee_timer.set_interval(EE_WRITE_PERIOD_MINUTES * 60 * 1000);
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

void reset_all_events() {
  events.nof_logged_events = 0;
  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[i].data = 0;
    events.entries[i].state = EVENT_STATE_INACTIVE;
    events.entries[i].timestamp = 0;
    events.entries[i].millisrolloverCount = 0;
    events.entries[i].occurences = 0;
    events.entries[i].log = true;
    events.entries[i].MQTTpublished = false;  // Not published by default
  }
  events.level = EVENT_LEVEL_INFO;
  update_bms_status();
#ifdef DEBUG_LOG
  logging.println("All events have been cleared.");
#endif
}

void set_event_MQTTpublished(EVENTS_ENUM_TYPE event) {
  events.entries[event].MQTTpublished = true;
}

const char* get_event_message_string(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CANMCP2517FD_INIT_FAILURE:
      return "CAN-FD initialization failed. Check hardware or bitrate settings";
    case EVENT_CANMCP2515_INIT_FAILURE:
      return "CAN-MCP addon initialization failed. Check hardware";
    case EVENT_CANFD_BUFFER_FULL:
      return "MCP2518FD message failed to send. Buffer full or no one on the bus to ACK the message!";
    case EVENT_CAN_BUFFER_FULL:
      return "MCP2515 message failed to send. Buffer full or no one on the bus to ACK the message!";
    case EVENT_CAN_OVERRUN:
      return "CAN message failed to send within defined time. Contact developers, CPU load might be too high.";
    case EVENT_CAN_CORRUPTED_WARNING:
      return "High amount of corrupted CAN messages detected. Check CAN wire shielding!";
    case EVENT_CAN_NATIVE_TX_FAILURE:
      return "CAN_NATIVE failed to transmit, or no one on the bus to ACK the message!";
    case EVENT_CAN_BATTERY_MISSING:
      return "Battery not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_BATTERY2_MISSING:
      return "Secondary battery not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_CHARGER_MISSING:
      return "Charger not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_INVERTER_MISSING:
      return "Inverter not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CONTACTOR_WELDED:
      return "Contactors sticking/welded. Inspect battery with caution!";
    case EVENT_CHARGE_LIMIT_EXCEEDED:
      return "Inverter is charging faster than battery is allowing.";
    case EVENT_DISCHARGE_LIMIT_EXCEEDED:
      return "Inverter is discharging faster than battery is allowing.";
    case EVENT_WATER_INGRESS:
      return "Water leakage inside battery detected. Operation halted. Inspect battery!";
    case EVENT_12V_LOW:
      return "12V battery source below required voltage to safely close contactors. Inspect the supply/battery!";
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      return "SOC reported by battery not plausible. Restart battery!";
    case EVENT_SOC_UNAVAILABLE:
      return "SOC not sent by BMS. Calibrate BMS via app.";
    case EVENT_STALE_VALUE:
      return "Important values detected as stale. System might have locked up!";
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      return "kWh remaining reported by battery not plausible. Battery needs cycling.";
    case EVENT_BALANCING_START:
      return "Balancing has started";
    case EVENT_BALANCING_END:
      return "Balancing has ended";
    case EVENT_BATTERY_EMPTY:
      return "Battery is completely discharged";
    case EVENT_BATTERY_FULL:
      return "Battery is fully charged";
    case EVENT_BATTERY_FUSE:
      return "Battery internal fuse blown. Inspect battery";
    case EVENT_BATTERY_FROZEN:
      return "Battery is too cold to operate optimally. Consider warming it up!";
    case EVENT_BATTERY_CAUTION:
      return "Battery has raised a general caution flag. Might want to inspect it closely.";
    case EVENT_BATTERY_CHG_STOP_REQ:
      return "Battery raised caution indicator AND requested charge stop. Inspect battery status!";
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      return "Battery raised caution indicator AND requested discharge stop. Inspect battery status!";
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      return "Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!";
    case EVENT_BATTERY_REQUESTS_HEAT:
      return "COLD BATTERY! Battery requesting heating pads to activate!";
    case EVENT_BATTERY_WARMED_UP:
      return "Battery requesting heating pads to stop. The battery is now warm enough.";
    case EVENT_BATTERY_OVERHEAT:
      return "Battery overheated. Shutting down to prevent thermal runaway!";
    case EVENT_BATTERY_OVERVOLTAGE:
      return "Battery exceeding maximum design voltage. Discharge battery to prevent damage!";
    case EVENT_BATTERY_UNDERVOLTAGE:
      return "Battery under minimum design voltage. Charge battery to prevent damage!";
    case EVENT_BATTERY_VALUE_UNAVAILABLE:
      return "Battery measurement unavailable. Check 12V power supply and battery wiring!";
    case EVENT_BATTERY_ISOLATION:
      return "Battery reports isolation error. High voltage might be leaking to ground. Check battery!";
    case EVENT_VOLTAGE_DIFFERENCE:
      return "Too large voltage diff between the batteries. Second battery cannot join the DC-link";
    case EVENT_SOH_DIFFERENCE:
      return "Large deviation in State of health between packs. Inspect battery.";
    case EVENT_SOH_LOW:
      return "State of health critically low. Battery internal resistance too high to continue. Recycle "
             "battery.";
    case EVENT_HVIL_FAILURE:
      return "Battery interlock loop broken. Check that high voltage / low voltage connectors are seated. "
             "Battery will be disabled!";
    case EVENT_PRECHARGE_FAILURE:
      return "Battery failed to precharge. Check that capacitor is seated on high voltage output.";
    case EVENT_AUTOMATIC_PRECHARGE_FAILURE:
      return "Automatic precharge failed to reach target voltae.";
    case EVENT_INTERNAL_OPEN_FAULT:
      return "High voltage cable removed while battery running. Opening contactors!";
    case EVENT_INVERTER_OPEN_CONTACTOR:
      return "Inverter side opened contactors. Normal operation.";
    case EVENT_INTERFACE_MISSING:
      return "Configuration trying to use CAN interface not baked into the software. Recompile software!";
    case EVENT_ERROR_OPEN_CONTACTOR:
      return "Too much time spent in error state. Opening contactors, not safe to continue charging. "
             "Check other error code for reason!";
    case EVENT_MODBUS_INVERTER_MISSING:
      return "Modbus inverter has not sent any data. Inspect communication wiring!";
    case EVENT_CELL_CRITICAL_UNDER_VOLTAGE:
      return "CELL VOLTAGE CRITICALLY LOW! Not possible to continue. Inspect battery!";
    case EVENT_CELL_UNDER_VOLTAGE:
      return "Cell undervoltage. Further discharge not possible. Check balancing of cells";
    case EVENT_CELL_OVER_VOLTAGE:
      return "Cell overvoltage. Further charging not possible. Check balancing of cells";
    case EVENT_CELL_CRITICAL_OVER_VOLTAGE:
      return "CELL VOLTAGE CRITICALLY HIGH! Not possible to continue. Inspect battery!";
    case EVENT_CELL_DEVIATION_HIGH:
      return "Large cell voltage deviation! Check balancing of cells";
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
    case EVENT_PERSISTENT_SAVE_INFO:
      return "Failed to save user settings. Namespace full?";
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
      return "The EEPROM was written";
    case EVENT_RESET_UNKNOWN:
      return "The board was reset unexpectedly, and reason can't be determined";
    case EVENT_RESET_POWERON:
      return "The board was reset from a power-on event. Normal operation";
    case EVENT_RESET_EXT:
      return "The board was reset from an external pin";
    case EVENT_RESET_SW:
      return "The board was reset via software, webserver or OTA. Normal operation";
    case EVENT_RESET_PANIC:
      return "The board was reset due to an exception or panic. Inform developers!";
    case EVENT_RESET_INT_WDT:
      return "The board was reset due to an interrupt watchdog timeout. Inform developers!";
    case EVENT_RESET_TASK_WDT:
      return "The board was reset due to a task watchdog timeout. Inform developers!";
    case EVENT_RESET_WDT:
      return "The board was reset due to other watchdog timeout. Inform developers!";
    case EVENT_RESET_DEEPSLEEP:
      return "The board was reset after exiting deep sleep mode";
    case EVENT_RESET_BROWNOUT:
      return "The board was reset due to a momentary low voltage condition. This is expected during certain "
             "operations like flashing via USB";
    case EVENT_RESET_SDIO:
      return "The board was reset over SDIO";
    case EVENT_RESET_USB:
      return "The board was reset by the USB peripheral";
    case EVENT_RESET_JTAG:
      return "The board was reset by JTAG";
    case EVENT_RESET_EFUSE:
      return "The board was reset due to an efuse error";
    case EVENT_RESET_PWR_GLITCH:
      return "The board was reset due to a detected power glitch";
    case EVENT_RESET_CPU_LOCKUP:
      return "The board was reset due to CPU lockup. Inform developers!";
    case EVENT_RJXZS_LOG:
      return "Error code active in RJXZS BMS. Clear via their smartphone app!";
    case EVENT_PAUSE_BEGIN:
      return "The emulator is trying to pause the battery.";
    case EVENT_PAUSE_END:
      return "The emulator is attempting to resume battery operation from pause.";
    case EVENT_WIFI_CONNECT:
      return "Wifi connected.";
    case EVENT_WIFI_DISCONNECT:
      return "Wifi disconnected.";
    case EVENT_MQTT_CONNECT:
      return "MQTT connected.";
    case EVENT_MQTT_DISCONNECT:
      return "MQTT disconnected.";
    case EVENT_EQUIPMENT_STOP:
      return "EQUIPMENT STOP ACTIVATED!!!";
    case EVENT_SD_INIT_FAILED:
      return "SD card initialization failed, check hardware. Power must be removed to reset the SD card.";
    case EVENT_PERIODIC_BMS_RESET:
      return "BMS Reset Event Completed.";
    case EVENT_PERIODIC_BMS_RESET_AT_INIT_SUCCESS:
      return "Successfully syncronised with the NTP Server. BMS will reset every 24 hours at defined time";
    case EVENT_PERIODIC_BMS_RESET_AT_INIT_FAILED:
      return "Failed to syncronise with the NTP Server. BMS will reset every 24 hours from when the emulator was "
             "powered on";
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
    events.entries[event].MQTTpublished = false;
    if (events.entries[event].log) {
      log_event(event, events.entries[event].millisrolloverCount, events.entries[event].timestamp, data);
    }
#ifdef DEBUG_LOG
    logging.print("Event: ");
    logging.println(get_event_message_string(event));
#endif
  }

  // We should set the event, update event info
  events.entries[event].timestamp = millis();
  events.entries[event].millisrolloverCount = datalayer.system.status.millisrolloverCount;
  events.entries[event].data = data;
  // Check if the event is latching
  events.entries[event].state = latched ? EVENT_STATE_ACTIVE_LATCHED : EVENT_STATE_ACTIVE;

  // Update event level, only upwards. Downward changes are done in Software.ino:loop()
  events.level = max(events.level, events.entries[event].level);

  update_bms_status();
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

// Function to compare events by timestamp descending
bool compareEventsByTimestampDesc(const EventData& a, const EventData& b) {
  if (a.event_pointer->millisrolloverCount != b.event_pointer->millisrolloverCount) {
    return a.event_pointer->millisrolloverCount > b.event_pointer->millisrolloverCount;
  }
  return a.event_pointer->timestamp > b.event_pointer->timestamp;
}

// Function to compare events by timestamp ascending
bool compareEventsByTimestampAsc(const EventData& a, const EventData& b) {
  if (a.event_pointer->millisrolloverCount != b.event_pointer->millisrolloverCount) {
    return a.event_pointer->millisrolloverCount < b.event_pointer->millisrolloverCount;
  }
  return a.event_pointer->timestamp < b.event_pointer->timestamp;
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

static void log_event(EVENTS_ENUM_TYPE event, uint8_t millisrolloverCount, uint32_t timestamp, uint8_t data) {
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
  EVENT_LOG_ENTRY_TYPE entry = {.event = event,
                                .millisrolloverCount = datalayer.system.status.millisrolloverCount,
                                .timestamp = timestamp,
                                .data = data};

  // Put the event in (what I guess is) the RAM EEPROM mirror, or write buffer
  EEPROM.put(entry_address, entry);

  // Store the new indices
  EEPROM.writeUShort(EE_EVENT_LOG_HEAD_INDEX_ADDRESS, events.event_log_head_index);
  EEPROM.writeUShort(EE_EVENT_LOG_TAIL_INDEX_ADDRESS, events.event_log_tail_index);
  //logging.println("Wrote event " + String(event) + " to " + String(entry_address));
  //logging.println("head: " + String(events.event_log_head_index) + ", tail: " + String(events.event_log_tail_index));

  // We don't need the exact number, it's just for deciding to store or not
  events.nof_logged_events += (events.nof_logged_events < 255) ? 1 : 0;
}

static void print_event_log(void) {
  // If the head actually points to the tail, the log is probably blank
  if (events.event_log_head_index == events.event_log_tail_index) {
#ifdef DEBUG_LOG
    logging.println("No events in log");
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
#ifdef DEBUG_LOG
    logging.println("Event: " + String(get_event_enum_string(entry.event)) + ", data: " + String(entry.data) +
                    ", time: " + String(entry.timestamp));
#endif
    if (index == events.event_log_head_index) {
      break;
    }
  }
}
