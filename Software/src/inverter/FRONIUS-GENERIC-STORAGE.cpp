#include "FRONIUS-GENERIC-STORAGE.h"
#include <esp_mac.h>  // esp_read_mac()
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/logging.h"
#include "../devboard/utils/version.h"
#include "../inverter/INVERTERS.h"

/* Emulator error events and the fault type each one is reported as. The types are those of the
   BYD2 table the inverter holds (see MSG_PREFIX), so its event log shows a text instead of a bare
   number. Only events at error level are reported: the inverter drops everything milder anyway. */
const FroniusGenericStorageInverter::FaultMapping FroniusGenericStorageInverter::FAULT_MAP[] = {
    {EVENT_CAN_BATTERY_MISSING, 3, 1},           // The BCU cannot communicate with all the battery modules
    {EVENT_DISCHARGE_LIMIT_EXCEEDED, 3, 7},      // Over current during discharge detected
    {EVENT_CELL_CRITICAL_UNDER_VOLTAGE, 3, 13},  // Cell Under Voltage 3
    {EVENT_BATTERY_UNDERVOLTAGE, 3, 13},         // Cell Under Voltage 3
    {EVENT_CHARGE_LIMIT_EXCEEDED, 3, 18},        // Charge Over Current
    {EVENT_CELL_CRITICAL_OVER_VOLTAGE, 3, 24},   // Cell Over Voltage 3
    {EVENT_BATTERY_OVERVOLTAGE, 3, 24},          // Cell Over Voltage 3
    {EVENT_BATTERY_OVERHEAT, 3, 27},             // Charge Over Temperature 3
    {EVENT_BATTERY_FROZEN, 3, 29},               // Charge Under Temperature 3
    {EVENT_CELL_DEVIATION_HIGH, 3, 32},          // Battery Imbalance
    {EVENT_THERMAL_RUNAWAY, 3, 38},              // Cell Failure
    {EVENT_CONTACTOR_WELDED, 3, 44},             // BCU_Main Circuit Failure
    {EVENT_CONTACTOR_OPEN, 3, 44},               // BCU_Main Circuit Failure
    {EVENT_BATTERY_FUSE, 3, 45},                 // Battery Break
    {EVENT_INTERNAL_OPEN_FAULT, 3, 45},          // Battery Break
    {EVENT_HVIL_FAILURE, 3, 47},                 // Hardware fault at the battery control unit (BCU)
    {EVENT_PRECHARGE_FAILURE, 1, 48},            // Pre-charge Fault
    {EVENT_AUTOMATIC_PRECHARGE_FAILURE, 1, 48},  // Pre-charge Fault
    {EVENT_BATTERY_ISOLATION, 3, 49},            // DCBusFault
};
const uint8_t FroniusGenericStorageInverter::FAULT_MAP_SIZE = sizeof(FAULT_MAP) / sizeof(FAULT_MAP[0]);

const char* FroniusGenericStorageInverter::mode_name(uint16_t mode) {
  static const char* const names[] = {"Standby", "Inactive", "Darkstart", "Active", "Fault", "Updating"};
  return (mode <= MODE_UPDATING) ? names[mode] : "none";
}

void FroniusGenericStorageInverter::put_u32(uint16_t reg, uint32_t value) {
  mbPV[reg] = value >> 16;
  mbPV[reg + 1] = value & 0xFFFF;
}

void FroniusGenericStorageInverter::put_u64(uint16_t reg, uint64_t value) {
  put_u32(reg, value >> 32);
  put_u32(reg + 2, value & 0xFFFFFFFF);
}

void FroniusGenericStorageInverter::put_string(uint16_t reg, uint8_t registers, const char* text) {
  const size_t length = text ? strlen(text) : 0;
  for (uint8_t i = 0; i < registers; i++) {
    const uint8_t high = (size_t)(i * 2) < length ? text[i * 2] : 0;
    const uint8_t low = (size_t)(i * 2 + 1) < length ? text[i * 2 + 1] : 0;
    mbPV[reg + i] = (high << 8) | low;
  }
}

void FroniusGenericStorageInverter::get_string(uint16_t reg, uint8_t registers, char* out) {
  size_t length = 0;
  for (uint8_t i = 0; i < registers; i++) {
    const uint16_t value = mbPV[reg + i];
    const char pair[2] = {(char)(value >> 8), (char)(value & 0xFF)};
    for (char c : pair) {
      if (c == 0) {
        out[length] = 0;
        return;
      }
      // The text ends up in the log and on a web page, and comes from outside. Letters, digits and
      // the punctuation a name or version number is made of pass, the rest does not.
      const bool plain = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                         strchr(" .-_/()+:,#", c) != nullptr;
      out[length++] = plain ? c : '?';
    }
  }
  out[length] = 0;
}

void FroniusGenericStorageInverter::handle_static_data() {
  // Create every register of the map up front. A read of a register the map does not hold yet
  // would insert it, from the Modbus task, while this one is writing to the same map.
  static const uint16_t blocks[][2] = {{REG_IDENTIFICATION, 122},
                                       {REG_NAMEPLATE, 35},
                                       {REG_MONITORING, 30},
                                       {REG_CONTROL, 10},
                                       {REG_FAULTS, FAULT_ENTRIES * FAULT_ENTRY_SIZE},
                                       {REG_UPDATE, 19},
                                       {REG_INVERTER_DETAILS, 42}};
  for (const auto& block : blocks) {
    for (uint16_t i = 0; i < block[1]; i++) {
      mbPV[block[0] + i] = 0;
    }
  }

  const char* battery_name = name_for_battery_type(user_selected_battery_type);
  if (battery_name == nullptr) {
    battery_name = "Battery";
  }

  // Unique per device: the inverter tells batteries apart by their serial number
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char serial[15];
  snprintf(serial, sizeof(serial), "BE%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // DeviceIdentificationData
  put_u32(REG_IDENTIFICATION, 0x53490001);                    // InterfaceID, "SI" and 1
  put_string(REG_IDENTIFICATION + 2, 16, "BatteryEmulator");  // Mn, manufacturer
  put_string(REG_IDENTIFICATION + 18, 16, battery_name);      // Md, model
  put_string(REG_IDENTIFICATION + 34, 8, "1.0");              // Opt
  put_string(REG_IDENTIFICATION + 42, 8, BUILD_VERSION);      // Vr, software version
  put_string(REG_IDENTIFICATION + 50, 16, serial);            // SN
  mbPV[REG_IDENTIFICATION + 66] = 2;                          // RegMapVrMajor
  mbPV[REG_IDENTIFICATION + 67] = 0;                          // RegMapVrMinor
  put_string(REG_IDENTIFICATION + 68, 50, battery_name);      // DisplayName
  put_string(REG_IDENTIFICATION + 118, 4, MSG_PREFIX);        // MsgPrefix

  mbPV[REG_CONTROL] = MODE_NO_REQUEST;  // opMode
  mbPV[REG_UPDATE] = 0;                 // UPD_TYPE, no firmware update through the inverter

  update_nameplate();
  update_monitoring();
}

void FroniusGenericStorageInverter::update_values() {
  verify_temperature();
  handle_inverter_control_data();  // WatchDogTimeout, UTC, RebootCommand and DarkstartEnable
  verify_inverter_modbus();
  handle_inverter_details();
  update_contactor_request();
  update_nameplate();
  update_monitoring();
  update_fault_buffers();
}

void FroniusGenericStorageInverter::update_nameplate() {
  // The inverter reads the capability flags once, while it sets the battery up
  put_u32(REG_NAMEPLATE, user_selected_fronius_iso_check ? CAPABILITY_ISOLATION_CHECK : 0);  // BatteryCapabilityFlags
  put_u32(REG_NAMEPLATE + 2, datalayer.aggregate.reported_total_capacity_Wh);                // NominalCapacity
  put_u32(REG_NAMEPLATE + 4, 40960);                                                         // ContMaxChargePwr
  put_u32(REG_NAMEPLATE + 6, 40960);                                                         // ContMaxDischargePwr
  if (user_selected_primo_gen24) {
    // MaxUDC. If the battery is higher the Gen24 forces discharge. Capped to 450.0V for the Primo
    mbPV[REG_NAMEPLATE + 8] = std::min(datalayer.aggregate.max_design_voltage_dV, static_cast<uint16_t>(4500u));
  } else {
    mbPV[REG_NAMEPLATE + 8] = datalayer.aggregate.max_design_voltage_dV;
  }
  mbPV[REG_NAMEPLATE + 9] = datalayer.aggregate.min_design_voltage_dV;  // MinUDC, below it the Gen24 disables battery
  put_u32(REG_NAMEPLATE + 10, 53248);                                   // PeakMaxChargePwr
  mbPV[REG_NAMEPLATE + 12] = 10;                                        // PeakMaxChargeT
  put_u32(REG_NAMEPLATE + 13, 53248);                                   // PeakMaxDischargePwr
  mbPV[REG_NAMEPLATE + 15] = 10;                                        // PeakMaxDischargeT
  // The SOC sent is already scaled to the window the user allows, so all of it is usable
  mbPV[REG_NAMEPLATE + 16] = 0;                                 // MinSoCUsable
  mbPV[REG_NAMEPLATE + 17] = 100;                               // MaxSoCUsable
  mbPV[REG_NAMEPLATE + 18] = 30;                                // TargetSoCServiceMode
  mbPV[REG_NAMEPLATE + 19] = (uint16_t)BATTERY_MINTEMPERATURE;  // TCellMin, where the emulator itself stops
  mbPV[REG_NAMEPLATE + 20] = (uint16_t)BATTERY_MAXTEMPERATURE;  // TCellMax
  put_u32(REG_NAMEPLATE + 21, CAL_MODE_NONE);                   // CalMode
  // CalWaitDischargeT, CalPower* and ImmediateChargeRequestStop (+23 to +32) stay 0
  mbPV[REG_NAMEPLATE + 33] = 1;  // NumberParallelStacks. Several packs are presented as one
  mbPV[REG_NAMEPLATE + 34] = 1;  // NumberModulesPerStack
}

void FroniusGenericStorageInverter::handle_inverter_details() {
  char firmware[21];
  char name[65];
  get_string(REG_INVERTER_DETAILS, 10, firmware);   // InvFwVer
  get_string(REG_INVERTER_DETAILS + 10, 32, name);  // InvName
  if (strcmp(firmware, inverter_firmware_text) != 0 || strcmp(name, inverter_name_text) != 0) {
    strcpy(inverter_firmware_text, firmware);
    strcpy(inverter_name_text, name);
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Inverter identifies as \"%s\", firmware %s\n", name, firmware);
  }

  // InverterRebooting. The inverter announces that it is about to go away for a while
  const uint16_t inverter_rebooting = mbPV[REG_CONTROL + 9];
  if (inverter_rebooting != last_inverter_rebooting) {
    last_inverter_rebooting = inverter_rebooting;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Inverter set InverterRebooting to %u\n", inverter_rebooting);
  }
}

void FroniusGenericStorageInverter::update_contactor_request() {
  // opMode is the mode the inverter wants the battery in. It keeps writing it until
  // MonitoringData.state says the same.
  const uint16_t op_mode = mbPV[REG_CONTROL];
  if (op_mode != requested_op_mode) {
    requested_op_mode = op_mode;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Inverter requests operating mode %u (%s)\n", op_mode, mode_name(op_mode));
  }

  // An inverter that has stopped talking wants nothing, whatever it asked for last. The time of its
  // last request is read before the clock: the Modbus task may move it on at any moment, and taken
  // the other way round it could end up later than "now".
  const uint32_t last_request = last_request_ms;
  const uint32_t now = millis();
  const bool silent = !inverter_detected || (now - last_request) > INVERTER_SILENCE_MS;
  const bool wants_battery = !silent && (op_mode == MODE_ACTIVE || op_mode == MODE_DARKSTART);

  switch (user_selected_inverter_contactor_mode) {
    case inverter_contactor_mode_enum::AlwaysClosed:
      contactor_closing_allowed = true;
      open_pending = false;
      return;
    case inverter_contactor_mode_enum::LockAfterFirstClose:
      contactor_lock = contactor_lock || wants_battery;
      contactor_closing_allowed = contactor_lock;
      open_pending = false;
      return;
    default:
      break;
  }

  if (wants_battery) {
    contactor_closing_allowed = true;
    open_pending = false;
  } else if (contactor_closing_allowed) {
    // The inverter has normally stopped all power transfer before it lets go of the battery. Make
    // sure of it, since the contactors must not open under load.
    if (!open_pending) {
      open_pending = true;
      open_pending_since_ms = now;
    }
    const int16_t current_dA = datalayer.aggregate.current_dA;
    const bool current_gone = current_dA <= OPEN_CURRENT_LIMIT_dA && current_dA >= -OPEN_CURRENT_LIMIT_dA;
    if (current_gone || (now - open_pending_since_ms) >= OPEN_WAIT_MS) {
      contactor_closing_allowed = false;
      open_pending = false;
    }
  }
}

uint16_t FroniusGenericStorageInverter::reported_state() {
  const system_status_enum status = datalayer.system.status.system_status;
  if (status == FAULT) {
    return MODE_FAULT;
  }
  if (status == UPDATING) {
    return MODE_UPDATING;
  }

  // The inverter is answered with the mode it asked for once that mode is reached, and not before:
  // an ACTIVE battery against a DC input that is still dead makes it complain.
  const bool connected = contactor_closing_allowed && datalayer.system.status.dc_bus_live;
  if (requested_op_mode == MODE_ACTIVE || requested_op_mode == MODE_DARKSTART) {
    reached_state = connected ? requested_op_mode : MODE_STANDBY;
  } else if (requested_op_mode == MODE_STANDBY || requested_op_mode == MODE_INACTIVE) {
    // With the contactors held closed by the workaround setting the request is still acknowledged,
    // so the inverter does not keep repeating it
    if (!open_pending) {
      reached_state = requested_op_mode;
    }
  } else {
    // Nothing asked yet. Only connected at this point if the contactors are kept closed regardless
    reached_state = connected ? MODE_ACTIVE : MODE_STANDBY;
  }
  return reached_state;
}

void FroniusGenericStorageInverter::update_monitoring() {
  const uint16_t state = reported_state();
  const bool delivering = (state == MODE_ACTIVE || state == MODE_DARKSTART);

  uint32_t flags = FLAG_LINK_OK;
  if (datalayer.aggregate.current_dA > 0) {
    flags |= FLAG_CHARGING;
  } else if (datalayer.aggregate.current_dA < 0) {
    flags |= FLAG_DISCHARGING;
  }
  if (datalayer.aggregate.reported_soc >= 10000) {
    flags |= FLAG_FULL;
  }

  // Charging is reported as negative power, discharging as positive: the opposite of the datalayer
  const int32_t power_W = -datalayer.aggregate.active_power_W;
  const uint32_t total_Wh = datalayer.aggregate.reported_total_capacity_Wh;

  mbPV[REG_MONITORING] = state;        // state
  put_u32(REG_MONITORING + 1, flags);  // flags
  // SOC. Never below 1%, to avoid overdischarge
  mbPV[REG_MONITORING + 3] = std::max(datalayer.aggregate.reported_soc, static_cast<uint16_t>(100u));
  put_u32(REG_MONITORING + 4, total_Wh);  // CurrTotalCapacity
  put_u32(REG_MONITORING + 6,
          std::min(datalayer.aggregate.reported_remaining_capacity_Wh, total_Wh));  // CurrRestCapacity
  // The power limits already carry the remote or manual current limit, applied by
  // update_aggregate_limits() for every protocol
  put_u32(REG_MONITORING + 8, datalayer.aggregate.max_discharge_power_W);         // CurrMaxDischargePwr
  put_u32(REG_MONITORING + 10, datalayer.aggregate.max_charge_power_W);           // CurrMaxChargePwr
  mbPV[REG_MONITORING + 12] = delivering ? datalayer.aggregate.voltage_dV : 0;    // UDC_Bus, after contactors
  put_u32(REG_MONITORING + 13, delivering ? static_cast<uint32_t>(power_W) : 0);  // PDC_Bus
  mbPV[REG_MONITORING + 15] = datalayer.aggregate.voltage_dV;                     // UDC_internal
  put_u32(REG_MONITORING + 16, static_cast<uint32_t>(power_W));                   // PDC_internal
  mbPV[REG_MONITORING + 18] = reported_temperature_min_dC;                        // TCellMinAct
  mbPV[REG_MONITORING + 19] = reported_temperature_max_dC;                        // TCellMaxAct
  put_u64(REG_MONITORING + 20,
          std::max(datalayer.aggregate.total_charged_battery_Wh, static_cast<int32_t>(0)));  // TotalEnergyCharged
  put_u64(REG_MONITORING + 24,
          std::max(datalayer.aggregate.total_discharged_battery_Wh, static_cast<int32_t>(0)));  // TotalEnergyDischarged
  mbPV[REG_MONITORING + 28] = TEMPERATURE_NOT_AVAILABLE;                                        // TAmbient
  mbPV[REG_MONITORING + 29] = datalayer.aggregate.soh_pptt;                                     // SOH
}

void FroniusGenericStorageInverter::push_fault(uint16_t type, uint8_t battery, int16_t data, uint64_t utc_s) {
  const uint16_t entry = REG_FAULTS + (fault_count % FAULT_ENTRIES) * FAULT_ENTRY_SIZE;
  fault_count++;
  // The inverter takes an entry as valid when its timestamp is set, so that goes in last
  put_u64(entry, 0);
  mbPV[entry + 4] = fault_count;              // Number, counts up with every fault
  mbPV[entry + 5] = type;                     // Type
  mbPV[entry + 6] = 0;                        // Severity. 0 = Fault2, the battery has stopped
  mbPV[entry + 7] = (uint16_t)data;           // Info_0, the data of the emulator event
  mbPV[entry + 8] = 0;                        // Info_1
  mbPV[entry + 9] = (battery << 8) | 0x00FF;  // Info_2, which pack. Low byte FF = the pack as a whole
  put_u64(entry, utc_s);                      // Timestamp
}

void FroniusGenericStorageInverter::update_fault_buffers() {
  static_assert(sizeof(FAULT_MAP) / sizeof(FAULT_MAP[0]) <= sizeof(fault_reported),
                "fault_reported is too small for FAULT_MAP");
  // The inverter rewrites its clock into the UTC register all the time. Keep the last value and
  // count on from it ourselves.
  if (inverter_modbus_utc_epoch_s != utc_reference_s) {
    utc_reference_s = inverter_modbus_utc_epoch_s;
    utc_reference_ms = millis();
  }
  if (utc_reference_s == 0) {
    return;  // No clock yet. The faults stay unreported, and go out once there is one
  }
  const uint64_t utc_s = utc_reference_s + (millis() - utc_reference_ms) / 1000;

  for (uint8_t i = 0; i < FAULT_MAP_SIZE; i++) {
    for (uint8_t variant = 0; variant < FAULT_MAP[i].variants; variant++) {
      const EVENTS_STRUCT_TYPE* event = get_event_pointer((EVENTS_ENUM_TYPE)(FAULT_MAP[i].event + variant));
      const bool active = event->level == EVENT_LEVEL_ERROR &&
                          (event->state == EVENT_STATE_ACTIVE || event->state == EVENT_STATE_ACTIVE_LATCHED);
      const uint8_t bit = 1 << variant;
      if (!active) {
        fault_reported[i] &= ~bit;
      } else if (!(fault_reported[i] & bit)) {
        fault_reported[i] |= bit;
        push_fault(FAULT_MAP[i].code, variant + 1, event->data, utc_s);
      }
    }
  }
}
