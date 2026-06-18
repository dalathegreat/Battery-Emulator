#include "BYD-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "INVERTERS.h"

/* Do not change code below unless you are sure what you are doing */

BydVoltageLimits BydCanInverter::calculate_dynamic_voltage_limits() {
  BydVoltageLimits limits = {0, UINT16_MAX};

  if (datalayer.battery.info.number_of_cells == 0)
    return limits;

  int32_t cell_max_limit_mV = datalayer.battery.info.max_cell_voltage_mV - 100;
  int32_t max_limit_dV = (cell_max_limit_mV * datalayer.battery.info.number_of_cells) / 100;
  int32_t cell_min_limit_mV = datalayer.battery.info.min_cell_voltage_mV + 100;
  int32_t min_limit_dV = (cell_min_limit_mV * datalayer.battery.info.number_of_cells) / 100;

  // Is it better to use the pack voltage since cellvoltages often refresh very slowly?
  int32_t cell_total_mV = datalayer.battery.status.voltage_dV * 100;

  // for (uint8_t cell_num = 0; cell_num < datalayer.battery.info.number_of_cells; cell_num++) {
  //   int32_t cell_voltage_mV = datalayer.battery.status.cell_voltages_mV[cell_num];
  //   if (cell_voltage_mV == 0 || cell_voltage_mV > datalayer.battery.info.max_cell_voltage_mV) {
  //     // Invalid cell voltage reading, bail
  //     return UINT16_MAX;
  //   }
  //   cell_total_mV += cell_voltage_mV;
  // }
  int32_t mean_cell_voltage_mV = cell_total_mV / datalayer.battery.info.number_of_cells;

  // How far is the highest cell above the mean?
  int32_t deviation_max_mV = datalayer.battery.status.cell_max_voltage_mV - mean_cell_voltage_mV;
  // Calculate how much to reduce the voltage limit by to account for this
  // deviation, to avoid overcharging the highest cell.
  int32_t deviation_reduction_mV = deviation_max_mV * datalayer.battery.info.number_of_cells;
  if (deviation_reduction_mV > 0) {
    max_limit_dV -= deviation_reduction_mV / 100;
  }

  // How far is the lowest cell below the mean?
  int32_t deviation_min_mV = mean_cell_voltage_mV - datalayer.battery.status.cell_min_voltage_mV;
  // Calculate how much to increase the voltage limit by to account for this
  // deviation, to avoid overdischarging the lowest cell.
  int32_t deviation_increase_mV = deviation_min_mV * datalayer.battery.info.number_of_cells;
  if (deviation_increase_mV > 0) {
    min_limit_dV += deviation_increase_mV / 100;
  }

  // Apply offsets
  max_limit_dV -= 8;
  min_limit_dV += 10;

  limits.max_voltage_dV = max(min(max_limit_dV, (int32_t)UINT16_MAX), 0);
  limits.min_voltage_dV = max(min(min_limit_dV, (int32_t)UINT16_MAX), 0);
  return limits;
}

BydVoltageLimits BydCanInverter::calculate_voltage_limits() {
  // Not sure why offset is subtracted from max and added to min, but this is
  // what the original code did.
  uint16_t max_voltage_dV = datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV;
  uint16_t min_voltage_dV = datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV;

  if (datalayer.battery.settings.user_set_voltage_limits_active) {
    if (datalayer.battery.settings.max_user_set_charge_voltage_dV > 0 &&
        datalayer.battery.settings.max_user_set_charge_voltage_dV < max_voltage_dV) {
      max_voltage_dV = datalayer.battery.settings.max_user_set_charge_voltage_dV;
    }
    if (datalayer.battery.settings.max_user_set_discharge_voltage_dV > 0 &&
        datalayer.battery.settings.max_user_set_discharge_voltage_dV > min_voltage_dV) {
      min_voltage_dV = datalayer.battery.settings.max_user_set_discharge_voltage_dV;
    }
  }

  auto dynamic_limits = calculate_dynamic_voltage_limits();
  if (dynamic_limits.max_voltage_dV < max_voltage_dV) {
    max_voltage_dV = dynamic_limits.max_voltage_dV;
  }
  if (dynamic_limits.min_voltage_dV > min_voltage_dV) {
    min_voltage_dV = dynamic_limits.min_voltage_dV;
  }

  return {min_voltage_dV, max_voltage_dV};
}

void BydCanInverter::
    update_values() {  //This function maps all the values fetched from battery CAN to the correct CAN messages

  /* Calculate temperature */
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  /* Calculate capacity in 0.1 Ah units. Use nominal (mid-design) voltage so the rated value
   * is stable across SOC; multiply before divide to avoid integer truncation. Fall back to
   * current pack voltage when design limits aren't set yet (generic BMS without configured
   * BATTPVMAX/MIN, or BMS that hasn't reported its limits). */
  uint16_t nominal_voltage_dV =
      (datalayer.battery.info.max_design_voltage_dV + datalayer.battery.info.min_design_voltage_dV) / 2;
  if (nominal_voltage_dV < 100) {
    nominal_voltage_dV = datalayer.battery.status.voltage_dV;
  }
  if (nominal_voltage_dV > 10) {
    remaining_capacity_ah = (datalayer.battery.status.reported_remaining_capacity_Wh * 100UL) / nominal_voltage_dV;
    fully_charged_capacity_ah = (datalayer.battery.info.reported_total_capacity_Wh * 100UL) / nominal_voltage_dV;
  }

  auto voltage_limits = calculate_voltage_limits();
  BYD_110.data.u8[0] = (voltage_limits.max_voltage_dV >> 8);
  BYD_110.data.u8[1] = (voltage_limits.max_voltage_dV & 0x00FF);
  BYD_110.data.u8[2] = (voltage_limits.min_voltage_dV >> 8);
  BYD_110.data.u8[3] = (voltage_limits.min_voltage_dV & 0x00FF);

  //Map values to CAN messages
  // if (datalayer.battery.settings.user_set_voltage_limits_active) {  //If user is requesting a specific voltage
  //   //Target charge voltage (eg 400.0V = 4000 , 16bits long)
  //   BYD_110.data.u8[0] = (datalayer.battery.settings.max_user_set_charge_voltage_dV >> 8);
  //   BYD_110.data.u8[1] = (datalayer.battery.settings.max_user_set_charge_voltage_dV & 0x00FF);
  //   //Target discharge voltage (eg 300.0V = 3000 , 16bits long)
  //   BYD_110.data.u8[2] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV >> 8);
  //   BYD_110.data.u8[3] = (datalayer.battery.settings.max_user_set_discharge_voltage_dV & 0x00FF);
  // } else {  //Use the voltage based on battery reported design voltage +- offset to avoid triggering events
  //   //Target charge voltage (eg 400.0V = 4000 , 16bits long)
  //   BYD_110.data.u8[0] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) >> 8);
  //   BYD_110.data.u8[1] = ((datalayer.battery.info.max_design_voltage_dV - VOLTAGE_OFFSET_DV) & 0x00FF);
  //   //Target discharge voltage (eg 300.0V = 3000 , 16bits long)
  //   BYD_110.data.u8[2] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) >> 8);
  //   BYD_110.data.u8[3] = ((datalayer.battery.info.min_design_voltage_dV + VOLTAGE_OFFSET_DV) & 0x00FF);
  // }

  //Maximum discharge power allowed (Unit: A+1)
  BYD_110.data.u8[4] = (datalayer.battery.status.max_discharge_current_dA >> 8);
  BYD_110.data.u8[5] = (datalayer.battery.status.max_discharge_current_dA & 0x00FF);
  //Maximum charge power allowed (Unit: A+1)
  BYD_110.data.u8[6] = (datalayer.battery.status.max_charge_current_dA >> 8);
  BYD_110.data.u8[7] = (datalayer.battery.status.max_charge_current_dA & 0x00FF);

  //SOC (100.00%)
  BYD_150.data.u8[0] = (datalayer.battery.status.reported_soc >> 8);
  BYD_150.data.u8[1] = (datalayer.battery.status.reported_soc & 0x00FF);
  if (user_selected_inverter_deye_workaround) {
    // Fix for avoiding offgrid Deye inverters to underdischarge batteries
    if (datalayer.battery.status.max_charge_current_dA == 0) {
      //Force to 100.00% incase battery no longer wants to charge
      BYD_150.data.u8[0] = (10000 >> 8);
      BYD_150.data.u8[1] = (10000 & 0x00FF);
    }
    if (datalayer.battery.status.max_discharge_current_dA == 0) {
      //Force to 0% incase battery no longer wants to discharge
      BYD_150.data.u8[0] = 0;
      BYD_150.data.u8[1] = 0;
    }
  }
  //StateOfHealth (100.00%)
  BYD_150.data.u8[2] = (datalayer.battery.status.soh_pptt >> 8);
  BYD_150.data.u8[3] = (datalayer.battery.status.soh_pptt & 0x00FF);
  //Remaining capacity (Ah+1)
  BYD_150.data.u8[4] = (remaining_capacity_ah >> 8);
  BYD_150.data.u8[5] = (remaining_capacity_ah & 0x00FF);
  //Fully charged capacity (Ah+1)
  BYD_150.data.u8[6] = (fully_charged_capacity_ah >> 8);
  BYD_150.data.u8[7] = (fully_charged_capacity_ah & 0x00FF);

  //Alarms
  //TODO: BYD Alarms are not implemented yet. Investigation needed on the bits in this message
  //BYD_190.data.u8[0] =

  //Voltage (ex 370.0)
  BYD_1D0.data.u8[0] = (datalayer.battery.status.voltage_dV >> 8);
  BYD_1D0.data.u8[1] = (datalayer.battery.status.voltage_dV & 0x00FF);
  //Current (ex 81.0A)
  BYD_1D0.data.u8[2] = (datalayer.battery.status.reported_current_dA >> 8);
  BYD_1D0.data.u8[3] = (datalayer.battery.status.reported_current_dA & 0x00FF);
  //Temperature average
  BYD_1D0.data.u8[4] = (temperature_average >> 8);
  BYD_1D0.data.u8[5] = (temperature_average & 0x00FF);

  //Temperature max
  BYD_210.data.u8[0] = (datalayer.battery.status.temperature_max_dC >> 8);
  BYD_210.data.u8[1] = (datalayer.battery.status.temperature_max_dC & 0x00FF);
  //Temperature min
  BYD_210.data.u8[2] = (datalayer.battery.status.temperature_min_dC >> 8);
  BYD_210.data.u8[3] = (datalayer.battery.status.temperature_min_dC & 0x00FF);
  //Capacity
  BYD_250.data.u8[4] = (uint8_t)((datalayer.battery.info.reported_total_capacity_Wh / 100) >> 8);
  BYD_250.data.u8[5] = (uint8_t)(datalayer.battery.info.reported_total_capacity_Wh / 100);
}

void BydCanInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x151:  //Message originating from BYD HVS compatible inverter. Reply with CAN identifier!
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      if (rx_frame.data.u8[0] & 0x01) {  //Battery requests identification
        send_initial_data();
      } else {  // We can identify what inverter type we are connected to
        for (uint8_t i = 0; i < 7; i++) {
          if ((rx_frame.data.u8[i] > 0x40) && (rx_frame.data.u8[i] > 0x7B)) {  //Filter out invalid chars
            datalayer.system.info.inverter_brand[i] = rx_frame.data.u8[i + 1];
          }
        }
        datalayer.system.info.inverter_brand[7] = '\0';
      }
      break;
    case 0x091:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_voltage = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) * 0.1;
      inverter_current = (int16_t)((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);
      inverter_temperature = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.1;
      if (useAsShunt) {
        datalayer.shunt.available = true;
        datalayer.shunt.precharging = false;
        datalayer.shunt.contactors_engaged = true;
        datalayer.shunt.measured_voltage_dV = inverter_voltage;
        datalayer.shunt.measured_voltage_mV = inverter_voltage * 100;
        datalayer.shunt.measured_outvoltage_mV = inverter_voltage * 100;
        datalayer.shunt.measured_amperage_dA = inverter_current / 10;
        datalayer.shunt.measured_amperage_mA = inverter_current * 10;
      }
      break;
    case 0x0D1:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_SOC = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) * 0.1;
      break;
    case 0x111:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      inverter_timestamp = ((rx_frame.data.u8[0] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[2] << 8) |
                            rx_frame.data.u8[3]);
      break;
    case 0x191:
      inverterStartedUp = true;
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void BydCanInverter::transmit_can(unsigned long currentMillis) {

  if (!inverterStartedUp) {
    //Avoid sending messages towards inverter, unless it has woken up and sent something to us first
    return;
  }

  // Send initial CAN data once on bootup
  if (!initialDataSent) {
    send_initial_data();
    initialDataSent = true;
  }

  // Send 2s CAN Message
  if (currentMillis - previousMillis2s >= INTERVAL_2_S) {
    previousMillis2s = currentMillis;

    transmit_can_frame(&BYD_110);  //Send Limits
  }
  // Send 10s CAN Message
  if (currentMillis - previousMillis10s >= INTERVAL_10_S) {
    previousMillis10s = currentMillis;

    transmit_can_frame(&BYD_150);  //Send States
    transmit_can_frame(&BYD_1D0);  //Send Battery Info
    transmit_can_frame(&BYD_210);  //Send Cell Info
  }
  //Send 60s message
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    transmit_can_frame(&BYD_190);  //Send Alarm
  }
}

void BydCanInverter::send_initial_data() {
  transmit_can_frame(&BYD_250);
  transmit_can_frame(&BYD_290);
  transmit_can_frame(&BYD_2D0);
  BYD_3D0.data = {0x00, 0x42, 0x61, 0x74, 0x74, 0x65, 0x72, 0x79};  //Battery
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x01, 0x2D, 0x42, 0x6F, 0x78, 0x20, 0x50, 0x72};  //-Box Pr
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x02, 0x65, 0x6D, 0x69, 0x75, 0x6D, 0x20, 0x48};  //emium H
  transmit_can_frame(&BYD_3D0);
  BYD_3D0.data = {0x03, 0x56, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00};  //VS
  transmit_can_frame(&BYD_3D0);
}

void BydCanInverter::enable_shunt() {
  strncpy(datalayer.system.info.shunt_protocol, Name, 31);
  datalayer.system.info.shunt_protocol[31] = '\0';
  useAsShunt = true;
}
