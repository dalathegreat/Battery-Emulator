#include "RENAULT-ZOE-GEN2-BATTERY.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
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
    crc = crctable[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) & 0xFF];
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

  datalayer_battery->status.voltage_dV = battery_pack_voltage_periodic_dV;

  datalayer_battery->status.current_dA = ((battery_current - 32640) * 0.3125f);

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

  datalayer_battery->status.cell_min_voltage_mV = battery_minimum_cell_voltage_mV;
  datalayer_battery->status.cell_max_voltage_mV = battery_maximum_cell_voltage_mV;

  if (battery_12v < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, battery_12v);
  }

  if (battery_interlock != 0xFFFE) {
    set_event(EVENT_HVIL_FAILURE, 0);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  for (int i = 0; i < 96; i++) {
    if (datalayer_battery->status.cell_balancing_status[i]) {
      set_event_latched(EVENT_BALANCING_START, datalayer_battery->status.cell_balancing_status[i]);
    }
  }

  // Update webserver datalayer
  datalayer_extended.zoePH2.battery_soc = battery_soc;
  datalayer_extended.zoePH2.battery_usable_soc = battery_usable_soc;
  datalayer_extended.zoePH2.battery_soh = battery_soh;
  datalayer_extended.zoePH2.battery_pack_voltage = battery_pack_voltage_polled_dV;
  datalayer_extended.zoePH2.battery_max_cell_voltage = battery_max_cell_voltage_polled;
  datalayer_extended.zoePH2.battery_min_cell_voltage = battery_min_cell_voltage_polled;
  datalayer_extended.zoePH2.battery_12v = battery_12v;
  datalayer_extended.zoePH2.battery_avg_temp = battery_avg_temp;
  datalayer_extended.zoePH2.battery_min_temp = battery_min_temp;
  datalayer_extended.zoePH2.battery_max_temp = battery_max_temp;
  datalayer_extended.zoePH2.battery_max_power = battery_max_power;
  datalayer_extended.zoePH2.battery_interlock = battery_interlock_polled;
  datalayer_extended.zoePH2.battery_kwh = battery_kwh;
  datalayer_extended.zoePH2.battery_current = battery_current;
  datalayer_extended.zoePH2.battery_current_offset = battery_current_offset;
  datalayer_extended.zoePH2.battery_max_generated = battery_max_generated;
  datalayer_extended.zoePH2.battery_max_available = battery_max_available;
  datalayer_extended.zoePH2.battery_current_voltage = battery_current_voltage;
  datalayer_extended.zoePH2.battery_charging_status = battery_charging_status;
  datalayer_extended.zoePH2.battery_remaining_charge = battery_remaining_charge;
  datalayer_extended.zoePH2.battery_balance_capacity_total = battery_balance_capacity_total;
  datalayer_extended.zoePH2.battery_balance_time_total = battery_balance_time_total;
  datalayer_extended.zoePH2.battery_balance_capacity_sleep = battery_balance_capacity_sleep;
  datalayer_extended.zoePH2.battery_balance_time_sleep = battery_balance_time_sleep;
  datalayer_extended.zoePH2.battery_balance_capacity_wake = battery_balance_capacity_wake;
  datalayer_extended.zoePH2.battery_balance_time_wake = battery_balance_time_wake;
  datalayer_extended.zoePH2.battery_bms_state = battery_bms_state;
  datalayer_extended.zoePH2.battery_energy_complete = battery_energy_complete;
  datalayer_extended.zoePH2.battery_energy_partial = battery_energy_partial;
  datalayer_extended.zoePH2.battery_slave_failures = battery_slave_failures;
  datalayer_extended.zoePH2.battery_mileage = battery_mileage;
  datalayer_extended.zoePH2.battery_fan_speed = battery_fan_speed;
  datalayer_extended.zoePH2.battery_fan_period = battery_fan_period;
  datalayer_extended.zoePH2.battery_fan_control = battery_fan_control;
  datalayer_extended.zoePH2.battery_fan_duty = battery_fan_duty;
  datalayer_extended.zoePH2.battery_temporisation = battery_temporisation;
  datalayer_extended.zoePH2.battery_time = battery_time;
  datalayer_extended.zoePH2.battery_pack_time = battery_pack_time;
  datalayer_extended.zoePH2.battery_soc_min = battery_soc_min;
  datalayer_extended.zoePH2.battery_soc_max = battery_soc_max;
}

void RenaultZoeGen2Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x0F8:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_interlock = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];  //Expected FF FE
      battery_pack_voltage_periodic_dV = ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) / 8;
      //battery_pack_current_periodic_dA = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 8; //4-5-6 current related
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

      if (rx_frame.data.u8[0] == 0x10) {  //First frame of a group
        transmit_can_frame(&ZOE_POLL_FLOW_CONTROL);
        //frame 2 & 3 contains which PID is sent
        reply_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      if (rx_frame.data.u8[0] < 0x10) {  //One line responses
        reply_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      }

      switch (reply_poll) {
        case POLL_SOC:
          battery_soc = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_USABLE_SOC:
          battery_usable_soc = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOH:
          battery_soh = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_PACK_VOLTAGE:
          battery_pack_voltage_polled_dV = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_CELL_VOLTAGE:
          temporary_variable = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          if (temporary_variable > 500) {  //Disregard messages with value unavailable
            battery_max_cell_voltage_polled = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          }
          break;
        case POLL_MIN_CELL_VOLTAGE:
          temporary_variable = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          if (temporary_variable > 500) {  //Disregard messages with value unavailable
            battery_min_cell_voltage_polled = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          }
          break;
        case POLL_12V:
          battery_12v = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_AVG_TEMP:
          battery_avg_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_TEMP:
          battery_min_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_TEMP:
          battery_max_temp = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_POWER:
          battery_max_power = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_INTERLOCK:
          battery_interlock_polled = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_KWH:
          battery_kwh = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT:
          battery_current = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT_OFFSET:
          battery_current_offset = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_GENERATED:
          battery_max_generated = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_AVAILABLE:
          battery_max_available = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CURRENT_VOLTAGE:
          battery_current_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_CHARGING_STATUS:
          battery_charging_status = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_REMAINING_CHARGE:
          battery_remaining_charge = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_TOTAL:
          battery_balance_capacity_total = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_TOTAL:
          battery_balance_time_total = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_SLEEP:
          battery_balance_capacity_sleep = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_SLEEP:
          battery_balance_time_sleep = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_CAPACITY_WAKE:
          battery_balance_capacity_wake = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_TIME_WAKE:
          battery_balance_time_wake = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BMS_STATE:
          battery_bms_state = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_BALANCE_SWITCHES:
          if (rx_frame.data.u8[0] == 0x23) {
            for (int i = 0; i < 32; i++) {
              datalayer_battery->status.cell_balancing_status[i] =
                  (rx_frame.data.u8[4 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
          if (rx_frame.data.u8[0] == 0x24) {
            for (int i = 0; i < 56; i++) {
              datalayer_battery->status.cell_balancing_status[32 + i] =
                  (rx_frame.data.u8[1 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
          if (rx_frame.data.u8[0] == 0x25) {
            for (int i = 0; i < 8; i++) {
              datalayer_battery->status.cell_balancing_status[88 + i] =
                  (rx_frame.data.u8[1 + (i / 8)] >> (7 - (i % 8))) & 0x01;
            }
          }
          break;
        case POLL_ENERGY_COMPLETE:
          battery_energy_complete = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_ENERGY_PARTIAL:
          battery_energy_partial = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SLAVE_FAILURES:
          battery_slave_failures = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MILEAGE:
          battery_mileage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_SPEED:
          battery_fan_speed = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_PERIOD:
          battery_fan_period = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_CONTROL:
          battery_fan_control = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_FAN_DUTY:
          battery_fan_duty = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_TEMPORISATION:
          battery_temporisation = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_TIME:
          battery_time = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_PACK_TIME:
          battery_pack_time = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_MIN:
          battery_soc_min = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_SOC_MAX:
          battery_soc_max = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        default:
          // Handle cell voltages
          if (reply_poll >= POLL_CELL_0 && reply_poll <= POLL_CELL_95) {
            int cell_index = reply_poll - POLL_CELL_0;

            // Three offsets are skipped in the polling sequence, account for that.
            if (reply_poll > POLL_CELL_30) {
              cell_index -= 1;  // Account for missing 0x9040
            }
            if (reply_poll > POLL_CELL_61) {
              cell_index -= 1;  // Account for missing 0x9060
            }
            if (reply_poll > POLL_CELL_92) {
              cell_index -= 1;  // Account for missing 0x9080
            }

            datalayer_battery->status.cell_voltages_mV[cell_index] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          }
          break;
      }
      break;
    default:
      break;
  }
}

void RenaultZoeGen2Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer_extended.zoePH2.UserRequestNVROLReset) {
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

  // Send 200ms polling CAN Message (Only if not NVROL in progress)
  if ((currentMillis - previousMillis200 >= INTERVAL_200_MS) && !datalayer_extended.zoePH2.UserRequestNVROLReset) {
    previousMillis200 = currentMillis;

    // Update current poll from the array
    currentpoll = poll_commands[poll_index];
    poll_index = (poll_index + 1) % 163;

    ZOE_POLL_18DADBF1.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
    ZOE_POLL_18DADBF1.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

    transmit_can_frame(&ZOE_POLL_18DADBF1);
  }

  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    // Time in seconds emulated
    ZOE_376_time_now_s++;  // Increment by 1 second

    transmit_can_frame(&ZOE_5F8);  //Vehicle ID
    transmit_can_frame(&ZOE_6BF);  //Total Boost Time
  }
}

void RenaultZoeGen2Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
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
      ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
      transmit_can_frame(&ZOE_POLL_18DADBF1);
      NVROLstateMachine = 1;
      break;
    case 1:  // wait 100 ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // NVROL reset, part 2: send 0x043101B00900AAAA
        ZOE_POLL_18DADBF1.data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 2;
      }
      break;
    case 2:  // wait 1 s
      if ((millis() - startTimeNVROL) > INTERVAL_1_S) {
        // Enable temporisation before sleep, part 1: send 0x021003AAAAAAAAAA
        ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1);
        startTimeNVROL = millis();  //Reset time start, so we can check time for next step
        NVROLstateMachine = 3;
      }
      break;
    case 3:  //Wait 100ms
      if ((millis() - startTimeNVROL) > INTERVAL_100_MS) {
        // Enable temporisation before sleep, part 2: send 0x042E928101AAAAAA
        ZOE_POLL_18DADBF1.data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA};
        transmit_can_frame(&ZOE_POLL_18DADBF1);
        // Set data back to init values, we are done with the ZOE_POLL_18DADBF1 frame
        ZOE_POLL_18DADBF1.data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00};
        poll_index = 0;
        NVROLstateMachine = 4;
      }
      break;
    case 4:  //Wait 30s
      //While waiting, stop streaming 0x373 to make battery go to sleep
      ZOE_373.data.u8[0] = 0x01;
      if ((millis() - startTimeNVROL) > INTERVAL_30_S) {
        // after sleeping, set the nvrol reset flag to false, to continue normal operation of sending CAN messages
        datalayer_extended.zoePH2.UserRequestNVROLReset = false;
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
