#include "FRONIUS-MODBUS-V2.h"
#include <esp_mac.h>         // esp_read_mac()
#include <mbedtls/sha256.h>  // mbedtls_sha256()
#include <cstdlib>           // std::abs()
#include <cstring>           // memcpy()
#include "../battery/BATTERIES.h"
#include "../datalayer/datalayer.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/logging.h"
#include "../devboard/utils/system_clock.h"
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
    {EVENT_EQUIPMENT_STOP, 1, 45},               // Battery Break
    {EVENT_HVIL_FAILURE, 3, 47},                 // Hardware fault at the battery control unit (BCU)
    {EVENT_PRECHARGE_FAILURE, 1, 48},            // Pre-charge Fault
    {EVENT_AUTOMATIC_PRECHARGE_FAILURE, 1, 48},  // Pre-charge Fault
    {EVENT_BATTERY_ISOLATION, 3, 49},            // DCBusFault
};
const uint8_t FroniusGenericStorageInverter::FAULT_MAP_SIZE = sizeof(FAULT_MAP) / sizeof(FAULT_MAP[0]);

const char* FroniusGenericStorageInverter::request_name(uint16_t request) {
  static const char* const names[] = {"Standby", "Inactive", "Active"};
  return (request <= REQUEST_ACTIVE) ? names[request] : "unknown";
}

const char* FroniusGenericStorageInverter::state_name(uint16_t state) {
  static const char* const names[] = {"Standby", "Inactive", "Darkstart", "Active", "Fault", "Updating"};
  return (state <= STATE_UPDATING) ? names[state] : "unknown";
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

  /* Unique per board, as the inverter tells batteries apart by their serial number: "BE" and the
     first 6 bytes of a hash of the MAC address, so the serial does not show the MAC. Anyone who
     knows how it is made can still work the MAC out, by trying all of Espressif's addresses. */
  static const char SERIAL_SALT[] = "Battery-Emulator Fronius Generic Storage";
  uint8_t hash_input[sizeof(SERIAL_SALT) - 1 + 6];
  memcpy(hash_input, SERIAL_SALT, sizeof(SERIAL_SALT) - 1);
  esp_read_mac(hash_input + sizeof(SERIAL_SALT) - 1, ESP_MAC_WIFI_STA);
  uint8_t hash[32];
  mbedtls_sha256(hash_input, sizeof(hash_input), hash, 0);
  char serial[15];
  snprintf(serial, sizeof(serial), "BE%02X%02X%02X%02X%02X%02X", hash[0], hash[1], hash[2], hash[3], hash[4], hash[5]);

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

  // NameplateData, all but the values update_nameplate() keeps following the battery
  // The inverter reads the capability flags once, while it sets the battery up
  put_u32(REG_NAMEPLATE, user_selected_fronius_iso_check ? CAPABILITY_ISOLATION_CHECK : 0);  // BatteryCapabilityFlags
  put_u32(REG_NAMEPLATE + 4, 40960);                                                         // ContMaxChargePwr
  put_u32(REG_NAMEPLATE + 6, 40960);                                                         // ContMaxDischargePwr
  put_u32(REG_NAMEPLATE + 10, 53248);                                                        // PeakMaxChargePwr
  mbPV[REG_NAMEPLATE + 12] = 10;                                                             // PeakMaxChargeT
  put_u32(REG_NAMEPLATE + 13, 53248);                                                        // PeakMaxDischargePwr
  mbPV[REG_NAMEPLATE + 15] = 10;                                                             // PeakMaxDischargeT
  // The SOC sent is already scaled to the window the user allows, so all of it is usable
  mbPV[REG_NAMEPLATE + 16] = 1;                                 // MinSoCUsable, Never below 1%, to avoid overdischarge
  mbPV[REG_NAMEPLATE + 17] = 100;                               // MaxSoCUsable
  mbPV[REG_NAMEPLATE + 18] = 30;                                // TargetSoCServiceMode
  mbPV[REG_NAMEPLATE + 19] = (uint16_t)BATTERY_MINTEMPERATURE;  // TCellMin, where the emulator itself stops
  mbPV[REG_NAMEPLATE + 20] = (uint16_t)BATTERY_MAXTEMPERATURE;  // TCellMax
  put_u32(REG_NAMEPLATE + 21, CAL_MODE_NONE);                   // CalMode
  // CalWaitDischargeT, CalPower* and ImmediateChargeRequestStop (+23 to +32) stay 0
  mbPV[REG_NAMEPLATE + 34] = 1;  // NumberModulesPerStack, as Battery Emulator knows nothing of modules

  mbPV[REG_CONTROL] = REQUEST_NONE;  // opMode
  mbPV[REG_UPDATE] = 0;              // UPD_TYPE, no firmware update through the inverter

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
  /* The nameplate values that come from the battery. They cannot go in with the rest in
     handle_static_data(): the inverter is set up before the battery, so at that point the
     aggregate holds no capacity and no voltage limits yet, and a 2nd or 3rd pack does not exist.
     They also move at runtime, when a 2nd or 3rd pack joins, when the battery driver learns its
     real capacity, or with the SOC scaling settings. BYD-Modbus refreshes the capacity and
     voltages in its p201 block for the same reason. */
  put_u32(REG_NAMEPLATE + 2, datalayer.aggregate.reported_total_capacity_Wh);  // NominalCapacity
  if (user_selected_primo_gen24) {
    // MaxUDC. If the battery is higher the Gen24 forces discharge. Capped to 450.0V for the Primo
    mbPV[REG_NAMEPLATE + 8] = std::min(datalayer.aggregate.max_design_voltage_dV, static_cast<uint16_t>(4500u));
  } else {
    mbPV[REG_NAMEPLATE + 8] = datalayer.aggregate.max_design_voltage_dV;
  }
  mbPV[REG_NAMEPLATE + 9] = datalayer.aggregate.min_design_voltage_dV;  // MinUDC, below it the Gen24 disables battery
  // A stack for each pack of a double or triple battery
  mbPV[REG_NAMEPLATE + 33] = datalayer.system.info.configured_batteries;  // NumberParallelStacks
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
    logging.printf("%sInverter identifies as \"%s\", firmware %s\n", log_prefix, name, firmware);
  }

  // InverterRebooting. The inverter announces that it is about to go away for a while
  const uint16_t inverter_rebooting = mbPV[REG_CONTROL + 9];
  if (inverter_rebooting != last_inverter_rebooting) {
    last_inverter_rebooting = inverter_rebooting;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("%sInverter set InverterRebooting to %u\n", log_prefix, inverter_rebooting);
  }
}

void FroniusGenericStorageInverter::update_contactor_request() {
  /* opMode is the mode the inverter wants the battery in. It keeps writing it until
     MonitoringData.state answers it. As a Symo GEN24 uses it:
       1 INACTIVE  contactors closed, no current. Where it brings the battery up to, from STANDBY
       2 ACTIVE    answered with state 3, with the contactors as they are
       0 STANDBY   contactors open. Only with no backup power configured: with backup the GEN24
                   keeps the battery ACTIVE, ready for a grid outage. Without, it parks the battery
                   in STANDBY at minimum SOC with no PV */
  const uint16_t op_mode = mbPV[REG_CONTROL];
  if (op_mode != requested_op_mode) {
    requested_op_mode = op_mode;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("%sInverter requests operating mode %u (%s)\n", log_prefix, op_mode, request_name(op_mode));
  }
  // Over with the first request, see darkstart_until_request
  darkstart_until_request = darkstart_until_request && op_mode == REQUEST_NONE;

  // An inverter that has stopped talking wants nothing, whatever it asked for last. The time of its
  // last request is read before the clock: the Modbus task may move it on at any moment, and taken
  // the other way round it could end up later than "now".
  const uint32_t last_request = last_request_ms;
  const uint32_t now = millis();
  const bool silent = !inverter_detected || (now - last_request) > INVERTER_SILENCE_MS;
  battery_wanted = !silent && (op_mode == REQUEST_INACTIVE || op_mode == REQUEST_ACTIVE);

  // The workarounds decide over the contactors only. The state still answers the request
  switch (user_selected_inverter_contactor_mode) {
    case inverter_contactor_mode_enum::AlwaysClosed:
      contactor_closing_allowed = true;
      standby_handover = false;
      return;
    case inverter_contactor_mode_enum::LockAfterFirstClose:
      contactor_lock = contactor_lock || battery_wanted;
      contactor_closing_allowed = contactor_lock;
      standby_handover = false;
      return;
    default:
      break;
  }

  if (battery_wanted) {
    contactor_closing_allowed = true;
    standby_handover = false;
  } else if (contactor_closing_allowed) {
    // STANDBY is reported from here on (see reported_state()). The contactors follow once the
    // inverter has seen it, and the current has gone: they must not open under load.
    if (!standby_handover) {
      standby_handover = true;
      standby_since_ms = now;
    }
    const uint32_t waited_ms = now - standby_since_ms;
    const bool seen = last_reported_state == STATE_STANDBY && (state_reads - state_reads_at_change) >= 2;
    const int16_t current_dA = datalayer.aggregate.current_dA;
    const bool current_gone = current_dA <= OPEN_CURRENT_LIMIT_dA && current_dA >= -OPEN_CURRENT_LIMIT_dA;
    const bool handed_over = seen && waited_ms >= STANDBY_HANDOVER_MS && (current_gone || waited_ms >= OPEN_WAIT_MS);
    if (handed_over || waited_ms >= STANDBY_UNSEEN_MS) {
      contactor_closing_allowed = false;
      standby_handover = false;
      LOG_SET_NEXT_SEVERITY(5);  // notice
      if (handed_over) {
        logging.printf("%sInverter has seen STANDBY, contactors may open (%lu ms)\n", log_prefix,
                       (unsigned long)waited_ms);
      } else {
        logging.printf("%sInverter has not read STANDBY, contactors may open all the same (%lu ms)\n", log_prefix,
                       (unsigned long)waited_ms);
      }
    }
  }
}

void FroniusGenericStorageInverter::on_registers_read(uint16_t first, uint16_t count) {
  // Runs in the Modbus task. A plain read and write: only this task ever changes the count
  if (first <= REG_MONITORING && REG_MONITORING - first < count) {
    state_reads = state_reads + 1;
  }
}

bool FroniusGenericStorageInverter::bus_connected() {
  return contactor_closing_allowed && datalayer.system.status.dc_bus_live;
}

uint16_t FroniusGenericStorageInverter::reported_state() {
  const system_status_enum status = datalayer.system.status.system_status;
  if (status == FAULT) {
    return STATE_FAULT;
  }
  if (status == UPDATING) {
    return STATE_UPDATING;
  }
  if (darkstart_until_request) {
    return STATE_DARKSTART;  // Until the inverter's first request, see darkstart_until_request
  }

  // STANDBY until the battery is both asked for and connected. That covers the precharge after a
  // request, during which the inverter must not count on the battery yet, and the hand-over to
  // STANDBY, where the state goes first and the contactors after. The DARKSTART of the start is
  // kept through the first precharge instead, see darkstart_until_request
  if (!battery_wanted) {
    return STATE_STANDBY;
  }
  if (!bus_connected()) {
    return (last_reported_state == STATE_DARKSTART) ? STATE_DARKSTART : STATE_STANDBY;
  }
  return (requested_op_mode == REQUEST_INACTIVE) ? STATE_INACTIVE : STATE_ACTIVE;
}

void FroniusGenericStorageInverter::update_monitoring() {
  const uint16_t state = reported_state();
  // The bus values go by the contactors, not by the state: in INACTIVE, and during the hand-over to
  // STANDBY, the inverter has the battery on its DC link all the same
  const bool delivering = bus_connected();

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

  mbPV[REG_MONITORING] = state;  // state
  if (state != last_reported_state) {
    // Taken after the register is written, so that only reads from here on count as seeing it
    last_reported_state = state;
    state_reads_at_change = state_reads;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("%sReporting state %u (%s) to the inverter\n", log_prefix, state, state_name(state));
  }
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
  // Lifetime counters, which the map has unsigned. Some batteries report discharged energy as a
  // negative number, so the sign is dropped rather than clamped to 0. Widened first, so that
  // INT32_MIN has an absolute value too.
  put_u64(REG_MONITORING + 20,
          std::abs(static_cast<int64_t>(datalayer.aggregate.total_charged_battery_Wh)));  // TotalEnergyCharged
  put_u64(REG_MONITORING + 24,
          std::abs(static_cast<int64_t>(datalayer.aggregate.total_discharged_battery_Wh)));  // TotalEnergyDischarged
  mbPV[REG_MONITORING + 28] = TEMPERATURE_NOT_AVAILABLE;                                     // TAmbient
  mbPV[REG_MONITORING + 29] = datalayer.aggregate.soh_pptt;                                  // SOH
}

void FroniusGenericStorageInverter::push_fault(uint16_t type, uint8_t battery, int16_t data, uint32_t now_ms) {
  const uint8_t slot = fault_count % FAULT_ENTRIES;
  const uint16_t entry = REG_FAULTS + slot * FAULT_ENTRY_SIZE;
  fault_count++;
  // The inverter takes an entry as valid when its timestamp is set, so that goes in last
  put_u64(entry, 0);
  mbPV[entry + 4] = fault_count;              // Number, counts up with every fault
  mbPV[entry + 5] = type;                     // Type
  mbPV[entry + 6] = 0;                        // Severity. 0 = Fault2, the battery has stopped
  mbPV[entry + 7] = (uint16_t)data;           // Info_0, the data of the emulator event
  mbPV[entry + 8] = 0;                        // Info_1
  mbPV[entry + 9] = (battery << 8) | 0x00FF;  // Info_2, which pack. Low byte FF = the pack as a whole
  const uint64_t time_s = clock_utc_s();
  fault_entry_ms[slot] = now_ms;
  fault_entry_unstamped[slot] = (time_s == 0);
  put_u64(entry, time_s);  // Timestamp. 0 while the clock is not valid, see stamp_faults()
}

void FroniusGenericStorageInverter::stamp_faults(uint32_t now_ms) {
  const uint64_t time_s = clock_utc_s();
  if (time_s == 0) {
    return;
  }
  for (uint8_t slot = 0; slot < FAULT_ENTRIES; slot++) {
    if (fault_entry_unstamped[slot]) {
      const uint32_t ago_s = (uint32_t)(now_ms - fault_entry_ms[slot]) / 1000;
      put_u64(REG_FAULTS + slot * FAULT_ENTRY_SIZE, ago_s < time_s ? time_s - ago_s : 1);
      fault_entry_unstamped[slot] = false;
    }
  }
}

void FroniusGenericStorageInverter::update_fault_buffers() {
  static_assert(sizeof(FAULT_MAP) / sizeof(FAULT_MAP[0]) <= sizeof(fault_reported),
                "fault_reported is too small for FAULT_MAP");
  const uint32_t now = millis();
  stamp_faults(now);

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
        push_fault(FAULT_MAP[i].code, variant + 1, event->data, now);
      }
    }
  }
}
