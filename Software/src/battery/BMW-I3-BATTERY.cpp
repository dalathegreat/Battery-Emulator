#include "BMW-I3-BATTERY.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

/* Do not change code below unless you are sure what you are doing */

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table_SAE_J1850_ZER0[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

uint8_t BmwI3Battery::increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

void BmwI3Battery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer
  if (datalayer.system.info.equipment_stop_active == true) {
    digitalWrite(wakeup_pin, LOW);  // Turn off wakeup pin
  } else if (millis() > INTERVAL_1_S) {
    digitalWrite(wakeup_pin, HIGH);  // Wake up the battery
  }

  if (!battery_awake) {
    return;
  }

  datalayer_battery->status.real_soc = (battery_display_SOC * 50);

  datalayer_battery->status.voltage_dV = battery_volts;  //Unit V+1 (5000 = 500.0V)

  datalayer_battery->status.current_dA = battery_current;

  datalayer_battery->info.total_capacity_Wh = battery_energy_content_maximum_Wh;

  datalayer_battery->status.remaining_capacity_Wh = battery_predicted_energy_charge_condition;

  datalayer_battery->status.soh_pptt = battery_soh * 100;

  datalayer_battery->status.max_discharge_power_W = battery_BEV_available_power_longterm_discharge;

  datalayer_battery->status.max_charge_power_W = battery_BEV_available_power_longterm_charge;

  datalayer_battery->status.temperature_min_dC = battery_temperature_min * 10;  // Add a decimal

  datalayer_battery->status.temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  if (battery_info_available) {
    // Start checking safeties. First up, cellvoltages!
    if (detectedBattery == BATTERY_60AH) {
      datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_60AH;
      datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60AH;
      datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_60AH;
      datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_60AH;
    } else if (detectedBattery == BATTERY_94AH) {
      datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_94AH;
      datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_94AH;
      datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_94AH;
      datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_94AH;
    } else {  // BATTERY_120AH
      datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_120AH;
      datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_120AH;
      datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_120AH;
      datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_120AH;
    }
  }

  // Perform other safety checks
  if (battery_status_error_locking == 2) {  // HVIL seated?
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (battery_status_error_disconnecting_switch > 0) {  // Check if contactors are sticking / welded
    set_event(EVENT_CONTACTOR_WELDED, 0);
  } else {
    clear_event(EVENT_CONTACTOR_WELDED);
  }
}

void BmwI3Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x112:  //BMS [10ms] Status Of High-Voltage Battery - 2
      battery_awake = true;
      datalayer_battery->status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //This message is only sent if 30C (Wakeup pin on battery) is energized with 12V
      battery_current = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) - 8192;  //deciAmps (-819.2 to 819.0A)
      battery_volts = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);           //500.0 V
      datalayer_battery->status.voltage_dV = battery_volts;  // Update the datalayer as soon as possible with this info
      battery_HVBatt_SOC = ((rx_frame.data.u8[5] & 0x0F) << 8 | rx_frame.data.u8[4]);
      battery_request_open_contactors = (rx_frame.data.u8[5] & 0xC0) >> 6;
      battery_request_open_contactors_instantly = (rx_frame.data.u8[6] & 0x03);
      battery_request_open_contactors_fast = (rx_frame.data.u8[6] & 0x0C) >> 2;
      battery_charging_condition_delta = (rx_frame.data.u8[6] & 0xF0) >> 4;
      battery_DC_link_voltage = rx_frame.data.u8[7];
      break;
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
      battery_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery_request_service = (rx_frame.data.u8[2] & 0x03);
      battery_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      battery_temperature_HV = (rx_frame.data.u8[4] - 50);
      battery_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      battery_temperature_min = (rx_frame.data.u8[6] - 50);
      battery_temperature_max = (rx_frame.data.u8[7] - 50);
      break;
    case 0x239:                                                                                      //BMS [200ms]
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x2BD:  //BMS [100ms] Status diagnosis high voltage - 1
      battery_awake = true;
      if (!skipCRCCheck) {
        if (calculateCRC(rx_frame, rx_frame.DLC, 0x15) != rx_frame.data.u8[0]) {
          // If calculated CRC does not match transmitted CRC, increase CANerror counter
          datalayer_battery->status.CAN_error_counter++;

          // If the CRC check has never passed before, set the flag to skip future checks. Some SMEs have differing CRC checks.
          if (!CRCCheckPassedPreviously) {
            skipCRCCheck = true;
          }
          break;
        } else {
          // If CRC check passes, update the flag
          CRCCheckPassedPreviously = true;
        }
      }

      // Process the data since CRC check is either passed or skipped
      battery_status_diagnostics_HV = (rx_frame.data.u8[2] & 0x0F);
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x2FF:  //BMS [100ms] Status Heating High-Voltage Battery
      battery_awake = true;
      battery_actual_value_power_heating = (rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4);
      break;
    case 0x363:  //BMS [1s] Identification High-Voltage Battery
      battery_serial_number =
          (rx_frame.data.u8[3] << 24 | rx_frame.data.u8[2] << 16 | rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      break;
    case 0x3C2:  //BMS (94AH exclusive) - Status diagnostics OBD 2 powertrain
      battery_status_diagnosis_powertrain_maximum_multiplexer =
          ((rx_frame.data.u8[1] & 0x03) << 4 | rx_frame.data.u8[0] >> 4);
      battery_status_diagnosis_powertrain_immediate_multiplexer = (rx_frame.data.u8[0] & 0xFC) >> 2;
      break;
    case 0x3EB:  //BMS [1s] Status of charging high-voltage storage - 3
      battery_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x41C:  //BMS [1s] Operating Mode Status Of Hybrid - 2
      battery_status_cooling_HV = (rx_frame.data.u8[1] & 0x03);
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery - 2
      battery_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x431:  //BMS [200ms] Data High-Voltage Battery Unit
      battery_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery_energy_content_maximum_Wh = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5]) * 20;
      if (battery_energy_content_maximum_Wh > 33000) {
        detectedBattery = BATTERY_120AH;
      } else if (battery_energy_content_maximum_Wh > 20000) {
        detectedBattery = BATTERY_94AH;
      } else {
        detectedBattery = BATTERY_60AH;
      }
      break;
    case 0x432:  //BMS [200ms] SOC% info
      battery_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery_display_SOC = rx_frame.data.u8[4];
      break;
    case 0x507:  //BMS [640ms] Network Management - 2 - This message is sent on the bus for sleep coordination purposes
      break;
    case 0x587:  //BMS [5s] Services
      battery_ID2 = rx_frame.data.u8[0];
      break;
    case 0x607:  //BMS - responses to message requests on 0x615
      if ((cmdState == CELL_VOLTAGE_CELLNO || cmdState == CELL_VOLTAGE_CELLNO_LAST) && (rx_frame.data.u8[0] == 0xF4)) {
        if (rx_frame.DLC == 6) {
          transmit_can_frame(&BMW_6F4_CELL_CONTINUE);  // tell battery to send the cellvoltage
        }
        if (rx_frame.DLC == 8) {  // We have the full value, map it
          datalayer_battery->status.cell_voltages_mV[current_cell_polled - 1] =
              (rx_frame.data.u8[6] << 8 | rx_frame.data.u8[7]);
        }
      }

      if (rx_frame.DLC > 6 && next_data == 0 && rx_frame.data.u8[0] == 0xf1) {
        uint8_t count = 6;
        while (count < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count++];
        }
        transmit_can_frame(&BMW_6F1_CONTINUE);  // tell battery to send additional messages

      } else if (rx_frame.DLC > 3 && next_data > 0 && rx_frame.data.u8[0] == 0xf1 &&
                 ((rx_frame.data.u8[1] & 0xF0) == 0x20)) {
        uint8_t count = 2;
        while (count < rx_frame.DLC && next_data < 49) {
          message_data[next_data++] = rx_frame.data.u8[count++];
        }

        switch (cmdState) {
          case CELL_VOLTAGE_MINMAX:
            if (next_data >= 4) {
              cellvoltage_temp_mV = (message_data[0] << 8 | message_data[1]);
              if (cellvoltage_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer_battery->status.cell_min_voltage_mV = cellvoltage_temp_mV;
              }
              cellvoltage_temp_mV = (message_data[2] << 8 | message_data[3]);
              if (cellvoltage_temp_mV < 4500) {  // Prevents garbage data from being read on bootup
                datalayer_battery->status.cell_max_voltage_mV = cellvoltage_temp_mV;
              }
            }
            break;
          case SOH:
            if (next_data >= 4) {
              battery_soh = message_data[3];
              battery_info_available = true;
            }
            break;
          case SOC:
            if (next_data >= 6) {
              battery_soc = (message_data[0] << 8 | message_data[1]);
              battery_soc_hvmax = (message_data[2] << 8 | message_data[3]);
              battery_soc_hvmin = (message_data[4] << 8 | message_data[5]);
            }
            break;
          default:
            break;
        }
      }
      break;
    default:
      break;
  }
}

void BmwI3Battery::transmit_can(unsigned long currentMillis) {

  if (battery_awake) {
    //Send 20ms message
    if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
      previousMillis20 = currentMillis;

      if (startup_counter_contactor < 160) {
        startup_counter_contactor++;
      } else {                      //After 160 messages, turn on the request
        BMW_10B.data.u8[1] = 0x10;  // Close contactors
      }

      BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
      BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

      alive_counter_20ms = increment_alive_counter(alive_counter_20ms);

      BMW_13E_counter++;
      BMW_13E.data.u8[4] = BMW_13E_counter;

      if (datalayer_battery->status.bms_status == FAULT) {
      } else if (allows_contactor_closing) {
        //If battery is not in Fault mode, and we are allowed to control contactors, we allow contactor to close by sending 10B
        *allows_contactor_closing = true;
        transmit_can_frame(&BMW_10B);
      } else if (contactor_closing_allowed && *contactor_closing_allowed) {
        transmit_can_frame(&BMW_10B);
      }
    }

    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      BMW_12F.data.u8[1] = ((BMW_12F.data.u8[1] & 0xF0) + alive_counter_100ms);
      BMW_12F.data.u8[0] = calculateCRC(BMW_12F, 8, 0x60);

      alive_counter_100ms = increment_alive_counter(alive_counter_100ms);

      transmit_can_frame(&BMW_12F);
    }
    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;

      BMW_19B.data.u8[1] = ((BMW_19B.data.u8[1] & 0xF0) + alive_counter_200ms);
      BMW_19B.data.u8[0] = calculateCRC(BMW_19B, 8, 0x6C);

      alive_counter_200ms = increment_alive_counter(alive_counter_200ms);

      transmit_can_frame(&BMW_19B);
    }
    // Send 500ms CAN Message
    if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
      previousMillis500 = currentMillis;

      BMW_30B.data.u8[1] = ((BMW_30B.data.u8[1] & 0xF0) + alive_counter_500ms);
      BMW_30B.data.u8[0] = calculateCRC(BMW_30B, 8, 0xBE);

      alive_counter_500ms = increment_alive_counter(alive_counter_500ms);

      transmit_can_frame(&BMW_30B);
    }
    // Send 640ms CAN Message
    if (currentMillis - previousMillis640 >= INTERVAL_640_MS) {
      previousMillis640 = currentMillis;

      transmit_can_frame(&BMW_512);  // Keep BMS alive
      transmit_can_frame(&BMW_5F8);
    }
    // Send 1000ms CAN Message
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;
      //BMW_328 byte0-3 = Second Counter (T_SEC_COU_REL) time_second_counter_relative
      // This signal shows the time in seconds since the system time was started (typically in the factory)
      BMW_328_seconds++;  // Used to increment seconds
      BMW_328.data.u8[0] = (uint8_t)(BMW_328_seconds & 0xFF);
      BMW_328.data.u8[1] = (uint8_t)((BMW_328_seconds >> 8) & 0xFF);
      BMW_328.data.u8[2] = (uint8_t)((BMW_328_seconds >> 16) & 0xFF);
      BMW_328.data.u8[3] = (uint8_t)((BMW_328_seconds >> 24) & 0xFF);
      //BMW_328 byte 4-5 = Day Counter (T_DAY_COU_ABSL) time_day_counter_absolute
      //This value goes from 1 ... 65543
      // Day 1 = 1.1.2000 ... Day 65543 = year 2179
      BMS_328_seconds_to_day++;
      if (BMS_328_seconds_to_day > 86400) {
        BMW_328_days++;
        BMS_328_seconds_to_day = 0;
      }
      BMW_328.data.u8[4] = (uint8_t)(BMW_328_days & 0xFF);
      BMW_328.data.u8[5] = (uint8_t)((BMW_328_days >> 8) & 0xFF);

      BMW_1D0.data.u8[1] = ((BMW_1D0.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_1D0.data.u8[0] = calculateCRC(BMW_1D0, 8, 0xF9);

      BMW_3F9.data.u8[1] = ((BMW_3F9.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3F9.data.u8[0] = calculateCRC(BMW_3F9, 8, 0x38);

      BMW_3EC.data.u8[1] = ((BMW_3EC.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3EC.data.u8[0] = calculateCRC(BMW_3EC, 8, 0x53);

      BMW_3A7.data.u8[1] = ((BMW_3A7.data.u8[1] & 0xF0) + alive_counter_1000ms);
      BMW_3A7.data.u8[0] = calculateCRC(BMW_3A7, 8, 0x05);

      alive_counter_1000ms = increment_alive_counter(alive_counter_1000ms);

      transmit_can_frame(&BMW_3E8);  //Order comes from CAN logs
      transmit_can_frame(&BMW_328);
      transmit_can_frame(&BMW_3F9);
      transmit_can_frame(&BMW_2E2);
      transmit_can_frame(&BMW_41D);
      transmit_can_frame(&BMW_3D0);
      transmit_can_frame(&BMW_3CA);
      transmit_can_frame(&BMW_3A7);
      transmit_can_frame(&BMW_2CA);
      transmit_can_frame(&BMW_3FB);
      transmit_can_frame(&BMW_418);
      transmit_can_frame(&BMW_1D0);
      transmit_can_frame(&BMW_3EC);
      transmit_can_frame(&BMW_192);
      transmit_can_frame(&BMW_13E);
      transmit_can_frame(&BMW_433);

      BMW_433.data.u8[1] = 0x01;  // First 433 message byte1 we send is unique, once we sent initial value send this
      BMW_3E8.data.u8[0] = 0xF1;  // First 3E8 message byte0 we send is unique, once we sent initial value send this

      if (UserRequestDTCreset) {
        cmdState = CLEAR_DTC;
        UserRequestDTCreset = false;
      }

      next_data = 0;
      switch (cmdState) {
        case SOC:
          transmit_can_frame(&BMW_6F1_CELL);
          cmdState = CELL_VOLTAGE_MINMAX;
          break;
        case CELL_VOLTAGE_MINMAX:
          transmit_can_frame(&BMW_6F1_SOH);
          cmdState = SOH;
          break;
        case SOH:
          transmit_can_frame(&BMW_6F1_CELL_VOLTAGE_AVG);
          cmdState = CELL_VOLTAGE_CELLNO;
          current_cell_polled = 0;

          break;
        case CELL_VOLTAGE_CELLNO:
          current_cell_polled++;
          if (current_cell_polled > 96) {
            cmdState = CELL_VOLTAGE_CELLNO_LAST;
          } else {
            cmdState = CELL_VOLTAGE_CELLNO;

            BMW_6F4_CELL_VOLTAGE_CELLNO.data.u8[6] = current_cell_polled;
            transmit_can_frame(&BMW_6F4_CELL_VOLTAGE_CELLNO);
          }
          break;
        case CELL_VOLTAGE_CELLNO_LAST:
          transmit_can_frame(&BMW_6F1_SOC);
          cmdState = SOC;
          break;
        case CLEAR_DTC:
          transmit_can_frame(&BMW_6F1_CLEAR_DTC);
          cmdState = SOC;  //jump back to normal polling
          break;
        default:
          //Should never end up here
          cmdState = SOC;
          break;
      }
    }
    // Send 5000ms CAN Message
    if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
      previousMillis5000 = currentMillis;

      BMW_3FC.data.u8[1] = ((BMW_3FC.data.u8[1] & 0xF0) + alive_counter_5000ms);
      BMW_3C5.data.u8[0] = ((BMW_3C5.data.u8[0] & 0xF0) + alive_counter_5000ms);

      transmit_can_frame(&BMW_3FC);  //Order comes from CAN logs
      transmit_can_frame(&BMW_3C5);
      transmit_can_frame(&BMW_3A0);
      transmit_can_frame(&BMW_592_0);
      transmit_can_frame(&BMW_592_1);

      alive_counter_5000ms = increment_alive_counter(alive_counter_5000ms);

      if (BMW_380_counter < 3) {
        transmit_can_frame(&BMW_380);  // This message stops after 3 times on startup
        BMW_380_counter++;
      }
    }
    // Send 10000ms CAN Message
    if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
      previousMillis10000 = currentMillis;

      transmit_can_frame(&BMW_3E5);  //Order comes from CAN logs
      transmit_can_frame(&BMW_3E4);
      transmit_can_frame(&BMW_37B);

      BMW_3E5.data.u8[0] = 0xFD;  // First 3E5 message byte0 we send is unique, once we sent initial value send this
    }
  } else {
    previousMillis20 = currentMillis;
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    previousMillis500 = currentMillis;
    previousMillis640 = currentMillis;
    previousMillis1000 = currentMillis;
    previousMillis5000 = currentMillis;
    previousMillis10000 = currentMillis;
  }
}

void BmwI3Battery::setup(void) {  // Performs one time setup at startup
  if (!esp32hal->alloc_pins(Name, wakeup_pin)) {
    return;
  }

  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //Before we have started up and detected which battery is in use, use 60AH values
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_60AH;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60AH;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
  datalayer_battery->info.number_of_cells = NUMBER_OF_CELLS;

  pinMode(wakeup_pin, OUTPUT);
  digitalWrite(wakeup_pin, LOW);  // Set pin to low, prepare to wakeup later on!
}
