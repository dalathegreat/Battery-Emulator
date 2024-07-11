#include "../include.h"
#ifdef RENAULT_ZOE_GEN2_BATTERY
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/CAN_config.h"
#include "../lib/miwagner-ESP32-Arduino-CAN/ESP32CAN.h"
#include "RENAULT-ZOE-GEN2-BATTERY.h"

/* Information in this file is based of the OVMS V3 vehicle_renaultzoe.cpp component 
https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3/blob/master/vehicle/OVMS.V3/components/vehicle_renaultzoe_ph2_obd/src/vehicle_renaultzoe_ph2_obd.cpp
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

CAN_frame_t ZOE_373 = {.FIR = {.B =
                                   {
                                       .DLC = 8,
                                       .FF = CAN_frame_std,
                                   }},
                       .MsgID = 0x373,
                       .data = {0xC1, 0x80, 0x5D, 0x5D, 0x00, 0x00, 0xff, 0xcb}};
CAN_frame_t ZOE_POLL_18DADBF1 = {.FIR = {.B =
                                             {
                                                 .DLC = 8,
                                                 .FF = CAN_frame_ext,
                                             }},
                                 .MsgID = 0x18DADBF1,
                                 .data = {0x03, 0x22, 0x90, 0x00, 0xff, 0xff, 0xff, 0xff}};

static unsigned long previousMillis200 = 0;  // will store last time a 200ms CAN Message was sent

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus
  datalayer.battery.status.soh_pptt = (LB_SOH * 100);  //Increase range from 99% -> 99.00%

  datalayer.battery.status.real_soc = (LB_SOC * 10);  //increase LB_SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = LB_Battery_Voltage;

  datalayer.battery.status.current_dA = LB_Current;

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.active_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

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

void receive_can_battery(CAN_frame_t rx_frame) {
  datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
  switch (rx_frame.MsgID) {
    case 0x18daf1db:  // LBC Reply from active polling
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
    ESP32Can.CANWriteFrame(&ZOE_373);
    ESP32Can.CANWriteFrame(&ZOE_POLL_18DADBF1);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Renault Zoe 50kWh battery selected");
#endif

  datalayer.battery.info.max_design_voltage_dV = 4040;
  datalayer.battery.info.min_design_voltage_dV = 3100;
}

#endif
