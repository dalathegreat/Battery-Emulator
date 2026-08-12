#include "STELLANTIS-SMALL-WIDE-4x4.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../devboard/utils/events.h"

static uint8_t CalculateCRC8SAEJ1850(CAN_frame rx_frame) {
  uint8_t crc = 0xFF;  // initial value 0xFF
  for (uint8_t j = 0; j < 7; j++) {
    crc = crc8_table_SAE_J1850_ZER0[crc ^ rx_frame.data.u8[j]];
  }
  return crc ^ 0xFF;  // final XOR 0xFF
}

void StellantisSmallWide4x4Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  //datalayer_battery->status.real_soc; //TODO: locate
  //datalayer_battery->status.soh_pptt; //TODO: locate
  //datalayer_battery->status.voltage_dV; //TODO: locate
  //datalayer_battery->status.current_dA; //TODO: locate
  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);
  datalayer_battery->status.max_discharge_power_W = HVBatDischrgPow30sec * 1000;  //Convert kW to W
  datalayer_battery->status.max_charge_power_W = TracBat_EChrgPowLong * 1000;     //Convert kW to W
  //datalayer_battery->status.cell_max_voltage_mV; //TODO: locate
  //datalayer_battery->status.cell_min_voltage_mV; //TODO: locate
  //datalayer_battery->status.temperature_min_dC; //TODO: locate
  //datalayer_battery->status.temperature_max_dC; //TODO: locate

  if (UserRequestDTCreset) {
    transmit_can_frame(&CLEAR_DTC);
    UserRequestDTCreset = false;
  }
}

void StellantisSmallWide4x4Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x305:  //BPCM_HV_PowerLimits 8 lenght 100ms, BPCM sender (HV battery power limit status)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      /*
      D0 = TracBat_EChrgPowLong;      // long/continuous charge power limit [kW]
      D1 = HVBatChrgPow10sec;         // charge power limit for 10 s [kW]
      D2 = HVBatDischrgPow10sec;      // discharge power limit for 10 s [kW]
      D3 = HVBatDischrgPow2sec;       // discharge power limit for 2 s [kW]
      D4 = HVBatChrgPow30sec;         // charge power limit for 30 s [kW]
      D5 = HVBatChrgPow2sec;          // charge power limit for 2 s [kW]
      D6 = HVBatDischrgPow30sec;      // discharge power limit for 30 s [kW]
      D7 bit 7 = TracBat_EChrgPowLongV;   // 0 = valid, 1 = not valid
      D7 bit 6 = HVBatChrgPow2secV;       // 0 = valid, 1 = not valid
      D7 bit 5 = HVBatChrgPow10secV;      // 0 = valid, 1 = not valid
      D7 bit 4 = HVBatChrgPow30secV;      // 0 = valid, 1 = not valid
      D7 bit 3 = HVBatDischrgPow10secV;   // 0 = valid, 1 = not valid
      D7 bit 2 = HVBatDischrgPow2secV;    // 0 = valid, 1 = not valid
      D7 bit 1 = HVBatDischrgPow30secV;   // 0 = valid, 1 = not valid
      D7 bit 0 = HVBatPwrLim_On_BPCM;     // 1 = power limitation active
      */
      TracBat_EChrgPowLong = rx_frame.data.u8[0];
      HVBatDischrgPow30sec = rx_frame.data.u8[6];

      break;
    case 0x7EF:
      break;
    default:
      break;
  }
}

void StellantisSmallWide4x4Battery::transmit_can(unsigned long currentMillis) {

  // Send 20ms CAN Message
  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    counter212 = (counter212 + 1) % 16;  //Counter goes from 0 to 15 and then resets to 0
    SMALLWIDE_212.data.u8[6] = (counter212 << 4) | 0x04;
    SMALLWIDE_212.data.u8[7] = CalculateCRC8SAEJ1850(SMALLWIDE_212);

    transmit_can_frame(&SMALLWIDE_212);  //Keepalive message
  }
}

void StellantisSmallWide4x4Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;

  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
}
