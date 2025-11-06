#include "RIVIAN-BATTERY.h"

#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

/*
Initial support for Rivian BIG battery (135kWh)
The battery has 3x CAN channels
- Failover CAN (CAN-FD) lots of content, not required for operation. Has all cellvoltages!
- Platform CAN (500kbps) with all the control messages needed to control the battery <- This is the one we want
- Battery CAN (500kbps) lots of content, not required for operation 
*/

void RivianBattery::update_values() {

  datalayer.battery.status.real_soc = battery_SOC;

  //datalayer.battery.status.soh_pptt; //TODO: Find usable SOH

  datalayer.battery.status.voltage_dV = battery_voltage;
  datalayer.battery.status.current_dA = ((int16_t)battery_current / 10.0 - 3200) * 10;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W = ((battery_voltage / 10) * battery_charge_limit_amp);
  datalayer.battery.status.max_discharge_power_W = ((battery_voltage / 10) * battery_discharge_limit_amp);

  //datalayer.battery.status.cell_min_voltage_mV = 3700; //TODO: Take from failover CAN?
  //datalayer.battery.status.cell_max_voltage_mV = 3700; //TODO: Take from failover CAN?

  datalayer.battery.status.temperature_min_dC = battery_min_temperature * 10;
  datalayer.battery.status.temperature_max_dC = battery_max_temperature * 10;
}

void RivianBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x160:  //Current [Platform CAN]+
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_current = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      break;
    case 0x151:  //Celltemps (requires other CAN channel)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x120:  //Voltages [Platform CAN]+
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage = (((rx_frame.data.u8[7] & 0x1F) << 8) | rx_frame.data.u8[6]);
      break;
    case 0x25A:  //SOC and kWh [Platform CAN]+
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      //battery_SOC = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0]);
      kWh_available_max =
          (((rx_frame.data.u8[3] & 0x03) << 14) | (rx_frame.data.u8[2] << 6) | (rx_frame.data.u8[1] >> 2)) / 200;
      kWh_available_total =
          (((rx_frame.data.u8[5] & 0x03) << 14) | (rx_frame.data.u8[4] << 6) | (rx_frame.data.u8[3] >> 2)) / 200;
      break;
    case 0x405:  //State [Platform CAN]+
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      BMS_state = (rx_frame.data.u8[0] & 0x03);
      break;
    case 0x100:  //Discharge/Charge speed
      battery_charge_limit_amp =
          (((rx_frame.data.u8[3] & 0x0F) << 8) | (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4)) / 20;
      battery_discharge_limit_amp =
          (((rx_frame.data.u8[5] & 0x0F) << 8) | (rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[3] >> 4)) / 20;
      break;
    case 0x153:  //Temperatures
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_max_temperature = (rx_frame.data.u8[5] / 2) - 40;
      battery_min_temperature = (rx_frame.data.u8[6] / 2) - 40;
      break;
    case 0x55B:  //Temperatures
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_SOC = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    default:
      break;
  }
}

void RivianBattery::transmit_can(unsigned long currentMillis) {
  // Send 500ms CAN Message, too fast and the pack can't change states (pre-charge is built in, seems we can't change state during pre-charge)
  // 100ms seems to draw too much current for a 5A supply during contactor pull in
  if (currentMillis - previousMillis10 >= (INTERVAL_200_MS)) {
    previousMillis10 = currentMillis;

    //If we want to close contactors, and preconditions are met
    if ((datalayer.system.status.inverter_allows_contactor_closing) && (datalayer.battery.status.bms_status != FAULT)) {
      //Standby -> Ready Mode
      if (BMS_state == STANDBY || BMS_state == SLEEP) {
        RIVIAN_150.data.u8[0] = 0x03;
        RIVIAN_150.data.u8[2] = 0x01;
        RIVIAN_420.data.u8[0] = 0x02;
        RIVIAN_200.data.u8[0] = 0x08;

        transmit_can_frame(&RIVIAN_150);
        transmit_can_frame(&RIVIAN_420);
        transmit_can_frame(&RIVIAN_41F);
        transmit_can_frame(&RIVIAN_207);
        transmit_can_frame(&RIVIAN_200);
      }
      //Ready mode -> Go Mode

      if (BMS_state == READY) {
        RIVIAN_150.data.u8[0] = 0x3E;
        RIVIAN_150.data.u8[2] = 0x03;
        RIVIAN_420.data.u8[0] = 0x03;

        transmit_can_frame(&RIVIAN_150);
        transmit_can_frame(&RIVIAN_420);
      }

    } else {  //If we want to open contactors, transition the other way
              //Go mode -> Ready Mode

      if (BMS_state == GO) {
        RIVIAN_150.data.u8[0] = 0x03;
        RIVIAN_150.data.u8[2] = 0x01;

        transmit_can_frame(&RIVIAN_150);
      }

      if (BMS_state == READY) {
        RIVIAN_150.data.u8[0] = 0x03;
        RIVIAN_150.data.u8[2] = 0x01;
        RIVIAN_420.data.u8[0] = 0x01;
        RIVIAN_200.data.u8[0] = 0x10;

        transmit_can_frame(&RIVIAN_245);
        transmit_can_frame(&RIVIAN_150);
        transmit_can_frame(&RIVIAN_420);
        transmit_can_frame(&RIVIAN_41F);
        transmit_can_frame(&RIVIAN_200);
      }
    }
    //disabled this because the battery didn't like it so fast (slowed to 100ms) and because the battery couldn't change states fast enough
    //as much as I don't like the "free-running" aspect of it, checking the BMS_state and acting on it should fix issues caused by that.
    //transmit_can_frame(&RIVIAN_150);
    //transmit_can_frame(&RIVIAN_420);
    //transmit_can_frame(&RIVIAN_41F);
    //transmit_can_frame(&RIVIAN_207);
    //transmit_can_frame(&RIVIAN_200);
  }
}

void RivianBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.total_capacity_Wh = 135000;
  datalayer.battery.info.chemistry = NMC;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
