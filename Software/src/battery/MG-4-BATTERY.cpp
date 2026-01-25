#include "MG-4-BATTERY.h"
#include <cmath>    //For unit test
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

static const uint16_t MAX_CHARGE_POWER_W = 12000;
static const uint16_t CHARGE_TRICKLE_POWER_W = 20;
static const uint16_t DERATE_CHARGE_ABOVE_SOC = 9000;  // in 0.01% units

static const uint16_t MAX_DISCHARGE_POWER_W = 12000;
static const uint16_t DERATE_DISCHARGE_BELOW_SOC = 1500;  // in 0.01% units
static const uint16_t DISCHARGE_MIN_SOC = 1000;

void Mg4Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  // Should be called every second

  uint32_t max_power_W = 10000;

  // Taper charge/discharge power at high/low SoC
  if (datalayer.battery.status.real_soc > 9000) {
    datalayer.battery.status.max_charge_power_W =
        max_power_W * (1.0f - ((datalayer.battery.status.real_soc - 9000) / 1000.0f));
  } else {
    datalayer.battery.status.max_charge_power_W = max_power_W;
  }
  if (datalayer.battery.status.real_soc < 1000) {
    datalayer.battery.status.max_discharge_power_W = max_power_W * (datalayer.battery.status.real_soc / 1000.0f);
  } else {
    datalayer.battery.status.max_discharge_power_W = max_power_W;
  }
}

void Mg4Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  uint32_t soc_times_ten;

  switch (rx_frame.ID) {
    case 0x12C:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if (rx_frame.DLC == 8) {
        datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[4] << 4) | (rx_frame.data.u8[5] >> 4)) * 2.5f;
        datalayer.battery.status.current_dA = -(((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) - 20000) * 0.5f;
      } else {
        // Longer FD one
        datalayer.battery.status.voltage_dV = ((rx_frame.data.u8[8] << 4) | (rx_frame.data.u8[9] >> 4)) * 2.5f;
        datalayer.battery.status.current_dA = -(((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) - 20000) * 0.5f;

        datalayer.battery.status.cell_min_voltage_mV = ((rx_frame.data.u8[30] << 8) | (rx_frame.data.u8[31])) / 8;
        datalayer.battery.status.cell_max_voltage_mV = ((rx_frame.data.u8[32] << 8) | (rx_frame.data.u8[33])) / 8;
      }

      break;
    case 0x401:
      soc_times_ten = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]) & 0x3FF;
      if (soc_times_ten <= 1000) {
        datalayer.battery.status.real_soc = soc_times_ten * 10;
      }

      break;
    default:
      break;
  }
}

const uint8_t FOURSEVEN_FIRST_BYTES[] = {
    0x81, 0xDC, 0xB4, 0xE9, 0xE8, 0xB5, 0xDD, 0x0F, 0x53, 0x81, 0x66, 0xB4, 0x3A, 0x67, 0x0F,
    0x81, 0x53, 0x3B, 0x66, 0xE8, 0xB5, 0xDD, 0x0F, 0x53, 0x0E, 0x66, 0xB4, 0x3A, 0xE8, 0x0F,
};

const uint8_t FOURSEVEN_FD_CYCLE_1[] = {0xF4, 0xA9, 0x4E, 0x13, 0x9D, 0xC0, 0x27, 0x7A,
                                        0x26, 0x7B, 0x9C, 0xC1, 0x4F, 0x12, 0xF5};

const uint8_t FOURSEVEN_FD_CYCLE_2[] = {0x61, 0x3C, 0xDB, 0x86, 0x08, 0x55, 0xB2, 0xEF,
                                        0xB3, 0xEE, 0x09, 0x54, 0xDA, 0x87, 0x60};

void Mg4Battery::transmit_can(unsigned long currentMillis) {
  if (datalayer.system.status.bms_reset_status != BMS_RESET_IDLE) {
    // Transmitting towards battery is halted while BMS is being reset
    previousMillis10 = currentMillis;
    previousMillis200 = currentMillis;
    return;
  }

  /* PT-EXT wakeup */
  if (true) {
    if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
      previousMillis10 = currentMillis;

      // if (sendPhase == 3 || sendPhase == 13 || sendPhase == 23) {
      //   transmit_can_frame(&MG4_4F3);
      // }

      MG4_047.data.u8[0] = FOURSEVEN_FIRST_BYTES[sendPhase];
      if (sendPhase >= 0xf) {
        MG4_047.data.u8[1] = sendPhase - 0xf;
      } else {
        MG4_047.data.u8[1] = sendPhase;
      }
      transmit_can_frame(&MG4_047);

      // sendPhase++;
      // if (sendPhase >= 30) {
      //   sendPhase = 0;
      // }
      //    }
      // }

      // /* PT wakeup */
      // if (true) {
      //   if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
      //     previousMillis10 = currentMillis;

      if (false) {
        int offset = sendPhase;
        if (sendPhase >= 15) {
          offset = sendPhase - 15;
        }

        MG4_047_FD.data.u8[4] = FOURSEVEN_FD_CYCLE_1[offset];
        MG4_047_FD.data.u8[5] = 0xF0 | offset;
        MG4_047_FD.data.u8[16] = FOURSEVEN_FD_CYCLE_2[offset];
        MG4_047_FD.data.u8[17] = 0xF0 | offset;
        transmit_can_frame(&MG4_047_FD);
      }

      if (sendPhase == 2 || sendPhase == 12 || sendPhase == 22) {
        transmit_can_frame(&MG4_4F3_FD);
      }

      sendPhase++;
      if (sendPhase >= 30) {
        sendPhase = 0;
      }
    }
  }

  transmit_uds_can(currentMillis);
}

static void update_soc(uint16_t soc_times_ten) {
  datalayer.battery.status.real_soc = soc_times_ten * 10;

  uint32_t remaining =
      (datalayer.battery.info.total_capacity_Wh * (datalayer.battery.status.real_soc - DISCHARGE_MIN_SOC)) /
      (10000 - DISCHARGE_MIN_SOC);
  if (remaining > 0) {
    datalayer.battery.status.remaining_capacity_Wh = remaining;
  } else {
    datalayer.battery.status.remaining_capacity_Wh = 0;
  }

  datalayer.battery.status.max_charge_power_W = taper_charge_power_linear(
      datalayer.battery.status.real_soc, MAX_CHARGE_POWER_W, CHARGE_TRICKLE_POWER_W, DERATE_CHARGE_ABOVE_SOC);

  datalayer.battery.status.max_discharge_power_W = taper_discharge_power_linear(
      datalayer.battery.status.real_soc, MAX_DISCHARGE_POWER_W, DISCHARGE_MIN_SOC, DERATE_DISCHARGE_BELOW_SOC);
}

uint16_t Mg4Battery::handle_pid(uint16_t pid, uint32_t value) {
  switch (pid) {
    case POLL_BATTERY_VOLTAGE:
      //datalayer.battery.status.voltage_dV = (value * 5) / 2;
      return POLL_BATTERY_CURRENT;
    case POLL_BATTERY_CURRENT:
      //datalayer.battery.status.current_dA = (value - 40000) / -4;
      return POLL_BATTERY_SOC;
    case POLL_BATTERY_SOC:
      update_soc(value);
      return POLL_MIN_CELL_TEMPERATURE;
    case POLL_MIN_CELL_TEMPERATURE:
      // No clue what the scale factor is here.
      datalayer.battery.status.temperature_min_dC = ((int16_t)value - 20000) / 20;
      return POLL_MAX_CELL_TEMPERATURE;
    case POLL_MAX_CELL_TEMPERATURE:
      // No clue what the scale factor is here.
      datalayer.battery.status.temperature_max_dC = ((int16_t)value - 20000) / 20;
      break;  // End of cycle
  }
  return 0;  // Continue normal PID cycling
}

void Mg4Battery::setup(void) {  // Performs one time setup at startup
  setup_uds(0x7DF, POLL_BATTERY_VOLTAGE);
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.system.status.battery_allows_contactor_closing = true;

  datalayer.battery.info.number_of_cells = 104;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  // Danger limits
  if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    datalayer.battery.info.max_cell_voltage_mV = 3650;
    datalayer.battery.info.min_cell_voltage_mV = 2500;
  } else {
    datalayer.battery.info.max_cell_voltage_mV = 4300;
    datalayer.battery.info.min_cell_voltage_mV = 2800;
  }

  datalayer.battery.info.max_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.max_cell_voltage_mV) / 100;
  datalayer.battery.info.min_design_voltage_dV =
      (datalayer.battery.info.number_of_cells * datalayer.battery.info.min_cell_voltage_mV) / 100;
}
