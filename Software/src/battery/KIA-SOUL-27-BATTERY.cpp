#include "../include.h"
#ifdef KIA_SOUL_27_BATTERY
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "KIA-SOUL-27-BATTERY.h"

/* Do not change code below unless you are sure what you are doing */
static unsigned long previousMillis1000 = 0;  // will store last time a 1s CAN Message was send
static unsigned long previousMillis200 = 0;   // will store last time a 200ms CAN Message was send
static unsigned long previousMillis100 = 0;   // will store last time a 100ms CAN Message was send
static unsigned long previousMillis50 = 0;    // will store last time a 50ms CAN Message was send
static unsigned long previousMillis20 = 0;    // will store last time a 20ms CAN Message was send

static uint8_t counter_200 = 0;
static bool startedUp = false;
static bool flipflop = false;

CAN_frame KIA_SOUL_H_100 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x100,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x9C, 0x01, 0xFF, 0x00}};
CAN_frame KIA_SOUL_H_101 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x101,
                            .data = {0x00, 0xFF, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_H_102 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x102,
                            .data = {0x01, 0x93, 0x01, 0x00, 0x00, 0x02, 0xCC, 0x00}};
CAN_frame KIA_SOUL_H_638 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x638,
                            .data = {0x4D, 0x4B, 0x48, 0x0A, 0x00, 0x00, 0x00, 0x01}};
CAN_frame KIA_SOUL_H_639 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x639,
                            .data = {0x11, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_55D = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x55D,
                            .data = {0xAE, 0xA5, 0x28, 0xA1, 0x22, 0x50, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_55E = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x55E,
                            .data = {0x39, 0x2D, 0x18, 0x4A, 0x48, 0x9C, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_4F2 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x4F2,
                            .data = {0x21, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_510 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x510,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_110 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x110,
                            .data = {0xE0, 0x38, 0x30, 0x09, 0x40, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_120 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x120, .data = {0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_517 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x517,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_50 = {.FD = false, .ext_ID = false, .DLC = 4, .ID = 0x50, .data = {0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_18 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x18,
                           .data = {0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x10}};
CAN_frame KIA_SOUL_C_34 = {.FD = false,
                           .ext_ID = false,
                           .DLC = 8,
                           .ID = 0x34,
                           .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_200 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x200,
                            .data = {0x00, 0x80, 0xD8, 0x04, 0x00, 0x17, 0xD0, 0x00}};
CAN_frame KIA_SOUL_C_2A1 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x2A1,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_523 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x523,
                            .data = {0x08, 0x38, 0x4B, 0x4C, 0x4D, 0x4C, 0x00, 0x01}};
CAN_frame KIA_SOUL_C_524 = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x524,
                            .data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
CAN_frame KIA_SOUL_C_57F = {.FD = false,
                            .ext_ID = false,
                            .DLC = 8,
                            .ID = 0x57F,
                            .data = {0x80, 0x0A, 0x72, 0x00, 0x00, 0x00, 0x00, 0x72}};

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc;

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV;

  datalayer.battery.status.current_dA;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.max_charge_power_W;

  datalayer.battery.status.max_discharge_power_W;

  datalayer.battery.status.temperature_min_dC;

  datalayer.battery.status.temperature_max_dC;

  datalayer.battery.status.cell_max_voltage_mV;

  datalayer.battery.status.cell_min_voltage_mV;
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x542:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x594:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x595:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x596:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x597:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x598:
      startedUp = true;
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();

  if (!startedUp) {
    return;  // Don't send any CAN messages towards battery until it has started up
  }

  //Send 20ms message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    transmit_can_frame(&KIA_SOUL_C_4F2, can_config.battery);
  }

  //Send 50ms message (Unclear why 0x200 message is so erratic in logs)
  if (currentMillis - previousMillis50 >= INTERVAL_50_MS) {
    previousMillis50 = currentMillis;
    switch (counter_200) {
      case 0:
        KIA_SOUL_C_200.data.u8[5] = 0x17;
        ++counter_200;
        break;
      case 1:
        KIA_SOUL_C_200.data.u8[5] = 0x57;
        ++counter_200;
        break;
      case 2:
        KIA_SOUL_C_200.data.u8[5] = 0x97;
        ++counter_200;
        break;
      case 3:
        KIA_SOUL_C_200.data.u8[5] = 0xD7;
        ++counter_200;
        break;
      case 4:
        KIA_SOUL_C_200.data.u8[3] = 0x10;
        KIA_SOUL_C_200.data.u8[5] = 0xFF;
        ++counter_200;
        break;
      case 5:
        KIA_SOUL_C_200.data.u8[5] = 0x3B;
        ++counter_200;
        break;
      case 6:
        KIA_SOUL_C_200.data.u8[5] = 0x7B;
        ++counter_200;
        break;
      case 7:
        KIA_SOUL_C_200.data.u8[5] = 0xBB;
        ++counter_200;
        break;
      case 8:
        KIA_SOUL_C_200.data.u8[5] = 0xFB;
        counter_200 = 5;
        break;
    }

    transmit_can_frame(&KIA_SOUL_C_200, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_523, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_524, can_config.battery);
  }

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
    flipflop = !flipflop;  // Toggle between true and false
    if (flipflop) {
      KIA_SOUL_H_638.data.u8[7] = 0x01;
    } else {
      KIA_SOUL_H_638.data.u8[7] = 0x00;
    }

    /* H-CAN Messages */
    transmit_can_frame(&KIA_SOUL_H_100, can_config.battery);
    transmit_can_frame(&KIA_SOUL_H_638, can_config.battery);
    transmit_can_frame(&KIA_SOUL_H_639, can_config.battery);  //TODO, add more permutations of this if needed
    transmit_can_frame(&KIA_SOUL_H_101, can_config.battery);  //TODO, add more permutations of this if needed
    transmit_can_frame(&KIA_SOUL_H_102, can_config.battery);  //TODO, add more permutations of this if needed
    /* C-CAN Messages */
    transmit_can_frame(&KIA_SOUL_C_55D, can_config.battery);  //TODO, add more permutations of this if needed
    transmit_can_frame(&KIA_SOUL_C_55E, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_510, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_110, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_2A1, can_config.battery);
  }

  //Send 200ms message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;

    transmit_can_frame(&KIA_SOUL_C_120, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_517, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_50, can_config.battery);
    transmit_can_frame(&KIA_SOUL_C_18, can_config.battery);  //TODO, add more permutations of this if needed
    transmit_can_frame(&KIA_SOUL_C_57F, can_config.battery);
  }

  //Send 1s message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;
    transmit_can_frame(&KIA_SOUL_C_34, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "Kia Soul 27kWh battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

#endif
