#include "CMFA-EV-BATTERY.h"
#include <cstring>  //unit tests memcpy
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "BATTERIES.h"

/* The raw SOC value sits at 90% when the battery is full, so we should report back 100% once this value is reached
Same goes for low point, when 10% is reached we report 0% */

uint16_t CmfaEvBattery::rescale_raw_SOC(uint32_t raw_SOC) {

  uint32_t calc_soc = raw_SOC / 4;
  if (calc_soc > MAXSOC) {  //Constrain if needed
    calc_soc = MAXSOC;
  }
  if (calc_soc < MINSOC) {  //Constrain if needed
    calc_soc = MINSOC;
  }
  // Perform scaling between the two points
  calc_soc = 10000 * (calc_soc - MINSOC);
  calc_soc = calc_soc / (MAXSOC - MINSOC);

  return (uint16_t)calc_soc;
}

void CmfaEvBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer_battery->status.soh_pptt = (SOH * 100);

  datalayer_battery->status.real_soc = rescale_raw_SOC(SOC_raw);

  datalayer_battery->status.current_dA = (((int32_t)current_raw * 10) / 4) - 5000;

  datalayer_battery->status.voltage_dV = pack_voltage * 5;

  datalayer_battery->info.total_capacity_Wh = 27000;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  if (user_selected_use_estimated_charge_limits) {  //Some packs are locked? and do not report allowed charge/discharge power
    datalayer_battery->status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;

    datalayer_battery->status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;
  } else {  //Use sane limits sent by battery
    datalayer_battery->status.max_charge_power_W = charge_power_w;

    datalayer_battery->status.max_discharge_power_W = discharge_power_w;
  }

  datalayer_battery->status.temperature_min_dC = (lowest_cell_temperature * 10);

  datalayer_battery->status.temperature_max_dC = (highest_cell_temperature * 10);

  datalayer_battery->status.cell_min_voltage_mV = lowest_cell_voltage_mv;

  datalayer_battery->status.cell_max_voltage_mV = highest_cell_voltage_mv;

  if (lead_acid_voltage < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, lead_acid_voltage, battery_index);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

// Balancing stats are stored in 1/1024 Ah or 1/1024 h units.
// Treat the INT32_MIN sentinel as zero.
static int32_t bal_mAh(int32_t v) {
  return (v == INT32_MIN) ? 0 : (v * 125) / 128;
}
static int32_t bal_s(int32_t v) {
  return (v == INT32_MIN) ? 0 : (v * 225) / 64;
}

String CmfaEvBattery::get_uds_info_html() {
  String content;
  content.reserve(900);

  // clang-format off
  content << "<h4>SOC U: " << soc_u << "percent</h4>"
             "<h4>SOC Z: " << soc_z << "percent</h4>"
             "<h4>SOH Average: " << soh_average << "pptt</h4>"
             "<h4>Instant current: " << (((int32_t)instant_current_raw - 48000) * 25) << "mA</h4>"
             "<h4>Average current: " << ((((int32_t)instant_current_raw - 32640) * 125) / 4) << "mA</h4>"
             "<h4>12V voltage: " << lead_acid_voltage << "mV</h4>"
             "<h4>Highest cell number: " << highest_cell_voltage_number << "</h4>"
             "<h4>Lowest cell number: " << lowest_cell_voltage_number << "</h4>"
             "<h4>Sum of cellvoltages: " << average_voltage_of_cells << "</h4>"
             "<h4>Max regen power: " << max_regen_power << "</h4>"
             "<h4>Max discharge power: " << max_discharge_power << "</h4>"
             "<h4>Max charge power: " << maximum_charge_power << "</h4>"
             "<h4>SOH available power: " << SOH_available_power << "</h4>"
             "<h4>SOH generated power: " << SOH_generated_power << "</h4>"
             "<h4>Average temperature: " << average_temperature << "dC</h4>"
             "<h4>Maximum temperature: " << maximum_temperature << "dC</h4>"
             "<h4>Minimum temperature: " << minimum_temperature << "dC</h4>"
             "<h4>Cumulative energy discharged: " << cumulative_energy_when_discharging << "Wh</h4>"
             "<h4>Cumulative energy charged: " << cumulative_energy_when_charging << "Wh</h4>"
             "<h4>Cumulative energy regen: " << cumulative_energy_in_regen << "Wh</h4>"
             "<h4>Balance capacity total: " << bal_mAh(balance_capacity_total) << "mAh (was " << bal_mAh(initial_balance_capacity_total) << "mAh)</h4>"
             "<h4>Balance time total: " << bal_s(balance_time_total) << "s (was " << bal_s(initial_balance_time_total) << "s)</h4>"
             "<h4>Balance capacity sleep: " << bal_mAh(balance_capacity_sleep) << "mAh (was " << bal_mAh(initial_balance_capacity_sleep) << "mAh)</h4>"
             "<h4>Balance time sleep: " << bal_s(balance_time_sleep) << "s (was " << bal_s(initial_balance_time_sleep) << "s)</h4>"
             "<h4>Balance capacity wake: " << bal_mAh(balance_capacity_wake) << "mAh (was " << bal_mAh(initial_balance_capacity_wake) << "mAh)</h4>"
             "<h4>Balance time wake: " << bal_s(balance_time_wake) << "s (was " << bal_s(initial_balance_time_wake) << "s)</h4>"
             "<h4>Temporisation: " << (
                temporisation==0xFF ? "UNKNOWN" : 
                temporisation==1 ? "ACTIVE" : "INACTIVE"
             ) << "</h4>"
             "<h4>Pack time life: " << pack_time_life << "m (was " << int32_t(initial_pack_time_life) << "m)</h4>"
             "<h4>Absolute time saved: " << absolute_time_saved << "m (was " << int32_t(initial_absolute_time_saved) << "m)</h4>";
  // clang-format on

  return content;
}

void CmfaEvBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x7BB PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {  //These frames are transmitted by the battery
    case 0x127:           //10ms , Same structure as old Zoe 0x155 message!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      current_raw = ((rx_frame.data.u8[1] & 0x0F) << 8) | rx_frame.data.u8[2];
      SOC_raw = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      pack_voltage = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x3D6:  //100ms, Same structure as old Zoe 0x424 message!
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      charge_power_w = rx_frame.data.u8[2] * 500;
      discharge_power_w = rx_frame.data.u8[3] * 500;
      lowest_cell_temperature = (rx_frame.data.u8[4] - 40);
      SOH = rx_frame.data.u8[5];
      heartbeat = rx_frame.data.u8[6];
      highest_cell_temperature = (rx_frame.data.u8[7] - 40);
      break;
    case 0x3D7:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3D8:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //counter_3D8 = rx_frame.data.u8[3]; //?
      //CRC_3D8 = rx_frame.data.u8[4]; //?
      break;
    case 0x43C:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      heartbeat2 = rx_frame.data.u8[2];  //Alternates between 0x55 and 0xAA every 5th frame
      break;
    case 0x431:  //100ms
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //byte0 9C always
      //byte1 40 always
      break;
    case 0x5A9:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5AB:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5C8:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5E1:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

// The balancing data has a offset that needs stripping
static int32_t decode_balance_word(uint32_t raw) {
  return (int32_t)(raw - 0x80000000u);
}

uint16_t CmfaEvBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case PID_POLL_SOCZ:
      soc_z = (uint16_t)value;
      break;
    case PID_POLL_USOC:
      soc_u = (uint16_t)value;
      break;
    case PID_POLL_SOH_AVERAGE:
      soh_average = (uint16_t)value;
      break;
    case PID_POLL_AVERAGE_VOLTAGE_OF_CELLS:
      average_voltage_of_cells = value;
      break;
    case PID_POLL_HIGHEST_CELL_VOLTAGE:
      highest_cell_voltage_mv = (uint16_t)(value * 0.976563f);
      break;
    case PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE:
      highest_cell_voltage_number = data[0];
      break;
    case PID_POLL_LOWEST_CELL_VOLTAGE:
      lowest_cell_voltage_mv = (uint16_t)(value * 0.976563f);
      break;
    case PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE:
      lowest_cell_voltage_number = data[0];
      break;
    case PID_POLL_CURRENT_OFFSET:
      // Not used
      break;
    case PID_POLL_INSTANT_CURRENT:
      instant_current_raw = value;
      break;
    case PID_POLL_100MS_CURRENT:
      averaged_current_raw = value;
      break;
    case PID_POLL_MAX_REGEN:
      max_regen_power = (uint16_t)value;
      break;
    case PID_POLL_MAX_DISCHARGE_POWER:
      max_discharge_power = (uint16_t)value;
      break;
    case PID_POLL_12V_BATTERY:
      lead_acid_voltage = (uint16_t)value;
      break;
    case PID_POLL_AVERAGE_TEMPERATURE:
      average_temperature = (int16_t)(((int32_t)value - 400) / 2);
      break;
    case PID_POLL_MIN_TEMPERATURE:
      minimum_temperature = (int16_t)(((int32_t)value - 400) / 2);
      break;
    case PID_POLL_MAX_TEMPERATURE:
      maximum_temperature = (int16_t)(((int32_t)value - 400) / 2);
      break;
    case PID_POLL_MAX_CHARGE_POWER:
      maximum_charge_power = (uint16_t)value;
      break;
    case PID_POLL_END_OF_CHARGE_FLAG:
      end_of_charge = data[0];
      break;
    case PID_POLL_INTERLOCK_FLAG:
      interlock_flag = data[0];
      break;
    case PID_POLL_SOH_AVAILABLE_POWER_CALCULATION:
      SOH_available_power = (uint16_t)value;
      break;
    case PID_POLL_SOH_GENERATED_POWER_CALCULATION:
      SOH_generated_power = (uint16_t)value;
      break;
    case PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING:
      cumulative_energy_when_discharging = value;
      break;
    case PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING:
      cumulative_energy_when_charging = value;
      break;
    case PID_POLL_CUMULATIVE_ENERGY_IN_REGEN:
      cumulative_energy_in_regen = value;
      break;
    case PID_POLL_BALANCE_CAPACITY_TOTAL:
      balance_capacity_total = decode_balance_word(value);
      if (initial_balance_capacity_total == INT32_MIN)
        initial_balance_capacity_total = balance_capacity_total;
      break;
    case PID_POLL_BALANCE_TIME_TOTAL:
      balance_time_total = decode_balance_word(value);
      if (initial_balance_time_total == INT32_MIN)
        initial_balance_time_total = balance_time_total;
      break;
    case PID_POLL_BALANCE_CAPACITY_SLEEP:
      balance_capacity_sleep = decode_balance_word(value);
      if (initial_balance_capacity_sleep == INT32_MIN)
        initial_balance_capacity_sleep = balance_capacity_sleep;
      break;
    case PID_POLL_BALANCE_TIME_SLEEP:
      balance_time_sleep = decode_balance_word(value);
      if (initial_balance_time_sleep == INT32_MIN)
        initial_balance_time_sleep = balance_time_sleep;
      break;
    case PID_POLL_BALANCE_CAPACITY_WAKE:
      balance_capacity_wake = decode_balance_word(value);
      if (initial_balance_capacity_wake == INT32_MIN)
        initial_balance_capacity_wake = balance_capacity_wake;
      break;
    case PID_POLL_BALANCE_TIME_WAKE:
      balance_time_wake = decode_balance_word(value);
      if (initial_balance_time_wake == INT32_MIN)
        initial_balance_time_wake = balance_time_wake;
      break;
    case PID_POLL_BMS_STATE:
      bms_state = (uint8_t)value;
      break;
    case PID_POLL_TEMPORISATION:
      temporisation = value;
      break;
    case PID_POLL_PACK_TIME_LIFE:
      pack_time_life = value;
      if (initial_pack_time_life == UINT32_MAX)
        initial_pack_time_life = pack_time_life;
      break;
    case PID_POLL_ABSOLUTE_TIME_SAVED:
      absolute_time_saved = value;
      if (initial_absolute_time_saved == UINT32_MAX)
        initial_absolute_time_saved = absolute_time_saved;
      break;
    case PID_POLL_BALANCE_SWITCHES: {
      // Assumed to use the same encoding as RENAULT-ZOE-2-BATTERY, with the
      // bitmap at the end of the payload, cell 1 in bit 0 of the last byte.
      // May not be correct!
      uint8_t cells = datalayer_battery->info.number_of_cells;
      if (length < (cells + 7) / 8) {
        break;
      }
      for (uint8_t i = 0; i < cells; i++) {
        datalayer_battery->status.cell_balancing_status[i] = (data[length - 1 - (i >> 3)] >> (i & 7)) & 0x01;
      }
      break;
    }
    default:  //Unknown pid, or a cellvoltage
      uint8_t cellnumber = 0;
      if (pid >= PID_POLL_CELL_1 && pid <= PID_POLL_CELL_31) {  //Cellvoltage PID reply
        cellnumber = (pid - PID_POLL_CELL_1);
      } else if (pid >= PID_POLL_CELL_32 && pid <= PID_POLL_CELL_62) {
        cellnumber = (pid - PID_POLL_CELL_1) - 1;
      } else if (pid >= PID_POLL_CELL_63 && pid <= PID_POLL_CELL_72) {
        cellnumber = (pid - PID_POLL_CELL_1) - 2;
      } else {  //Unknown pid
        break;
      }

      if (cellnumber < MAX_AMOUNT_CELLS) {
        uint16_t cellvoltage_reading = (uint16_t)value;
        if (cellvoltage_reading == 0) {
          //Blown fuse/celltap. Force value to 10mV so user sees this in cellmonitor page. Also fire event
          cellvoltage_reading = 10;
          set_event(EVENT_BATTERY_FUSE, cellnumber, battery_index);
        }
        datalayer_battery->status.cell_voltages_mV[cellnumber] = (uint16_t)(cellvoltage_reading * 0.976563f);
      }

      break;
  }

  // The instantaneous current changes quickly, so sample it every other PID by
  // requesting it out-of-sequence next; the scan list then resumes where it
  // left off when the instant current response comes back (which returns 0).
  if (pid != PID_POLL_INSTANT_CURRENT) {
    return PID_POLL_INSTANT_CURRENT;
  }
  return 0;  //Continue scanning the PID list in order
}

void CmfaEvBattery::enable_temporisation() {
  start_sequence(CMFA_STATE_TEMPORISATION_START);
}

void CmfaEvBattery::on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) {
  // Called by the superclass when a response in a UDS sequence is received.
  // Timeouts are ignored for now: each step only advances on a response.
  switch (state) {
      // Temporisation sequence

    case CMFA_STATE_TEMPORISATION_START:
      // Enter an extended diagnostic session first.
      send_sequence_message(CMFA_STATE_TEMPORISATION_DIAG, SID::DiagnosticSessionControl, (const uint8_t*)"\x03", 1,
                            CMFA_UDS_TIMEOUT_SESSION_CONTROL, 2);
      break;
    case CMFA_STATE_TEMPORISATION_DIAG:
      // Extended diagnostic session entered, write the temporisation value.
      send_sequence_message(CMFA_STATE_TEMPORISATION_SEND, SID::WriteDataByIdentifier, (const uint8_t*)"\x92\x81\x01",
                            3, CMFA_UDS_TIMEOUT_WRITE, 2);
      break;
    case CMFA_STATE_TEMPORISATION_SEND:
      logging.println("[CMFA] Temporisation enabled (DID 0x9281 set to 0x01)");
      break;

      // NVROL reset sequence

    case CMFA_STATE_NVROL_START:
      // NVROL reset, part 1: enter an extended diagnostic session (0x10 0x03).
      send_sequence_message(CMFA_STATE_NVROL_SESSION, SID::DiagnosticSessionControl, (const uint8_t*)"\x03", 1,
                            CMFA_NVROL_TIMEOUT_TICKS, 1);
      break;
    case CMFA_STATE_NVROL_SESSION:
      // NVROL reset, part 2: run the NVROL reset routine (0x31 01 B0 09).
      send_sequence_message(CMFA_STATE_NVROL_ROUTINE, SID::RoutineControl, (const uint8_t*)"\x01\xB0\x09", 3,
                            CMFA_NVROL_TIMEOUT_TICKS, 2);
      break;
    case CMFA_STATE_NVROL_ROUTINE:
      // Enable temporisation before sleep, part 1: extended session again.
      send_sequence_message(CMFA_STATE_NVROL_SLEEP_SESSION, SID::DiagnosticSessionControl, (const uint8_t*)"\x03", 1,
                            CMFA_NVROL_TIMEOUT_TICKS, 1);
      break;
    case CMFA_STATE_NVROL_SLEEP_SESSION:
      // Enable temporisation before sleep, part 2: write DID 0x9281 = 0x01.
      send_sequence_message(CMFA_STATE_NVROL_SLEEP_WRITE, SID::WriteDataByIdentifier, (const uint8_t*)"\x92\x81\x01", 3,
                            CMFA_NVROL_TIMEOUT_TICKS, 1);
      break;
    case CMFA_STATE_NVROL_SLEEP_WRITE:
      // The temporisation write is done: stop streaming the awake byte on 0x1EA
      // so the pack saves its state and falls asleep, then hold the bus for 30 s.
      CMFA_1EA.data.u8[0] = CMFA_1EA_SLEEP;
      sequence_wait(CMFA_STATE_NVROL_SLEEP_WAIT, CMFA_SLEEP_TICKS);
      break;
    case CMFA_STATE_NVROL_SLEEP_WAIT:
      // Wake the pack back up. The sequence ends here (no further steps sent).
      CMFA_1EA.data.u8[0] = CMFA_1EA_AWAKE;
      logging.println("[CMFA] NVROL reset complete");
      break;

      // Plain sleep sequence

    case CMFA_STATE_SLEEP_START:
      // Put the pack to sleep (no diagnostic session or reset) and hold the
      // bus for 30 s while it saves its state and falls asleep.
      CMFA_1EA.data.u8[0] = CMFA_1EA_SLEEP;
      sequence_wait(CMFA_STATE_SLEEP_WAIT, CMFA_SLEEP_TICKS);
      break;
    case CMFA_STATE_SLEEP_WAIT:
      // Wake the pack back up. The sequence ends here (no further steps sent).
      CMFA_1EA.data.u8[0] = CMFA_1EA_AWAKE;
      logging.println("[CMFA] Sleep cycle complete");
      break;

    default:
      break;
  }
}

void CmfaEvBattery::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;
    transmit_can_frame(&CMFA_1EA);
    transmit_can_frame(&CMFA_135);
    transmit_can_frame(&CMFA_134);
    transmit_can_frame(&CMFA_125);

    CMFA_135.data.u8[1] = content_135[counter_10ms];
    CMFA_125.data.u8[3] = content_125[counter_10ms];
    counter_10ms = (counter_10ms + 1) % 16;  // counter_10ms cycles between 0-1-2-3..15-0-1...
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    transmit_can_frame(&CMFA_59B);
    transmit_can_frame(&CMFA_3D3);
  }

  // Automatically enable temporisation if the BMS reports it as disabled.
  // Don't queue it while a UDS sequence (e.g. an NVROL reset) is in progress.
  if (!uds_is_busy() && temporisation != 0xFF &&
      currentMillis - previousMillisTemporisation >= TEMPORISATION_RETRY_MS) {
    if (temporisation == 0) {
      previousMillisTemporisation = currentMillis;
      enable_temporisation();
    }
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);
}

void CmfaEvBattery::setup(void) {  // Performs one time setup at startup
  // UDS: send requests to 0x79B, accept replies from the BMS on 0x7BB.
  setup_uds(0x79B, 0x7BB);

  static const uint16_t pid_scan_list[] = {
      PID_POLL_SOH_AVERAGE,
      PID_POLL_AVERAGE_VOLTAGE_OF_CELLS,
      PID_POLL_HIGHEST_CELL_VOLTAGE,
      PID_POLL_LOWEST_CELL_VOLTAGE,
      PID_POLL_CELL_NUMBER_HIGHEST_VOLTAGE,
      PID_POLL_CELL_NUMBER_LOWEST_VOLTAGE,
      PID_POLL_12V_BATTERY,
      PID_POLL_CUMULATIVE_ENERGY_WHEN_CHARGING,
      PID_POLL_CUMULATIVE_ENERGY_WHEN_DISCHARGING,
      PID_POLL_CUMULATIVE_ENERGY_IN_REGEN,
      PID_POLL_SOCZ,
      PID_POLL_USOC,
      //PID_POLL_CURRENT_OFFSET,
      PID_POLL_100MS_CURRENT,
      PID_POLL_MAX_REGEN,
      PID_POLL_MAX_DISCHARGE_POWER,
      PID_POLL_MAX_CHARGE_POWER,
      PID_POLL_AVERAGE_TEMPERATURE,
      PID_POLL_MIN_TEMPERATURE,
      PID_POLL_MAX_TEMPERATURE,
      PID_POLL_END_OF_CHARGE_FLAG,
      PID_POLL_INTERLOCK_FLAG,
      PID_POLL_BALANCE_CAPACITY_TOTAL,
      PID_POLL_BALANCE_TIME_TOTAL,
      PID_POLL_BALANCE_CAPACITY_SLEEP,
      PID_POLL_BALANCE_TIME_SLEEP,
      PID_POLL_BALANCE_CAPACITY_WAKE,
      PID_POLL_BALANCE_TIME_WAKE,
      PID_POLL_BMS_STATE,
      PID_POLL_TEMPORISATION,
      PID_POLL_PACK_TIME_LIFE,
      PID_POLL_ABSOLUTE_TIME_SAVED,
      PID_POLL_BALANCE_SWITCHES,
      PID_POLL_CELL_1,
      PID_POLL_CELL_2,
      PID_POLL_CELL_3,
      PID_POLL_CELL_4,
      PID_POLL_CELL_5,
      PID_POLL_CELL_6,
      PID_POLL_CELL_7,
      PID_POLL_CELL_8,
      PID_POLL_CELL_9,
      PID_POLL_CELL_10,
      PID_POLL_CELL_11,
      PID_POLL_CELL_12,
      PID_POLL_CELL_13,
      PID_POLL_CELL_14,
      PID_POLL_CELL_15,
      PID_POLL_CELL_16,
      PID_POLL_CELL_17,
      PID_POLL_CELL_18,
      PID_POLL_CELL_19,
      PID_POLL_CELL_20,
      PID_POLL_CELL_21,
      PID_POLL_CELL_22,
      PID_POLL_CELL_23,
      PID_POLL_CELL_24,
      PID_POLL_CELL_25,
      PID_POLL_CELL_26,
      PID_POLL_CELL_27,
      PID_POLL_CELL_28,
      PID_POLL_CELL_29,
      PID_POLL_CELL_30,
      PID_POLL_CELL_31,
      PID_POLL_CELL_32,
      PID_POLL_CELL_33,
      PID_POLL_CELL_34,
      PID_POLL_CELL_35,
      PID_POLL_CELL_36,
      PID_POLL_CELL_37,
      PID_POLL_CELL_38,
      PID_POLL_CELL_39,
      PID_POLL_CELL_40,
      PID_POLL_CELL_41,
      PID_POLL_CELL_42,
      PID_POLL_CELL_43,
      PID_POLL_CELL_44,
      PID_POLL_CELL_45,
      PID_POLL_CELL_46,
      PID_POLL_CELL_47,
      PID_POLL_CELL_48,
      PID_POLL_CELL_49,
      PID_POLL_CELL_50,
      PID_POLL_CELL_51,
      PID_POLL_CELL_52,
      PID_POLL_CELL_53,
      PID_POLL_CELL_54,
      PID_POLL_CELL_55,
      PID_POLL_CELL_56,
      PID_POLL_CELL_57,
      PID_POLL_CELL_58,
      PID_POLL_CELL_59,
      PID_POLL_CELL_60,
      PID_POLL_CELL_61,
      PID_POLL_CELL_62,
      PID_POLL_CELL_63,
      PID_POLL_CELL_64,
      PID_POLL_CELL_65,
      PID_POLL_CELL_66,
      PID_POLL_CELL_67,
      PID_POLL_CELL_68,
      PID_POLL_CELL_69,
      PID_POLL_CELL_70,
      PID_POLL_CELL_71,
      PID_POLL_CELL_72,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 72;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
