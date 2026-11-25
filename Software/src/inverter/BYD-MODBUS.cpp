#include "BYD-MODBUS.h"
#include "../battery/BATTERIES.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../communication/rs485/comm_rs485.h"
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/safety/safety.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "../devboard/utils/system_clock.h"
#include "../inverter/INVERTERS.h"

// For modbus register definitions, see https://gitlab.com/pelle8/inverter_resources/-/blob/main/byd_registers_modbus_rtu.md

/* The emulator events reported to the inverter, and the fault type each one is reported as. The
   types are those of the BYD2 table the inverter holds for the BYD Battery-Box Premium HVS/HVM. Its
   event log shows the code with a short text, such as "BYD2-45 Battery Break", and Solar.web shows a
   cause and a solution for it. Each event goes to the code whose cause fits it, given after the line:
   the cause in Solar.web, or for MODBUS_INVERTER_MISSING the inverter's text. An event at error level
   goes in as a Fault2, any other as a Fault1. */
const BydModbusInverter::FaultMapping BydModbusInverter::FAULT_MAP[] = {
    {EVENT_CAN_BATTERY2_MISSING, 2, 0},          // The BCU cannot communicate with one of the battery modules
    {EVENT_CAN_BATTERY_MISSING, 1, 1},           // BCU communication error
    {EVENT_DISCHARGE_LIMIT_EXCEEDED, 3, 7},      // The discharge current is too high
    {EVENT_CELL_UNDER_VOLTAGE, 3, 13},           // Undervoltage of the cells, voltage below 2.0 V
    {EVENT_BATTERY_UNDERVOLTAGE, 3, 13},         // Undervoltage of the cells, voltage below 2.0 V
    {EVENT_CHARGE_LIMIT_EXCEEDED, 3, 18},        // The charging current is too high
    {EVENT_CELL_OVER_VOLTAGE, 3, 24},            // Cell-Overvoltage
    {EVENT_CELL_CRITICAL_OVER_VOLTAGE, 3, 24},   // Cell-Overvoltage
    {EVENT_BATTERY_OVERVOLTAGE, 3, 24},          // Cell-Overvoltage
    {EVENT_BATTERY_OVERHEAT, 3, 27},             // The maximum cell temperature is more than 70 degrees centigrade
    {EVENT_THERMAL_RUNAWAY, 3, 27},              // The maximum cell temperature is more than 70 degrees centigrade
    {EVENT_BATTERY_FROZEN, 3, 29},               // The minimum cell temperature is lower than -10 degrees centigrade
    {EVENT_BATTERY_COLD, 3, 29},                 // The minimum cell temperature is lower than -10 degrees centigrade
    {EVENT_CELL_DEVIATION_HIGH, 3, 32},          // Voltage differences between the cells
    {EVENT_CELL_CRITICAL_UNDER_VOLTAGE, 3, 38},  // The minimum cell voltage is lower than 1.5 V
    {EVENT_BATTERY_DISCHG_STOP_REQ, 3, 44},      // The battery can't be discharged
    {EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 3, 44},  // The battery can't be discharged
    {EVENT_CONTACTOR_OPEN, 3, 45},               // Battery main switch has tripped
    {EVENT_BYD_CONTACTOR_FORCE_OPEN, 3, 45},     // Battery main switch has tripped
    {EVENT_BATTERY_FUSE, 3, 45},                 // Battery main switch has tripped
    {EVENT_ERROR_OPEN_CONTACTOR, 1, 45},         // Battery main switch has tripped
    {EVENT_EQUIPMENT_STOP, 1, 45},               // Battery main switch has tripped
    {EVENT_MODBUS_INVERTER_MISSING, 1, 46},      // Communication error between inverter and battery
    {EVENT_RESET_UNKNOWN, 1, 46},                // A restart process has been initiated: unknown cause
    {EVENT_RESET_PANIC, 4, 46},                  // A restart process has been initiated: panic or a watchdog
    {EVENT_RESET_BROWNOUT, 1, 46},               // A restart process has been initiated: low supply voltage
    {EVENT_RESET_EFUSE, 3, 46},                  // A restart process has been initiated: efuse, glitch, lockup
    {EVENT_CONTACTOR_WELDED, 3, 47},             // Internal hardware failure at the battery control unit (BCU)
    {EVENT_WATER_INGRESS, 3, 47},                // Internal hardware failure at the battery control unit (BCU)
    {EVENT_PRECHARGE_FAILURE, 1, 48},            // Precharging takes too long
    {EVENT_AUTOMATIC_PRECHARGE_FAILURE, 1, 48},  // Precharging takes too long
    {EVENT_CONTACTOR_WATCHDOG_FAILED, 3, 48},    // Precharging takes too long
    {EVENT_INTERNAL_OPEN_FAULT, 3, 49},          // Faulty wiring of the battery supply line
    {EVENT_HVIL_FAILURE, 3, 49},                 // Faulty wiring of the battery supply line
    {EVENT_BATTERY_ISOLATION, 3, 49},            // Faulty wiring of the battery supply line
};
// The unexpected reset causes, taken above as runs of the enum: PANIC to WDT, EFUSE to CPU_LOCKUP
static_assert(EVENT_RESET_WDT == EVENT_RESET_PANIC + 3 && EVENT_RESET_CPU_LOCKUP == EVENT_RESET_EFUSE + 2,
              "FAULT_MAP takes the reset causes as runs of the enum");
const uint8_t BydModbusInverter::FAULT_MAP_SIZE = sizeof(FAULT_MAP) / sizeof(FAULT_MAP[0]);

void BydModbusInverter::update_values() {
  verify_temperature();
  handle_inverter_control_data();
  verify_inverter_modbus();
  handle_update_data_modbusp201_byd();
  handle_update_data_modbusp301_byd();
  update_fault_buffers();
}

void BydModbusInverter::handle_static_data() {
  // Store the data into the array
  static uint16_t si_data[] = {21321, 1};
  static uint16_t byd_data[] = {16985, 17408, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  static uint16_t battery_data[] = {16985, 17440, 16993, 29812, 25970, 31021, 17007, 30752,
                                    20594, 25965, 26997, 27936, 18518, 0,     0,     0};
  static uint16_t volt_data[] = {13614, 12288, 0, 0, 0, 0, 0, 0, 13102, 12854, 0, 0, 0, 0, 0, 0};
  static uint16_t serial_data[] = {20528, 13104, 21552, 12848, 23089, 14641, 12593, 14384,
                                   12336, 12337, 0,     0,     0,     0,     0,     0};
  static uint16_t static_data[] = {1, 0};
  static uint16_t* data_array_pointers[] = {si_data, byd_data, battery_data, volt_data, serial_data, static_data};
  static uint16_t data_sizes[] = {sizeof(si_data),   sizeof(byd_data),    sizeof(battery_data),
                                  sizeof(volt_data), sizeof(serial_data), sizeof(static_data)};
  static uint16_t i = 100;
  for (uint8_t arr_idx = 0; arr_idx < sizeof(data_array_pointers) / sizeof(uint16_t*); arr_idx++) {
    uint16_t data_size = data_sizes[arr_idx];
    for (int j = 0; j < data_size / sizeof(uint16_t); j++) {
      mbPV[i + j] = data_array_pointers[arr_idx][j];
    }
    i += data_size / sizeof(uint16_t);
  }
  static uint16_t init_p201[13] = {0, 0, 0, 40960, 40960, 0, 0, 53248, 10, 53248, 10, 0, 0};  //40960=MAX power
  for (int i = 0; i < sizeof(init_p201) / sizeof(uint16_t); i++) {
    mbPV[200 + i] = init_p201[i];
  }
  static uint16_t init_p301[24] = {0,  0,  128, 0, 0,  0,     0, 0, 0,  2000,  0,   2000,
                                   75, 95, 0,   0, 16, 22741, 0, 0, 13, 52064, 230, 9900};
  for (int i = 0; i < sizeof(init_p301) / sizeof(uint16_t); i++) {
    mbPV[300 + i] = init_p301[i];
  }
  // FaultBuffers, filled in by update_fault_buffers(). Created here, before the Modbus task starts:
  // a read of a register the map does not hold yet would insert it, from that task, while this one
  // writes to the same map.
  for (uint16_t reg = REG_FAULTS; reg < REG_FAULTS + FAULT_ENTRIES * FAULT_ENTRY_SIZE; reg++) {
    mbPV[reg] = 0;
  }
}

void BydModbusInverter::handle_update_data_modbusp201_byd() {
  mbPV[200] = 0;  //BatteryCapabilityFlags uint32
  mbPV[201] = 0;  //BatteryCapabilityFlags uint32
  mbPV[202] = std::min(datalayer.aggregate.reported_total_capacity_Wh,
                       static_cast<uint32_t>(57960u));  //NominalCapacity, cap to 58kWh
  mbPV[203] = 40960;                                    //ContMaxChargePwr uint16_t
  mbPV[204] = 40960;                                    //ContMaxDischargePwr uint16_t
  if (user_selected_primo_gen24) {
    mbPV[205] =  // Max Voltage, if higher Gen24 forces discharge, cap to 450.0V for Primo to avoid constant warning
        std::min(datalayer.aggregate.max_design_voltage_dV, static_cast<uint16_t>(4500u));
  } else {  //Symo inverter which can take up to 700V, so we can use the real max voltage of the battery without capping
    mbPV[205] = datalayer.aggregate.max_design_voltage_dV;
  }
  mbPV[206] = (datalayer.aggregate.min_design_voltage_dV);  // Min Voltage, if lower Gen24 disables battery
  mbPV[207] = 53248;                                        //PeakMaxChargePwr uint16_t
  mbPV[208] = 10;                                           //PeakMaxChargeT uint16_t
  mbPV[209] = 53248;                                        //PeakMaxDischargePwr uint16_t
  mbPV[210] = 10;                                           //PeakMaxDischargeT uint16_t
  mbPV[211] = 0;                                            //MaxSlope uint16_t
  mbPV[212] = 0;                                            //MinSlope uint16_t
}

/* Battery power in the sign convention of the emulated BYD, which reports charging as a negative
   value and discharging as positive - the opposite of datalayer active_power_W. Clamped to int16
   range before the cast, since a pack above 32.7kW would otherwise wrap and flip sign. */
int16_t BydModbusInverter::byd_power_W() {
  int32_t power_W = -datalayer.aggregate.active_power_W;
  if (power_W > 32767) {
    power_W = 32767;
  } else if (power_W < -32768) {
    power_W = -32768;
  }
  return static_cast<int16_t>(power_W);
}

void BydModbusInverter::handle_update_data_modbusp301_byd() {
  /* Status bits reported back to the inverter in mbPV[302] (register 303): bit 7 marks normal
     operation, bit 0 means charging, bit 1 means discharging - so 128/129/130 is idle/charging/
     discharging. The DISCHARGING(1) and CHARGING(2) defines in types.h are the inverse of that
     bit order, so build the value directly instead of adding the define to 128. */
  if (datalayer.aggregate.current_dA == 0) {
    bms_char_dis_status = BYD_MODE_IDLE;
  } else if (datalayer.aggregate.current_dA < 0) {  //Negative value = Discharging
    bms_char_dis_status = BYD_MODE_DISCHARGING;
  } else {  //Positive value = Charging
    bms_char_dis_status = BYD_MODE_CHARGING;
  }
  // Don't advertise ACTIVE to the inverter until the DC bus is actually live. During the boot-gate +
  // precharge window the emulator drives contactors on its own schedule (BYD-Modbus has no inverter
  // handshake), so a polling Fronius would otherwise see an ACTIVE battery against a dead DC input and
  // complain. STANDBY is the protocol-native "present but not delivering" resting state to report here.
  // Only the ACTIVE case is remapped; FAULT/UPDATING/STANDBY pass through unchanged.
  system_status_enum reported_status = datalayer.system.status.system_status;
  if (reported_status == ACTIVE && !datalayer.system.status.dc_bus_live) {
    reported_status = STANDBY;
  }

  if (reported_status == ACTIVE) {
    // DC and Power values after contactors (outter values).
    mbPV[308] = datalayer.aggregate.voltage_dV;  // DC outter voltage
    mbPV[309] = byd_power_W();                   // DC outter power, BYD reports charging as negative.
  } else {
    mbPV[308] = 0;
    mbPV[309] = 0;
  }
  mbPV[300] = reported_status;
  mbPV[302] = bms_char_dis_status;
  if (datalayer.aggregate.reported_soc < 100) {
    mbPV[303] = 100;  //Force SOC to never go below 1% to avoid overdischarge
  } else {
    mbPV[303] = datalayer.aggregate.reported_soc;
  }
  // Both capacity registers report the scaled (reported_) values, matching mbPV[202] in the p201 block.
  // update_calculated_values() already sums battery 2 and 3 into the reported_ fields of battery 1,
  // so no per-battery addition is needed here.
  // Fronius derives SOC from remaining/total, so scale remaining by the same cap to keep the ratio.
  const uint32_t total_Wh = datalayer.aggregate.reported_total_capacity_Wh;
  const uint32_t capped_total_Wh = std::min(total_Wh, static_cast<uint32_t>(57960u));  //Cap to 58kWh
  const uint32_t remaining_Wh = std::min(datalayer.aggregate.reported_remaining_capacity_Wh, total_Wh);
  mbPV[304] = capped_total_Wh;
  mbPV[305] = total_Wh ? (uint64_t)remaining_Wh * capped_total_Wh / total_Wh : 0;
  // The power limits already carry the remote or manual current limit, applied by
  // update_aggregate_limits() for every protocol, so it is not applied again here.
  mbPV[306] = std::min(datalayer.aggregate.max_discharge_power_W, static_cast<uint32_t>(30000u));  //Cap to 30000
  mbPV[307] = std::min(datalayer.aggregate.max_charge_power_W, static_cast<uint32_t>(30000u));     //Cap to 30000
  mbPV[310] = datalayer.aggregate.voltage_dV;  // DC inner voltage, UDC_internal uint16
  mbPV[311] = byd_power_W();  // DC inner power (before contactors), same inverted sign as mbPV[309], PDC_internal int16
  mbPV[312] = reported_temperature_min_dC;  //TCellMin
  mbPV[313] = reported_temperature_max_dC;  //TCellMax
  // U64 for total charged/discharged Wh (314-317 and 318-321), but datalayer uses only 32-bit.
  mbPV[316] = datalayer.aggregate.total_charged_battery_Wh >> 16;
  mbPV[317] = datalayer.aggregate.total_charged_battery_Wh & 0xFFFF;
  mbPV[320] = datalayer.aggregate.total_discharged_battery_Wh >> 16;
  mbPV[321] = datalayer.aggregate.total_discharged_battery_Wh & 0xFFFF;
  mbPV[322] = reported_temperature_max_dC;   //Ambient temperature, max temp used since we don't have ambient
  mbPV[323] = datalayer.aggregate.soh_pptt;  //SOH
}

void BydModbusInverter::put_u64(uint16_t reg, uint64_t value) {
  for (uint8_t i = 0; i < 4; i++) {
    mbPV[reg + i] = (value >> (48 - 16 * i)) & 0xFFFF;
  }
}

void BydModbusInverter::push_fault(uint16_t type, uint16_t severity, uint8_t battery, int16_t data, uint32_t now_ms) {
  const uint8_t slot = fault_count % FAULT_ENTRIES;
  const uint16_t entry = REG_FAULTS + slot * FAULT_ENTRY_SIZE;
  fault_count++;
  // The inverter takes an entry as valid when its timestamp is set, so that goes in last
  put_u64(entry, 0);
  mbPV[entry + 4] = fault_count;              // Number, counts up with every fault
  mbPV[entry + 5] = type;                     // Type
  mbPV[entry + 6] = severity;                 // Severity
  mbPV[entry + 7] = (uint16_t)data;           // Info_0, the data of the emulator event
  mbPV[entry + 8] = 0;                        // Info_1
  mbPV[entry + 9] = (battery << 8) | 0x00FF;  // Info_2, which pack. Low byte FF = the pack as a whole
  const uint64_t time_s = clock_utc_s();
  fault_entry_ms[slot] = now_ms;
  fault_entry_unstamped[slot] = (time_s == 0);
  put_u64(entry, time_s);  // Timestamp. 0 while the clock is not valid, see stamp_faults()
}

void BydModbusInverter::stamp_faults(uint32_t now_ms) {
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

void BydModbusInverter::update_fault_buffers() {
  const uint32_t now = millis();
  stamp_faults(now);

  for (uint8_t i = 0; i < FAULT_MAP_SIZE; i++) {
    for (uint8_t n = 0; n < FAULT_MAP[i].count; n++) {
      const EVENTS_ENUM_TYPE id = (EVENTS_ENUM_TYPE)(FAULT_MAP[i].event + n);
      const EVENTS_STRUCT_TYPE* event = get_event_pointer(id);
      const bool active = event->state == EVENT_STATE_ACTIVE || event->state == EVENT_STATE_ACTIVE_LATCHED;
      if (!active) {
        fault_reported.reset(id);
      } else if (!fault_reported.test(id)) {
        fault_reported.set(id);
        const uint8_t pack = event_battery_number(id);
        push_fault(FAULT_MAP[i].code, event->level == EVENT_LEVEL_ERROR ? SEVERITY_FAULT2 : SEVERITY_FAULT1,
                   pack > 0 ? pack : 1, event->data, now);
      }
    }
  }
}

void BydModbusInverter::verify_temperature() {
  reported_temperature_min_dC = datalayer.aggregate.temperature_min_dC;
  reported_temperature_max_dC = datalayer.aggregate.temperature_max_dC;
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    return;  // Skip the following section
  }
  // This section checks if the battery temperature is negative, and incase it falls between -9.0 and -20.0C degrees
  // The Fronius Gen24 (and other Fronius inverters also affected), will stop charge/discharge if the battery gets colder than -10°C.
  // This is due to the original battery pack (BYD HVM), is a lithium iron phosphate battery, that cannot be charged in cold weather.
  // When using EV packs with NCM/LMO/NCA chemsitry, this is not a problem, since these chemistries are OK for outdoor cold use.
  // The cap is applied to the registers this inverter writes, not to the datalayer. Clamping the
  // shared value in place would hand the web page, MQTT, ESP-NOW and every other inverter a
  // temperature the packs never reported.
  reported_temperature_min_dC = clamp_cold_temperature(datalayer.aggregate.temperature_min_dC);
  reported_temperature_max_dC = clamp_cold_temperature(datalayer.aggregate.temperature_max_dC);
}

int16_t BydModbusInverter::clamp_cold_temperature(int16_t temperature_dC) {
  if (temperature_dC < -90 && temperature_dC > -200) {  // Between -9.0 and -20.0C degrees
    return -90;                                         //Cap value to -9.0C
  }
  return temperature_dC;
}

void BydModbusInverter::verify_inverter_modbus() {
  // Once per watchdog period, the Gen24 writes to this 401 register, alternating between 00FF and FF00.
  // We sample the register at the same period. In case the value has not changed for HISTORY_LENGTH
  // periods, we raise an event. The period defaults to 60s and follows register 402 from the moment
  // the inverter declares a different one.
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis60s >= (inverter_modbus_watchdog_timeout_s * 1000UL)) {
    previousMillis60s = currentMillis;

    all_401_values_equal = true;
    for (int i = 0; i < HISTORY_LENGTH; ++i) {
      if (register_401_history[i] != mbPV[401]) {
        all_401_values_equal = false;
        break;
      }
    }

    if (all_401_values_equal) {
      set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
    } else {
      clear_event(EVENT_MODBUS_INVERTER_MISSING);
    }

    // Update history
    register_401_history[history_index] = mbPV[401];
    history_index = (history_index + 1) % HISTORY_LENGTH;
  }
}

void BydModbusInverter::handle_inverter_control_data() {
  // WatchDogTimeout (402). The inverter declares the period it kicks register 401 with. Follow it, so
  // inverter-missing detection tracks the inverter instead of a hardcoded assumption. Only flagged
  // when it actually changes, so a boot with the value we already hold never writes NVM. The write
  // itself belongs to the core loop, keeping this driver free of any storage dependency.
  const uint32_t declared_timeout_s = mbPV[402];
  if (declared_timeout_s >= WATCHDOG_TIMEOUT_MIN_S && declared_timeout_s <= WATCHDOG_TIMEOUT_MAX_S &&
      declared_timeout_s != inverter_modbus_watchdog_timeout_s) {
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Inverter changed the WatchDog Timeout from %u s to %u s\n", inverter_modbus_watchdog_timeout_s,
                   declared_timeout_s);
    inverter_modbus_watchdog_timeout_s = declared_timeout_s;
    inverter_modbus_watchdog_changed = true;
  }

  // UTC (403-406), a big-endian uint64 holding Unix epoch seconds. The inverter writes its clock here
  // once an hour. Each new time sets the ESP32's clock, which keeps it running in between.
  const uint64_t inverter_utc =
      ((uint64_t)mbPV[403] << 48) | ((uint64_t)mbPV[404] << 32) | ((uint64_t)mbPV[405] << 16) | (uint64_t)mbPV[406];
  if (inverter_utc > 0 && inverter_utc != last_inverter_utc_s) {
    last_inverter_utc_s = inverter_utc;
    const bool first = !clock_valid;
    if (clock_set_utc(inverter_utc) && first) {
      logging.println("Clock set from the inverter's time");
    }
  }

  // RebootCommand (407). The inverter writes 0 here as part of every ControlData block write, so only
  // a change to a non-zero value is a command.
  const uint16_t reboot_command = mbPV[407];
  if (reboot_command != last_register_407) {
    last_register_407 = reboot_command;
    if (reboot_command != 0) {
      if (user_selected_accept_inverter_reboot) {
        LOG_SET_NEXT_SEVERITY(5);  // notice
        logging.println("The emulator is restarting as requested by the inverter");
        hold_pins_across_reset();
        graceful_restart();  // Pauses charge/discharge and opens contactors before the restart
      } else {
        // Raised as an event rather than only logged: declining leaves the inverter believing it
        // asked for something that never happened, which the user should see on the status page
        // without having to have logging switched on at the time.
        set_event(EVENT_INVERTER_REBOOT_DECLINED, reboot_command);
      }
    }
  }

  // DarkstartEnable (408). Nothing acts on it yet, but the inverter announcing that it expects the
  // battery to be able to black start the system is worth having in the log. Only changes are
  // logged; a value of 0 here is a real setting rather than an absent command, so an inverter that
  // writes 0 from the start is indistinguishable from one that never writes the register at all.
  // BYD Battery-Box Premium HVS&HVM have a button, for black start it needs to be pressed for 3 seconds
  const uint16_t darkstart_enable = mbPV[408];
  if (darkstart_enable != last_register_408) {
    last_register_408 = darkstart_enable;
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Inverter set DarkstartEnable to %u\n", darkstart_enable);
  }
}

bool BydModbusInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  // Init Static data to the RTU Modbus
  handle_static_data();

  // Init Serial2 connected to the RTU Modbus
  RTUutils::prepareHardwareSerial(Serial2);

  if (!rs485_begin(Name, Serial2, 9600, SERIAL_8N1)) {
    return false;
  }

  // Start ModbusRTU background task
  MBserver.begin(Serial2, esp32hal->MODBUS_CORE());

  return true;
}
