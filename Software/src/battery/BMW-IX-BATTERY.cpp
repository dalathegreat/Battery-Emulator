#include "BMW-IX-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../include.h"

// Function to check if a value has gone stale over a specified time period
bool BmwIXBattery::isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
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

uint8_t BmwIXBattery::increment_uds_req_id_counter(uint8_t index) {
  index++;
  if (index >= numUDSreqs) {
    index = 0;
  }
  return index;
}

uint8_t BmwIXBattery::increment_alive_counter(uint8_t counter) {
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

void BmwIXBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

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

void BmwIXBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x12B8D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x20B:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2E2:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x31F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EA:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x453:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x486:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x49C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4A1:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4BB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x4D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x507:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x587:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
#endif  // DEBUG_LOG
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
    case 0x7AB:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x8F:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0xD0D087:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void BmwIXBattery::transmit_can(unsigned long currentMillis) {
  // We can always send CAN as the iX BMS will wake up on vehicle comms
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;
    ContactorCloseRequest.present = contactorCloseReq;
    // Detect edge
    if (ContactorCloseRequest.previous == false && ContactorCloseRequest.present == true) {
      // Rising edge detected
#ifdef DEBUG_LOG
      logging.println("Rising edge detected. Resetting 10ms counter.");
#endif                   // DEBUG_LOG
      counter_10ms = 0;  // reset counter
    } else if (ContactorCloseRequest.previous == true && ContactorCloseRequest.present == false) {
      // Dropping edge detected
#ifdef DEBUG_LOG
      logging.println("Dropping edge detected. Resetting 10ms counter.");
#endif                   // DEBUG_LOG
      counter_10ms = 0;  // reset counter
    }
    ContactorCloseRequest.previous = ContactorCloseRequest.present;
    HandleBmwIxCloseContactorsRequest(counter_10ms);
    HandleBmwIxOpenContactorsRequest(counter_10ms);
    counter_10ms++;

    // prevent counter overflow: 2^16-1 = 65535
    if (counter_10ms == 65535) {
      counter_10ms = 1;  // set to 1, to differentiate the counter being set to 0 by the functions above
    }
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    HandleIncomingInverterRequest();

    //Loop through and send a different UDS request once the contactors are closed
    if (contactorCloseReq == true &&
        ContactorState.closed ==
            true) {  // Do not send unless the contactors are requested to be closed and are closed, as sending these does not allow the contactors to close
      uds_req_id_counter = increment_uds_req_id_counter(uds_req_id_counter);
      transmit_can_frame(UDS_REQUESTS100MS[uds_req_id_counter],
                         can_config.battery);  // FIXME: sending these does not allow the contactors to close
    } else {  // FIXME: hotfix: If contactors are not requested to be closed, ensure the battery is reported as alive, even if no CAN messages are received
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
    }

    // Keep contactors closed if needed
    BmwIxKeepContactorsClosed(counter_100ms);
    counter_100ms++;
    if (counter_100ms == 140) {
      counter_100ms = 0;  // reset counter every 14 seconds
    }

    //Send SME Keep alive values 100ms
    //transmit_can_frame(&BMWiX_510, can_config.battery);
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    //Send SME Keep alive values 200ms
    //BMWiX_C0.data.u8[0] = increment_C0_counter(BMWiX_C0.data.u8[0]);  //Keep Alive 1
    //transmit_can_frame(&BMWiX_C0, can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    HandleIncomingUserRequest();
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START2, can_config.battery);
    //transmit_can_frame(&BMWiX_6F4_REQUEST_BALANCING_START, can_config.battery);
  }
}

void BmwIXBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //Reset Battery at bootup
  //transmit_can_frame(&BMWiX_6F4_REQUEST_HARD_RESET, can_config.battery);

  //Before we have started up and detected which battery is in use, use 108S values
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

void BmwIXBattery::HandleIncomingUserRequest(void) {
  // Debug user request to open or close the contactors
#ifdef DEBUG_LOG
  logging.print("User request: contactor close: ");
  logging.print(datalayer_extended.bmwix.UserRequestContactorClose);
  logging.print("  User request: contactor open: ");
  logging.println(datalayer_extended.bmwix.UserRequestContactorOpen);
#endif  // DEBUG_LOG
  if ((datalayer_extended.bmwix.UserRequestContactorClose == false) &&
      (datalayer_extended.bmwix.UserRequestContactorOpen == false)) {
    // do nothing
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == true) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == false)) {
    BmwIxCloseContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorClose = false;
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == false) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == true)) {
    BmwIxOpenContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorOpen = false;
  } else if ((datalayer_extended.bmwix.UserRequestContactorClose == true) &&
             (datalayer_extended.bmwix.UserRequestContactorOpen == true)) {
    // these flasgs should not be true at the same time, therefore open contactors, as that is the safest state
    BmwIxOpenContactors();
    // set user request to false
    datalayer_extended.bmwix.UserRequestContactorClose = false;
    datalayer_extended.bmwix.UserRequestContactorOpen = false;
// print error, as both these flags shall not be true at the same time
#ifdef DEBUG_LOG
    logging.println(
        "Error: user requested contactors to close and open at the same time. Contactors have been opened.");
#endif  // DEBUG_LOG
  }
}

void BmwIXBattery::HandleIncomingInverterRequest(void) {
  InverterContactorCloseRequest.present = datalayer.system.status.inverter_allows_contactor_closing;
  // Detect edge
  if (InverterContactorCloseRequest.previous == false && InverterContactorCloseRequest.present == true) {
// Rising edge detected
#ifdef DEBUG_LOG
    logging.println("Inverter requests to close contactors");
#endif  // DEBUG_LOG
    BmwIxCloseContactors();
  } else if (InverterContactorCloseRequest.previous == true && InverterContactorCloseRequest.present == false) {
// Falling edge detected
#ifdef DEBUG_LOG
    logging.println("Inverter requests to open contactors");
#endif  // DEBUG_LOG
    BmwIxOpenContactors();
  }  // else: do nothing

  // Update state
  InverterContactorCloseRequest.previous = InverterContactorCloseRequest.present;
}

void BmwIXBattery::BmwIxCloseContactors(void) {
#ifdef DEBUG_LOG
  logging.println("Closing contactors");
#endif  // DEBUG_LOG
  contactorCloseReq = true;
}

void BmwIXBattery::BmwIxOpenContactors(void) {
#ifdef DEBUG_LOG
  logging.println("Opening contactors");
#endif  // DEBUG_LOG
  contactorCloseReq = false;
  counter_100ms = 0;  // reset counter, such that keep contactors closed message sequence starts from the beginning
}

void BmwIXBattery::HandleBmwIxCloseContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == true) {  // Only when contactor close request is set to true
    if (ContactorState.closed == false &&
        ContactorState.open ==
            true) {  // Only when the following commands have not been completed yet, because it shall not be run when commands have already been run, AND only when contactor open commands have finished
      // Initially 0x510[2] needs to be 0x02, and 0x510[5] needs to be 0x00
      BMWiX_510.data = {0x40, 0x10,
                        0x02,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                        0x00, 0x00,
                        0x00,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                        0x01,   // 0x01 at contactor closing
                        0x00};  // Explicit declaration, to prevent modification by other functions
      BMWiX_16E.data = {
          0x00,  // Almost any possible number in 0x00 and 0xFF
          0xA0,  // Almost any possible number in 0xA0 and 0xAF
          0xC9, 0xFF, 0x60,
          0xC9, 0x3A, 0xF7};  // Explicit declaration of default values, to prevent modification by other functions

      if (counter_10ms == 0) {
        // @0 ms
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 1/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 5) {
        // @50 ms
        transmit_can_frame(&BMWiX_276, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x276 - 2/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 10) {
        // @100 ms
        BMWiX_510.data.u8[2] = 0x04;  // TODO: check if needed
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 3/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 20) {
        // @200 ms
        BMWiX_510.data.u8[2] = 0x10;  // TODO: check if needed
        BMWiX_510.data.u8[5] = 0x80;  // needed to close contactors
        transmit_can_frame(&BMWiX_510, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x510 - 4/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 30) {
        // @300 ms
        BMWiX_16E.data.u8[0] = 0x6A;
        BMWiX_16E.data.u8[1] = 0xAD;
        transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x16E - 5/6");
#endif  // DEBUG_LOG
      } else if (counter_10ms == 50) {
        // @500 ms
        BMWiX_16E.data.u8[0] = 0x03;
        BMWiX_16E.data.u8[1] = 0xA9;
        transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
        logging.println("Transmitted 0x16E - 6/6");
#endif  // DEBUG_LOG
        ContactorState.closed = true;
        ContactorState.open = false;
      }
    }
  }
}

void BmwIXBattery::BmwIxKeepContactorsClosed(uint8_t counter_100ms) {
  if ((ContactorState.closed == true) && (ContactorState.open == false)) {
    BMWiX_510.data = {0x40, 0x10,
                      0x04,  // 0x02 at contactor closing, afterwards 0x04 and 0x10, 0x00 to open contactors
                      0x00, 0x00,
                      0x80,   // 0x00 at start of contactor closing, changing to 0x80, afterwards 0x80
                      0x01,   // 0x01 at contactor closing
                      0x00};  // Explicit declaration, to prevent modification by other functions
    BMWiX_16E.data = {0x00,   // Almost any possible number in 0x00 and 0xFF
                      0xA0,   // Almost any possible number in 0xA0 and 0xAF
                      0xC9, 0xFF, 0x60,
                      0xC9, 0x3A, 0xF7};  // Explicit declaration, to prevent modification by other functions

    if (counter_100ms == 0) {
#ifdef DEBUG_LOG
      logging.println("Sending keep contactors closed messages started");
#endif  // DEBUG_LOG
      // @0 ms
      transmit_can_frame(&BMWiX_510, can_config.battery);
    } else if (counter_100ms == 7) {
      // @ 730 ms
      BMWiX_16E.data.u8[0] = 0x8C;
      BMWiX_16E.data.u8[1] = 0xA0;
      transmit_can_frame(&BMWiX_16E, can_config.battery);
    } else if (counter_100ms == 24) {
      // @2380 ms
      transmit_can_frame(&BMWiX_510, can_config.battery);
    } else if (counter_100ms == 29) {
      // @ 2900 ms
      BMWiX_16E.data.u8[0] = 0x02;
      BMWiX_16E.data.u8[1] = 0xA7;
      transmit_can_frame(&BMWiX_16E, can_config.battery);
#ifdef DEBUG_LOG
      logging.println("Sending keep contactors closed messages finished");
#endif  // DEBUG_LOG
    } else if (counter_100ms == 140) {
      // @14000 ms
      // reset counter (outside of this function)
    }
  }
}

void BmwIXBattery::HandleBmwIxOpenContactorsRequest(uint16_t counter_10ms) {
  if (contactorCloseReq == false) {  // if contactors are not requested to be closed, they are requested to be opened
    if (ContactorState.open == false) {  // only if contactors are not open yet
      // message content to quickly open contactors
      if (counter_10ms == 0) {
        // @0 ms (0.00) RX0 510 [8] 40 10 00 00 00 80 00 00
        BMWiX_510.data = {0x40, 0x10, 0x00, 0x00,
                          0x00, 0x80, 0x00, 0x00};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_510, can_config.battery);
        // set back to default values
        BMWiX_510.data = {0x40, 0x10, 0x04, 0x00, 0x00, 0x80, 0x01, 0x00};  // default values
      } else if (counter_10ms == 6) {
        // @60 ms  (0.06) RX0 16E [8] E6 A4 C8 FF 60 C9 33 F0
        BMWiX_16E.data = {0xE6, 0xA4, 0xC8, 0xFF,
                          0x60, 0xC9, 0x33, 0xF0};  // Explicit declaration, to prevent modification by other functions
        transmit_can_frame(&BMWiX_16E, can_config.battery);
        // set back to default values
        BMWiX_16E.data = {0x00, 0xA0, 0xC9, 0xFF, 0x60, 0xC9, 0x3A, 0xF7};  // default values
        ContactorState.closed = false;
        ContactorState.open = true;
      }
    }
  }
}
