#include "../include.h"
#ifdef RENAULT_ZOE_GEN1_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "RENAULT-ZOE-GEN1-BATTERY.h"

/* Information in this file is based of the OVMS V3 vehicle_renaultzoe.cpp component 
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe/src/vehicle_renaultzoe.cpp
/*

/* Do not change code below unless you are sure what you are doing */
static uint16_t LB_SOC = 50;
static uint16_t LB_SOH = 99;
static int16_t LB_Average_Temperature = 0;
static uint32_t LB_Charge_Power_W = 0;
static int32_t LB_Current = 0;
static uint16_t LB_kWh_Remaining = 0;
static uint16_t LB_Cell_Max_Voltage = 3700;
static uint16_t LB_Cell_Min_Voltage = 3700;
static uint16_t LB_Battery_Voltage = 3700;

CAN_frame ZOE_423 = {.FD = false,
                     .ext_ID = false,
                     .DLC = 8,
                     .ID = 0x423,
                     .data = {0x07, 0x1d, 0x00, 0x02, 0x5d, 0x80, 0x5d, 0xc8}};
CAN_frame ZOE_POLL_79B = {.FD = false,
                          .ext_ID = false,
                          .DLC = 8,
                          .ID = 0x79B,  //0x41 = cell bat module 1-62 , 0x42 = cell bat module 63-96
                          .data = {0x02, 0x21, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00}};

static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was sent
static unsigned long previousMillis5000 = 0;  // will store last time a 1000ms CAN Message was sent
static uint8_t counter_423 = 0;

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt = (LB_SOH * 100);  // Increase range from 99% -> 99.00%

  datalayer.battery.status.real_soc = (LB_SOC * 10);  // Increase LB_SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = LB_Battery_Voltage;

  datalayer.battery.status.current_dA = LB_Current;  //TODO: Take from CAN

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  if (emulator_pause_request_ON) {
    datalayer.battery.status.max_discharge_power_W = 0;
    datalayer.battery.status.max_charge_power_W = 0;
  } else {
    datalayer.battery.status.max_discharge_power_W = 5000;  //TODO: Take from CAN
    datalayer.battery.status.max_charge_power_W = LB_Charge_Power_W;
  }

  datalayer.battery.status.active_power_W;

  datalayer.battery.status.temperature_min_dC = LB_Average_Temperature;

  datalayer.battery.status.temperature_max_dC = LB_Average_Temperature;

  datalayer.battery.status.cell_min_voltage_mV;

  datalayer.battery.status.cell_max_voltage_mV;

  if (LB_Cell_Max_Voltage >= ABSOLUTE_CELL_MAX_VOLTAGE) {
    set_event(EVENT_CELL_OVER_VOLTAGE, 0);
  }
  if (LB_Cell_Min_Voltage <= ABSOLUTE_CELL_MIN_VOLTAGE) {
    set_event(EVENT_CELL_UNDER_VOLTAGE, 0);
  }

#ifdef DEBUG_VIA_USB

#endif
}

void receive_can_battery(CAN_frame rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.ID) {
    case 0x427:
      LB_Charge_Power_W = rx_frame.data.u8[5] * 300;
      LB_kWh_Remaining = (((((rx_frame.data.u8[6] << 8) | (rx_frame.data.u8[7])) >> 6) & 0x3ff) * 0.1);
      break;
    case 0x42E:  //HV SOC & Battery Temp & Charging Power
      LB_Battery_Voltage = (((((rx_frame.data.u8[3] << 8) | (rx_frame.data.u8[4])) >> 5) & 0x3ff) * 0.5);  //0.5V/bit
      LB_Average_Temperature = (((((rx_frame.data.u8[5] << 8) | (rx_frame.data.u8[6])) >> 5) & 0x7F) - 40);
      break;
    case 0x654:  //SOC
      LB_SOC = rx_frame.data.u8[3];
      break;
    case 0x658:  //SOH
      LB_SOH = (rx_frame.data.u8[4] & 0x7F);
      break;
    case 0x7BB:  //Reply from active polling
      // TODO: Handle the cellvoltages

      break;
    default:
      break;
  }
}

void send_can_battery() {
  unsigned long currentMillis = millis();
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis100 >= INTERVAL_100_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis100));
    }
    previousMillis100 = currentMillis;
    transmit_can(&ZOE_423, can_config.battery);

    if ((counter_423 / 5) % 2 == 0) {  // Alternate every 5 messages between these two
      ZOE_423.data.u8[4] = 0xB2;
      ZOE_423.data.u8[6] = 0xB2;
    } else {
      ZOE_423.data.u8[4] = 0x5D;
      ZOE_423.data.u8[6] = 0x5D;
    }
    counter_423 = (counter_423 + 1) % 10;
  }
  // 5000ms CAN handling
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;
    transmit_can(&ZOE_POLL_79B, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Renault Zoe 22/40kWh battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;
  datalayer.battery.info.min_design_voltage_dV = 2700;
}

#endif
