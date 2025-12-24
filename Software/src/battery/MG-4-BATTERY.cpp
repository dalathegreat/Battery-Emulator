#include "MG-4-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

const uint16_t voltage_NMC[101] = {
    4200, 4173, 4148, 4124, 4102, 4080, 4060, 4041, 4023, 4007, 3993, 3980, 3969, 3959, 3953, 3950, 3941,
    3932, 3924, 3915, 3907, 3898, 3890, 3881, 3872, 3864, 3855, 3847, 3838, 3830, 3821, 3812, 3804, 3795,
    3787, 3778, 3770, 3761, 3752, 3744, 3735, 3727, 3718, 3710, 3701, 3692, 3684, 3675, 3667, 3658, 3650,
    3641, 3632, 3624, 3615, 3607, 3598, 3590, 3581, 3572, 3564, 3555, 3547, 3538, 3530, 3521, 3512, 3504,
    3495, 3487, 3478, 3470, 3461, 3452, 3444, 3435, 3427, 3418, 3410, 3401, 3392, 3384, 3375, 3367, 3358,
    3350, 3338, 3325, 3313, 3299, 3285, 3271, 3255, 3239, 3221, 3202, 3180, 3156, 3127, 3090, 3000};

uint16_t estimate_soc_from_cell(uint16_t cellVoltage) {
  if (cellVoltage >= voltage_NMC[0]) {
    return 1000;
  }
  if (cellVoltage <= voltage_NMC[100]) {
    return 0;
  }

  for (int i = 1; i < 101; ++i) {
    if (cellVoltage >= voltage_NMC[i]) {
      // Cast to float for proper division
      float t = (float)(cellVoltage - voltage_NMC[i]) / (float)(voltage_NMC[i - 1] - voltage_NMC[i]);

      // Calculate interpolated SOC value
      uint16_t socDiff = (i - 1) * 10 - i * 10;
      uint16_t interpolatedValue = i * 10 + (uint16_t)(t * socDiff);

      return interpolatedValue;
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void Mg4Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second

  // Crude coulomb-counting SoC estimation
  int32_t charge_mC = 1 * datalayer.battery.status.current_dA * 100;

  // A positive current = charging
  if (charge_mC > 0) {
    // Charging
    if (charge_mC > discharged_capacity_mC) {
      // Hit full charge
      discharged_capacity_mC = 0;
    } else {
      discharged_capacity_mC -= charge_mC;
    }
  } else {
    // Discharging
    discharged_capacity_mC += -charge_mC;
  }

  uint32_t max_power_W = 10000;

  if (datalayer.battery.status.voltage_dV >= datalayer.battery.info.max_design_voltage_dV) {
    // At max voltage, assume full charge
    discharged_capacity_mC = 0;
    datalayer.battery.status.real_soc = 10000;
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = max_power_W;
  } else if (datalayer.battery.status.voltage_dV <= datalayer.battery.info.min_design_voltage_dV) {
    // At min voltage, assume empty
    //discharged_capacity_mC = full_capacity_mC;
    datalayer.battery.status.real_soc = 0;
    datalayer.battery.status.max_charge_power_W = max_power_W;
    datalayer.battery.status.max_discharge_power_W = 0;
  } else if (discharged_capacity_mC > full_capacity_mC) {
    // Nearly empty
    datalayer.battery.status.real_soc = 100;
    datalayer.battery.status.max_charge_power_W = max_power_W;
    datalayer.battery.status.max_discharge_power_W = 1000;
  } else {
    // Estimate SoC based on discharged capacity
    uint16_t soc = (uint16_t)(((full_capacity_mC - discharged_capacity_mC) * 9800 / full_capacity_mC) + 100);
    datalayer.battery.status.real_soc = soc;

    // Taper charge/discharge power at high/low SoC
    if (soc > 9000) {
      datalayer.battery.status.max_charge_power_W = max_power_W * (1.0f - ((soc - 9000) / 1000.0f));
    } else {
      datalayer.battery.status.max_charge_power_W = max_power_W;
    }
    if (soc < 1000) {
      datalayer.battery.status.max_discharge_power_W = max_power_W * (soc / 1000.0f);
    } else {
      datalayer.battery.status.max_discharge_power_W = max_power_W;
    }
  }
}

void Mg4Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x12C:
      datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 2.5f;
      datalayer.battery.status.current_dA = -(((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 20000) * 0.5f;

      break;
    default:
      break;
  }
}
void Mg4Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis100 = currentMillis;
    previousMillis200 = currentMillis;
    return;
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&MG4_4F3);
    transmit_can_frame(&MG4_047);
  }
}

void Mg4Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.number_of_cells = 104;
}
