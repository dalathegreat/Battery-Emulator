#include "../include.h"
#ifdef ORION_BMS
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "ORION-BMS.h"

/* Do not change code below unless you are sure what you are doing */
static uint16_t cellvoltages[MAX_AMOUNT_CELLS];  //array with all the cellvoltages
static uint16_t Maximum_Cell_Voltage = 3700;
static uint16_t Minimum_Cell_Voltage = 3700;
static uint16_t Pack_Health = 99;
static int16_t Pack_Current = 0;
static int16_t Average_Temperature = 0;
static uint16_t Pack_Summed_Voltage = 0;
static int16_t Average_Current = 0;
static uint16_t High_Temperature = 0;
static uint16_t Pack_SOC_ppt = 0;
static uint16_t Pack_CCL = 0;  //Charge current limit (A)
static uint16_t Pack_DCL = 0;  //Discharge current limit (A)
static uint16_t Maximum_Pack_Voltage = 0;
static uint16_t Minimum_Pack_Voltage = 0;
static uint16_t CellID = 0;
static uint16_t CellVoltage = 0;
static uint16_t CellResistance = 0;
static uint16_t CellOpenVoltage = 0;
static uint16_t Checksum = 0;
static uint16_t CellBalancing = 0;
static uint8_t amount_of_detected_cells = 0;

void findMinMaxCellvoltages(const uint16_t arr[], size_t size, uint16_t& Minimum_Cell_Voltage,
                            uint16_t& Maximum_Cell_Voltage) {
  Minimum_Cell_Voltage = std::numeric_limits<uint16_t>::max();
  Maximum_Cell_Voltage = 0;
  bool foundValidValue = false;

  for (size_t i = 0; i < size; ++i) {
    if (arr[i] != 0) {  // Skip zero values
      if (arr[i] < Minimum_Cell_Voltage)
        Minimum_Cell_Voltage = arr[i];
      if (arr[i] > Maximum_Cell_Voltage)
        Maximum_Cell_Voltage = arr[i];
      foundValidValue = true;
    }
  }

  // If all values were zero, set min and max to 3700
  if (!foundValidValue) {
    Minimum_Cell_Voltage = 3700;
    Maximum_Cell_Voltage = 3700;
  }
}

void update_values_battery() {

  datalayer.battery.status.real_soc = Pack_SOC_ppt * 10;

  datalayer.battery.status.soh_pptt = Pack_Health * 100;

  datalayer.battery.status.voltage_dV = (Pack_Summed_Voltage / 10);

  datalayer.battery.status.current_dA = Average_Current;

  datalayer.battery.status.max_charge_power_W = (Pack_CCL * datalayer.battery.status.voltage_dV) / 100;

  datalayer.battery.status.max_discharge_power_W = (Pack_DCL * datalayer.battery.status.voltage_dV) / 100;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.temperature_min_dC = (High_Temperature - 10);

  datalayer.battery.status.temperature_max_dC = High_Temperature;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, MAX_AMOUNT_CELLS * sizeof(uint16_t));

  //Find min and max cellvoltage from the array
  findMinMaxCellvoltages(cellvoltages, MAX_AMOUNT_CELLS, Minimum_Cell_Voltage, Maximum_Cell_Voltage);

  datalayer.battery.status.cell_max_voltage_mV = Maximum_Cell_Voltage;

  datalayer.battery.status.cell_min_voltage_mV = Minimum_Cell_Voltage;

  //If user did not configure amount of cells correctly in the header file, update the value
  if ((amount_of_detected_cells > NUMBER_OF_CELLS) && (amount_of_detected_cells < MAX_AMOUNT_CELLS)) {
    datalayer.battery.info.number_of_cells = amount_of_detected_cells;
  }
}

void handle_incoming_can_frame_battery(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x356:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      Pack_Summed_Voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      Average_Current = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      High_Temperature = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      break;
    case 0x351:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      Maximum_Pack_Voltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0];
      Pack_CCL = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      Pack_DCL = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      Minimum_Pack_Voltage = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];
      break;
    case 0x355:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      Pack_Health = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      Pack_SOC_ppt = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[4];
      break;
    case 0x35A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x36:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      CellID = rx_frame.data.u8[0];
      CellBalancing = (rx_frame.data.u8[3] & 0x80) >> 7;
      CellVoltage = (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[2];
      CellResistance = ((rx_frame.data.u8[3] & 0x7F) << 8) | rx_frame.data.u8[4];
      CellOpenVoltage = (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
      Checksum = rx_frame.data.u8[7];  //Value = (0x36 + 8 + byte0 + byte1 + ... + byte6) & 0xFF

      if (CellID >= MAX_AMOUNT_CELLS) {
        CellID = MAX_AMOUNT_CELLS;
      }
      cellvoltages[CellID] = (CellVoltage / 10);
      if (CellID > amount_of_detected_cells) {
        amount_of_detected_cells = CellID;
      }
      break;
    default:
      break;
  }
}

void transmit_can_battery() {
  unsigned long currentMillis = millis();
  // No transmission needed for this integration
}

void setup_battery(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "DIY battery with Orion BMS (Victron setting)", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = NUMBER_OF_CELLS;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}

#endif
