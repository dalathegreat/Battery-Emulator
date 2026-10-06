#include "BYD-BATTERY-BOX-BATTERY.h"

#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"

void BYDBatteryBoxBattery::setup(void) {
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';

  //datalayer.battery.info.max_design_voltage_dV //Gets set once battery communicatates
  //datalayer.battery.info.min_design_voltage_dV //Gets set once battery communicatates
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

void BYDBatteryBoxBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x250:  //FW version and capacity
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x290:  //Unknown startup message
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x2D0:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      for (uint8_t i = 0; i < 7; i++) {
        manufacturer[i] = rx_frame.data.u8[i + 1];
      }
      manufacturer[7] = '\0';
      we_have_identified_battery = true;
      break;
    case 0x3D0:  // Contains name of battery manufacturer / product
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.DLC < 8) {
        break;  // Something is wrong. Abort processing
      }
      if (rx_frame.data.u8[0] <= 3) {  // 4 frames * 7 chars = 28 chars max
        uint8_t offset = rx_frame.data.u8[0] * 7;

        for (uint8_t i = 0; i < 7; i++) {
          model[offset + i] = rx_frame.data.u8[i + 1];
        }

        // Last frame received: make sure the string is terminated
        if (rx_frame.data.u8[0] == 3) {
          model[28] = '\0';
        }
      }
      break;
    case 0x110:  //Limits (1 second)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      target_charge_voltage_dV = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      target_discharge_voltage_dV = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      maximum_discharge_power_allowed_dA = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      maximum_charge_power_allowed_dA = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x150:  //States (10seconds)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      SOH = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      remaining_capacity_dAh = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      fullcharge_capacity_dAh = (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
      break;
    case 0x190:  //Alarm (60seconds) Mappings unknown
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D0:  //Battery Info (10seconds)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      voltage_dV = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];  //Voltage (ex 370.0)
      current_dA = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];  //Current (ex 81.0A) int16_t
      temperature_average = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
      break;
    case 0x210:  //Cell info (10seconds)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperature_max_dC = (rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1];
      temperature_min_dC = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      break;
    default:
      //Unknown incoming CAN message
      break;
  }
}

void BYDBatteryBoxBattery::update_values() {
  datalayer.battery.status.real_soc = SOC;

  datalayer.battery.status.soh_pptt = SOH;

  datalayer.battery.status.voltage_dV = voltage_dV;

  datalayer.battery.status.current_dA = current_dA;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_discharge_power_W = (maximum_discharge_power_allowed_dA * voltage_dV) / 100;

  datalayer.battery.status.max_charge_power_W = (maximum_charge_power_allowed_dA * voltage_dV) / 100;

  datalayer.battery.status.temperature_min_dC = temperature_min_dC;

  datalayer.battery.status.temperature_max_dC = temperature_max_dC;

  if (fullcharge_capacity_dAh > 0) {
    datalayer.battery.info.total_capacity_Wh = (fullcharge_capacity_dAh * voltage_dV) / 100;
  }

  if (target_charge_voltage_dV > 0) {
    datalayer.battery.info.max_design_voltage_dV = target_charge_voltage_dV;
  }

  if (target_discharge_voltage_dV > 0) {
    datalayer.battery.info.min_design_voltage_dV = target_discharge_voltage_dV;
  }
}

void BYDBatteryBoxBattery::transmit_can(unsigned long currentMillis) {

  // Send 10s message
  if (currentMillis - previousMillis10s < INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    //TODO: Map 0x091 if needed. It contains voltage/current/temp
    transmit_can_frame(&BYD_091);

    BYD_0D1.data.u8[0] = (SOC >> 8) & 0xFF;
    BYD_0D1.data.u8[1] = SOC & 0x00FF;
    transmit_can_frame(&BYD_0D1);

    //TODO: Send 0x111 if needed. It contains timestamp
  }

  // Send 60s message
  if (currentMillis - previousMillis60s < INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    if (!we_have_identified_battery) {
      BYD_151.data.u8[0] = 0x01;
    } else {
      BYD_151.data.u8[0] = 0x00;
    }

    transmit_can_frame(&BYD_151);
  }
}
