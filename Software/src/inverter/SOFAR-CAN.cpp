#include "SOFAR-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

/* This implementation of the SOFAR can protocol is halfway done. What's missing is implementing the inverter replies, all the CAN messages are listed, but the can sending is missing. */

void SofarInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  //Maxvoltage (eg 400.0V = 4000 , 16bits long) Charge Cutoff Voltage
  SOFAR_351.data.u8[0] = (datalayer.battery.info.max_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOFAR_351.data.u8[2] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);
  SOFAR_351.data.u8[3] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOFAR_351.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  SOFAR_351.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  //Minvoltage (eg 300.0V = 3000 , 16bits long) Discharge Cutoff Voltage
  SOFAR_351.data.u8[6] = (datalayer.battery.info.min_design_voltage_dV & 0x00FF);
  SOFAR_351.data.u8[7] = (datalayer.battery.info.min_design_voltage_dV >> 8);

  //SOC
  SOFAR_355.data.u8[0] = (datalayer.battery.status.reported_soc / 100);
  SOFAR_355.data.u8[2] = (datalayer.battery.status.soh_pptt / 100);
  //SOFAR_355.data.u8[6] = (AH_remaining & 0x00FF);
  //SOFAR_355.data.u8[7] = (AH_remaining >> 8);

  //Voltage (370.0)
  SOFAR_356.data.u8[0] = (datalayer.battery.status.voltage_dV & 0x00FF);
  SOFAR_356.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  SOFAR_356.data.u8[2] = (datalayer.battery.status.current_dA & 0x00FF);
  SOFAR_356.data.u8[3] = (datalayer.battery.status.current_dA >> 8);
  SOFAR_356.data.u8[4] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  SOFAR_356.data.u8[5] = (datalayer.battery.status.temperature_max_dC >> 8);

  // frame 0x35E – Manufacturer Name ASCII
  memset(SOFAR_35E.data.u8, 0, 8);
  strncpy((char*)SOFAR_35E.data.u8, BatteryType, 8);

  //Gets automatically rescaled with SOC scaling. Calculated with max design voltage, better would be to calculate with nominal voltage
  calculated_capacity_AH =
      (datalayer.battery.info.reported_total_capacity_Wh / (datalayer.battery.info.max_design_voltage_dV * 0.1));
  //Battery Nominal Capacity
  SOFAR_35F.data.u8[4] = calculated_capacity_AH & 0x00FF;
  SOFAR_35F.data.u8[5] = (calculated_capacity_AH >> 8);

  // Charge and discharge consent dependent on SoC with hysteresis at 99% soc
  //SoC deception only to CAN (we do not touch datalayer)
  uint16_t spoofed_soc = datalayer.battery.status.reported_soc;
  if (spoofed_soc >= 10000) {
    spoofed_soc = 9900;  // limit to 99%
  }

  // Frame 0x355 – SoC and SoH
  SOFAR_355.data.u8[0] = spoofed_soc / 100;
  SOFAR_355.data.u8[2] = datalayer.battery.status.soh_pptt / 100;

  // Set charge and discharge consent flags
  uint8_t soc_percent = spoofed_soc / 100;
  uint8_t enable_flags = 0x00;

  if (soc_percent <= 1) {
    enable_flags = 0x02;  // Only charging allowed
  } else if (soc_percent >= 100) {
    enable_flags = 0x01;  // Only discharge allowed
  } else {
    enable_flags = 0x03;  // Both charge and discharge allowed
  }

  // Frame 0x30F – operation mode
  SOFAR_30F.data.u8[0] = 0x00;  // Normal mode
  SOFAR_30F.data.u8[1] = enable_flags;
}

void SofarInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x605:
    case 0x705:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      switch (rx_frame.data.u8[0]) {
        case 0x00:
          transmit_can_frame(&SOFAR_683, can_config.inverter);
          break;
        case 0x01:
          transmit_can_frame(&SOFAR_684, can_config.inverter);
          break;
        case 0x02:
          transmit_can_frame(&SOFAR_685, can_config.inverter);
          break;
        case 0x03:
          transmit_can_frame(&SOFAR_690, can_config.inverter);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void SofarInverter::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    //Frames actively reported by BMS
    transmit_can_frame(&SOFAR_351, can_config.inverter);
    transmit_can_frame(&SOFAR_355, can_config.inverter);
    transmit_can_frame(&SOFAR_356, can_config.inverter);
    transmit_can_frame(&SOFAR_30F, can_config.inverter);
    transmit_can_frame(&SOFAR_359, can_config.inverter);
    transmit_can_frame(&SOFAR_35E, can_config.inverter);
    transmit_can_frame(&SOFAR_35F, can_config.inverter);
    transmit_can_frame(&SOFAR_35A, can_config.inverter);
  }
}

void SofarInverter::setup(void) {  // Performs one time setup at startup over CAN bus
  // Dymanically set CAN ID according to which battery index we are on
  uint16_t base_offset = (datalayer.battery.settings.sofar_user_specified_battery_id << 12);
  auto init_frame = [&](CAN_frame& frame, uint16_t base_id) {
    frame.FD = false;
    frame.ext_ID = true;
    frame.DLC = 8;
    frame.ID = base_id + base_offset;
    memset(frame.data.u8, 0, 8);
  };

  init_frame(SOFAR_351, 0x351);
  init_frame(SOFAR_355, 0x355);
  init_frame(SOFAR_356, 0x356);
  init_frame(SOFAR_30F, 0x30F);
  init_frame(SOFAR_359, 0x359);
  init_frame(SOFAR_35E, 0x35E);
  init_frame(SOFAR_35F, 0x35F);
  init_frame(SOFAR_35A, 0x35A);

  init_frame(SOFAR_683, 0x683);
  init_frame(SOFAR_684, 0x684);
  init_frame(SOFAR_685, 0x685);
  init_frame(SOFAR_690, 0x690);

  strncpy(datalayer.system.info.inverter_protocol, Name, 63);
  datalayer.system.info.inverter_protocol[63] = '\0';
  String tempStr(datalayer.battery.settings.sofar_user_specified_battery_id);
  strncpy(datalayer.system.info.inverter_brand, tempStr.c_str(), 7);
  datalayer.system.info.inverter_brand[7] = '\0';
}
