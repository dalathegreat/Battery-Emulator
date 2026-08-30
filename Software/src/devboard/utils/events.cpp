#include "events.h"
#include <Arduino.h>
#include <string.h>  // memchr, for the notice_events lookup
#include "../../datalayer/datalayer.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/utils/logging.h"

typedef struct {
  EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];
  EVENTS_LEVEL_TYPE level;
} EVENT_TYPE;

/* Local variables */
static EVENT_TYPE events;
static const char* EVENTS_ENUM_TYPE_STRING[] = {EVENTS_ENUM_TYPE(GENERATE_STRING)};
static const char* EVENTS_LEVEL_TYPE_STRING[] = {EVENTS_LEVEL_TYPE(GENERATE_STRING)};
static const char* EMULATOR_STATUS_STRING[] = {EMULATOR_STATUS(GENERATE_STRING)};

// Timed "ignore CAN errors" window per interface. Uses the 64-bit clock so there is no wraparound.
static uint64_t can_errors_ignore_until_ms[NO_CAN_INTERFACE] = {0};

/* Local function prototypes */
static void set_event_internal(EVENTS_ENUM_TYPE event, int16_t data, bool latched);

/* Offgrid downgrade.
 *
 * Some events describe the loss of something an offgrid system never had.
 * The inverter going missing is a genuine fault when it is the grid-tied
 * sink, and is normal when the system is meant to run standalone - and today
 * it raises system_status to FAULT, which gates precharge and so blocks a
 * black start outright.
 *
 * Downgraded here rather than by editing the table in init_events(), so the
 * declared severity stays readable in one place and the exception is a short,
 * auditable list. Applied at every point the level is READ, so what gets
 * aggregated, logged and shown on the events page is the effective level -
 * not a FAULT that merely displays as a warning.
 *
 * Extending this is a matter of adding an event id: the mechanism assumes
 * nothing about which events belong here. */
static const EVENTS_ENUM_TYPE OFFGRID_DOWNGRADED_EVENTS[] = {
    EVENT_CAN_INVERTER_MISSING,
};

static EVENTS_LEVEL_TYPE effective_level(EVENTS_ENUM_TYPE event) {
  EVENTS_LEVEL_TYPE level = events.entries[event].level;
  if (!user_selected_inverter_offgrid || level <= EVENT_LEVEL_WARNING) {
    return level;
  }
  for (EVENTS_ENUM_TYPE downgraded : OFFGRID_DOWNGRADED_EVENTS) {
    if (event == downgraded) {
      return EVENT_LEVEL_WARNING;
    }
  }
  return level;
}
static bool can_error_ignored(EVENTS_ENUM_TYPE event);
static void update_event_level(void);
static void update_bms_status(void);

// Map a Battery-Emulator event level to an RFC 5424 syslog severity.
static uint8_t event_level_to_syslog(EVENTS_LEVEL_TYPE lvl) {
  switch (lvl) {
    case EVENT_LEVEL_ERROR:
      return 3;  // err
    case EVENT_LEVEL_WARNING:
      return 4;  // warning
    case EVENT_LEVEL_UPDATE:
      return 5;  // notice
    case EVENT_LEVEL_INFO:
      return 6;  // info
    case EVENT_LEVEL_DEBUG:
      return 7;  // debug
    default:
      return 6;
  }
}

/* Operational milestones that a syslog server should show at its default verbosity:
   one-shot-per-boot lifecycle transitions and deliberate operator actions.

   This is deliberately NOT expressed by raising the event's EVENTS_LEVEL_TYPE.
   EVENT_LEVEL_UPDATE is the only level that already maps to notice, but it is a state
   level, not a logging level: update_bms_status() turns it into system_status = UPDATING,
   which drives the LED pattern, the web UI status and inverter behaviour. Marking, say,
   EVENT_MQTT_CONNECT as UPDATE would park the emulator in UPDATING forever.

   Stored as a flat uint8_t table so it costs one byte per entry and a single memchr,
   rather than the ~8 bytes of compare-and-branch per case a switch would emit. */
static_assert(EVENT_NOF_EVENTS <= 256, "notice_events[] indexes events as uint8_t");
static const uint8_t notice_events[] = {
    // Peer detection - latched in check_can_component_alive(), so one line per boot
    EVENT_CAN_BATTERY_DETECTED,
    EVENT_CAN_BATTERY2_DETECTED,
    EVENT_CAN_BATTERY3_DETECTED,
    EVENT_CAN_INVERTER_DETECTED,
    EVENT_MODBUS_INVERTER_DETECTED,
    EVENT_CAN_CHARGER_DETECTED,
    // Connectivity - the "down" halves are listed so a syslog view never shows an
    // unterminated session. Wi-Fi/battery/inverter loss is already >= warning.
    EVENT_MQTT_CONNECT,
    EVENT_MQTT_DISCONNECT,
    EVENT_WIFI_DISCONNECT,
    // Deliberate state changes. PAUSE_BEGIN is already warning; without PAUSE_END the
    // pause window never appears to close.
    EVENT_PAUSE_END,
    EVENT_WIFI_AP_PROVISION_TIMEOUT,
    EVENT_WIFI_AP_PASSWORD_DEFAULT,
    EVENT_PERIODIC_BMS_RESET,
    EVENT_BMS_RESET_REQ_SUCCESS,
    // Reset cause - fires exactly once per boot and answers "why did it come back".
    // The WDT/panic/lockup causes are already warning.
    EVENT_RESET_UNKNOWN,
    EVENT_RESET_POWERON,
    EVENT_RESET_EXT,
    EVENT_RESET_SW,
    EVENT_RESET_DEEPSLEEP,
    EVENT_RESET_SDIO,
    EVENT_RESET_USB,
    EVENT_RESET_JTAG,
    EVENT_RESET_EFUSE,
    EVENT_RESET_PWR_GLITCH,
};

// Syslog severity for an event: its level, raised to notice for the milestones above.
// The sev > 5 test means the table can only ever raise severity, so re-levelling an
// event to warning or error later cannot be silently undone here.
static uint8_t event_syslog_severity(EVENTS_ENUM_TYPE event) {
  uint8_t sev = event_level_to_syslog(effective_level(event));
  if (sev > 5 && memchr(notice_events, (uint8_t)event, sizeof(notice_events)) != nullptr) {
    sev = 5;  // notice
  }
  return sev;
}

/* Initialization function */
void init_events(void) {
  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[i].data = 0;
    events.entries[i].timestamp = 0;
    events.entries[i].occurences = 0;
    events.entries[i].MQTTpublished = false;  // Not published by default
  }

  events.entries[EVENT_CANMCP2518FD_INIT_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANMCP2515_INIT_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_NATIVE_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANFD_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANFD_2_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANMCP2515_BUFFER_FULL].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_TASK_OVERRUN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_THERMAL_RUNAWAY].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_CORRUPTED_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_NATIVE_BUS_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANMCP2515_BUS_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANFD_BUS_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CANFD_2_BUS_ERROR].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_BATTERY_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_BATTERY2_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_BATTERY3_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_BATTERY_MISSING].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_BATTERY2_MISSING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_BATTERY3_MISSING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CAN_CHARGER_MISSING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_CHARGER_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CAN_INVERTER_MISSING].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CAN_INVERTER_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CONTACTOR_WELDED].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CONTACTOR_OPEN].level = EVENT_LEVEL_WARNING;
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
  events.entries[EVENT_BATTERY2_EMPTY].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_EMPTY].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_FULL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_FULL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_FULL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_FUSE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_FUSE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_FUSE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_FROZEN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_FROZEN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_FROZEN].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_CAUTION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_CAUTION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_CAUTION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY2_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY3_CHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY2_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY3_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY2_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY3_CHG_DISCHG_STOP_REQ].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_OVERHEAT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY2_OVERHEAT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY3_OVERHEAT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_OVERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_OVERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_OVERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_UNDERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_UNDERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_UNDERVOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_VALUE_UNAVAILABLE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_VALUE_UNAVAILABLE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_VALUE_UNAVAILABLE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_ISOLATION].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_ISOLATION].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_ISOLATION].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_SOC_RECALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_SOC_RECALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_SOC_RECALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_AUTO_SOC_CALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_CHARGE_TERMINATED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_CONTACTOR_MISMATCH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BYD_CONTACTOR_FORCE_OPEN].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BYD_CONTACTOR_OPEN_REQ].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_CONTACTOR_CLOSE_REQ].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_SOC_RESET_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_SOC_RESET_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_SOC_RESET_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_SOC_RESET_FAIL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_SOC_RESET_FAIL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_SOC_RESET_FAIL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_VOLTAGE_DIFFERENCE_BAT2].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_VOLTAGE_DIFFERENCE_BAT3].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_SOH_DIFFERENCE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SOH_LOW].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_HVIL_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_LOW_HEAP_MEMORY].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_PRECHARGE_FAILURE].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_AUTOMATIC_PRECHARGE_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INTERNAL_OPEN_FAULT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INVERTER_OPEN_CONTACTOR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_INTERFACE_MISSING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MODBUS_INVERTER_MISSING].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_MODBUS_INVERTER_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_NO_ENABLE_DETECTED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_ERROR_OPEN_CONTACTOR].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_CELL_CRITICAL_UNDER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_CRITICAL_OVER_VOLTAGE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_CELL_UNDER_VOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CELL_OVER_VOLTAGE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_CELL_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_UNKNOWN_EVENT_SET].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_OTA_UPDATE].level = EVENT_LEVEL_UPDATE;
  events.entries[EVENT_OTA_UPDATE_TIMEOUT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_OTA_ROLLBACK].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RESTARTING].level = EVENT_LEVEL_UPDATE;  // Stops Fronius erroring out during restarts
  events.entries[EVENT_DUMMY_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_DUMMY_DEBUG].level = EVENT_LEVEL_DEBUG;
  events.entries[EVENT_DUMMY_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_DUMMY_ERROR].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_PERSISTENT_SAVE_INFO].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_SERIAL_RX_WARNING].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_SERIAL_RX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TX_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SERIAL_TRANSMITTER_FAILURE].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SMA_PAIRING].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_RECOVERY_START].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_RECOVERY_END].level = EVENT_LEVEL_WARNING;
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
  events.entries[EVENT_PID_FAILED].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_CONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_DISCONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_AP_PASSWORD_DEFAULT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_WIFI_AP_PROVISION_TIMEOUT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MQTT_CONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_MQTT_DISCONNECT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_EQUIPMENT_STOP].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_SD_INIT_FAILED].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_PERIODIC_BMS_RESET].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BMS_RESET_REQ_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BMS_RESET_REQ_FAIL].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_TEMP_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY2_TEMP_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY3_TEMP_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BATTERY_REQUESTS_HEAT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_REQUESTS_HEAT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_REQUESTS_HEAT].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_WARMED_UP].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY2_WARMED_UP].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY3_WARMED_UP].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_PERIODIC_BMS_RESET_FAILURE].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_GPIO_CONFLICT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_GPIO_NOT_DEFINED].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_INVERTER_REBOOT_DECLINED].level = EVENT_LEVEL_WARNING;
}

void set_event(EVENTS_ENUM_TYPE event, int16_t data) {
  set_event_internal(event, data, false);
}

/* The per-battery event variants are laid out as contiguous 1,2,3 triplets, so the concrete
   event for a pack is the EVENT_BATTERY_* variant plus (battery - 1). Nothing about that
   layout is enforced by the type system, so lock it at compile time: inserting or reordering
   an entry inside the block fails the build here rather than silently misdirecting events at
   runtime. Checking the two ends plus one interior triplet is enough, because the block is
   generated as whole triplets and its length is checked too. */
static_assert(EVENT_BATTERY2_EMPTY == EVENT_BATTERY_EMPTY + 1 && EVENT_BATTERY3_EMPTY == EVENT_BATTERY_EMPTY + 2,
              "Per-battery event variants must stay contiguous and in 1,2,3 order");
static_assert(EVENT_BATTERY2_OVERHEAT == EVENT_BATTERY_OVERHEAT + 1 &&
                  EVENT_BATTERY3_OVERHEAT == EVENT_BATTERY_OVERHEAT + 2,
              "Per-battery event variants must stay contiguous and in 1,2,3 order");
static_assert(EVENT_BATTERY2_TEMP_DEVIATION_HIGH == EVENT_BATTERY_TEMP_DEVIATION_HIGH + 1 &&
                  EVENT_BATTERY3_TEMP_DEVIATION_HIGH == EVENT_BATTERY_TEMP_DEVIATION_HIGH + 2,
              "Per-battery event variants must stay contiguous and in 1,2,3 order");
static_assert((EVENT_BATTERY3_TEMP_DEVIATION_HIGH - EVENT_BATTERY_EMPTY + 1) % 3 == 0,
              "The per-battery event block must consist of whole 1,2,3 triplets");

/* Returns the pack a battery specific event belongs to (1/2/3), or 0 when the event is not
   battery specific. Derived from the enum, so it cannot disagree with the event that was set. */
static uint8_t event_battery_number(EVENTS_ENUM_TYPE event) {
  if (event < EVENT_BATTERY_EMPTY || event > EVENT_BATTERY3_TEMP_DEVIATION_HIGH) {
    return 0;
  }
  return static_cast<uint8_t>((event - EVENT_BATTERY_EMPTY) % 3 + 1);
}

/* Resolve the EVENT_BATTERY_* variant plus a pack number into the concrete event.
   Resolution is index arithmetic over the enum, so a bad argument would quietly land on an
   unrelated event. Anything that is not a EVENT_BATTERY_* base with a pack number of 1..3
   returns EVENT_NOF_EVENTS, which callers report rather than acting on. */
static EVENTS_ENUM_TYPE resolve_battery_event(EVENTS_ENUM_TYPE event, uint8_t battery) {
  const bool valid_base = (event >= EVENT_BATTERY_EMPTY && event <= EVENT_BATTERY3_TEMP_DEVIATION_HIGH &&
                           (event - EVENT_BATTERY_EMPTY) % 3 == 0);
  if (!valid_base || battery < 1 || battery > 3) {
    DEBUG_PRINTF("Bad battery event %d for battery %u\n", (int)event, (unsigned)battery);
    return EVENT_NOF_EVENTS;
  }
  return static_cast<EVENTS_ENUM_TYPE>(event + (battery - 1));
}

void set_event(EVENTS_ENUM_TYPE event, int16_t data, uint8_t battery) {
  const EVENTS_ENUM_TYPE resolved = resolve_battery_event(event, battery);
  if (resolved != EVENT_NOF_EVENTS) {
    set_event_internal(resolved, data, false);
  }
}

void set_event_latched(EVENTS_ENUM_TYPE event, int16_t data, uint8_t battery) {
  const EVENTS_ENUM_TYPE resolved = resolve_battery_event(event, battery);
  if (resolved != EVENT_NOF_EVENTS) {
    set_event_internal(resolved, data, true);
  }
}

void clear_event(EVENTS_ENUM_TYPE event, uint8_t battery) {
  const EVENTS_ENUM_TYPE resolved = resolve_battery_event(event, battery);
  if (resolved != EVENT_NOF_EVENTS) {
    clear_event(resolved);
  }
}

void set_event_latched(EVENTS_ENUM_TYPE event, int16_t data) {
  set_event_internal(event, data, true);
}

void clear_event(EVENTS_ENUM_TYPE event) {
  if (events.entries[event].state == EVENT_STATE_ACTIVE) {
    events.entries[event].state = EVENT_STATE_INACTIVE;
    update_event_level();
    update_bms_status();
  }
}

void ignore_can_errors_for(CAN_Interface interface, uint32_t duration_ms) {
  // Suppress the buffer-full / bus-error events of a single CAN interface for a while.
  if ((uint8_t)interface >= NO_CAN_INTERFACE) {
    return;
  }
  can_errors_ignore_until_ms[interface] = millis64() + duration_ms;
}

void reset_all_events() {
  for (uint16_t i = 0; i < EVENT_NOF_EVENTS; i++) {
    events.entries[i].data = 0;
    events.entries[i].state = EVENT_STATE_INACTIVE;
    events.entries[i].timestamp = 0;
    events.entries[i].occurences = 0;
    events.entries[i].MQTTpublished = false;  // Not published by default
  }
  events.level = EVENT_LEVEL_INFO;
  update_bms_status();
}

void set_event_MQTTpublished(EVENTS_ENUM_TYPE event) {
  events.entries[event].MQTTpublished = true;
}

static String get_event_base_message(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CANMCP2518FD_INIT_FAILURE:
      return "CAN-FD initialization failed. Check hardware or bitrate settings";
    case EVENT_CANMCP2515_INIT_FAILURE:
      return "CAN-MCP addon initialization failed. Check hardware";
    case EVENT_CAN_NATIVE_BUFFER_FULL:
    case EVENT_CANMCP2515_BUFFER_FULL:
    case EVENT_CANFD_BUFFER_FULL:
    case EVENT_CANFD_2_BUFFER_FULL:
      return "CAN failed to send. Buffer full or no one on the bus to ACK the message!";
    case EVENT_TASK_OVERRUN:
      return "Task took too long to complete. CPU load might be too high. Info message, no action required.";
    case EVENT_THERMAL_RUNAWAY:
      return "THERMAL RUNAWAY! POTENTIAL FIRE OR EXPLOSION IMMINENT!";
    case EVENT_CAN_CORRUPTED_WARNING:
      return "High amount of corrupted CAN messages detected. Check CAN wire shielding!";
    case EVENT_CAN_NATIVE_BUS_ERROR:
    case EVENT_CANMCP2515_BUS_ERROR:
    case EVENT_CANFD_BUS_ERROR:
    case EVENT_CANFD_2_BUS_ERROR:
      return "Multiple CAN TX/RX errors. Check wiring!";
    case EVENT_CAN_BATTERY_DETECTED:
      return "Successfully communicating with battery. Battery detected!";
    case EVENT_CAN_BATTERY2_DETECTED:
      return "Successfully communicating with secondary battery. Secondary battery detected!";
    case EVENT_CAN_BATTERY3_DETECTED:
      return "Successfully communicating with third battery. Third battery detected!";
    case EVENT_CAN_BATTERY_MISSING:
      return "Battery not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_BATTERY2_MISSING:
      return "Secondary battery not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_BATTERY3_MISSING:
      return "Third battery not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_CHARGER_DETECTED:
      return "Successfully communicating with charger. Charger detected!";
    case EVENT_CAN_CHARGER_MISSING:
      return "Charger not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CAN_INVERTER_DETECTED:
      return "Successfully communicating with inverter. Inverter detected!";
    case EVENT_CAN_INVERTER_MISSING:
      return "Inverter not sending messages via CAN for the last 60 seconds. Check wiring!";
    case EVENT_CONTACTOR_WELDED:
      return "Contactors sticking/welded. Inspect battery with caution!";
    case EVENT_CONTACTOR_OPEN:
      return "Battery decided to open contactors. Inspect battery!";
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
    case EVENT_BATTERY2_EMPTY:
    case EVENT_BATTERY3_EMPTY:
      return "Battery is completely discharged";
    case EVENT_BATTERY_FULL:
    case EVENT_BATTERY2_FULL:
    case EVENT_BATTERY3_FULL:
      return "Battery is fully charged";
    case EVENT_BATTERY_FUSE:
    case EVENT_BATTERY2_FUSE:
    case EVENT_BATTERY3_FUSE:
      return "Battery internal fuse blown. Inspect battery";
    case EVENT_BATTERY_FROZEN:
    case EVENT_BATTERY2_FROZEN:
    case EVENT_BATTERY3_FROZEN:
      return "Battery is too cold to operate optimally. Consider warming it up!";
    case EVENT_BATTERY_CAUTION:
    case EVENT_BATTERY2_CAUTION:
    case EVENT_BATTERY3_CAUTION:
      return "Battery has raised a general caution flag. Might want to inspect it closely.";
    case EVENT_BATTERY_CHG_STOP_REQ:
    case EVENT_BATTERY2_CHG_STOP_REQ:
    case EVENT_BATTERY3_CHG_STOP_REQ:
      return "Battery raised caution indicator AND requested charge stop. Inspect battery status!";
    case EVENT_BATTERY_DISCHG_STOP_REQ:
    case EVENT_BATTERY2_DISCHG_STOP_REQ:
    case EVENT_BATTERY3_DISCHG_STOP_REQ:
      return "Battery raised caution indicator AND requested discharge stop. Inspect battery status!";
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
    case EVENT_BATTERY2_CHG_DISCHG_STOP_REQ:
    case EVENT_BATTERY3_CHG_DISCHG_STOP_REQ:
      return "Battery raised caution indicator AND requested charge/discharge stop. Inspect battery status!";
    case EVENT_BATTERY_REQUESTS_HEAT:
    case EVENT_BATTERY2_REQUESTS_HEAT:
    case EVENT_BATTERY3_REQUESTS_HEAT:
      return "COLD BATTERY! Battery requesting heating pads to activate!";
    case EVENT_BATTERY_WARMED_UP:
    case EVENT_BATTERY2_WARMED_UP:
    case EVENT_BATTERY3_WARMED_UP:
      return "Battery requesting heating pads to stop. The battery is now warm enough.";
    case EVENT_BATTERY_OVERHEAT:
    case EVENT_BATTERY2_OVERHEAT:
    case EVENT_BATTERY3_OVERHEAT:
      return "Battery overheated. Shutting down to prevent thermal runaway!";
    case EVENT_BATTERY_OVERVOLTAGE:
    case EVENT_BATTERY2_OVERVOLTAGE:
    case EVENT_BATTERY3_OVERVOLTAGE:
      return "Battery exceeding maximum design voltage. Discharge battery to prevent damage!";
    case EVENT_BATTERY_UNDERVOLTAGE:
    case EVENT_BATTERY2_UNDERVOLTAGE:
    case EVENT_BATTERY3_UNDERVOLTAGE:
      return "Battery under minimum design voltage. Charge battery to prevent damage!";
    case EVENT_BATTERY_VALUE_UNAVAILABLE:
    case EVENT_BATTERY2_VALUE_UNAVAILABLE:
    case EVENT_BATTERY3_VALUE_UNAVAILABLE:
      return "Battery measurement unavailable. Check 12V power supply and battery wiring!";
    case EVENT_BATTERY_TEMP_DEVIATION_HIGH:
    case EVENT_BATTERY2_TEMP_DEVIATION_HIGH:
    case EVENT_BATTERY3_TEMP_DEVIATION_HIGH:
      return "Battery temperature sensors reporting large difference between hottest and coldest cell!";
    case EVENT_BATTERY_ISOLATION:
    case EVENT_BATTERY2_ISOLATION:
    case EVENT_BATTERY3_ISOLATION:
      return "Battery reports isolation error. High voltage might be leaking to ground. Check battery!";
    case EVENT_BATTERY_SOC_RECALIBRATION:
    case EVENT_BATTERY2_SOC_RECALIBRATION:
    case EVENT_BATTERY3_SOC_RECALIBRATION:
      return "The BMS updated the HV battery State of Charge (SOC) by more than 3pct based on SocByOcv.";
    case EVENT_BYD_AUTO_SOC_CALIBRATION:
      return "Auto SOC recalibration to 100% triggered. Data column shows drift% below 100%.";
    case EVENT_BYD_CHARGE_TERMINATED:
      return "Battery ended the charge itself and recalibrated SOC. Data column shows cell spread in tens of mV.";
    case EVENT_BYD_CONTACTOR_MISMATCH:
      return "Battery did not confirm the contactor command in time. Data: 2 = open not confirmed, 3 = close not "
             "confirmed, 4 = close retries exhausted, pack left open.";
    case EVENT_BYD_CONTACTOR_FORCE_OPEN:
      return "Contactors force-opened: pack current was not confirmed safe before the timeout. Data: 0 = current "
             "stayed high, 1 = no fresh current reading. Check the inverter ramped down.";
    case EVENT_BYD_CONTACTOR_OPEN_REQ:
      return "Contactor open commanded. Power is set to zero and the contactors open once current stops. Data: 1 = "
             "from an emergency stop saved across reboot.";
    case EVENT_BYD_CONTACTOR_CLOSE_REQ:
      return "Contactor close commanded. The battery precharges and closes its contactors. Data: 1 = cancelled a "
             "pending open.";
    case EVENT_BATTERY_SOC_RESET_SUCCESS:
    case EVENT_BATTERY2_SOC_RESET_SUCCESS:
    case EVENT_BATTERY3_SOC_RESET_SUCCESS:
      return "SOC reset routine was successful.";
    case EVENT_BATTERY_SOC_RESET_FAIL:
    case EVENT_BATTERY2_SOC_RESET_FAIL:
    case EVENT_BATTERY3_SOC_RESET_FAIL:
      return "SOC reset routine failed - check SOC is < 15 or > 90, and contactors are open.";
    case EVENT_VOLTAGE_DIFFERENCE_BAT2:
      return "Too large voltage diff between the batteries. Second battery cannot join the DC-link";
    case EVENT_VOLTAGE_DIFFERENCE_BAT3:
      return "Too large voltage diff between the batteries. Third battery cannot join the DC-link";
    case EVENT_SOH_DIFFERENCE:
      return "Large deviation in State of health between packs. Inspect battery.";
    case EVENT_SOH_LOW:
      return "State of health critically low. Battery internal resistance too high to continue. Recycle "
             "battery.";
    case EVENT_HVIL_FAILURE:
      return "Battery interlock loop broken. Check that high voltage / low voltage connectors are seated. "
             "Battery will be disabled!";
    case EVENT_LOW_HEAP_MEMORY:
      return "Memory almost full. Inform developers.";
    case EVENT_PRECHARGE_FAILURE:
      return "Battery failed to precharge. Check that capacitor is seated on high voltage output.";
    case EVENT_AUTOMATIC_PRECHARGE_FAILURE:
      return "Automatic precharge FAILURE. Failed to reach target voltage or BMS timeout. Reboot emulator to retry!";
    case EVENT_INTERNAL_OPEN_FAULT:
      return "High voltage cable removed while battery running. Opening contactors!";
    case EVENT_INVERTER_OPEN_CONTACTOR:
      return "Inverter side opened contactors. Normal operation.";
    case EVENT_INTERFACE_MISSING:
      return "Configuration trying to use CAN interface not baked into the software. Recompile software!";
    case EVENT_ERROR_OPEN_CONTACTOR:
      return "Too much time spent in error state. Opening contactors, not safe to continue. "
             "Check other active ERROR code for reason. Reboot emulator after problem is solved!";
    case EVENT_MODBUS_INVERTER_MISSING:
      return "Modbus inverter has not sent any data. Inspect communication wiring!";
    case EVENT_MODBUS_INVERTER_DETECTED:
      return "Successfully communicating with inverter over Modbus/RS485. Inverter detected!";
    case EVENT_INVERTER_REBOOT_DECLINED:
      return "Inverter asked the emulator to restart, but the request was declined. "
             "Enable 'Accept reboot command from inverter' in the settings if you want to allow it next time.";
    case EVENT_NO_ENABLE_DETECTED:
      return "Inverter Enable line has not been active for a long time. Check Wiring!";
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
    case EVENT_SMA_PAIRING:
      return "SMA inverter trying to pair, contactors will close and open according to Enable line";
    case EVENT_OTA_UPDATE:
      return "OTA update started!";
    case EVENT_OTA_UPDATE_TIMEOUT:
      return "OTA update timed out!";
    case EVENT_OTA_ROLLBACK:
      return "A firmware update did not start up and was rolled back. This board is running the previous firmware; "
             "the log line names which version failed.";
    case EVENT_RECOVERY_START:
      return "CAUTION! Emergency low charge recovery started! Make sure battery cells do not overheat!";
    case EVENT_RECOVERY_END:
      return "Emergency charge recovery max time reached. Reboot and inspect if battery is able to continue normally";
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
    case EVENT_RESTARTING:
      return "The emulator is restarting.";
    case EVENT_RJXZS_LOG:
      return "Error code active in RJXZS BMS. Clear via their smartphone app!";
    case EVENT_PAUSE_BEGIN:
      return "The emulator is trying to pause the battery.";
    case EVENT_PAUSE_END:
      return "The emulator is attempting to resume battery operation from pause.";
    case EVENT_PID_FAILED:
      return "Failed to write PID request to battery";
    case EVENT_WIFI_CONNECT:
      return "Wi-Fi connected.";
    case EVENT_WIFI_DISCONNECT:
      return "Wi-Fi disconnected.";
    case EVENT_WIFI_AP_PASSWORD_DEFAULT:
      return "The AP will be disabled after 5 idle minutes. Change default password to keep AP constantly on!";
    case EVENT_WIFI_AP_PROVISION_TIMEOUT:
      return "Wi-Fi AP disabled due to cybersecurity concern. Change default password to keep AP "
             "constantly on! Reboot/Hold BOOT button 5-15 seconds to re-enable AP temporarily.";
    case EVENT_MQTT_CONNECT:
      return "MQTT connected.";
    case EVENT_MQTT_DISCONNECT:
      return "MQTT disconnected.";
    case EVENT_EQUIPMENT_STOP:
      return "User requested stop, either via equipment stop circuit or webserver Open Contactor button";
    case EVENT_SD_INIT_FAILED:
      return "SD card initialization failed, check hardware. Power must be removed to reset the SD card.";
    case EVENT_PERIODIC_BMS_RESET:
      return "BMS reset event completed.";
    case EVENT_PERIODIC_BMS_RESET_FAILURE:
      return "BMS reset aborted - contactors were still under load.";
    case EVENT_BMS_RESET_REQ_SUCCESS:
      return "BMS reset request completed successfully.";
    case EVENT_BMS_RESET_REQ_FAIL:
      return "BMS reset request failed - check contactors are open.";
    case EVENT_GPIO_CONFLICT:
      return "GPIO Pin Conflict: The pin used by '" + esp32hal->failed_allocator() + "' is already allocated by '" +
             esp32hal->conflicting_allocator() + "'. Please check your configuration and assign different pins.";
    case EVENT_GPIO_NOT_DEFINED:
      return "Missing GPIO Assignment: The component '" + esp32hal->failed_allocator() +
             "' requires a GPIO pin that isn't configured. Please define a valid pin number in your settings.";
    default:
      return "";
  }
}

String get_event_message_string(EVENTS_ENUM_TYPE event) {
  String message = get_event_base_message(event);
  /* The three variants of a battery event share one message string, so name the pack here
     rather than storing 57 near-identical literals in flash. 0 = not battery specific. */
  const uint8_t battery = event_battery_number(event);
  if (battery) {
    // Built into a plain buffer and appended as const char*. The native unit-test build
    // (test/emul/WString.h) only provides String::operator+=(const String&/std::string/const char*),
    // and has no F() macro, so the Arduino-only integer and char overloads cannot be used here.
    char suffix[16];
    snprintf(suffix, sizeof(suffix), " (Battery %u)", (unsigned)battery);
    message += suffix;
  }
  return message;
}

const char* get_event_enum_string(EVENTS_ENUM_TYPE event) {
  // Return the event name but skip "EVENT_" that should always be first
  return EVENTS_ENUM_TYPE_STRING[event] + 6;
}

const char* get_event_level_string(EVENTS_ENUM_TYPE event) {
  // Return the event level but skip "EVENT_LEVEL_" that should always be first
  return EVENTS_LEVEL_TYPE_STRING[effective_level(event)] + 12;
}

const char* get_event_level_string(EVENTS_LEVEL_TYPE event_level) {
  // Return the event level but skip "EVENT_LEVEL_TYPE_" that should always be first
  return EVENTS_LEVEL_TYPE_STRING[event_level] + 17;
}

const EVENTS_STRUCT_TYPE* get_event_pointer(EVENTS_ENUM_TYPE event) {
  return &events.entries[event];
}

EVENTS_LEVEL_TYPE get_event_level(void) {
  return events.level;
}

EMULATOR_STATUS get_emulator_status() {
  switch (events.level) {
    case EVENT_LEVEL_DEBUG:
    case EVENT_LEVEL_INFO:
      return EMULATOR_STATUS::STATUS_OK;
    case EVENT_LEVEL_WARNING:
      return EMULATOR_STATUS::STATUS_WARNING;
    case EVENT_LEVEL_UPDATE:
      return EMULATOR_STATUS::STATUS_UPDATING;
    case EVENT_LEVEL_ERROR:
      return EMULATOR_STATUS::STATUS_ERROR;
    default:
      return EMULATOR_STATUS::STATUS_OK;
  }
}

const char* get_emulator_status_string(EMULATOR_STATUS status) {
  // Return the status string but skip "STATUS_" that should always be first
  return EMULATOR_STATUS_STRING[status] + 7;
}

/* Local functions */

// True if 'event' is one of the two comm-error events belonging to 'interface'.
static bool is_can_error_of_interface(EVENTS_ENUM_TYPE event, CAN_Interface interface) {
  switch (interface) {
    case CAN_NATIVE:
      return event == EVENT_CAN_NATIVE_BUFFER_FULL || event == EVENT_CAN_NATIVE_BUS_ERROR;
    case CANFD_NATIVE:  // routed through the MCP2518 path, shares the CANFD events
    case CANFD_ADDON_MCP2518:
      return event == EVENT_CANFD_BUFFER_FULL || event == EVENT_CANFD_BUS_ERROR;
    case CAN_ADDON_MCP2515:
      return event == EVENT_CANMCP2515_BUFFER_FULL || event == EVENT_CANMCP2515_BUS_ERROR;
    case CANFD_ADDON_MCP2518_2:
      return event == EVENT_CANFD_2_BUFFER_FULL || event == EVENT_CANFD_2_BUS_ERROR;
    default:
      return false;
  }
}

// Returns true while any interface has an open ignore window that 'event' belongs to.
// Checked per interface so several windows can be active at once.
static bool can_error_ignored(EVENTS_ENUM_TYPE event) {
  uint64_t now = millis64();
  for (uint8_t i = 0; i < NO_CAN_INTERFACE; i++) {
    if (can_errors_ignore_until_ms[i] > now && is_can_error_of_interface(event, (CAN_Interface)i)) {
      return true;
    }
  }
  return false;
}

static void set_event_internal(EVENTS_ENUM_TYPE event, int16_t data, bool latched) {
  // Just some defensive stuff if someone sets an unknown event
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }

  // Drop transient CAN comm errors on the specified interface
  if (can_error_ignored(event)) {
    return;
  }

  // Store the payload before the logging below, so the log and syslog lines carry this
  // occurrence's value rather than the previous one's.
  events.entries[event].data = data;

  // If the event is already set, no reason to continue
  if ((events.entries[event].state != EVENT_STATE_ACTIVE) &&
      (events.entries[event].state != EVENT_STATE_ACTIVE_LATCHED)) {
    events.entries[event].MQTTpublished = false;

    LOG_SET_NEXT_SEVERITY(event_syslog_severity(event));
    DEBUG_PRINTF("%s (event)\n", get_event_message_string(event).c_str());
  }

  // We should set the event, update event info
  events.entries[event].occurences++;
  events.entries[event].timestamp = millis64();
  // Check if the event is latching
  events.entries[event].state = latched ? EVENT_STATE_ACTIVE_LATCHED : EVENT_STATE_ACTIVE;

  // Update event level, only upwards. Downward changes are done in Software.ino:loop()
  events.level = (EVENTS_LEVEL_TYPE)max(events.level, effective_level(event));

  update_bms_status();
}

static void update_bms_status(void) {
  switch (events.level) {
    case EVENT_LEVEL_INFO:
    case EVENT_LEVEL_WARNING:
    case EVENT_LEVEL_DEBUG:
      datalayer.system.status.system_status = ACTIVE;
      break;
    case EVENT_LEVEL_UPDATE:
      datalayer.system.status.system_status = UPDATING;
      break;
    case EVENT_LEVEL_ERROR:
      // Normally FAULT mode is set if a catastrophic event has triggered, but incase user has forced a recovery charge, we override any FAULT and continue temporarily in active mode
      if (datalayer.battery.settings.user_requests_forced_charging_recovery_mode) {
        datalayer.system.status.system_status = ACTIVE;  //Edge case which is active for 30min max
      } else {
        datalayer.system.status.system_status = FAULT;  //We will in 99.999% of the time go here
      }
      break;
    default:
      break;
  }
}

// Function to compare events by timestamp descending
bool compareEventsByTimestampDesc(const EventData& a, const EventData& b) {
  return a.event_pointer->timestamp > b.event_pointer->timestamp;
}

// Function to compare events by timestamp ascending
bool compareEventsByTimestampAsc(const EventData& a, const EventData& b) {
  return a.event_pointer->timestamp < b.event_pointer->timestamp;
}

static void update_event_level(void) {
  EVENTS_LEVEL_TYPE temporary_level = EVENT_LEVEL_INFO;
  for (uint8_t i = 0u; i < EVENT_NOF_EVENTS; i++) {
    if ((events.entries[i].state == EVENT_STATE_ACTIVE) || (events.entries[i].state == EVENT_STATE_ACTIVE_LATCHED)) {
      temporary_level = (EVENTS_LEVEL_TYPE)max(effective_level((EVENTS_ENUM_TYPE)i), temporary_level);
    }
  }
  events.level = temporary_level;
}
