#include "../include.h"
#ifdef RENAULT_ZOE_GEN2_BATTERY
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
/*

/* Do not change code below unless you are sure what you are doing */
static uint16_t battery_soc = 0;
static uint16_t battery_usable_soc = 5000;
static uint16_t battery_soh = 10000;
static uint16_t battery_pack_voltage = 370;
static uint16_t battery_max_cell_voltage = 3700;
static uint16_t battery_min_cell_voltage = 3700;
static uint16_t battery_12v = 0;
static uint16_t battery_avg_temp = 920;
static uint16_t battery_min_temp = 920;
static uint16_t battery_max_temp = 920;
static uint16_t battery_max_power = 0;
static uint16_t battery_interlock = 0;
static uint16_t battery_kwh = 0;
static int32_t battery_current = 32640;
static uint16_t battery_current_offset = 0;
static uint16_t battery_max_generated = 0;
static uint16_t battery_max_available = 0;
static uint16_t battery_current_voltage = 0;
static uint16_t battery_charging_status = 0;
static uint16_t battery_remaining_charge = 0;
static uint16_t battery_balance_capacity_total = 0;
static uint16_t battery_balance_time_total = 0;
static uint16_t battery_balance_capacity_sleep = 0;
static uint16_t battery_balance_time_sleep = 0;
static uint16_t battery_balance_capacity_wake = 0;
static uint16_t battery_balance_time_wake = 0;
static uint16_t battery_bms_state = 0;
static uint16_t battery_balance_switches = 0;
static uint16_t battery_energy_complete = 0;
static uint16_t battery_energy_partial = 0;
static uint16_t battery_slave_failures = 0;
static uint16_t battery_mileage = 0;
static uint16_t battery_fan_speed = 0;
static uint16_t battery_fan_period = 0;
static uint16_t battery_fan_control = 0;
static uint16_t battery_fan_duty = 0;
static uint16_t battery_temporisation = 0;
static uint16_t battery_time = 0;
static uint16_t battery_pack_time = 0;
static uint16_t battery_soc_min = 0;
static uint16_t battery_soc_max = 0;

CAN_frame ZOE_373 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x373,
                     .data = {0xC1, 0x80, 0x5D, 0x5D, 0x00, 0x00, 0xff, 0xcb}};
CAN_frame ZOE_POLL_18DADBF1 = {.FD = false,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x18DADBF1,
                               .data = {0x03, 0x22, 0x90, 0x00, 0x00, 0x00, 0x00, 0x00}};
//NVROL Reset
CAN_frame ZOE_NVROL_1_18DADBF1 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 8,
                                  .ID = 0x18DADBF1,
                                  .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
CAN_frame ZOE_NVROL_2_18DADBF1 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 8,
                                  .ID = 0x18DADBF1,
                                  .data = {0x04, 0x31, 0x01, 0xB0, 0x09, 0x00, 0xAA, 0xAA}};
//Enable temporisation before sleep
CAN_frame ZOE_SLEEP_1_18DADBF1 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 8,
                                  .ID = 0x18DADBF1,
                                  .data = {0x02, 0x10, 0x03, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA}};
CAN_frame ZOE_SLEEP_2_18DADBF1 = {.FD = false,
                                  .ext_ID = true,
                                  .DLC = 8,
                                  .ID = 0x18DADBF1,
                                  .data = {0x04, 0x2E, 0x92, 0x81, 0x01, 0xAA, 0xAA, 0xAA}};

const uint16_t poll_commands[41] = {POLL_SOC,
                                    POLL_USABLE_SOC,
                                    POLL_SOH,
                                    POLL_PACK_VOLTAGE,
                                    POLL_MAX_CELL_VOLTAGE,
                                    POLL_MIN_CELL_VOLTAGE,
                                    POLL_12V,
                                    POLL_AVG_TEMP,
                                    POLL_MIN_TEMP,
                                    POLL_MAX_TEMP,
                                    POLL_MAX_POWER,
                                    POLL_INTERLOCK,
                                    POLL_KWH,
                                    POLL_CURRENT,
                                    POLL_CURRENT_OFFSET,
                                    POLL_MAX_GENERATED,
                                    POLL_MAX_AVAILABLE,
                                    POLL_CURRENT_VOLTAGE,
                                    POLL_CHARGING_STATUS,
                                    POLL_REMAINING_CHARGE,
                                    POLL_BALANCE_CAPACITY_TOTAL,
                                    POLL_BALANCE_TIME_TOTAL,
                                    POLL_BALANCE_CAPACITY_SLEEP,
                                    POLL_BALANCE_TIME_SLEEP,
                                    POLL_BALANCE_CAPACITY_WAKE,
                                    POLL_BALANCE_TIME_WAKE,
                                    POLL_BMS_STATE,
                                    POLL_BALANCE_SWITCHES,
                                    POLL_ENERGY_COMPLETE,
                                    POLL_ENERGY_PARTIAL,
                                    POLL_SLAVE_FAILURES,
                                    POLL_MILEAGE,
                                    POLL_FAN_SPEED,
                                    POLL_FAN_PERIOD,
                                    POLL_FAN_CONTROL,
                                    POLL_FAN_DUTY,
                                    POLL_TEMPORISATION,
                                    POLL_TIME,
                                    POLL_PACK_TIME,
                                    POLL_SOC_MIN,
                                    POLL_SOC_MAX};
static uint8_t poll_index = 0;
static uint16_t currentpoll = POLL_SOC;
static uint16_t reply_poll = 0;

static unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was sent

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
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

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame rx_frame) {
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

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis200 >= INTERVAL_200_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis200));
    }
    previousMillis200 = currentMillis;

    // Update current poll from the array
    currentpoll = poll_commands[poll_index];
    poll_index = (poll_index + 1) % 41;

    ZOE_POLL_18DADBF1.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
    ZOE_POLL_18DADBF1.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

    transmit_can(&ZOE_POLL_18DADBF1, can_config.battery);
    transmit_can(&ZOE_373, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Renault Zoe Gen2 50kWh",
          sizeof(datalayer.system.info.battery_protocol) - 1);
  datalayer.system.info.battery_protocol[sizeof(datalayer.system.info.battery_protocol) - 1] =
      '\0';  // Ensure null termination
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
