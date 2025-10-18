#include "RJXZS-BMS.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

// https://www.topbms.com/static/upload/file/20230804/1691122017124559.pdf
// https://github.com/puffnfresh/Battery-Emulator/commit/7b0f817ef138b22902c27c5cff3fbe2df4ba341b

const uint16_t voltage_NMC[101] = {
    4200, 4173, 4148, 4124, 4102, 4080, 4060, 4041, 4023, 4007, 3993, 3980, 3969, 3959, 3953, 3950, 3941,
    3932, 3924, 3915, 3907, 3898, 3890, 3881, 3872, 3864, 3855, 3847, 3838, 3830, 3821, 3812, 3804, 3795,
    3787, 3778, 3770, 3761, 3752, 3744, 3735, 3727, 3718, 3710, 3701, 3692, 3684, 3675, 3667, 3658, 3650,
    3641, 3632, 3624, 3615, 3607, 3598, 3590, 3581, 3572, 3564, 3555, 3547, 3538, 3530, 3521, 3512, 3504,
    3495, 3487, 3478, 3470, 3461, 3452, 3444, 3435, 3427, 3418, 3410, 3401, 3392, 3384, 3375, 3367, 3358,
    3350, 3338, 3325, 3313, 3299, 3285, 3271, 3255, 3239, 3221, 3202, 3180, 3156, 3127, 3090, 3000};

static uint16_t cell_voltage_to_soc(uint16_t v) {
  // Returns SOC in hundredths of a percent (0-10000)

  if (v >= voltage_NMC[0]) {
    return 10000;
  }
  if (v <= voltage_NMC[100]) {
    return 0;
  }

  for (int i = 1; i < 101; ++i) {
    if (v >= voltage_NMC[i]) {
      uint16_t t = (100 * (v - voltage_NMC[i])) / (voltage_NMC[i - 1] - voltage_NMC[i]);
      return 10000 - (i * 100) + t;
    }
  }
  return 0;
}

uint16_t RjxzsBms::cell_voltage_to_soc_scaled(uint16_t v) {
  // Scales the SOC according to user set min and max cell voltage limits.
  // Note that this is not the same as SoC scaling, which will happen after this.
  int32_t soc = cell_voltage_to_soc(v);
  if (soc <= min_cell_equivalent_soc)
    return 0;
  soc = (10000 * (soc - min_cell_equivalent_soc)) / (max_cell_equivalent_soc - min_cell_equivalent_soc);
  if (soc < 0)
    return 0;
  if (soc > 10000)
    return 10000;
  return soc;
}

void RjxzsBms::update_values() {

  datalayer.battery.status.real_soc = battery_capacity_percentage * 100;
  if (battery_capacity_percentage == 0) {
    //SOC% not available. Raise warning event if we go too long without SOC
    timespent_without_soc++;
    if (timespent_without_soc > FIVE_MINUTES) {
      set_event(EVENT_SOC_UNAVAILABLE, 0);
    }
  } else {  //SOC is available, stop counting and clear error
    timespent_without_soc = 0;
    clear_event(EVENT_SOC_UNAVAILABLE);
  }

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt;  // This BMS does not have a SOH% formula

  datalayer.battery.status.voltage_dV = total_voltage;

  if (charging_active) {
    datalayer.battery.status.current_dA = total_current;
  } else if (discharging_active) {
    datalayer.battery.status.current_dA = -total_current;
  } else {  //No direction data. Should never occur, but send current as charging, better than nothing
    datalayer.battery.status.current_dA = total_current;
  }

  // Charge power is manually set
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        datalayer.battery.status.override_charge_power_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  // Discharge power is manually set
  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  uint16_t temperatures[] = {
      module_1_temperature,  module_2_temperature,  module_3_temperature,  module_4_temperature,
      module_5_temperature,  module_6_temperature,  module_7_temperature,  module_8_temperature,
      module_9_temperature,  module_10_temperature, module_11_temperature, module_12_temperature,
      module_13_temperature, module_14_temperature, module_15_temperature, module_16_temperature};

  uint16_t min_temp = std::numeric_limits<uint16_t>::max();
  uint16_t max_temp = 0;

  // Loop through the array to find min and max temperatures, ignoring 0 values
  for (int i = 0; i < 16; i++) {
    if (temperatures[i] != 0) {  // Ignore unavailable temperatures
      if (temperatures[i] < min_temp) {
        min_temp = temperatures[i];
      }
      if (temperatures[i] > max_temp) {
        max_temp = temperatures[i];
      }
    }
  }

  datalayer.battery.status.temperature_min_dC = min_temp;

  datalayer.battery.status.temperature_max_dC = max_temp;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, MAX_AMOUNT_CELLS * sizeof(uint16_t));

  datalayer.battery.info.number_of_cells = populated_cellvoltages;  // 1-192S

  datalayer.battery.info.max_design_voltage_dV;  // Set according to cells?

  datalayer.battery.info.min_design_voltage_dV;  // Set according to cells?

  // battery_pack_capacity = 150;
  // battery_capacity_percentage = 100 - ((battery_usage_capacity * 100) / battery_pack_capacity);
  // minimum_cell_voltage = 3900;
  // maximum_cell_voltage = 3900;
  // setup_completed = true;

  datalayer.battery.status.cell_max_voltage_mV = maximum_cell_voltage;

  datalayer.battery.status.cell_min_voltage_mV = minimum_cell_voltage;
}

void RjxzsBms::handle_incoming_can_frame(CAN_frame rx_frame) {

  /*
  // All CAN messages recieved will be logged via serial
  logging.print(millis());  // Example printout, time, ID, length, data: 7553  1DB  8  FF C0 B9 EA 0 0 2 5D
  logging.print("  ");
  logging.print(rx_frame.ID, HEX);
  logging.print("  ");
  logging.print(rx_frame.DLC);
  logging.print("  ");
  for (int i = 0; i < rx_frame.DLC; ++i) {
    logging.print(rx_frame.data.u8[i], HEX);
    logging.print(" ");
  }
  logging.println("");
  */
  switch (rx_frame.ID) {
    case 0xF5:                 // This is the only message is sent from BMS
      setup_completed = true;  // Let the function know we no longer need to send startup messages
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = rx_frame.data.u8[0];

      if (mux == 0x01) {  // discharge protection voltage, protective current, battery pack capacity
        discharge_protection_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        protective_current = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        battery_pack_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x02) {  // Number of strings, charge protection voltage, protection temperature
        number_of_battery_strings = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        charging_protection_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        protection_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x03) {  // Total voltage/current/power
        total_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        total_current = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        total_power = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x04) {  // Capacity, percentages
        battery_usage_capacity = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        battery_capacity_percentage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        charging_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x05) {  //Recovery / capacity
        charging_recovery_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        discharging_recovery_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        remaining_capacity = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x06) {  // temperature, equalization
        host_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        status_accounting = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        equalization_starting_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        if ((rx_frame.data.u8[4] & 0x40) >> 6) {
          charging_active = true;
          discharging_active = false;
        } else {
          charging_active = false;
          discharging_active = true;
        }
      } else if (mux >= 0x07 && mux <= 0x46) {
        // Cell voltages 1-192 (3 per message, 0x07=1-3, 0x08=4-6, ..., 0x46=190-192)
        int cell_index = (mux - 0x07) * 3;
        for (int i = 0; i < 3; i++) {
          if (cell_index + i >= MAX_AMOUNT_CELLS) {
            break;
          }
          cellvoltages[cell_index + i] = (rx_frame.data.u8[1 + i * 2] << 8) | rx_frame.data.u8[2 + i * 2];
        }
        if (cell_index + 2 >= populated_cellvoltages) {
          populated_cellvoltages = cell_index + 2 + 1;
        }
      } else if (mux == 0x47) {
        temperature_below_zero_mod1_4 = rx_frame.data.u8[2];
        module_1_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x48) {
        module_2_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_3_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x49) {
        module_4_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4A) {
        temperature_below_zero_mod5_8 = rx_frame.data.u8[2];
        module_5_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4B) {
        module_6_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_7_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4C) {
        module_8_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      } else if (mux == 0x4D) {
        temperature_below_zero_mod9_12 = rx_frame.data.u8[2];
        module_9_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_10_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4E) {
        module_11_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_12_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_13_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x4F) {
        module_14_temperature = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        module_15_temperature = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        module_16_temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x50) {
        low_voltage_power_outage_protection = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        low_voltage_power_outage_delayed = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        num_of_triggering_protection_cells = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x51) {
        balanced_reference_voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        minimum_cell_voltage = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        maximum_cell_voltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      } else if (mux == 0x52) {
        accumulated_total_capacity_high = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        accumulated_total_capacity_low = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        pre_charge_delay_time = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        LCD_status = rx_frame.data.u8[7];
      } else if (mux == 0x53) {
        differential_pressure_setting_value = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        use_capacity_to_automatically_reset = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        low_temperature_protection_setting_value = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        protecting_historical_logs = rx_frame.data.u8[7];

        if ((protecting_historical_logs & 0x0F) > 0) {
          set_event(EVENT_RJXZS_LOG, 0);
        } else {
          clear_event(EVENT_RJXZS_LOG);
        }

        if (protecting_historical_logs == 0x01) {
          // Overcurrent protection
          set_event(EVENT_DISCHARGE_LIMIT_EXCEEDED, 0);  // could also be EVENT_CHARGE_LIMIT_EXCEEDED
        } else if (protecting_historical_logs == 0x02) {
          // over discharge protection
          set_event(EVENT_BATTERY_UNDERVOLTAGE, 0);
        } else if (protecting_historical_logs == 0x03) {
          // overcharge protection
          set_event(EVENT_BATTERY_OVERVOLTAGE, 0);
        } else if (protecting_historical_logs == 0x04) {
          // Over temperature protection
          set_event(EVENT_BATTERY_OVERHEAT, 0);
        } else if (protecting_historical_logs == 0x05) {
          // Battery string error protection
          set_event(EVENT_BATTERY_CAUTION, 0);
        } else if (protecting_historical_logs == 0x06) {
          // Damaged charging relay
          set_event(EVENT_BATTERY_CHG_STOP_REQ, 0);
        } else if (protecting_historical_logs == 0x07) {
          // Damaged discharge relay
          set_event(EVENT_BATTERY_DISCHG_STOP_REQ, 0);
        } else if (protecting_historical_logs == 0x08) {
          // Low voltage power outage protection
          set_event(EVENT_12V_LOW, 0);
        } else if (protecting_historical_logs == 0x09) {
          // Voltage difference protection
          set_event(EVENT_VOLTAGE_DIFFERENCE, differential_pressure_setting_value);
        } else if (protecting_historical_logs == 0x0A) {
          // Low temperature protection
          set_event(EVENT_BATTERY_FROZEN, low_temperature_protection_setting_value);
        }
      } else if (mux == 0x54) {
        hall_sensor_type = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
        fan_start_setting_value = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
        ptc_heating_start_setting_value = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
        default_channel_state = rx_frame.data.u8[7];
      }
      break;
    default:
      break;
  }
}

void RjxzsBms::transmit_can(unsigned long currentMillis) {
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    if (datalayer.battery.status.bms_status == FAULT) {
      // Incase we loose BMS comms, resend CAN start
      setup_completed = false;
    }

    if (!setup_completed) {
      transmit_can_frame(&RJXZS_10);  // Communication connected flag
      transmit_can_frame(&RJXZS_1C);  // CAN OK
    }
  }

  // Send 100ms CAN message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if (setup_completed) {
      // Contactor state
      if (datalayer.battery.status.bms_status == FAULT || !datalayer.system.status.inverter_allows_contactor_closing) {
        RJXZS_07.data.u8[2] = 0x02;  // Contactor 'off'
      } else {
        RJXZS_07.data.u8[2] = 0x01;  // Contactor 'on'
      }
      transmit_can_frame(&RJXZS_07);
    }
  }

  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    if (setup_completed) {
      uint16_t derived_soc = cell_voltage_to_soc_scaled(
          (datalayer.battery.status.cell_min_voltage_mV + datalayer.battery.status.cell_max_voltage_mV) / 2);
      int16_t soc_error = datalayer.battery.status.real_soc - derived_soc;
      int16_t ah_error = (soc_error * battery_pack_capacity) / 10000;

      if (ah_error > 1 || ah_error < -1) {
        // Usage has drifted by more than 1Ah

        // Limit to 1Ah correction either way
        int16_t correction = ah_error > 0 ? 1 : -1;

        int16_t new_usage_capacity = battery_usage_capacity + correction;
        if (new_usage_capacity < 0) {
          new_usage_capacity = 0;
        }

        logging.printf("[RJXZS] SoC drift detected: R:%dd%% D:%dd%% (%dAh), setting usage to %dAh.\n",
                       datalayer.battery.status.real_soc, derived_soc, ah_error, new_usage_capacity);

        RJXZS_20.data.u8[1] = (new_usage_capacity >> 8) & 0xFF;
        RJXZS_20.data.u8[2] = new_usage_capacity & 0xFF;
        transmit_can_frame(&RJXZS_20);

        // HACK for testing
        //battery_usage_capacity = new_usage_capacity;
      }
    }
  }
}

void RjxzsBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  max_cell_equivalent_soc = cell_voltage_to_soc(user_selected_max_cell_voltage_mV);
  min_cell_equivalent_soc = cell_voltage_to_soc(user_selected_min_cell_voltage_mV);
  //battery_usage_capacity = 123;
}
