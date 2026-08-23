#include "RENAULT-ZOE-GEN2-BATTERY.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

/* TODO
- Add //NVROL Reset
- Add //Enable temporisation before sleep (see ljames28 repo)

"If the pack is in a state where it is confused about the time, you may need to reset it's NVROL memory. 
However, if the power is later power cycled, it will revert back to his previous confused state. 
Therefore, after resetting the NVROL you must enable "temporisation before sleep", and then stop streaming 373. 
It will then save the data and go to sleep. When the pack is confused, the state of charge may reset back to incorrect value 
every time the power is reset which can be dangerous. In this state, the voltage will still be accurate"
*/

/* Information in this file is based on:
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe_ph2_obd/src/vehicle_renaultzoe_ph2_obd.cpp
https://github.com/ljames28/Renault-Zoe-PH2-ZE50-Canbus-LBC-Information?tab=readme-ov-file
https://github.com/fesch/CanZE/tree/master/app/src/main/assets/ZOE_Ph2
*/

uint8_t RenaultZoeGen2Battery::calculate_crc_zoe(CAN_frame& rx_frame, uint8_t crc_xor) {
  uint8_t crc = 0;  //init value 0x00
  for (uint8_t j = 0; j < 7; j++) {
    crc = crc8_table_SAE_J1850_ZER0[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) & 0xFF];
  }
  return crc ^ crc_xor;
}

bool RenaultZoeGen2Battery::is_message_corrupt(CAN_frame rx_frame, uint8_t crc_xor) {
  uint8_t crc = calculate_crc_zoe(rx_frame, crc_xor);
  return crc != rx_frame.data.u8[7];
}

void RenaultZoeGen2Battery::update_values() {

  datalayer_battery->status.soh_pptt = battery_soh;

  if (battery_soc >= 300) {
    datalayer_battery->status.real_soc = battery_soc - 300;
  } else {
    datalayer_battery->status.real_soc = 0;
  }

  if (battery_pack_voltage_periodic_dV < 5000) {  //If periodic value is available, use it!
    datalayer_battery->status.voltage_dV = battery_pack_voltage_periodic_dV;
  } else {  //Fallback on polled value if periodic value is not available. This is a workaround for some batteries that do not send periodic voltage updates
    datalayer_battery->status.voltage_dV = battery_pack_voltage_polled_dV;
  }

  datalayer_battery->status.current_dA = ((battery_current - 32640) * 0.3125f);

  //Calculate the total Wh amount from SOH%
  datalayer_battery->info.total_capacity_Wh = 52000 * (datalayer_battery->status.soh_pptt / 10000.0);

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_discharge_power_W = battery_max_available * 10;

  datalayer_battery->status.max_charge_power_W = battery_max_generated * 10;

  //Temperatures and voltages update at slow rate. Only publish new values once both have been sampled to avoid events
  if ((battery_min_temp != 920) && (battery_max_temp != 920)) {
    datalayer_battery->status.temperature_min_dC = ((battery_min_temp - 640) * 0.625f);
    datalayer_battery->status.temperature_max_dC = ((battery_max_temp - 640) * 0.625f);
  }

  if (battery_minimum_cell_voltage_mV < 4400) {  // Value is initialized large for some reason
    datalayer_battery->status.cell_min_voltage_mV = battery_minimum_cell_voltage_mV;
  }
  if (battery_maximum_cell_voltage_mV < 4400) {  // Value is initialized large for some reason
    datalayer_battery->status.cell_max_voltage_mV = battery_maximum_cell_voltage_mV;
  }

  if (battery_12v < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, battery_12v);
  }

  if (battery_interlock != 0xFFFE) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  for (int i = 0; i < 96; i++) {
    //balancing_status_cell has cells ordered 96-1, while datalayer_battery->status.cell_balancing_status has cells ordered 1-96
    //Due to this we need to invert the index when writing to datalayer_battery->status.cell_balancing_status
    datalayer_battery->status.cell_balancing_status[95 - i] = balancing_status_cell[i];
    if (balancing_status_cell[i]) {
      set_event_latched(EVENT_BALANCING_START, (95 - i));
      datalayer_battery->status.balancing_status = BALANCING_STATUS_ACTIVE;
    }
  }

  /* Removed until we have a way to clear failures
  if (battery_slave_failures > 0) {
    set_event(EVENT_BATTERY_CAUTION, 0);
  } else {
    clear_event(EVENT_BATTERY_CAUTION);
  }*/
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String RenaultZoeGen2Battery::get_uds_info_html() {
  String content;
  content.reserve(1600);

  // clang-format off
  content << "<h4>SoC: "                    << battery_soc                    << " pptt</h4>"
             "<h4>usable SoC: "             << battery_usable_soc             << " pptt</h4>"
             "<h4>SoH: "                    << battery_soh                    << " pptt</h4>"
             "<h4>Pack voltage: "           << battery_pack_voltage_polled_dV << " dV</h4>"
             "<h4>Max cell voltage: "       << battery_max_cell_voltage_polled<< " mV</h4>"
             "<h4>Min cell voltage: "       << battery_min_cell_voltage_polled<< " mV</h4>"
             "<h4>12v: "                    << battery_12v                    << " mV</h4>"
             "<h4>Avg temp: "               << battery_avg_temp               << "</h4>"
             "<h4>Min temp: "               << battery_min_temp               << "</h4>"
             "<h4>Max temp: "               << battery_max_temp               << "</h4>"
             "<h4>Max power: "              << battery_max_power              << "</h4>"
             "<h4>Interlock: "              << battery_interlock              << "</h4>"
             "<h4>kWh: "                    << battery_kwh                    << "</h4>"
             "<h4>Current: "               << battery_current                << "</h4>"
             "<h4>Current offset: "         << battery_current_offset         << "</h4>"
             "<h4>Max generated: "          << battery_max_generated          << "</h4>"
             "<h4>Max available: "          << battery_max_available          << "</h4>"
             "<h4>Current voltage: "        << battery_current_voltage        << "</h4>"
             "<h4>Charging status: "        << battery_charging_status        << "</h4>"
             "<h4>Remaining charge: "       << battery_remaining_charge       << "</h4>"
             "<h4>Balance capacity total: " << battery_balance_capacity_total << "</h4>"
             "<h4>Balance time total: "     << battery_balance_time_total     << "</h4>"
             "<h4>Balance capacity sleep: " << battery_balance_capacity_sleep << "</h4>"
             "<h4>Balance time sleep: "     << battery_balance_time_sleep     << "</h4>"
             "<h4>Balance capacity wake: "  << battery_balance_capacity_wake  << "</h4>"
             "<h4>Balance time wake: "      << battery_balance_time_wake      << "</h4>"
             "<h4>BMS state: "              << battery_bms_state              << "</h4>"
             "<h4>Energy complete: "        << battery_energy_complete        << "</h4>"
             "<h4>Energy partial: "         << battery_energy_partial         << "</h4>"
             "<h4>Slave failures: "         << battery_slave_failures         << "</h4>"
             "<h4>Mileage: "                << battery_mileage                << "</h4>"
             "<h4>Fan speed: "              << battery_fan_speed              << "</h4>"
             "<h4>Fan period: "             << battery_fan_period             << "</h4>"
             "<h4>Fan control: "            << battery_fan_control            << "</h4>"
             "<h4>Fan duty: "               << battery_fan_duty               << "</h4>"
             "<h4>Time: "                   << battery_time                   << "</h4>"
             "<h4>Pack time: "              << battery_pack_time              << "</h4>"
             "<h4>SoC min: "                << battery_soc_min                << "</h4>"
             "<h4>SoC max: "                << battery_soc_max                << "</h4>";

  content << "<h4>Temporisation: ";
  if      (battery_temporisation == 255) content << "Not read yet";
  else if (battery_temporisation == 0)   content << "0 Activated!";
  else if (battery_temporisation == 1)   content << "1 Disabled!";
  else                                     content << battery_temporisation;
  content << "</h4>";
  // clang-format on

  return content;
}

void RenaultZoeGen2Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x0F8:
      //Filter out the first 50 messages to avoid false positives on startup. The battery sends a few messages with wrong data on startup.
      startup_counter++;
      if (startup_counter >= 50) {
        startup_counter = 50;
        datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
        battery_interlock = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];  //Expected FF FE
        battery_pack_voltage_periodic_dV = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) / 8;
        //battery_pack_current_periodic_dA = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 8; //4-5-6 current related
      }
      break;
    case 0x381:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //frame0 - 373 Related
      //frame1 - 373 Related
      //frame2 - Maximum_Available_Power_related
      //frame3 - Maximum_Available_Power_related/was charge complete or partial
      //frame4 - max power/SOC_related
      //frame5-6 -  SOC_related
      //frame7 - Unknown status
      break;
    case 0x382:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //Frame0-2 Max gen power
      //frame6 cooling temp OK
      //frame7 max temp OK
      break;
    case 0x387:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x388:  //Blower/Cooling/Maxpower
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3EF:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x36C:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (is_message_corrupt(rx_frame, 0x01)) {
        datalayer_battery->status.CAN_error_counter++;
      }
      break;
    case 0x4DB:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_maximum_cell_voltage_mV = ((rx_frame.data.u8[0] << 4) | (rx_frame.data.u8[1] & 0xF0) >> 4) + 1000;
      battery_minimum_cell_voltage_mV = (((rx_frame.data.u8[1] & 0x0F) << 8) | (rx_frame.data.u8[2])) + 1000;
      break;
    case 0x4AE:
    case 0x4AF:
    case 0x5A1:
    case 0x5AC:
    case 0x5AD:
    case 0x5B4:
    case 0x5B5:
    case 0x5B7:
    case 0x5C9:
    case 0x5CB:
    case 0x5CC:
    case 0x5D6:
    case 0x5D7:
    case 0x5D9:
    case 0x5DC:
    case 0x5DD:
    case 0x5EA:
    case 0x5ED:
    case 0x5F0:
    case 0x5F1:
    case 0x5F2:
    case 0x5F4:
    case 0x5F7:
    case 0x612:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x18DAF1DB:  // LBC Reply from active polling
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // Hand the reply to the UDS superclass: ISO-TP reassembly, then handle_pid()
      // for PID scan responses and the DTC handlers for the rest.
      handle_incoming_uds_can_frame(rx_frame);
      break;
    default:
      break;
  }
}

uint16_t RenaultZoeGen2Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case POLL_SOC:
      battery_soc = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_USABLE_SOC:
      battery_usable_soc = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_SOH:
      battery_soh = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_PACK_VOLTAGE:
      battery_pack_voltage_polled_dV = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MAX_CELL_VOLTAGE:
      temporary_variable = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (temporary_variable > 500) {          //Disregard messages with value unavailable
        battery_max_cell_voltage_polled = temporary_variable;
      }
      break;
    case POLL_MIN_CELL_VOLTAGE:
      temporary_variable = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      if (temporary_variable > 500) {          //Disregard messages with value unavailable
        battery_min_cell_voltage_polled = temporary_variable;
      }
      break;
    case POLL_12V:
      battery_12v = ((uint16_t)(value)) +
                    350;  //((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 350;  //350, calibration from testing
      break;
    case POLL_AVG_TEMP:
      battery_avg_temp = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MIN_TEMP:
      battery_min_temp = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MAX_TEMP:
      battery_max_temp = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MAX_POWER:
      battery_max_power = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_INTERLOCK:
      battery_interlock_polled = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_KWH:
      battery_kwh = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_CURRENT:
      battery_current = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_CURRENT_OFFSET:
      battery_current_offset = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MAX_GENERATED:
      battery_max_generated = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_MAX_AVAILABLE:
      battery_max_available = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_CURRENT_VOLTAGE:
      battery_current_voltage = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_CHARGING_STATUS:
      battery_charging_status = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_REMAINING_CHARGE:
      battery_remaining_charge = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_CAPACITY_TOTAL:
      battery_balance_capacity_total = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_TIME_TOTAL:
      battery_balance_time_total = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_CAPACITY_SLEEP:
      battery_balance_capacity_sleep = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_TIME_SLEEP:
      battery_balance_time_sleep = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_CAPACITY_WAKE:
      battery_balance_capacity_wake = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_TIME_WAKE:
      battery_balance_time_wake = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BMS_STATE:
      battery_bms_state = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_BALANCE_SWITCHES:
      // TODO: HOW TO HANDLE THIS?
      /*
          if (rx_frame.data.u8[0] == 0x23) {
            for (int i = 0; i < 32; i++) {
              balancing_status_cell[i] = (rx_frame.data.u8[4 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
          if (rx_frame.data.u8[0] == 0x24) {
            for (int i = 0; i < 56; i++) {
              balancing_status_cell[32 + i] = (rx_frame.data.u8[1 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
          if (rx_frame.data.u8[0] == 0x25) {
            for (int i = 0; i < 8; i++) {
              balancing_status_cell[88 + i] = (rx_frame.data.u8[1 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
            */
      break;
    case POLL_ENERGY_COMPLETE:
      battery_energy_complete = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_ENERGY_PARTIAL:
      battery_energy_partial = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_SLAVE_FAILURES:
      battery_slave_failures = value;
      break;
    case POLL_MILEAGE:
      battery_mileage = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_FAN_SPEED:
      battery_fan_speed = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_FAN_PERIOD:
      battery_fan_period = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_FAN_CONTROL:
      battery_fan_control = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_FAN_DUTY:
      battery_fan_duty = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_TEMPORISATION:
      battery_temporisation = (bool)(value);  //rx_frame.data.u8[4] >> 7;
      break;
    case POLL_TIME:
      battery_time = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_PACK_TIME:
      battery_pack_time = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_SOC_MIN:
      battery_soc_min = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case POLL_SOC_MAX:
      battery_soc_max = (uint16_t)(value);  //(rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    default:
      // Handle cell voltages
      if (pid >= POLL_CELL_0 && pid <= POLL_CELL_95) {
        int cell_index = pid - POLL_CELL_0;

        // Three offsets are skipped in the polling sequence, account for that.
        if (pid > POLL_CELL_30) {
          cell_index -= 1;  // Account for missing 0x9040
        }
        if (pid > POLL_CELL_61) {
          cell_index -= 1;  // Account for missing 0x9060
        }
        if (pid > POLL_CELL_92) {
          cell_index -= 1;  // Account for missing 0x9080
        }

        datalayer_battery->status.cell_voltages_mV[cell_index] = (uint16_t)((value) * 0.976563);
      }
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void RenaultZoeGen2Battery::transmit_can(unsigned long currentMillis) {
  if (UserRequestNVROLReset) {
    // Send NVROL reset frames
    transmit_reset_nvrol_frames();
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    counter_10ms = (counter_10ms + 1) % 16;

    ZOE_0EE.data.u8[6] = counter_10ms;
    ZOE_0EE.data.u8[7] = calculate_crc_zoe(ZOE_0EE, 0xAC);

    transmit_can_frame(&ZOE_0EE);  //Pedal position
    //transmit_can_frame(&ZOE_133);  //Vehicle speed (CRC is frame3 B1A670 55 0006FFFF)
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    ZOE_373.data.u8[1] = 0x40;  //40 vehicle locked, 80 vehicle unlocked

    if ((counter_373 / 5) % 2 == 0) {  // Alternate every 5 messages between these two patterns
      ZOE_373.data.u8[2] = 0xB2;
      ZOE_373.data.u8[3] = 0x5D;
    } else {
      ZOE_373.data.u8[2] = 0x5D;
      ZOE_373.data.u8[3] = 0xB2;
    }
    counter_373 = (counter_373 + 1) % 10;

    transmit_can_frame(&ZOE_373);  //HEVC Wakeup / Sleep message
    transmit_can_frame(&ZOE_375);  //HEVC Status message
    transmit_can_frame_376();      //HEVC Time and Date
  }

  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    // Time in seconds emulated
    ZOE_376_time_now_s++;  // Increment by 1 second

    transmit_can_frame(&ZOE_5F8);  //Vehicle ID
    transmit_can_frame(&ZOE_6BF);  //Total Boost Time
  }

  if (!UserRequestNVROLReset) {
    // UDS PID polling and DTC handling (Only if not NVROL reset in progress)
    transmit_uds_can(currentMillis);
  }
}

void RenaultZoeGen2Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.total_capacity_Wh = 52000;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer_battery->status.balancing_status = BALANCING_STATUS_READY;

  // UDS: send requests to 0x18DADBF1, accept replies from the BMS on 0x18DAF1DB.
  setup_uds(0x18DADBF1, 0x18DAF1DB);
  static const uint16_t pid_scan_list[] = {
      POLL_SOC,
      POLL_USABLE_SOC,
      POLL_SOH,
      POLL_PACK_VOLTAGE,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_MAX_CELL_VOLTAGE,
      POLL_MIN_CELL_VOLTAGE,
      POLL_12V,
      POLL_AVG_TEMP,
      POLL_MIN_TEMP,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_MAX_TEMP,
      POLL_MAX_POWER,
      POLL_INTERLOCK,
      POLL_KWH,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CURRENT_OFFSET,
      POLL_MAX_GENERATED,
      POLL_MAX_AVAILABLE,
      POLL_CURRENT_VOLTAGE,
      POLL_CHARGING_STATUS,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_REMAINING_CHARGE,
      POLL_BALANCE_CAPACITY_TOTAL,
      POLL_BALANCE_TIME_TOTAL,
      POLL_BALANCE_CAPACITY_SLEEP,
      POLL_BALANCE_TIME_SLEEP,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_BALANCE_CAPACITY_WAKE,
      POLL_BALANCE_TIME_WAKE,
      POLL_BMS_STATE,
      POLL_BALANCE_SWITCHES,
      POLL_ENERGY_COMPLETE,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_ENERGY_PARTIAL,
      POLL_SLAVE_FAILURES,
      POLL_MILEAGE,
      POLL_FAN_SPEED,
      POLL_FAN_PERIOD,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_FAN_CONTROL,
      POLL_FAN_DUTY,
      POLL_TEMPORISATION,
      POLL_TIME,
      POLL_PACK_TIME,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_SOC_MIN,
      POLL_SOC_MAX,
      POLL_CELL_0,
      POLL_CELL_1,
      POLL_CELL_2,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_3,
      POLL_CELL_4,
      POLL_CELL_5,
      POLL_CELL_6,
      POLL_CELL_7,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_8,
      POLL_CELL_9,
      POLL_CELL_10,
      POLL_CELL_11,
      POLL_CELL_12,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_13,
      POLL_CELL_14,
      POLL_CELL_15,
      POLL_CELL_16,
      POLL_CELL_17,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_18,
      POLL_CELL_19,
      POLL_CELL_20,
      POLL_CELL_21,
      POLL_CELL_22,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_23,
      POLL_CELL_24,
      POLL_CELL_25,
      POLL_CELL_26,
      POLL_CELL_27,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_28,
      POLL_CELL_29,
      POLL_CELL_30,
      POLL_CELL_31,
      POLL_CELL_32,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_33,
      POLL_CELL_34,
      POLL_CELL_35,
      POLL_CELL_36,
      POLL_CELL_37,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_38,
      POLL_CELL_39,
      POLL_CELL_40,
      POLL_CELL_41,
      POLL_CELL_42,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_43,
      POLL_CELL_44,
      POLL_CELL_45,
      POLL_CELL_46,
      POLL_CELL_47,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_48,
      POLL_CELL_49,
      POLL_CELL_50,
      POLL_CELL_51,
      POLL_CELL_52,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_53,
      POLL_CELL_54,
      POLL_CELL_55,
      POLL_CELL_56,
      POLL_CELL_57,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_58,
      POLL_CELL_59,
      POLL_CELL_60,
      POLL_CELL_61,
      POLL_CELL_62,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_63,
      POLL_CELL_64,
      POLL_CELL_65,
      POLL_CELL_66,
      POLL_CELL_67,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_68,
      POLL_CELL_69,
      POLL_CELL_70,
      POLL_CELL_71,
      POLL_CELL_72,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_73,
      POLL_CELL_74,
      POLL_CELL_75,
      POLL_CELL_76,
      POLL_CELL_77,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_78,
      POLL_CELL_79,
      POLL_CELL_80,
      POLL_CELL_81,
      POLL_CELL_82,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_83,
      POLL_CELL_84,
      POLL_CELL_85,
      POLL_CELL_86,
      POLL_CELL_87,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_88,
      POLL_CELL_89,
      POLL_CELL_90,
      POLL_CELL_91,
      POLL_CELL_92,
      POLL_CURRENT,  //Repeated to speed up update rate on this critical measurement
      POLL_CELL_93,
      POLL_CELL_94,
      POLL_CELL_95,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}

void RenaultZoeGen2Battery::transmit_can_frame_376(void) {
  unsigned int secondsSinceProduction = ZOE_376_time_now_s - kProductionTimestamp_s;
  float minutesSinceProduction = (float)secondsSinceProduction / 60.0f;
  float yearUnfloored = minutesSinceProduction / 255.0f / 255.0f;
  int yearSeg = floor(yearUnfloored);
  float remainderYears = yearUnfloored - yearSeg;
  float remainderHoursUnfloored = (remainderYears * 255.0f);
  int hourSeg = floor(remainderHoursUnfloored);
  float remainderHours = remainderHoursUnfloored - hourSeg;
  int minuteSeg = floor(remainderHours * 255.0f);

  ZOE_376.data.u8[0] = yearSeg;
  ZOE_376.data.u8[1] = hourSeg;
  ZOE_376.data.u8[2] = minuteSeg;
  ZOE_376.data.u8[3] = yearSeg;
  ZOE_376.data.u8[4] = hourSeg;
  ZOE_376.data.u8[5] = minuteSeg;

  transmit_can_frame(&ZOE_376);
}

void RenaultZoeGen2Battery::transmit_reset_nvrol_frames(void) {
  switch (NVROLstateMachine) {
    case 0:
      startTimeNVROL = millis();
      // NVROL reset, part 1: send 0x021003AAAAAAAAAA
      ZOE_UDS_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
      transmit_can_frame(&ZOE_UDS_18DADBF1);
      NVROLstateMachine = 1;
      break;
    case 1:  // wait 100 ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // NVROL reset, part 2: send 0x043101B00900AAAA
        ZOE_UDS_18DADBF1.data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_UDS_18DADBF1);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 2;
      }
      break;
    case 2:  // wait 1 s
      if ((millis() - startTimeNVROL) > INTERVAL_1_S) {
        // Enable temporisation before sleep, part 1: send 0x021003AAAAAAAAAA
        ZOE_UDS_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_UDS_18DADBF1);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 3;
      }
      break;
    case 3:  //Wait 100ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // Enable temporisation before sleep, part 2: send 0x042E928101AAAAAA
        ZOE_UDS_18DADBF1.data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_UDS_18DADBF1);
        NVROLstateMachine = 4;
      }
      break;
    case 4:  //Wait 30s
      //While waiting, stop streaming 0x373 to make battery go to sleep
      ZOE_373.data.u8[0] = 0x01;
      if ((millis() - startTimeNVROL) > INTERVAL_30_S) {
        // after sleeping, set the nvrol reset flag to false, to continue normal operation of sending CAN messages
        UserRequestNVROLReset = false;
        // Wake battery back up
        ZOE_373.data.u8[0] = 0xC1;
        // reset state machine, we are done!
        NVROLstateMachine = 0;
      }
      break;
    default:  //Something went catastrophically wrong. Reset state machine
      NVROLstateMachine = 0;
      break;
  }
}
