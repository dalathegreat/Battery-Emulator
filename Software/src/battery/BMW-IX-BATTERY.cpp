#include "../include.h"
#ifdef BMW_IX_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "BMW-IX-BATTERY.h"

#include "../communication/can/comm_can.h"

/* Do not change code below unless you are sure what you are doing */

const unsigned long STALE_PERIOD =
    STALE_PERIOD_CONFIG;  // Time in milliseconds to check for staleness (e.g., 5000 ms = 5 seconds)

// Function to check if a value has gone stale over a specified time period
static bool isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
  unsigned long currentTime = millis();

  // Check if the value has changed
  if (currentValue != lastValue) {
    // Update the last change time and value
    lastChangeTime = currentTime;
    lastValue = currentValue;
    return false;  // Value is fresh because it has changed
  }

  // Check if the value has stayed the same for the specified staleness period
  return (currentTime - lastChangeTime >= STALE_PERIOD);
}

uint8_t BMWIXBattery::increment_uds_req_id_counter(uint8_t index) {
  index++;
  if (index >= numUDSreqs) {
    index = 0;
  }
  return index;
}

static uint8_t increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

static byte increment_C0_counter(byte counter) {
  counter++;
  // Reset to 0xF0 if it exceeds 0xFE
  if (counter > 0xFE) {
    counter = 0xF0;
  }
  return counter;
}

void BMWIXBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = avg_soc_state;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh = max_capacity;

  datalayer.battery.status.remaining_capacity_Wh = remaining_capacity;

  datalayer.battery.status.soh_pptt = min_soh_state;

  datalayer.battery.status.max_discharge_power_W = MAX_DISCHARGE_POWER_ALLOWED_W;

  //datalayer.battery.status.max_charge_power_W = 3200; //10000; //Aux HV Port has 100A Fuse  Moved to Ramping

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        MAX_CHARGE_POWER_ALLOWED_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_ALLOWED_W;
  }

  datalayer.battery.status.temperature_min_dC = min_battery_temperature;

  datalayer.battery.status.temperature_max_dC = max_battery_temperature;

  //Check stale values. As values dont change much during idle only consider stale if both parts of this message freeze.
  bool isMinCellVoltageStale =
      isStale(min_cell_voltage, datalayer.battery.status.cell_min_voltage_mV, min_cell_voltage_lastchanged);
  bool isMaxCellVoltageStale =
      isStale(max_cell_voltage, datalayer.battery.status.cell_max_voltage_mV, max_cell_voltage_lastchanged);

  if (isMinCellVoltageStale && isMaxCellVoltageStale) {
    datalayer.battery.status.cell_min_voltage_mV = 9999;  //Stale values force stop
    datalayer.battery.status.cell_max_voltage_mV = 9999;  //Stale values force stop
    set_event(EVENT_STALE_VALUE, 0);
  } else {
    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;  //Value is alive
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;  //Value is alive
  }

  datalayer.battery.info.max_design_voltage_dV = max_design_voltage;

  datalayer.battery.info.min_design_voltage_dV = min_design_voltage;

  datalayer.battery.info.number_of_cells = detected_number_of_cells;

  datalayer_extended.bmwix.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwix.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  datalayer_extended.bmwix.T30_Voltage = terminal30_12v_voltage;

  datalayer_extended.bmwix.hvil_status = hvil_status;

  datalayer_extended.bmwix.bms_uptime = sme_uptime;

  datalayer_extended.bmwix.pyro_status_pss1 = pyro_status_pss1;

  datalayer_extended.bmwix.pyro_status_pss4 = pyro_status_pss4;

  datalayer_extended.bmwix.pyro_status_pss6 = pyro_status_pss6;

  datalayer_extended.bmwix.iso_safety_positive = iso_safety_positive;

  datalayer_extended.bmwix.iso_safety_negative = iso_safety_negative;

  datalayer_extended.bmwix.iso_safety_parallel = iso_safety_parallel;

  datalayer_extended.bmwix.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwix.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwix.balancing_status = balancing_status;

  datalayer_extended.bmwix.battery_voltage_after_contactor = battery_voltage_after_contactor;

  if (battery_info_available) {
    // If we have data from battery - override the defaults to suit
    datalayer.battery.info.max_design_voltage_dV = max_design_voltage;
    datalayer.battery.info.min_design_voltage_dV = min_design_voltage;
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  }
}

void BMWIXBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x112:
      break;
    case 0x607:  //SME responds to UDS requests on 0x607

      if (rx_frame.DLC > 6 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x10 &&
          rx_frame.data.u8[2] == 0xE3 && rx_frame.data.u8[3] == 0x62 && rx_frame.data.u8[4] == 0xE5) {
        //First of multi frame data - Parse the first frame
        if (rx_frame.DLC = 64 && rx_frame.data.u8[5] == 0x54) {  //Individual Cell Voltages - First Frame
          int start_index = 6;                                   //Data starts here
          int voltage_index = 0;                                 //Start cell ID
          int num_voltages = 29;                                 //  number of voltage readings to get
          for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
            uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
            if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
              datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
            }
            voltage_index++;
          }
        }

        //Frame has continued data  - so request it
        transmit_can_frame(&BMWiX_6F4_CONTINUE_DATA, can_config.battery);
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x21) {  //Individual Cell Voltages - 1st Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 29;                          //Start cell ID
        int num_voltages = 31;                           //  number of voltage readings to get
        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x22) {  //Individual Cell Voltages - 2nd Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 60;                          //Start cell ID
        int num_voltages = 31;                           //  number of voltage readings to get
        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[0] == 0xF4 &&
                         rx_frame.data.u8[1] == 0x23) {  //Individual Cell Voltages - 3rd Continue frame
        int start_index = 2;                             //Data starts here
        int voltage_index = 91;                          //Start cell ID
        int num_voltages;
        if (rx_frame.data.u8[12] == 0xFF && rx_frame.data.u8[13] == 0xFF) {  //97th cell is blank - assume 96S Battery
          num_voltages = 5;  //  number of voltage readings to get - 6 more to get on 96S
          detected_number_of_cells = 96;
        } else {              //We have data in 97th cell, assume 108S Battery
          num_voltages = 17;  //  number of voltage readings to get - 17 more to get on 108S
          detected_number_of_cells = 108;
        }

        for (int i = start_index; i < (start_index + num_voltages * 2); i += 2) {
          uint16_t voltage = (rx_frame.data.u8[i] << 8) | rx_frame.data.u8[i + 1];
          if (voltage < 10000) {  //Check reading is plausible - otherwise ignore
            datalayer.battery.status.cell_voltages_mV[voltage_index] = voltage;
          }
          voltage_index++;
        }
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4D) {  //Main Battery Voltage (Pre Contactor)
        battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0x4A) {  //Main Battery Voltage (After Contactor)
        battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x61) {  //Current amps 32bit signed MSB. dA . negative is discharge
        battery_current = ((int32_t)((rx_frame.data.u8[6] << 24) | (rx_frame.data.u8[7] << 16) |
                                     (rx_frame.data.u8[8] << 8) | rx_frame.data.u8[9])) *
                          0.1;
      }

      if (rx_frame.DLC = 64 && rx_frame.data.u8[4] == 0xE4 && rx_frame.data.u8[5] == 0xCA) {  //Balancing Data
        balancing_status = (rx_frame.data.u8[6]);  //4 = No symmetry mode active, invalid qualifier
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0xCE) {  //Min/Avg/Max SOC%
        min_soc_state = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        avg_soc_state = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        max_soc_state = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]);
      }

      if (rx_frame.DLC =
              12 && rx_frame.data.u8[4] == 0xE5 &&
              rx_frame.data.u8[5] == 0xC7) {  //Current and max capacity kWh. Stored in kWh as 0.01 scale with -50  bias
        remaining_capacity = ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) * 10) - 50000;
        max_capacity = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) * 10) - 50000;
      }

      if (rx_frame.DLC = 20 && rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x45) {  //SOH Max Min Mean Request
        min_soh_state = ((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]));
        avg_soh_state = ((rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]));
        max_soh_state = ((rx_frame.data.u8[12] << 8 | rx_frame.data.u8[13]));
      }

      if (rx_frame.DLC = 10 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x62) {  //Max allowed charge and discharge current - Signed 16bit
        allowable_charge_amps = (int16_t)((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7])) / 10;
        allowable_discharge_amps = (int16_t)((rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9])) / 10;
      }

      if (rx_frame.DLC = 9 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x4B) {    //Max allowed charge and discharge current - Signed 16bit
        voltage_qualifier_status = (rx_frame.data.u8[8]);  // Request HV Voltage Qualifier
      }

      if (rx_frame.DLC =
              48 && rx_frame.data.u8[4] == 0xA8 && rx_frame.data.u8[5] == 0x60) {  // Safety Isolation Measurements
        iso_safety_positive = (rx_frame.data.u8[34] << 24) | (rx_frame.data.u8[35] << 16) |
                              (rx_frame.data.u8[36] << 8) | rx_frame.data.u8[37];  //Assuming 32bit
        iso_safety_negative = (rx_frame.data.u8[38] << 24) | (rx_frame.data.u8[39] << 16) |
                              (rx_frame.data.u8[40] << 8) | rx_frame.data.u8[41];  //Assuming 32bit
        iso_safety_parallel = (rx_frame.data.u8[42] << 24) | (rx_frame.data.u8[43] << 16) |
                              (rx_frame.data.u8[44] << 8) | rx_frame.data.u8[45];  //Assuming 32bit
      }

      if (rx_frame.DLC =
              48 && rx_frame.data.u8[4] == 0xE4 && rx_frame.data.u8[5] == 0xC0) {  // Uptime and Vehicle Time Status
        sme_uptime = (rx_frame.data.u8[10] << 24) | (rx_frame.data.u8[11] << 16) | (rx_frame.data.u8[12] << 8) |
                     rx_frame.data.u8[13];  //Assuming 32bit
      }

      if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xAC && rx_frame.data.u8[4] == 0x93) {  // Pyro Status
        pyro_status_pss1 = (rx_frame.data.u8[5]);
        pyro_status_pss4 = (rx_frame.data.u8[6]);
        pyro_status_pss6 = (rx_frame.data.u8[7]);
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[4] == 0xE5 &&
                         rx_frame.data.u8[5] == 0x53) {  //Min and max cell voltage   10V = Qualifier Invalid

        datalayer.battery.status.CAN_battery_still_alive =
            CAN_STILL_ALIVE;  //This is the most important safety values, if we receive this we reset CAN alive counter.

        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) == 10000 ||
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) == 10000) {  //Qualifier Invalid Mode - Request Reboot
#ifdef DEBUG_LOG
          logging.println("Cell MinMax Qualifier Invalid - Requesting BMS Reset");
#endif
          //set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, (millis())); //Eventually need new Info level event type
          transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET, can_config.battery);
        } else {  //Only ingest values if they are not the 10V Error state
          min_cell_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
          max_cell_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        }
      }

      if (rx_frame.DLC = 16 && rx_frame.data.u8[4] == 0xDD && rx_frame.data.u8[5] == 0xC0) {  //Battery Temperature
        min_battery_temperature = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) / 10;
        avg_battery_temperature = (rx_frame.data.u8[10] << 8 | rx_frame.data.u8[11]) / 10;
        max_battery_temperature = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) / 10;
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xA3) {  //Main Contactor Temperature CHECK FINGERPRINT 2 LEVEL
        main_contactor_temperature = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if (rx_frame.DLC = 7 && rx_frame.data.u8[4] == 0xA7) {  //Terminal 30 Voltage (12V SME supply)
        terminal30_12v_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
      }
      if (rx_frame.DLC = 6 && rx_frame.data.u8[0] == 0xF4 && rx_frame.data.u8[1] == 0x04 &&
                         rx_frame.data.u8[2] == 0x62 && rx_frame.data.u8[3] == 0xE5 &&
                         rx_frame.data.u8[4] == 0x69) {  //HVIL Status
        hvil_status = (rx_frame.data.u8[5]);
      }

      if (rx_frame.DLC = 12 && rx_frame.data.u8[2] == 0x07 && rx_frame.data.u8[3] == 0x62 &&
                         rx_frame.data.u8[4] == 0xE5 && rx_frame.data.u8[5] == 0x4C) {  //Pack Voltage Limits
        if ((rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]) < 4700 &&
            (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]) > 2600) {  //Make sure values are plausible
          battery_info_available = true;
          max_design_voltage = (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
          min_design_voltage = (rx_frame.data.u8[8] << 8 | rx_frame.data.u8[9]);
        }
      }

      if (rx_frame.DLC = 16 && rx_frame.data.u8[3] == 0xF1 && rx_frame.data.u8[4] == 0x8C) {  //Battery Serial Number
        //Convert hex bytes to ASCII characters and combine them into a string
        char numberString[11];  // 10 characters + null terminator
        for (int i = 0; i < 10; i++) {
          numberString[i] = char(rx_frame.data.u8[i + 6]);
        }
        numberString[10] = '\0';  // Null-terminate the string
        // Step 3: Convert the string to an unsigned long integer
        battery_serial_number = strtoul(numberString, NULL, 10);
      }
      break;
    default:
      break;
  }
}

void BMWIXBattery::transmit_can() {
  unsigned long currentMillis = millis();

  //if (battery_awake) { //We can always send CAN as the iX BMS will wake up on vehicle comms
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    //Loop through and send a different UDS request each cycle
    uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
    transmit_can_frame(UDS_REQUESTS100MS[uds_req_id_counter], can_config.battery);

    //Send SME Keep alive values 100ms
    transmit_can_frame(&BMWiX_510, can_config.battery);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    //Send SME Keep alive values 200ms
    BMWiX_C0.data.u8[0] = increment_C0_counter(BMWiX_C0.data.u8[0]);  //Keep Alive 1
    transmit_can_frame(&BMWiX_C0, can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START2, can_config.battery);
    transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START, can_config.battery);
  }
}
//We can always send CAN as the iX BMS will wake up on vehicle comms
// else {
//   previousMillis20 = currentMillis;
//   previousMillis100 = currentMillis;
//   previousMillis200 = currentMillis;
//   previousMillis500 = currentMillis;
//   previousMillis640 = currentMillis;
//   previousMillis1000 = currentMillis;
//   previousMillis5000 = currentMillis;
//   previousMillis10000 = currentMillis;
// }
//} //We can always send CAN as the iX BMS will wake up on vehicle comms

void BMWIXBattery::setup(void) {  // Performs one time setup at startup
  //Reset Battery at bootup
  transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET, can_config.battery);

  //Before we have started up and detected which battery is in use, use 108S values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  allow_contactor_closing();
}

#endif
