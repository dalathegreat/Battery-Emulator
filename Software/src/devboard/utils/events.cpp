#include "events.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../devboard/hal/hal.h"
#include "../../devboard/utils/logging.h"
#include "../i18n/tr.h"

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
static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched);

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
  events.entries[EVENT_BATTERY_SOC_RECALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_AUTO_SOC_CALIBRATION].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_CONTACTOR_MISMATCH].level = EVENT_LEVEL_WARNING;
  events.entries[EVENT_BYD_CONTACTOR_FORCE_OPEN].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BYD_CONTACTOR_OPEN_REQ].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BYD_CONTACTOR_CLOSE_REQ].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_SOC_RESET_SUCCESS].level = EVENT_LEVEL_INFO;
  events.entries[EVENT_BATTERY_SOC_RESET_FAIL].level = EVENT_LEVEL_INFO;
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
  events.entries[EVENT_GPIO_CONFLICT].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_GPIO_NOT_DEFINED].level = EVENT_LEVEL_ERROR;
  events.entries[EVENT_BATTERY_TEMP_DEVIATION_HIGH].level = EVENT_LEVEL_WARNING;
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

String get_event_message_string(EVENTS_ENUM_TYPE event) {
  switch (event) {
    case EVENT_CANMCP2518FD_INIT_FAILURE:
      return TR_RAW(TrKey::EVENT_CANMCP2518FD_INIT_FAILURE);
    case EVENT_CANMCP2515_INIT_FAILURE:
      return TR_RAW(TrKey::EVENT_CANMCP2515_INIT_FAILURE);
    case EVENT_CAN_NATIVE_BUFFER_FULL:
    case EVENT_CANMCP2515_BUFFER_FULL:
    case EVENT_CANFD_BUFFER_FULL:
    case EVENT_CANFD_2_BUFFER_FULL:
      return TR_RAW(TrKey::EVENT_CAN_NATIVE_BUFFER_FULL);
    case EVENT_TASK_OVERRUN:
      return TR_RAW(TrKey::EVENT_TASK_OVERRUN);
    case EVENT_THERMAL_RUNAWAY:
      return TR_RAW(TrKey::EVENT_THERMAL_RUNAWAY);
    case EVENT_CAN_CORRUPTED_WARNING:
      return TR_RAW(TrKey::EVENT_CAN_CORRUPTED_WARNING);
    case EVENT_CAN_NATIVE_BUS_ERROR:
    case EVENT_CANMCP2515_BUS_ERROR:
    case EVENT_CANFD_BUS_ERROR:
    case EVENT_CANFD_2_BUS_ERROR:
      return TR_RAW(TrKey::EVENT_CAN_NATIVE_BUS_ERROR);
    case EVENT_CAN_BATTERY_DETECTED:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY_DETECTED);
    case EVENT_CAN_BATTERY2_DETECTED:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY2_DETECTED);
    case EVENT_CAN_BATTERY3_DETECTED:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY3_DETECTED);
    case EVENT_CAN_BATTERY_MISSING:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY_MISSING);
    case EVENT_CAN_BATTERY2_MISSING:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY2_MISSING);
    case EVENT_CAN_BATTERY3_MISSING:
      return TR_RAW(TrKey::EVENT_CAN_BATTERY3_MISSING);
    case EVENT_CAN_CHARGER_DETECTED:
      return TR_RAW(TrKey::EVENT_CAN_CHARGER_DETECTED);
    case EVENT_CAN_CHARGER_MISSING:
      return TR_RAW(TrKey::EVENT_CAN_CHARGER_MISSING);
    case EVENT_CAN_INVERTER_DETECTED:
      return TR_RAW(TrKey::EVENT_CAN_INVERTER_DETECTED);
    case EVENT_CAN_INVERTER_MISSING:
      return TR_RAW(TrKey::EVENT_CAN_INVERTER_MISSING);
    case EVENT_CONTACTOR_WELDED:
      return TR_RAW(TrKey::EVENT_CONTACTOR_WELDED);
    case EVENT_CONTACTOR_OPEN:
      return TR_RAW(TrKey::EVENT_CONTACTOR_OPEN);
    case EVENT_CHARGE_LIMIT_EXCEEDED:
      return TR_RAW(TrKey::EVENT_CHARGE_LIMIT_EXCEEDED);
    case EVENT_DISCHARGE_LIMIT_EXCEEDED:
      return TR_RAW(TrKey::EVENT_DISCHARGE_LIMIT_EXCEEDED);
    case EVENT_WATER_INGRESS:
      return TR_RAW(TrKey::EVENT_WATER_INGRESS);
    case EVENT_12V_LOW:
      return TR_RAW(TrKey::EVENT_12V_LOW);
    case EVENT_SOC_PLAUSIBILITY_ERROR:
      return TR_RAW(TrKey::EVENT_SOC_PLAUSIBILITY_ERROR);
    case EVENT_SOC_UNAVAILABLE:
      return TR_RAW(TrKey::EVENT_SOC_UNAVAILABLE);
    case EVENT_STALE_VALUE:
      return TR_RAW(TrKey::EVENT_STALE_VALUE);
    case EVENT_KWH_PLAUSIBILITY_ERROR:
      return TR_RAW(TrKey::EVENT_KWH_PLAUSIBILITY_ERROR);
    case EVENT_BALANCING_START:
      return TR_RAW(TrKey::EVENT_BALANCING_START);
    case EVENT_BALANCING_END:
      return TR_RAW(TrKey::EVENT_BALANCING_END);
    case EVENT_BATTERY_EMPTY:
      return TR_RAW(TrKey::EVENT_BATTERY_EMPTY);
    case EVENT_BATTERY_FULL:
      return TR_RAW(TrKey::EVENT_BATTERY_FULL);
    case EVENT_BATTERY_FUSE:
      return TR_RAW(TrKey::EVENT_BATTERY_FUSE);
    case EVENT_BATTERY_FROZEN:
      return TR_RAW(TrKey::EVENT_BATTERY_FROZEN);
    case EVENT_BATTERY_CAUTION:
      return TR_RAW(TrKey::EVENT_BATTERY_CAUTION);
    case EVENT_BATTERY_CHG_STOP_REQ:
      return TR_RAW(TrKey::EVENT_BATTERY_CHG_STOP_REQ);
    case EVENT_BATTERY_DISCHG_STOP_REQ:
      return TR_RAW(TrKey::EVENT_BATTERY_DISCHG_STOP_REQ);
    case EVENT_BATTERY_CHG_DISCHG_STOP_REQ:
      return TR_RAW(TrKey::EVENT_BATTERY_CHG_DISCHG_STOP_REQ);
    case EVENT_BATTERY_REQUESTS_HEAT:
      return TR_RAW(TrKey::EVENT_BATTERY_REQUESTS_HEAT);
    case EVENT_BATTERY_WARMED_UP:
      return TR_RAW(TrKey::EVENT_BATTERY_WARMED_UP);
    case EVENT_BATTERY_OVERHEAT:
      return TR_RAW(TrKey::EVENT_BATTERY_OVERHEAT);
    case EVENT_BATTERY_OVERVOLTAGE:
      return TR_RAW(TrKey::EVENT_BATTERY_OVERVOLTAGE);
    case EVENT_BATTERY_UNDERVOLTAGE:
      return TR_RAW(TrKey::EVENT_BATTERY_UNDERVOLTAGE);
    case EVENT_BATTERY_VALUE_UNAVAILABLE:
      return TR_RAW(TrKey::EVENT_BATTERY_VALUE_UNAVAILABLE);
    case EVENT_BATTERY_ISOLATION:
      return TR_RAW(TrKey::EVENT_BATTERY_ISOLATION);
    case EVENT_BATTERY_SOC_RECALIBRATION:
      return TR_RAW(TrKey::EVENT_BATTERY_SOC_RECALIBRATION);
    case EVENT_BYD_AUTO_SOC_CALIBRATION:
      return TR_RAW(TrKey::EVENT_BYD_AUTO_SOC_CALIBRATION);
    case EVENT_BYD_CONTACTOR_MISMATCH:
      return TR_RAW(TrKey::EVENT_BYD_CONTACTOR_MISMATCH);
    case EVENT_BYD_CONTACTOR_FORCE_OPEN:
      return TR_RAW(TrKey::EVENT_BYD_CONTACTOR_FORCE_OPEN);
    case EVENT_BYD_CONTACTOR_OPEN_REQ:
      return TR_RAW(TrKey::EVENT_BYD_CONTACTOR_OPEN_REQ);
    case EVENT_BYD_CONTACTOR_CLOSE_REQ:
      return TR_RAW(TrKey::EVENT_BYD_CONTACTOR_CLOSE_REQ);
    case EVENT_BATTERY_SOC_RESET_SUCCESS:
      return TR_RAW(TrKey::EVENT_BATTERY_SOC_RESET_SUCCESS);
    case EVENT_BATTERY_SOC_RESET_FAIL:
      return TR_RAW(TrKey::EVENT_BATTERY_SOC_RESET_FAIL);
    case EVENT_VOLTAGE_DIFFERENCE_BAT2:
      return TR_RAW(TrKey::EVENT_VOLTAGE_DIFFERENCE_BAT2);
    case EVENT_VOLTAGE_DIFFERENCE_BAT3:
      return TR_RAW(TrKey::EVENT_VOLTAGE_DIFFERENCE_BAT3);
    case EVENT_SOH_DIFFERENCE:
      return TR_RAW(TrKey::EVENT_SOH_DIFFERENCE);
    case EVENT_SOH_LOW:
      return TR_RAW(TrKey::EVENT_SOH_LOW);
    case EVENT_HVIL_FAILURE:
      return TR_RAW(TrKey::EVENT_HVIL_FAILURE);
    case EVENT_LOW_HEAP_MEMORY:
      return TR_RAW(TrKey::EVENT_LOW_HEAP_MEMORY);
    case EVENT_PRECHARGE_FAILURE:
      return TR_RAW(TrKey::EVENT_PRECHARGE_FAILURE);
    case EVENT_AUTOMATIC_PRECHARGE_FAILURE:
      return TR_RAW(TrKey::EVENT_AUTOMATIC_PRECHARGE_FAILURE);
    case EVENT_INTERNAL_OPEN_FAULT:
      return TR_RAW(TrKey::EVENT_INTERNAL_OPEN_FAULT);
    case EVENT_INVERTER_OPEN_CONTACTOR:
      return TR_RAW(TrKey::EVENT_INVERTER_OPEN_CONTACTOR);
    case EVENT_INTERFACE_MISSING:
      return TR_RAW(TrKey::EVENT_INTERFACE_MISSING);
    case EVENT_ERROR_OPEN_CONTACTOR:
      return TR_RAW(TrKey::EVENT_ERROR_OPEN_CONTACTOR);
    case EVENT_MODBUS_INVERTER_MISSING:
      return TR_RAW(TrKey::EVENT_MODBUS_INVERTER_MISSING);
    case EVENT_MODBUS_INVERTER_DETECTED:
      return TR_RAW(TrKey::EVENT_MODBUS_INVERTER_DETECTED);
    case EVENT_NO_ENABLE_DETECTED:
      return TR_RAW(TrKey::EVENT_NO_ENABLE_DETECTED);
    case EVENT_CELL_CRITICAL_UNDER_VOLTAGE:
      return TR_RAW(TrKey::EVENT_CELL_CRITICAL_UNDER_VOLTAGE);
    case EVENT_CELL_UNDER_VOLTAGE:
      return TR_RAW(TrKey::EVENT_CELL_UNDER_VOLTAGE);
    case EVENT_CELL_OVER_VOLTAGE:
      return TR_RAW(TrKey::EVENT_CELL_OVER_VOLTAGE);
    case EVENT_CELL_CRITICAL_OVER_VOLTAGE:
      return TR_RAW(TrKey::EVENT_CELL_CRITICAL_OVER_VOLTAGE);
    case EVENT_CELL_DEVIATION_HIGH:
      return TR_RAW(TrKey::EVENT_CELL_DEVIATION_HIGH);
    case EVENT_UNKNOWN_EVENT_SET:
      return TR_RAW(TrKey::EVENT_UNKNOWN_EVENT_SET);
    case EVENT_DUMMY_INFO:
      return TR_RAW(TrKey::EVENT_DUMMY_INFO);  // Don't change this event message!
    case EVENT_DUMMY_DEBUG:
      return TR_RAW(TrKey::EVENT_DUMMY_DEBUG);  // Don't change this event message!
    case EVENT_DUMMY_WARNING:
      return TR_RAW(TrKey::EVENT_DUMMY_WARNING);  // Don't change this event message!
    case EVENT_DUMMY_ERROR:
      return TR_RAW(TrKey::EVENT_DUMMY_ERROR);  // Don't change this event message!
    case EVENT_PERSISTENT_SAVE_INFO:
      return TR_RAW(TrKey::EVENT_PERSISTENT_SAVE_INFO);
    case EVENT_SERIAL_RX_WARNING:
      return TR_RAW(TrKey::EVENT_SERIAL_RX_WARNING);
    case EVENT_SERIAL_RX_FAILURE:
      return TR_RAW(TrKey::EVENT_SERIAL_RX_FAILURE);
    case EVENT_SERIAL_TX_FAILURE:
      return TR_RAW(TrKey::EVENT_SERIAL_TX_FAILURE);
    case EVENT_SERIAL_TRANSMITTER_FAILURE:
      return TR_RAW(TrKey::EVENT_SERIAL_TRANSMITTER_FAILURE);
    case EVENT_SMA_PAIRING:
      return TR_RAW(TrKey::EVENT_SMA_PAIRING);
    case EVENT_OTA_UPDATE:
      return TR_RAW(TrKey::EVENT_OTA_UPDATE);
    case EVENT_OTA_UPDATE_TIMEOUT:
      return TR_RAW(TrKey::EVENT_OTA_UPDATE_TIMEOUT);
    case EVENT_RECOVERY_START:
      return TR_RAW(TrKey::EVENT_RECOVERY_START);
    case EVENT_RECOVERY_END:
      return TR_RAW(TrKey::EVENT_RECOVERY_END);
    case EVENT_RESET_UNKNOWN:
      return TR_RAW(TrKey::EVENT_RESET_UNKNOWN);
    case EVENT_RESET_POWERON:
      return TR_RAW(TrKey::EVENT_RESET_POWERON);
    case EVENT_RESET_EXT:
      return TR_RAW(TrKey::EVENT_RESET_EXT);
    case EVENT_RESET_SW:
      return TR_RAW(TrKey::EVENT_RESET_SW);
    case EVENT_RESET_PANIC:
      return TR_RAW(TrKey::EVENT_RESET_PANIC);
    case EVENT_RESET_INT_WDT:
      return TR_RAW(TrKey::EVENT_RESET_INT_WDT);
    case EVENT_RESET_TASK_WDT:
      return TR_RAW(TrKey::EVENT_RESET_TASK_WDT);
    case EVENT_RESET_WDT:
      return TR_RAW(TrKey::EVENT_RESET_WDT);
    case EVENT_RESET_DEEPSLEEP:
      return TR_RAW(TrKey::EVENT_RESET_DEEPSLEEP);
    case EVENT_RESET_BROWNOUT:
      return TR_RAW(TrKey::EVENT_RESET_BROWNOUT);
    case EVENT_RESET_SDIO:
      return TR_RAW(TrKey::EVENT_RESET_SDIO);
    case EVENT_RESET_USB:
      return TR_RAW(TrKey::EVENT_RESET_USB);
    case EVENT_RESET_JTAG:
      return TR_RAW(TrKey::EVENT_RESET_JTAG);
    case EVENT_RESET_EFUSE:
      return TR_RAW(TrKey::EVENT_RESET_EFUSE);
    case EVENT_RESET_PWR_GLITCH:
      return TR_RAW(TrKey::EVENT_RESET_PWR_GLITCH);
    case EVENT_RESET_CPU_LOCKUP:
      return TR_RAW(TrKey::EVENT_RESET_CPU_LOCKUP);
    case EVENT_RESTARTING:
      return TR_RAW(TrKey::EVENT_RESTARTING);
    case EVENT_RJXZS_LOG:
      return TR_RAW(TrKey::EVENT_RJXZS_LOG);
    case EVENT_PAUSE_BEGIN:
      return TR_RAW(TrKey::EVENT_PAUSE_BEGIN);
    case EVENT_PAUSE_END:
      return TR_RAW(TrKey::EVENT_PAUSE_END);
    case EVENT_PID_FAILED:
      return TR_RAW(TrKey::EVENT_PID_FAILED);
    case EVENT_WIFI_CONNECT:
      return TR_RAW(TrKey::EVENT_WIFI_CONNECT);
    case EVENT_WIFI_DISCONNECT:
      return TR_RAW(TrKey::EVENT_WIFI_DISCONNECT);
    case EVENT_WIFI_AP_PASSWORD_DEFAULT:
      return TR_RAW(TrKey::EVENT_WIFI_AP_PASSWORD_DEFAULT);
    case EVENT_WIFI_AP_PROVISION_TIMEOUT:
      return TR_RAW(TrKey::EVENT_WIFI_AP_PROVISION_TIMEOUT);
    case EVENT_MQTT_CONNECT:
      return TR_RAW(TrKey::EVENT_MQTT_CONNECT);
    case EVENT_MQTT_DISCONNECT:
      return TR_RAW(TrKey::EVENT_MQTT_DISCONNECT);
    case EVENT_EQUIPMENT_STOP:
      return TR_RAW(TrKey::EVENT_EQUIPMENT_STOP);
    case EVENT_SD_INIT_FAILED:
      return TR_RAW(TrKey::EVENT_SD_INIT_FAILED);
    case EVENT_PERIODIC_BMS_RESET:
      return TR_RAW(TrKey::EVENT_PERIODIC_BMS_RESET);
    case EVENT_PERIODIC_BMS_RESET_FAILURE:
      return TR_RAW(TrKey::EVENT_PERIODIC_BMS_RESET_FAILURE);
    case EVENT_BMS_RESET_REQ_SUCCESS:
      return TR_RAW(TrKey::EVENT_BMS_RESET_REQ_SUCCESS);
    case EVENT_BMS_RESET_REQ_FAIL:
      return TR_RAW(TrKey::EVENT_BMS_RESET_REQ_FAIL);
    case EVENT_GPIO_CONFLICT:
      return tr_expand(TR_RAW(TrKey::EVENT_GPIO_CONFLICT), esp32hal->failed_allocator(),
                       esp32hal->conflicting_allocator());
    case EVENT_GPIO_NOT_DEFINED:
      return tr_expand(TR_RAW(TrKey::EVENT_GPIO_NOT_DEFINED), esp32hal->failed_allocator());
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

static void set_event(EVENTS_ENUM_TYPE event, uint8_t data, bool latched) {
  // Just some defensive stuff if someone sets an unknown event
  if (event >= EVENT_NOF_EVENTS) {
    event = EVENT_UNKNOWN_EVENT_SET;
  }

  // Drop transient CAN comm errors on the specified interface
  if (can_error_ignored(event)) {
    return;
  }

  // If the event is already set, no reason to continue
  if ((events.entries[event].state != EVENT_STATE_ACTIVE) &&
      (events.entries[event].state != EVENT_STATE_ACTIVE_LATCHED)) {
    events.entries[event].MQTTpublished = false;

    LOG_SET_NEXT_SEVERITY(event_level_to_syslog(effective_level(event)));
    DEBUG_PRINTF("Event: %s\n", get_event_message_string(event).c_str());
  }

  // We should set the event, update event info
  events.entries[event].occurences++;
  events.entries[event].timestamp = millis64();
  events.entries[event].data = data;
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
