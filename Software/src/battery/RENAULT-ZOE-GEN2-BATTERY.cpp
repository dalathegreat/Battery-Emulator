#include "../include.h"
#ifdef RENAULT_ZOE_GEN2_BATTERY
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "RENAULT-ZOE-GEN2-BATTERY.h"

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

void RenaultZoeGen2Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt = battery_soh;

  if (battery_soc >= 300) {
    datalayer.battery.status.real_soc = battery_soc - 300;
  } else {
    datalayer.battery.status.real_soc = 0;
  }

  datalayer.battery.status.voltage_dV = battery_pack_voltage;

  datalayer.battery.status.current_dA = ((battery_current - 32640) * 0.3125);

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = battery_max_available * 10;

  datalayer.battery.status.max_charge_power_W = battery_max_generated * 10;

  datalayer.battery.status.temperature_min_dC = ((battery_min_temp - 640) * 0.625);

  datalayer.battery.status.temperature_max_dC = ((battery_max_temp - 640) * 0.625);

  datalayer.battery.status.cell_min_voltage_mV = (battery_min_cell_voltage * 0.976563);

  datalayer.battery.status.cell_max_voltage_mV = (battery_max_cell_voltage * 0.976563);

  if (battery_12v < 11000) {  //11.000V
    set_event(EVENT_12V_LOW, battery_12v);
  }

  // Update webserver datalayer
  datalayer_extended.zoePH2.battery_soc = battery_soc;
  datalayer_extended.zoePH2.battery_usable_soc = battery_usable_soc;
  datalayer_extended.zoePH2.battery_soh = battery_soh;
  datalayer_extended.zoePH2.battery_pack_voltage = battery_pack_voltage;
  datalayer_extended.zoePH2.battery_max_cell_voltage = battery_max_cell_voltage;
  datalayer_extended.zoePH2.battery_min_cell_voltage = battery_min_cell_voltage;
  datalayer_extended.zoePH2.battery_12v = battery_12v;
  datalayer_extended.zoePH2.battery_avg_temp = battery_avg_temp;
  datalayer_extended.zoePH2.battery_min_temp = battery_min_temp;
  datalayer_extended.zoePH2.battery_max_temp = battery_max_temp;
  datalayer_extended.zoePH2.battery_max_power = battery_max_power;
  datalayer_extended.zoePH2.battery_interlock = battery_interlock;
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
  datalayer_extended.zoePH2.battery_balance_switches = battery_balance_switches;
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
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x18DAF1DB:  // LBC Reply from active polling
      //frame 2 & 3 contains
      reply_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

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
          battery_pack_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MAX_CELL_VOLTAGE:
          battery_max_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_MIN_CELL_VOLTAGE:
          battery_min_cell_voltage = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
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
          battery_interlock = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
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
          battery_balance_switches = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
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
        default:  // Unknown reply
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
    // after transmitting the NVROL reset frames, set the nvrol reset flag to false, to continue normal operation
    datalayer_extended.zoePH2.UserRequestNVROLReset = false;
  } else {
    // Send 100ms CAN Message
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      /* FIXME: remove if not needed
      if ((counter_373 / 5) % 2 == 0) {  // Alternate every 5 messages between these two
        ZOE_373.data.u8[2] = 0xB2;
        ZOE_373.data.u8[3] = 0xB2;
      } else {
        ZOE_373.data.u8[2] = 0x5D;
        ZOE_373.data.u8[3] = 0x5D;
      }
      counter_373 = (counter_373 + 1) % 10;
      */

      transmit_can_frame(&ZOE_373, can_config.battery);
      transmit_can_frame_376();
    }

    // Send 200ms CAN Message
    if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
      previousMillis200 = currentMillis;

      // Update current poll from the array
      currentpoll = poll_commands[poll_index];
      poll_index = (poll_index + 1) % 48;

      ZOE_POLL_18DADBF1.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
      ZOE_POLL_18DADBF1.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

      transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
    }

    // 1000mss
    if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
      previousMillis1000 = currentMillis;

      // Time in seconds emulated
      ZOE_376_time_now_s++;  // Increment by 1 second
    }
  }
}

void RenaultZoeGen2Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Renault Zoe Gen2 50kWh", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

void RenaultZoeGen2Battery::transmit_can_frame_376(void) {
  unsigned int secondsSinceProduction = ZOE_376_time_now_s - kProductionTimestamp_s;
  float minutesSinceProduction = (float)secondsSinceProduction / 60.0;
  float yearUnfloored = minutesSinceProduction / 255.0 / 255.0;
  int yearSeg = floor(yearUnfloored);
  float remainderYears = yearUnfloored - yearSeg;
  float remainderHoursUnfloored = (remainderYears * 255.0);
  int hourSeg = floor(remainderHoursUnfloored);
  float remainderHours = remainderHoursUnfloored - hourSeg;
  int minuteSeg = floor(remainderHours * 255.0);

  ZOE_376.data.u8[0] = yearSeg;
  ZOE_376.data.u8[1] = hourSeg;
  ZOE_376.data.u8[2] = minuteSeg;
  ZOE_376.data.u8[3] = yearSeg;
  ZOE_376.data.u8[4] = hourSeg;
  ZOE_376.data.u8[5] = minuteSeg;

  transmit_can_frame(&ZOE_376, can_config.battery);
}

void RenaultZoeGen2Battery::transmit_reset_nvrol_frames(void) {
  // NVROL reset, part 1: send 0x021003AAAAAAAAAA
  ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
  // wait 100 ms
  wait_ms(100);
  // NVROL reset, part 2: send 0x043101B00900AAAA
  ZOE_POLL_18DADBF1.data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA};
  transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);

  // wait 1 s
  wait_ms(1000);

  // Enable temporisation before sleep, part 1: send 0x021003AAAAAAAAAA
  ZOE_POLL_18DADBF1.data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
  transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);
  // wait 100 ms
  wait_ms(100);
  // Enable temporisation before sleep, part 2: send 0x042E928101AAAAAA
  ZOE_POLL_18DADBF1.data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA};
  transmit_can_frame(&ZOE_POLL_18DADBF1, can_config.battery);

  // Set data back to init values
  ZOE_POLL_18DADBF1.data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00};
  poll_index = 0;

  // after transmitting these frames, wait 30 s
  wait_ms(30000);
}

void RenaultZoeGen2Battery::wait_ms(int duration_ms) {
  unsigned long freezeMillis = millis();
  while (millis() - freezeMillis < duration_ms) {
    // Do nothing - just wait
  }
}

#endif
