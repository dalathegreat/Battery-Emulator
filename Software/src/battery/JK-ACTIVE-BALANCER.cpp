#include "JK-ACTIVE-BALANCER.h"
#include <cmath>
#include <cstdlib>
#include "../communication/can/comm_can.h"
#include "../communication/nvm/comm_nvm.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "BATTERIES.h"

static_assert(MAX_BALANCERS_WEB >= JKActiveBalancer::MAX_BALANCERS,
              "MAX_BALANCERS_WEB (datalayer_extended.h) must be >= MAX_BALANCERS, web data arrays would overflow");

void JKActiveBalancer::setup(void) {
  // Only the primary battery sets the system's battery protocol name.
  if (battery_index == 1) {
    strncpy(datalayer.system.info.battery_protocol, Name, 63);
    datalayer.system.info.battery_protocol[63] = '\0';
  }

  // --- Pack layout from the settings ---
  JK_FW_VERSION = user_selected_jk_fw_version;
  JK_FW_MASK = user_selected_jk_fw_mask;
  JK_BRIDGE_MODE = user_selected_jk_bridge;
  JK_BALANCER_LOW_VOLTAGE = user_selected_jk_low_voltage;
  JK_NUMBER_OF_CELLS = user_selected_jk_cells;
  JK_CELLS_BALANCER = user_selected_jk_cells_per_balancer;

  // Validate the settings. On error the battery is not run (see update_values / transmit_can) and the page shows why.
  config_error = JK_CFG_OK;
  if (JK_CELLS_BALANCER < 1 || JK_CELLS_BALANCER > MAX_CELLS_PER_BALANCER || JK_NUMBER_OF_CELLS < 1 ||
      JK_NUMBER_OF_CELLS > MAX_AMOUNT_CELLS) {
    config_error = JK_CFG_BAD_CELL_COUNT;
  } else if (JK_BRIDGE_MODE && JK_BALANCER_LOW_VOLTAGE) {
    config_error = JK_CFG_BRIDGE_AND_LOW_VOLTAGE;
  } else if (JK_BALANCER_LOW_VOLTAGE && (JK_NUMBER_OF_CELLS % JK_CELLS_BALANCER != 0)) {
    config_error = JK_CFG_CELLS_NOT_MULTIPLE;
  } else if (JK_FW_VERSION < 1 || JK_FW_VERSION > 3) {
    config_error = JK_CFG_BAD_FW_VERSION;
  } else if (!(user_selected_jk_cell_min_mV < user_selected_jk_vempty_on_mV &&
               user_selected_jk_vempty_on_mV < user_selected_jk_vempty_off_mV &&
               user_selected_jk_vempty_off_mV < user_selected_jk_vfull_off_mV &&
               user_selected_jk_vfull_off_mV < user_selected_jk_vfull_on_mV &&
               user_selected_jk_vfull_on_mV < user_selected_jk_cell_max_mV) ||
             user_selected_jk_pack_min_mVpc < 2000 ||
             user_selected_jk_pack_min_mVpc >= user_selected_jk_pack_max_mVpc) {
    config_error = JK_CFG_BAD_VOLTAGE_ORDER;
  } else if (user_selected_jk_soc_min_dpct >= user_selected_jk_soc_max_dpct) {
    config_error = JK_CFG_BAD_SOC_WINDOW;
  }
  if (config_error != JK_CFG_OK) {  // keep the arithmetic below safe
    JK_CELLS_BALANCER = MAX_CELLS_PER_BALANCER;
    JK_NUMBER_OF_CELLS = MAX_CELLS_PER_BALANCER;
  }
  JK_BRIDGE_CELLS = JK_CELLS_BALANCER - 1;
  // Electrical series count: one pack in low-voltage mode (parallel packs share the same voltage), all cells otherwise
  JK_SERIES_CELLS = JK_BALANCER_LOW_VOLTAGE ? JK_CELLS_BALANCER : JK_NUMBER_OF_CELLS;

  // Calculate how many balancers are needed based on the total cell count.
  if (JK_BRIDGE_MODE) {
    if (JK_NUMBER_OF_CELLS <= JK_CELLS_BALANCER) {
      number_of_balancers = 1;  // Single balancer: no bridge, full cell count usable
    } else {
      number_of_balancers =
          1 + static_cast<int>(ceilf((JK_NUMBER_OF_CELLS - (float)JK_BRIDGE_CELLS) / (float)JK_BRIDGE_CELLS));
    }
  } else {
    number_of_balancers = static_cast<int>(ceilf(JK_NUMBER_OF_CELLS / (float)JK_CELLS_BALANCER));
  }

  if (number_of_balancers > MAX_BALANCERS) {
    if (config_error == JK_CFG_OK) {
      config_error = JK_CFG_TOO_MANY_BALANCERS;
    }
    number_of_balancers = MAX_BALANCERS;
  }
  // Initialize the data structures for each balancer to zero.
  for (int i = 0; i < number_of_balancers; i++) {
    for (int j = 0; j < JK_CELLS_BALANCER; j++) {
      balancers[i].cell_voltages_mV[j] = 0;
    }
    balancers[i].temperature_dC = 0;
    balancers[i].total_voltage_10mV = 0;
    balancers[i].max_balance_current_mA = 0;
    balancers[i].balance_current_mA = 0;
    balancers[i].max_delta_mV = 0;
    balancers[i].identified_cells = 0;
    balancers[i].set_cells = 0;
    balancers[i].balance_switch_on = false;
    balancers[i].is_charge_balancing = false;
    balancers[i].is_discharge_balancing = false;
    balancers[i].alarm_cell_count_mismatch = false;
    balancers[i].alarm_wire_resistance = false;
    balancers[i].raw_status_byte = 0;
    // Alarm bit polarity per balancer from the firmware version setting (idx 0 = balancer #1 = bit 15)
    if (JK_FW_VERSION == 1) {
      balancers[i].fw_inverted = true;
    } else if (JK_FW_VERSION == 2) {
      balancers[i].fw_inverted = false;
    } else {
      balancers[i].fw_inverted = (JK_FW_MASK >> (15 - i)) & 0x01;
    }
  }
  datalayer_jk->config_error = config_error;

  ct_cab500_boot_millis = millis();  // Start of the 30 s error-free boot grace period
  // Balancers powered together with the emulator put errors on the bus while they boot
  ignore_can_errors_for(can_interface, BOOT_POLL_WINDOW_MS);

  // --- Limits, ramps and SOC settings; they stay fixed until the next reboot ---
  max_charge_W_active = user_selected_jk_max_charge_W;
  max_discharge_W_active = user_selected_jk_max_discharge_W;
  dis_ramp_pct_active = user_selected_jk_dis_ramp_dpct / 10.0f;
  dis_ramp_bottom_pct_active = user_selected_jk_dis_ramp_bottom_dpct / 10.0f;
  dis_min_power_W_active = user_selected_jk_dis_min_W;
  dis_cutoff_hyst_dV_active = user_selected_jk_dis_cutoff_hyst_dV;
  chg_ramp_pct_active = user_selected_jk_chg_ramp_dpct / 10.0f;
  chg_min_power_W_active = user_selected_jk_chg_min_W;
  MAX_PACK_VOLTAGE_DV = JK_SERIES_CELLS * (user_selected_jk_pack_max_mVpc / 100.0f);
  MIN_PACK_VOLTAGE_DV = JK_SERIES_CELLS * (user_selected_jk_pack_min_mVpc / 100.0f);
  MAX_CELL_DEVIATION_MV = user_selected_jk_cell_dev_mV;
  MAX_CELL_VOLTAGE_MV = user_selected_jk_cell_max_mV;
  MIN_CELL_VOLTAGE_MV = user_selected_jk_cell_min_mV;
  reverse_current_active = user_selected_jk_reverse_current;
  cab500_can_id = (battery_index == 2) ? user_selected_jk2_cab500_can_id : user_selected_jk_cab500_can_id;
  datalayer_jk->ct_cab500_can_id = cab500_can_id;
  max_balancers_active = user_selected_jk_max_balancers;
  if (max_balancers_active < 1 || max_balancers_active > MAX_BALANCERS) {
    max_balancers_active = 8;
  }
  if (number_of_balancers > max_balancers_active) {
    number_of_balancers = max_balancers_active;
  }

  // Configure the SOC library
  Battery_Ah = rated_Ah();
  soc_cfg.capacity_Ah = Battery_Ah;
  soc_cfg.charge_efficiency = 0.99f;
  soc_cfg.current_deadband_A = user_selected_jk_deadband_mA / 1000.0f;
  // Library convention is +discharge/-charge; the driver feeds +charge/-discharge. The "reverse current
  // sensor direction" setting flips BMS_current_A itself, so this stays fixed.
  soc_cfg.invert_current_sign = true;
  soc_cfg.LFP_Chemestry_Type = user_selected_jk_lfp;  // Selects the SOC table
  soc_cfg.vfull_on_mV = user_selected_jk_vfull_on_mV;
  soc_cfg.vfull_off_mV = user_selected_jk_vfull_off_mV;
  soc_cfg.vempty_on_mV = user_selected_jk_vempty_on_mV;
  soc_cfg.vempty_off_mV = user_selected_jk_vempty_off_mV;
  soc_cfg.soc_max_percent = user_selected_jk_soc_max_dpct / 10.0f;
  soc_cfg.soc_min_percent = user_selected_jk_soc_min_dpct / 10.0f;
  soc_cfg.sample_period_ms = SOC_UPDATE_MS;
  soc_cfg.recal_rest_ms = user_selected_jk_recal_rest_ms;
  soc_cfg.recal_rest_current_A = user_selected_jk_recal_rest_mA / 1000.0f;
  soc_cfg.recal_soc_low_pct = user_selected_jk_recal_soc_bottom_dpct / 10.0f;
  soc_cfg.recal_soc_high_pct = user_selected_jk_recal_soc_top_dpct / 10.0f;
  soc_cfg.init_min_cell_mV = 1000;  // Require a confirmed voltage before LUT init
  soc = CoulombSOC(soc_cfg);

  // Cycle counter persisted in NVS (8.13: comm_nvm key JK_BMS_Cycles)
  {
    BatteryEmulatorSettingsStore settings(true);
    datalayer_jk->BMS_Cycles = settings.getUInt(cycles_key(), 0);
  }

  // Set up the main datalayer with battery pack information.
  datalayer_battery->info.number_of_cells = JK_NUMBER_OF_CELLS;
  datalayer_battery->info.max_design_voltage_dV = (uint16_t)MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = (uint16_t)MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  // The JK chemistry setting wins over the generic chemistry selector
  datalayer_battery->info.chemistry = user_selected_jk_lfp ? battery_chemistry_enum::LFP : battery_chemistry_enum::NMC;
  // Placeholder until the pack has been read once and the coulomb counter starts: the SOC is
  // unknown for the first seconds, and 0 % would raise a "Battery empty" event at every boot.
  // Nothing is allowed in that window anyway (0 W, contactors not permitted).
  datalayer_battery->status.real_soc = 5000;
}

void JKActiveBalancer::handle_incoming_can_frame(CAN_frame rx_frame) {
  // --- CAB500 UDS reply (speed reconfiguration), consumed by cab500_speed_machine() ---
  if (!rx_frame.ext_ID && rx_frame.ID == CAB500_UDS_RX_ID && rx_frame.DLC >= 2) {
    uint8_t sid = rx_frame.data.u8[1];
    cab500_uds_reply_sid = sid;
    if (sid == 0x62 && rx_frame.DLC >= 6 && rx_frame.data.u8[2] == 0xF0 && rx_frame.data.u8[3] == 0x12) {
      cab500_uds_reply_kbps = ((uint16_t)rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
    }
    return;
  }

  // --- CAB500 current sensor frame (must be checked BEFORE the balancer ID filter below) ---
  if (!rx_frame.ext_ID && rx_frame.ID == cab500_can_id && rx_frame.DLC == 8) {
    cab500_frames_seen++;  // also counted while searching the sensor at other bus speeds
    if (config_error != JK_CFG_OK) {
      return;  // settings are inconsistent, battery not run
    }
    ct_cab500_last_frame_millis = millis();
    ct_cab500_fault_counter = 0;  // Same behavior as balancers: counter resets on any valid frame

    // Byte order: big endian (Motorola). IP_VALUE = bytes 0-3, offset-encoded: 0x80000000 = 0 mA
    uint32_t raw = ((uint32_t)rx_frame.data.u8[0] << 24) | ((uint32_t)rx_frame.data.u8[1] << 16) |
                   ((uint32_t)rx_frame.data.u8[2] << 8) | (uint32_t)rx_frame.data.u8[3];
    uint8_t error_byte = rx_frame.data.u8[4];
    bool error_now = (error_byte & 0x01);  // b0 = ERROR_INDICATION

    if (error_now) {
      if (!ct_cab500_error_active) {
        ct_cab500_error_active = true;
        ct_cab500_error_start_millis = millis();
      } else if (!ct_cab500_hardware_error && (millis() - ct_cab500_error_start_millis > CAB500_ERROR_PERSIST_MS)) {
        ct_cab500_hardware_error = true;    // Latched until reboot
        ct_cab500_error_code = error_byte;  // Latched FAILURE MODE (0x41/0x42/0x44/0x46)
      }
    } else {
      ct_cab500_error_active = false;  // hardware_error + error_code stay latched
    }

    // Accept current only if no error and not the failure sentinel (Ip = 0xFFFFFFFF during DTC)
    if (!error_now && raw != 0xFFFFFFFFu) {
      // unsigned subtraction wraps correctly, reinterpret as signed mA
      int32_t current_mA = (int32_t)(raw - 0x80000000u);
      if (current_mA >= -CAB500_MAX_PLAUSIBLE_mA && current_mA <= CAB500_MAX_PLAUSIBLE_mA) {
        ct_cab500_current_sum_A += (float)current_mA / 1000.0f;
        ct_cab500_sample_count++;
      }
    }
    return;  // CAB500 frame fully handled
  }

  if (config_error != JK_CFG_OK) {
    return;  // settings are inconsistent, battery not run
  }

  if ((rx_frame.ID & 0x7F0) != 0) {
    return;
  }

  uint8_t balancer_address = rx_frame.ID & 0x0F;

  if (balancer_address > 0 && balancer_address <= number_of_balancers) {
    uint8_t balancer_idx = balancer_address - 1;
    uint8_t data_type = rx_frame.data.u8[0];

    battery_can_alive = true;
    last_response_millis[balancer_idx] = millis();
    fault_counters[balancer_idx] = 0;
    datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

    switch (data_type) {
      case 0x01: {
        balancers[balancer_idx].temperature_dC = ((int16_t)((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2])) * 10;
        balancers[balancer_idx].total_voltage_10mV = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        balancers[balancer_idx].identified_cells = rx_frame.data.u8[7];
        break;
      }
      case 0x02: {  // Balancing and alarm status byte, max cell delta, live balancing current
        uint8_t status_byte = rx_frame.data.u8[3];
        balancers[balancer_idx].max_delta_mV = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        balancers[balancer_idx].balance_current_mA = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        balancers[balancer_idx].raw_status_byte = status_byte;
        balancers[balancer_idx].is_charge_balancing = (status_byte & 0x01);     // Bit 0
        balancers[balancer_idx].is_discharge_balancing = (status_byte & 0x02);  // Bit 1
        bool raw_cell_count = (status_byte & 0x10);                             // Bit 4
        bool raw_wire_res = (status_byte & 0x20);                               // Bit 5
        if (balancers[balancer_idx].fw_inverted) {
          // V11.55: 1 = OK, 0 = ALARM
          balancers[balancer_idx].alarm_cell_count_mismatch = !raw_cell_count;
          balancers[balancer_idx].alarm_wire_resistance = !raw_wire_res;
        } else {
          // V11.56 (protocol document): 1 = ALARM, 0 = OK
          balancers[balancer_idx].alarm_cell_count_mismatch = raw_cell_count;
          balancers[balancer_idx].alarm_wire_resistance = raw_wire_res;
        }
        // Consecutive status frames with the alarm present (saturating), reset by a clean frame
        if (balancers[balancer_idx].alarm_cell_count_mismatch) {
          if (cell_count_alarm_streak[balancer_idx] < 255) {
            cell_count_alarm_streak[balancer_idx]++;
          }
        } else {
          cell_count_alarm_streak[balancer_idx] = 0;
        }
        if (balancers[balancer_idx].alarm_wire_resistance) {
          if (wire_alarm_streak[balancer_idx] < 255) {
            wire_alarm_streak[balancer_idx]++;
          }
        } else {
          wire_alarm_streak[balancer_idx] = 0;
        }
        break;
      }
      case 0x03: {
        balancers[balancer_idx].max_balance_current_mA = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        balancers[balancer_idx].balance_switch_on = (rx_frame.data.u8[5] == 1);
        balancers[balancer_idx].set_cells = rx_frame.data.u8[6];
        break;
      }
      case 0x04: {
        uint8_t start_cell_num = rx_frame.data.u8[1];
        if (start_cell_num < JK_CELLS_BALANCER) {
          balancers[balancer_idx].cell_voltages_mV[start_cell_num] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
        }
        if ((start_cell_num + 1) < JK_CELLS_BALANCER) {
          balancers[balancer_idx].cell_voltages_mV[start_cell_num + 1] =
              (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
        }
        if ((start_cell_num + 2) < JK_CELLS_BALANCER) {
          balancers[balancer_idx].cell_voltages_mV[start_cell_num + 2] =
              (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
        }
        break;
      }
      default:
        break;
    }
  }
}

void JKActiveBalancer::update_values() {
  if (config_error != JK_CFG_OK) {
    // Inconsistent settings: stay in fault, no power, contactors not allowed. Fix on the Settings page and reboot.
    datalayer_battery->status.real_bms_status = BMS_FAULT;
    set_event(EVENT_JK_CONFIG_ERROR, config_error, battery_index);
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
    datalayer_jk->config_error = config_error;
    return;
  }

  // --- Data aggregation from all balancers ---
  uint32_t total_voltage_mV = 0;
  int16_t min_temp = 32767;
  int16_t max_temp = -32768;
  uint16_t min_cell_voltage = 65535;
  uint16_t max_cell_voltage = 0;
  int cells_processed = 0;
  int valid_cells = 0;  // cells that have reported a voltage; 0 mV = not received yet or tap broken
  uint32_t balancer_sum_mV[MAX_BALANCERS] = {0};  // per balancer, for the page (V11.55 total frame is not usable)

  for (int i = 0; i < number_of_balancers; i++) {
    if (balancers[i].temperature_dC > max_temp) {
      max_temp = balancers[i].temperature_dC;
    }
    if (balancers[i].temperature_dC < min_temp) {
      min_temp = balancers[i].temperature_dC;
    }

    int cells_in_this_balancer;
    if (JK_BRIDGE_MODE) {
      if (i == 0 && number_of_balancers > 1) {
        cells_in_this_balancer = JK_BRIDGE_CELLS;
      } else if (i == 0 && number_of_balancers == 1) {
        cells_in_this_balancer = JK_NUMBER_OF_CELLS;
      } else {
        int remaining_cells = JK_NUMBER_OF_CELLS - cells_processed;
        cells_in_this_balancer = (remaining_cells > JK_BRIDGE_CELLS) ? JK_BRIDGE_CELLS : remaining_cells;
      }
    } else {
      int remaining_cells = JK_NUMBER_OF_CELLS - cells_processed;
      cells_in_this_balancer = (remaining_cells > JK_CELLS_BALANCER) ? JK_CELLS_BALANCER : remaining_cells;
    }

    for (int j = 0; j < cells_in_this_balancer; j++) {
      uint16_t current_cell_voltage = balancers[i].cell_voltages_mV[j];
      if (current_cell_voltage > 0) {
        valid_cells++;
        total_voltage_mV += current_cell_voltage;
        balancer_sum_mV[i] += current_cell_voltage;
        datalayer_battery->status.cell_voltages_mV[cells_processed] = current_cell_voltage;
        if (current_cell_voltage > max_cell_voltage) {
          max_cell_voltage = current_cell_voltage;
        }
        if (current_cell_voltage < min_cell_voltage) {
          min_cell_voltage = current_cell_voltage;
        }
      }
      cells_processed++;
      if (cells_processed >= JK_NUMBER_OF_CELLS) {
        break;
      }
    }
    if (cells_processed >= JK_NUMBER_OF_CELLS) {
      break;
    }
  }
  // Cell extremes for the SOC calculation
  BMS_cell_max_mV = max_cell_voltage;
  BMS_cell_min_mV = (min_cell_voltage == 65535) ? 0 : min_cell_voltage;

  // The pack is only known once every configured cell has a voltage. Before that (balancers still
  // booting, first polls in flight) nothing is published: a half-read pack looks like cells at 0 mV
  // and would trip the latching critical-undervoltage protection.
  const bool cells_complete = (valid_cells >= JK_NUMBER_OF_CELLS);
  if (cells_complete) {
    cells_ever_complete = true;
  }

  // --- Hardware alarms from any balancer, latched until reboot ---
  bool hardware_alarm = false;
  for (int i = 0; i < number_of_balancers; i++) {
    if (cell_count_alarm_streak[i] >= ALARM_DEBOUNCE_FRAMES && !cell_count_alarm_latched[i]) {
      cell_count_alarm_latched[i] = true;
      set_event_latched(EVENT_JK_CELL_COUNT_MISMATCH, i + 1, battery_index);
    }
    if (wire_alarm_streak[i] >= ALARM_DEBOUNCE_FRAMES && !wire_alarm_latched[i]) {
      wire_alarm_latched[i] = true;
      set_event_latched(EVENT_JK_WIRE_RESISTANCE, i + 1, battery_index);
    }
    if (cell_count_alarm_latched[i] || wire_alarm_latched[i]) {
      hardware_alarm = true;
    }
  }

  // --- Communication timeouts, latched until reboot ---
  bool comms_fault = false;
  for (int i = 0; i < number_of_balancers; i++) {
    if (fault_counters[i] >= MAX_FAULT_COUNT && !balancer_comms_latched[i]) {
      balancer_comms_latched[i] = true;
      set_event_latched(EVENT_JK_BALANCER_COMM_FAULT, i + 1, battery_index);
    }
    if (balancer_comms_latched[i]) {
      comms_fault = true;
    }
  }

  // --- Missed frames below the fault threshold: warning, cleared when frames return ---
  int first_silent_balancer = 0;
  for (int i = 0; i < number_of_balancers; i++) {
    if (fault_counters[i] > 0 && !balancer_comms_latched[i]) {
      first_silent_balancer = i + 1;
      break;
    }
  }
  if (first_silent_balancer != 0 && !balancer_comm_warn_active) {
    balancer_comm_warn_active = true;
    set_event(EVENT_JK_BALANCER_COMM_WARNING, first_silent_balancer, battery_index);
  } else if (first_silent_balancer == 0 && balancer_comm_warn_active) {
    balancer_comm_warn_active = false;
    clear_event(EVENT_JK_BALANCER_COMM_WARNING, battery_index);
  }
  bool cab500_silent = (ct_cab500_fault_counter > 0 && !ct_cab500_comms_fault);
  if (cab500_silent && !cab500_comm_warn_active) {
    cab500_comm_warn_active = true;
    set_event(EVENT_JK_CAB500_COMM_WARNING, ct_cab500_fault_counter, battery_index);
  } else if (!cab500_silent && cab500_comm_warn_active) {
    cab500_comm_warn_active = false;
    clear_event(EVENT_JK_CAB500_COMM_WARNING, battery_index);
  }

  // CAB500 faults join the same safe-state logic: dead sensor -> comms_fault,
  // persistent ERROR_INDICATION -> hardware_alarm. Both already latch in the driver.
  if (ct_cab500_comms_fault) {
    comms_fault = true;
    if (!cab500_comm_event_raised) {
      cab500_comm_event_raised = true;
      set_event_latched(EVENT_JK_CAB500_COMM_FAULT, 0, battery_index);
    }
  }
  if (ct_cab500_hardware_error) {
    hardware_alarm = true;
    if (!cab500_hw_event_raised) {
      cab500_hw_event_raised = true;
      set_event_latched(EVENT_JK_CAB500_HW_ERROR, ct_cab500_error_code, battery_index);
    }
  }

  if (comms_fault || hardware_alarm) {
    datalayer_battery->status.real_bms_status = BMS_FAULT;
    datalayer_battery->status.max_charge_power_W = FAULT_POWER_W;
    datalayer_battery->status.max_discharge_power_W = FAULT_POWER_W;
  } else if (battery_can_alive && cells_complete) {
    datalayer_battery->status.real_bms_status = BMS_ACTIVE;
    *allows_contactor_closing = true;
  } else if (cells_ever_complete) {
    // A cell that was there has gone silent without an alarm bit: stop, keep the last good readings
    datalayer_battery->status.real_bms_status = BMS_DISCONNECTED;
    if (!cells_lost_event_raised) {
      cells_lost_event_raised = true;
      set_event_latched(EVENT_JK_CELL_COUNT_MISMATCH, 0, battery_index);
    }
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  } else {
    // Waiting for the first complete data set after boot. No stop request yet: nothing is allowed anyway
    // (0 W, contactors not permitted) and v13 raises its own "battery missing" error if nothing ever comes.
    datalayer_battery->status.real_bms_status = BMS_DISCONNECTED;
    datalayer_battery->status.max_charge_power_W = 0;
    datalayer_battery->status.max_discharge_power_W = 0;
  }

  datalayer_battery->status.current_dA = BMS_current_A * 10;
  if (cells_complete) {
    if (JK_BALANCER_LOW_VOLTAGE) {
      // Parallel packs: pack voltage = average of the per-balancer sums. total_voltage_mV holds the sum of
      // ALL cells of ALL balancers, so dividing by number_of_balancers gives the average pack voltage.
      datalayer_battery->status.voltage_dV = (total_voltage_mV / number_of_balancers) / 100;
    } else {
      datalayer_battery->status.voltage_dV = total_voltage_mV / 100;
    }
    datalayer_battery->status.temperature_min_dC = min_temp;
    datalayer_battery->status.temperature_max_dC = max_temp;
    datalayer_battery->status.cell_min_voltage_mV = BMS_cell_min_mV;
    datalayer_battery->status.cell_max_voltage_mV = max_cell_voltage;
  }

  // --- Extended datalayer for the web interface ---
  datalayer_jk->number_of_balancers = number_of_balancers;
  for (int i = 0; i < number_of_balancers; i++) {
    datalayer_jk->balancer_temperatures_dC[i] = balancers[i].temperature_dC;
    // V11.55 (inverted alarm bits) does not report a usable total voltage: sum its cells instead
    datalayer_jk->balancer_total_voltage_10mV[i] =
        balancers[i].fw_inverted ? (uint16_t)(balancer_sum_mV[i] / 10) : balancers[i].total_voltage_10mV;
    datalayer_jk->fault_counters[i] = fault_counters[i];
    datalayer_jk->is_connected[i] =
        !balancer_comms_latched[i] && (fault_counters[i] < MAX_FAULT_COUNT) && (last_response_millis[i] != 0);
    datalayer_jk->max_balance_current_mA[i] = balancers[i].max_balance_current_mA;
    datalayer_jk->balance_current_mA[i] = balancers[i].balance_current_mA;
    datalayer_jk->balancer_max_delta_mV[i] = balancers[i].max_delta_mV;
    datalayer_jk->balancer_identified_cells[i] = balancers[i].identified_cells;
    datalayer_jk->balancer_set_cells[i] = balancers[i].set_cells;
    datalayer_jk->balance_switch_status[i] = balancers[i].balance_switch_on;
    datalayer_jk->charge_balancing_status[i] = balancers[i].is_charge_balancing;
    datalayer_jk->discharge_balancing_status[i] = balancers[i].is_discharge_balancing;
    datalayer_jk->cell_count_mismatch_alarm[i] = cell_count_alarm_latched[i];
    datalayer_jk->wire_resistance_alarm[i] = wire_alarm_latched[i];
  }
  datalayer_jk->ct_cab500_comms_fault = ct_cab500_comms_fault;
  datalayer_jk->ct_cab500_hardware_error = ct_cab500_hardware_error;
  datalayer_jk->ct_cab500_error_code = ct_cab500_error_code;
  datalayer_jk->ct_cab500_fault_counter = ct_cab500_fault_counter;
  datalayer_jk->ct_cab500_is_connected = (ct_cab500_last_frame_millis != 0) && !ct_cab500_comms_fault;

  if (!cells_complete) {
    return;  // SOC, capacity and power limits need the whole pack
  }

  // --- Cycle counter: web reset request, and persistence when the library counted a new cycle ---
  if (cycles_reset_request) {
    cycles_reset_request = false;
    soc.setCycleCount(0);
    datalayer_jk->BMS_Cycles = 0;
    save_cycles(0);
    apply_cycle_degradation(0);
  }
  if (soc_inited && (soc.getCycleCount() != datalayer_jk->BMS_Cycles)) {
    datalayer_jk->BMS_Cycles = soc.getCycleCount();
    save_cycles(datalayer_jk->BMS_Cycles);
    apply_cycle_degradation(datalayer_jk->BMS_Cycles);
  }

  // SOC library starts the first time the balancers answered (needs real cell voltages)
  if (!soc_inited && battery_can_alive) {
    apply_cycle_degradation(datalayer_jk->BMS_Cycles);
    soc.begin(BMS_cell_max_mV, BMS_cell_min_mV);  // Only initializes once the cells read above init_min_cell_mV
    soc.setCycleCount(datalayer_jk->BMS_Cycles);
    soc_inited = true;
  }

  // --- SOC, SOH and remaining capacity ---
  datalayer_battery->status.real_soc = (uint16_t)lroundf(soc.getSoCPercent() * 100.0f);

  float battery_SOH_pct = 100.0f * powf(1.0f - DEGRADATION_PER_CYCLE, datalayer_jk->BMS_Cycles);
  datalayer_battery->status.soh_pptt = (uint16_t)lroundf(battery_SOH_pct * 100.0f);

  Calculated_capacity_Wh = (uint32_t)(Battery_Calc_Ah * (MAX_PACK_VOLTAGE_DV / 10.0f));
  datalayer_battery->status.remaining_capacity_Wh = remaining_Wh_by_soc(soc.getSoCPercent(), Calculated_capacity_Wh);
  datalayer_jk->Calc_capacity_Wh = Calculated_capacity_Wh;
  datalayer_jk->rated_capacity_mAh = (uint32_t)(Battery_Ah * 1000.0f);
  datalayer_jk->calc_capacity_mAh = (uint32_t)(Battery_Calc_Ah * 1000.0f);

  // --- Allowed charge / discharge power: protections first, then the SOC ramps ---
  uint16_t cell_deviation_mV =
      std::abs(datalayer_battery->status.cell_max_voltage_mV - datalayer_battery->status.cell_min_voltage_mV);
  const uint16_t discharge_cutoff_dV = datalayer.battery_settings.max_user_set_discharge_voltage_dV;

  if (comms_fault || hardware_alarm) {
    datalayer_battery->status.max_discharge_power_W = FAULT_POWER_W;
    datalayer_battery->status.max_charge_power_W = FAULT_POWER_W;
  } else if (datalayer_battery->status.voltage_dV < discharge_cutoff_dV) {
    // Below the discharge cut-off: no discharge, but charging stays possible so the pack can come back up
    // (8.13 left the charge limit untouched here, which froze it at 0 W after a boot below the cut-off)
    datalayer_battery->status.max_discharge_power_W = 0;
    datalayer_battery->status.max_charge_power_W = charge_power_limit_W(
        max_charge_W_active, soc.getSoCPercent(), soc_cfg.soc_max_percent, chg_ramp_pct_active, chg_min_power_W_active);
  } else if (datalayer_battery->status.voltage_dV > discharge_cutoff_dV + dis_cutoff_hyst_dV_active) {
    datalayer_battery->status.max_discharge_power_W =
        discharge_power_limit_W(max_discharge_W_active, soc.getSoCPercent(), dis_ramp_bottom_pct_active,
                                dis_ramp_pct_active, dis_min_power_W_active);
    datalayer_battery->status.max_charge_power_W = charge_power_limit_W(
        max_charge_W_active, soc.getSoCPercent(), soc_cfg.soc_max_percent, chg_ramp_pct_active, chg_min_power_W_active);
  }

  // Cell spread above the deviation setting: derate both limits linearly, full power at the setting,
  // FAULT_POWER_W at setting + DEVIATION_DERATE_MV and beyond (8.13 jumped straight to 50 W at that point).
  // The safety layer raises its own warning at the setting; this one explains the reduced power.
  if (!comms_fault && !hardware_alarm && cell_deviation_mV > MAX_CELL_DEVIATION_MV) {
    float over_mV = (float)(cell_deviation_mV - MAX_CELL_DEVIATION_MV);
    float factor = 1.0f - over_mV / (float)DEVIATION_DERATE_MV;
    if (factor < 0.0f) {
      factor = 0.0f;
    }
    uint32_t dis = (uint32_t)(datalayer_battery->status.max_discharge_power_W * factor);
    uint32_t chg = (uint32_t)(datalayer_battery->status.max_charge_power_W * factor);
    datalayer_battery->status.max_discharge_power_W = (dis < FAULT_POWER_W) ? FAULT_POWER_W : dis;
    datalayer_battery->status.max_charge_power_W = (chg < FAULT_POWER_W) ? FAULT_POWER_W : chg;
    if (!deviation_warn_active) {
      deviation_warn_active = true;
      set_event(EVENT_JK_CELL_DEVIATION, cell_deviation_mV, battery_index);
    }
  } else if (deviation_warn_active && cell_deviation_mV <= MAX_CELL_DEVIATION_MV) {
    deviation_warn_active = false;
    clear_event(EVENT_JK_CELL_DEVIATION, battery_index);
  }

  // SOC window reached: no further charge / discharge. The full/empty events themselves are owned by the
  // safety layer (it raises and clears them from real_soc every cycle), so the driver only forces the power.
  battery_full = (soc.getSoCPercent() >= soc_cfg.soc_max_percent);
  battery_empty = (soc.getSoCPercent() <= soc_cfg.soc_min_percent);
  if (battery_full) {
    datalayer_battery->status.max_charge_power_W = 0;
  }
  if (battery_empty) {
    datalayer_battery->status.max_discharge_power_W = 0;
  }
}

void JKActiveBalancer::transmit_can(unsigned long currentMillis) {
  // --- CAB500 speed reconfiguration: while it runs the bus is not at the balancer speed, so nothing else is sent ---
  if (cab500_speed_state != JK_CAB500_SPD_IDLE && cab500_speed_state < JK_CAB500_SPD_DONE) {
    cab500_speed_machine(currentMillis);
    return;
  }
  if (cab500_speed_request_kbps != 0) {
    uint16_t kbps = cab500_speed_request_kbps;
    cab500_speed_request_kbps = 0;
    if (kbps == 125 || kbps == 250 || kbps == 500) {
      bool contactors_closed = (battery_index == 1)   ? (datalayer.system.status.contactors_engaged == 1)
                               : (battery_index == 2) ? datalayer.system.status.contactors_battery2_engaged
                                                      : datalayer.system.status.contactors_battery3_engaged;
      cab500_speed_target_kbps = kbps;
      cab500_speed_found_kbps = 0;
      if (contactors_closed) {
        cab500_speed_enter(JK_CAB500_SPD_REFUSED, currentMillis);  // bus speed never changed, nothing to restore
      } else {
        cab500_detect_index = 0;
        change_can_speed(CAN_Speed::CAN_SPEED_500KBPS);  // first candidate, the factory speed
        cab500_speed_enter(JK_CAB500_SPD_DETECT, currentMillis);
        return;
      }
    }
  }

  if (config_error != JK_CFG_OK) {
    return;  // settings are inconsistent, battery not run
  }

  if (!battery_can_alive && (currentMillis < BOOT_POLL_WINDOW_MS)) {
    if (currentMillis - last_poll_millis >= POLL_INTERVAL_MS) {
      last_poll_millis = currentMillis;
      JK_REQUEST_FRAME.ID = 0x01;
      transmit_can_frame(&JK_REQUEST_FRAME);
    }
    return;
  } else if (!battery_can_alive) {
    datalayer_battery->status.real_bms_status = BMS_DISCONNECTED;
    return;
  }

  if (currentMillis - last_poll_millis >= POLL_INTERVAL_MS) {
    last_poll_millis = currentMillis;

    current_balancer_to_poll = (current_balancer_to_poll + 1) % number_of_balancers;
    if (last_response_millis[current_balancer_to_poll] >= last_request_millis[current_balancer_to_poll]) {
      JK_REQUEST_FRAME.ID = current_balancer_to_poll + 1;
      transmit_can_frame(&JK_REQUEST_FRAME);
      last_request_millis[current_balancer_to_poll] = currentMillis;
    }
  }

  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;
    for (int i = 0; i < number_of_balancers; i++) {
      if (last_request_millis[i] > last_response_millis[i]) {
        if (currentMillis - last_request_millis[i] > POLL_TIMEOUT_MS) {
          fault_counters[i]++;
          last_request_millis[i] = last_response_millis[i];
        }
      }
    }
    // CAB500 timeout: the sensor transmits every 10 ms; >1 s of silence = 1 fault.
    // 30 s error-free grace after boot. Counter resets in RX on any valid frame.
    if (currentMillis - ct_cab500_boot_millis > CAB500_BOOT_GRACE_MS) {
      if ((ct_cab500_last_frame_millis == 0) || (currentMillis - ct_cab500_last_frame_millis > 1000)) {
        if (ct_cab500_fault_counter < MAX_FAULT_COUNT) {
          ct_cab500_fault_counter++;
        }
      }
      if (ct_cab500_fault_counter >= MAX_FAULT_COUNT) {
        ct_cab500_comms_fault = true;  // latched until reboot
      }
    }
  }

  // Average the CAB500 samples every 50 ms and feed the SOC library
  if (currentMillis - previousMillis50 >= SOC_UPDATE_MS) {
    previousMillis50 = currentMillis;
    if (ct_cab500_sample_count > 0) {
      BMS_current_A = ct_cab500_current_sum_A / (float)ct_cab500_sample_count;
      if (reverse_current_active) {
        BMS_current_A = -BMS_current_A;
      }
      ct_cab500_current_sum_A = 0.0f;
      ct_cab500_sample_count = 0;
    } else {
      // No fresh samples (sensor silent, boot grace): count nothing rather than integrate a stale reading
      BMS_current_A = 0.0f;
    }
    soc.update(BMS_cell_max_mV, BMS_cell_min_mV, BMS_current_A);
  }
}

// --- CAB500 CAN speed reconfiguration ---
// Sequence from the CAB500 diagnostic notes: at the sensor's current speed send WriteDataByIdentifier
// 0xF012 (05 2E F0 12 hi lo), wait >= 1 s, send ECUReset HardReset (02 11 01), switch the bus to the
// new speed and optionally confirm with ReadDataByIdentifier 0xF012 (03 22 F0 12 -> 05 62 F0 12 hi lo).
// Speed bytes: 125 kbps = 00 7D, 250 kbps = 00 FA, 500 kbps = 01 F4.

CAN_Speed JKActiveBalancer::kbps_to_can_speed(uint16_t kbps) {
  switch (kbps) {
    case 125:
      return CAN_Speed::CAN_SPEED_125KBPS;
    case 500:
      return CAN_Speed::CAN_SPEED_500KBPS;
    default:
      return CAN_Speed::CAN_SPEED_250KBPS;
  }
}

void JKActiveBalancer::cab500_send_uds(uint8_t dlc, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4,
                                       uint8_t b5) {
  CAN_frame frame = {
      .FD = false, .ext_ID = false, .DLC = dlc, .ID = CAB500_UDS_TX_ID, .data = {b0, b1, b2, b3, b4, b5}};
  cab500_uds_reply_sid = 0;
  cab500_uds_reply_kbps = 0;
  transmit_can_frame(&frame);
}

void JKActiveBalancer::cab500_speed_enter(uint8_t state, unsigned long now) {
  cab500_speed_state = state;
  cab500_state_since = now;
  cab500_frames_mark = cab500_frames_seen;
  datalayer_jk->ct_cab500_speed_state = state;
  datalayer_jk->ct_cab500_speed_target_kbps = cab500_speed_target_kbps;
  datalayer_jk->ct_cab500_speed_found_kbps = cab500_speed_found_kbps;
}

// Terminal state: the bus goes back to the balancer speed and normal operation resumes
void JKActiveBalancer::cab500_speed_finish(uint8_t state, unsigned long now) {
  reset_can_speed();
  ct_cab500_last_frame_millis = now;  // no comms faults for the time the bus was elsewhere
  cab500_speed_enter(state, now);
}

void JKActiveBalancer::cab500_speed_machine(unsigned long now) {
  static const uint16_t candidates[3] = {500, 250, 125};
  const uint8_t speed_hi = (cab500_speed_target_kbps >> 8) & 0xFF;
  const uint8_t speed_lo = cab500_speed_target_kbps & 0xFF;

  switch (cab500_speed_state) {
    case JK_CAB500_SPD_DETECT:
      if (cab500_frames_seen != cab500_frames_mark) {  // the 0x3C2 stream is visible at this speed
        cab500_speed_found_kbps = candidates[cab500_detect_index];
        cab500_send_uds(6, 0x05, 0x2E, 0xF0, 0x12, speed_hi, speed_lo);  // WriteDataByIdentifier 0xF012
        cab500_speed_enter(JK_CAB500_SPD_WRITE, now);
      } else if (now - cab500_state_since >= CAB500_DETECT_LISTEN_MS) {
        cab500_detect_index++;
        if (cab500_detect_index >= 3) {
          cab500_speed_finish(JK_CAB500_SPD_NOT_FOUND, now);
        } else {
          change_can_speed(kbps_to_can_speed(candidates[cab500_detect_index]));
          cab500_speed_enter(JK_CAB500_SPD_DETECT, now);
        }
      }
      break;

    case JK_CAB500_SPD_WRITE:
      if (cab500_uds_reply_sid == 0x6E) {  // positive response
        cab500_speed_enter(JK_CAB500_SPD_PAUSE, now);
      } else if (cab500_uds_reply_sid == 0x7F || now - cab500_state_since >= CAB500_UDS_TIMEOUT_MS) {
        cab500_speed_finish(JK_CAB500_SPD_WRITE_FAILED, now);
      }
      break;

    case JK_CAB500_SPD_PAUSE:
      if (now - cab500_state_since >= CAB500_WRITE_SETTLE_MS) {
        cab500_send_uds(3, 0x02, 0x11, 0x01, 0x00, 0x00, 0x00);  // ECUReset HardReset
        cab500_speed_enter(JK_CAB500_SPD_RESET, now);
      }
      break;

    case JK_CAB500_SPD_RESET:
      if (cab500_uds_reply_sid == 0x51 || now - cab500_state_since >= CAB500_RESET_REPLY_MS) {
        change_can_speed(kbps_to_can_speed(cab500_speed_target_kbps));
        cab500_speed_enter(JK_CAB500_SPD_REBOOT, now);
      }
      break;

    case JK_CAB500_SPD_REBOOT:
      if (now - cab500_state_since >= CAB500_REBOOT_MS) {
        cab500_send_uds(4, 0x03, 0x22, 0xF0, 0x12, 0x00, 0x00);  // ReadDataByIdentifier 0xF012
        cab500_speed_enter(JK_CAB500_SPD_VERIFY, now);
      }
      break;

    case JK_CAB500_SPD_VERIFY:
      if (cab500_uds_reply_sid == 0x62 && cab500_uds_reply_kbps == cab500_speed_target_kbps) {
        cab500_speed_finish(JK_CAB500_SPD_DONE, now);
      } else if (now - cab500_state_since >= CAB500_UDS_TIMEOUT_MS) {
        // No readback: the 0x3C2 stream at the new speed is confirmation enough
        bool stream_seen = (cab500_frames_seen != cab500_frames_mark);
        cab500_speed_finish(stream_seen ? JK_CAB500_SPD_DONE : JK_CAB500_SPD_UNVERIFIED, now);
      }
      break;

    default:
      cab500_speed_finish(JK_CAB500_SPD_IDLE, now);
      break;
  }
}

bool JKActiveBalancer::soc_plausible() {
  // Pack above (max design - 2.5 %) with a coulomb-counted SOC below 65 % means the counter drifted.
  // The margin scales with the series cell count (a fixed 100 dV only fits ~96S).
  const uint16_t vmax = datalayer_battery->info.max_design_voltage_dV;
  if (datalayer_battery->status.voltage_dV > (vmax - vmax / 40)) {
    return datalayer_battery->status.real_soc >= 6500;
  }
  return true;
}

// Rated capacity in Ah from the configured Wh and the max pack voltage
float JKActiveBalancer::rated_Ah() const {
  float max_pack_V = MAX_PACK_VOLTAGE_DV / 10.0f;
  if (max_pack_V <= 0.0f) {
    return 1.0f;  // only reachable with a config_error, the battery is not run then
  }
  return (float)datalayer_battery->info.total_capacity_Wh / max_pack_V;
}

// Degrade the rated capacity by the cycle count and hand it to the SOC library
void JKActiveBalancer::apply_cycle_degradation(uint32_t cycles) {
  Battery_Ah = rated_Ah();
  if (cycles == 0) {
    Battery_Calc_Ah = Battery_Ah;
  } else {
    Battery_Calc_Ah = Battery_Ah * powf(1.0f - DEGRADATION_PER_CYCLE, cycles);
  }
  soc.setCapacity(Battery_Calc_Ah);
}

// Cycles change at most once per full discharge, so writing NVS here is cheap enough
void JKActiveBalancer::save_cycles(uint32_t cycles) {
  BatteryEmulatorSettingsStore settings;
  settings.saveUInt(cycles_key(), cycles);
}

// Discharge power limit in W, ramped linearly from min_power_W at bottom_soc_pct up to max_power_W at
// bottom_soc_pct + ramp_width_pct. Zero below bottom_soc_pct.
uint32_t JKActiveBalancer::discharge_power_limit_W(uint32_t max_power_W, float soc_pct, float bottom_soc_pct,
                                                   float ramp_width_pct, uint32_t min_power_W) {
  float upper_threshold = bottom_soc_pct + ramp_width_pct;

  if (soc_pct < bottom_soc_pct) {
    return 0;
  } else if (soc_pct >= upper_threshold || ramp_width_pct <= 0.0f) {
    return max_power_W;
  }
  float scale = (soc_pct - bottom_soc_pct) / ramp_width_pct;
  float limited_power_W = (float)max_power_W * scale;
  if (limited_power_W < (float)min_power_W) {
    limited_power_W = (float)min_power_W;
  }
  if (limited_power_W > (float)max_power_W) {
    limited_power_W = (float)max_power_W;
  }
  return (uint32_t)limited_power_W;
}

// Charge power limit in W, ramped linearly from max_power_W at max_soc_pct - ramp_width_pct down to
// min_power_W at max_soc_pct. Zero above max_soc_pct.
uint32_t JKActiveBalancer::charge_power_limit_W(uint32_t max_power_W, float soc_pct, float max_soc_pct,
                                                float ramp_width_pct, uint32_t min_power_W) {
  float lower_threshold = max_soc_pct - ramp_width_pct;

  if (soc_pct > max_soc_pct) {
    return 0;
  } else if (soc_pct <= lower_threshold || ramp_width_pct <= 0.0f) {
    return max_power_W;
  }
  float scale = (max_soc_pct - soc_pct) / ramp_width_pct;
  float limited_power_W = (float)max_power_W * scale;
  if (limited_power_W < (float)min_power_W) {
    limited_power_W = (float)min_power_W;
  }
  if (limited_power_W > (float)max_power_W) {  // handles min_power_W > max_power_W
    limited_power_W = (float)max_power_W;
  }
  return (uint32_t)limited_power_W;
}

uint32_t JKActiveBalancer::remaining_Wh_by_soc(float soc_pct, uint32_t capacity_Wh) {
  if (soc_pct < 0.0f) {
    soc_pct = 0.0f;
  } else if (soc_pct > 100.0f) {
    soc_pct = 100.0f;
  }
  return static_cast<uint32_t>(roundf(static_cast<float>(capacity_Wh) * (soc_pct / 100.0f)));
}
