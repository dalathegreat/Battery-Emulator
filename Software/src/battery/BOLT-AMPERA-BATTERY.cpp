#include "BOLT-AMPERA-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"

/*
TODOs left for this implementation
- The battery has 3 CAN ports. One of the internal modules is responsible for the 7E4 polls, the battery for the 7E7 polls
- Current implementation only seems to get the 7E7 polls working.

- The values missing for a fully working implementation is:
- SOC% missing! (now estimated based on voltage)
- Capacity (kWh) (now estimated)
- Charge max power (now estimated)
- Discharge max power (now estimated)
- Current is updating extremely slow, consider switching to sensed_ value
- Balancing info seems to be available via OBD/ISO-TP service $22OBD/ISO-TP service $22 , 4340 and onwards
*/

/*TODO, messages we might need to send towards the battery to keep it happy and close contactors
0x214 Charger coolant temp info HV
0x20E Hybrid balancing request HV
0x30E High Voltage Charger Command HV
0x30C HVEM Provide Charging HV
0x316 OBHV Charge Process PEV HV
0x30F OBHV Charg Statn Current stat HV
0x312 OBHV Charg Statns Energy allocation HV
0x310 OBHV Charg Statn Vlt Energy Power HV
0x306 Off board HVCS Limit HV
0x309 Off board HVCS Min Limit HV
0x305 Vehicle Charging limit stat HV
0x314 Vehicle req energy transfer HV <<<<<<<<<< Sounds like contactor request resides here TODO
0x460 Energy Storage System Temp HV (Who sends this? Battery?)
*/

// Define the data points for %SOC depending on cell voltage
// NCM Discharge Curve Lookup Table (100 points, 4.2V-3.0V)
// SOC[100] = State of Charge (0.01% units, e.g., 10000 = 100.00%)
// voltage_lookup[100] = Pack voltage (mV)
const uint8_t numEntries = 100;
const uint16_t SOC[100] = {
    10000, 9985, 9970, 9955, 9940, 9925, 9910, 9895, 9880, 9865,  // 4.20V - 4.15V (High plateau)
    9850,  9820, 9790, 9760, 9730, 9700, 9660, 9620, 9580, 9540,  // 4.14V - 4.00V
    9500,  9450, 9400, 9350, 9300, 9250, 9200, 9150, 9100, 9050,  // 3.99V - 3.90V
    9000,  8900, 8800, 8700, 8600, 8500, 8400, 8300, 8200, 8100,  // 3.89V - 3.80V
    8000,  7850, 7700, 7550, 7400, 7250, 7100, 6950, 6800, 6650,  // 3.79V - 3.70V
    6500,  6300, 6100, 5900, 5700, 5500, 5300, 5100, 4900, 4700,  // 3.69V - 3.60V
    4500,  4300, 4100, 3900, 3700, 3500, 3300, 3100, 2900, 2700,  // 3.59V - 3.50V
    2500,  2250, 2000, 1750, 1500, 1250, 1000, 800,  600,  400,   // 3.49V - 3.40V (Steep drop)
    300,   200,  150,  100,  80,   60,   40,   30,   20,   10,    // 3.39V - 3.30V
    5,     2,    1,    0,    0,    0,    0,    0,    0,    0      // <3.30V (Cutoff)
};

const uint16_t voltage_lookup[100] = {
    4200, 4195, 4190, 4185, 4180, 4175, 4170, 4165, 4160, 4155,  // High plateau
    4150, 4140, 4130, 4120, 4110, 4100, 4090, 4080, 4070, 4060, 4050, 4040, 4030, 4020, 4010, 4000, 3990, 3980,
    3970, 3960, 3950, 3940, 3930, 3920, 3910, 3900, 3890, 3880, 3870, 3860, 3850, 3840, 3830, 3820, 3810, 3800,
    3790, 3780, 3770, 3760, 3750, 3740, 3730, 3720, 3710, 3700, 3690, 3680, 3670, 3660, 3650, 3640, 3630, 3620,
    3610, 3600, 3590, 3580, 3570, 3560, 3550, 3540, 3530, 3520, 3510, 3500, 3490, 3480, 3470, 3460, 3450, 3440,
    3430, 3420, 3410, 3400, 3390, 3380, 3370, 3360, 3350, 3340, 3330, 3320, 3310, 3300, 3290, 3280, 3270, 3260};
static uint16_t estimateSOC(uint16_t cellVoltage) {  // Linear interpolation function
  if (cellVoltage >= voltage_lookup[0]) {
    return SOC[0];
  }
  if (cellVoltage <= voltage_lookup[numEntries - 1]) {
    return SOC[numEntries - 1];
  }

  for (int i = 1; i < numEntries; ++i) {
    if (cellVoltage >= voltage_lookup[i]) {
      double t = (cellVoltage - voltage_lookup[i]) / (voltage_lookup[i - 1] - voltage_lookup[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void BoltAmperaBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = estimateSOC(battery_cell_voltage_max_mV);  //TODO, this is bad and barely works

  datalayer.battery.status.voltage_dV = battery_voltage_periodic_dV;

  datalayer.battery.status.current_dA = (sensed_current_sensor_1 * 0.2);  //TODO: Is sensor 1 OK?

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = 9900;

  // Charge power is set in .h file (TODO: Remove this estimation when real value has been found)
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        datalayer.battery.status.override_charge_power_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  // Discharge power is also set in .h file (TODO: Remove this estimation when real value has been found)
  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  datalayer.battery.status.temperature_min_dC = temperature_lowest_C * 10;

  datalayer.battery.status.temperature_max_dC = temperature_highest_C * 10;

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellblock_voltage, 96 * sizeof(uint16_t));

  datalayer.battery.status.cell_max_voltage_mV = battery_cell_voltage_max_mV;

  datalayer.battery.status.cell_min_voltage_mV = battery_cell_voltage_min_mV;

  // Update webserver datalayer
  datalayer_extended.boltampera.battery_5V_ref = battery_5V_ref;
  datalayer_extended.boltampera.battery_module_temp_1 = battery_module_temp_1;
  datalayer_extended.boltampera.battery_module_temp_2 = battery_module_temp_2;
  datalayer_extended.boltampera.battery_module_temp_3 = battery_module_temp_3;
  datalayer_extended.boltampera.battery_module_temp_4 = battery_module_temp_4;
  datalayer_extended.boltampera.battery_module_temp_5 = battery_module_temp_5;
  datalayer_extended.boltampera.battery_module_temp_6 = battery_module_temp_6;
  datalayer_extended.boltampera.battery_cell_average_voltage = battery_cell_average_voltage;
  datalayer_extended.boltampera.battery_cell_average_voltage_2 = battery_cell_average_voltage_2;
  datalayer_extended.boltampera.battery_terminal_voltage = battery_terminal_voltage;
  datalayer_extended.boltampera.battery_ignition_power_mode = battery_ignition_power_mode;
  datalayer_extended.boltampera.battery_current_7E7 = battery_current_7E7;
  datalayer_extended.boltampera.battery_capacity_my17_18 = battery_capacity_my17_18;
  datalayer_extended.boltampera.battery_capacity_my19plus = battery_capacity_my19plus;
  datalayer_extended.boltampera.battery_SOC_display = battery_SOC_display;
  datalayer_extended.boltampera.battery_SOC_raw_highprec = battery_SOC_raw_highprec;
  datalayer_extended.boltampera.battery_max_temperature = battery_max_temperature;
  datalayer_extended.boltampera.battery_min_temperature = battery_min_temperature;
  datalayer_extended.boltampera.battery_min_cell_voltage = battery_min_cell_voltage;
  datalayer_extended.boltampera.battery_max_cell_voltage = battery_max_cell_voltage;
  datalayer_extended.boltampera.battery_lowest_cell = battery_lowest_cell;
  datalayer_extended.boltampera.battery_highest_cell = battery_highest_cell;
  datalayer_extended.boltampera.battery_internal_resistance = battery_internal_resistance;
  datalayer_extended.boltampera.battery_voltage_polled = battery_voltage_polled;
  datalayer_extended.boltampera.battery_vehicle_isolation = battery_vehicle_isolation;
  datalayer_extended.boltampera.battery_isolation_kohm = battery_isolation_kohm;
  datalayer_extended.boltampera.battery_HV_locked = battery_HV_locked;
  datalayer_extended.boltampera.battery_crash_event = battery_crash_event;
  datalayer_extended.boltampera.battery_HVIL = battery_HVIL;
  datalayer_extended.boltampera.battery_HVIL_status = battery_HVIL_status;
  datalayer_extended.boltampera.battery_current_7E4 = battery_current_7E4;
}

void BoltAmperaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x200:  //High voltage Battery Cell Voltage Matrix 1
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //Goes from 0-7
      switch (cellbank_mux) {
        case 0:
          cellblock_voltage[0] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[1] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[2] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 1:
          cellblock_voltage[3] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[4] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[5] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 2:
          cellblock_voltage[6] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[7] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[8] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 3:
          cellblock_voltage[9] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[10] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[11] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 4:
          cellblock_voltage[12] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[13] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[14] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 5:
          cellblock_voltage[15] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[16] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[17] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 6:
          cellblock_voltage[18] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[19] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[20] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 7:
          cellblock_voltage[21] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[22] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[23] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        default:
          break;
      }
      break;
    case 0x202:  //High voltage Battery Cell Voltage Matrix 2
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      switch (cellbank_mux) {
        case 0:
          cellblock_voltage[24] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[25] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[26] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 1:
          cellblock_voltage[27] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[28] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[29] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 2:
          cellblock_voltage[30] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[31] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[32] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 3:
          cellblock_voltage[33] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[34] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[35] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 4:
          cellblock_voltage[36] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[37] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[38] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 5:
          cellblock_voltage[39] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[40] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[41] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 6:
          cellblock_voltage[42] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[43] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[44] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 7:
          cellblock_voltage[45] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[46] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[47] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        default:
          break;
      }
      break;
    case 0x204:  //High voltage Battery Cell Voltage Matrix 3
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      switch (cellbank_mux) {
        case 0:
          cellblock_voltage[48] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[49] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[50] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 1:
          cellblock_voltage[51] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[52] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[53] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 2:
          cellblock_voltage[54] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[55] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[56] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 3:
          cellblock_voltage[57] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[58] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[59] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 4:
          cellblock_voltage[60] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[61] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[62] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 5:
          cellblock_voltage[63] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[64] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[65] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 6:
          cellblock_voltage[66] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[67] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[68] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 7:
          cellblock_voltage[69] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[70] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[71] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        default:
          break;
      }
      break;
    case 0x206:  //High voltage Battery Cell Voltage Matrix 4
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      switch (cellbank_mux) {
        case 0:
          cellblock_voltage[72] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[73] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[74] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 1:
          cellblock_voltage[75] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[76] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[77] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 2:
          cellblock_voltage[78] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[79] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[80] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 3:
          cellblock_voltage[81] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[82] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[83] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 4:
          cellblock_voltage[84] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[85] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[86] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 5:
          cellblock_voltage[87] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[88] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[89] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 6:
          cellblock_voltage[90] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[91] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[92] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        case 7:
          cellblock_voltage[93] = (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[94] = (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25;
          cellblock_voltage[95] = (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25;
          break;
        default:
          break;
      }
      break;
    case 0x208:  //High voltage Battery Cell Voltage Matrix 5 (Empty on most packs)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      break;
    case 0x20A:  //VICM Status HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x20C:  //VITM Status HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_isolation_kohm = (rx_frame.data.u8[1] * 25);
      battery_cell_voltage_max_mV = (rx_frame.data.u8[4] * 20);
      battery_cell_voltage_min_mV = (rx_frame.data.u8[5] * 20);
      break;
    case 0x216:  // High voltage battery sensed Output HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sensed_battery_voltage_mV = (((rx_frame.data.u8[1] & 0x0F) << 4) | rx_frame.data.u8[2]) * 125;  //mV
      sensed_current_sensor_1 = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
      sensed_current_sensor_2 = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
      break;
    case 0x2C7:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage_periodic_dV = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4)) * 1.25;
      /*355V 2C7 [6] 03 20 00 AF A0 00
      360V 2C7 [6] 03 20 00 AD D0 00
      396V 2C7 [6] 03 20 53 C7 30 00*/
      break;
    case 0x260:  //VITM Diagnostic Status 1 HV (Contains which DTCs are active)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x262:  //Battery block voltage diagnostic status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x270:  //Battery VoltageSensor BalancingSwitches diagnostic status
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x272:  //Battery Cell Voltage Diagnostic Status HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x274:  //Battery Temperature Sensor diagnostic status HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x302:  // High Voltage Battery Temperature Matrix
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperature_1 = ((rx_frame.data.u8[1] / 2) - 40);  //Module 1 Temperature
      temperature_2 = ((rx_frame.data.u8[2] / 2) - 40);  //Module 2 Temperature
      temperature_3 = ((rx_frame.data.u8[3] / 2) - 40);  //Module 3 Temperature
      temperature_4 = ((rx_frame.data.u8[4] / 2) - 40);  //Module 4 Temperature
      temperature_5 = ((rx_frame.data.u8[5] / 2) - 40);  //Module 5 Temperature
      temperature_6 = ((rx_frame.data.u8[6] / 2) - 40);  //Module 6 Temperature
      //There is also a mux here to get more temps, but not required for our integration
      //since we only care about min and max temps (from message 3E3)
      break;
    case 0x308:  //24 92 49 24 90
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E3:  //Min and maximum values
      //Frame0 is cellvoltage min * 20
      //Frame1 is cellvoltage max * 20
      //Frame7 is cellvoltage avg * 20
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperature_lowest_C = ((rx_frame.data.u8[2] / 2) - 40);
      temperature_highest_C = ((rx_frame.data.u8[4] / 2) - 40);
      break;
    case 0x460:  //Energy Storage System Temp HV
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      inlet_coolant_temperature = ((((rx_frame.data.u8[0] & 0x03) << 8) | rx_frame.data.u8[1]) / 2) - 40;
      outlet_coolant_temperature = ((((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]) / 2) - 40;
      break;
    case 0x5EF:  //OBD7E7 Unsolicited tester responce (UUDT)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5EC:  //OBD7E4 Unsolicited tester responce (ECU to tester)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:  //When polling 7E4 BMS replies with 7EC (This is not working for some reason)

      if (rx_frame.data.u8[0] == 0x10) {  //"PID Header"
        transmit_can_frame(&BOLT_ACK_7E4);
      }

      //Frame 2 & 3 contains reply
      reply_poll_7E4 = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

      switch (reply_poll_7E4) {
        case POLL_7E4_CAPACITY_EST_GEN1:
          battery_capacity_my17_18 = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case POLL_7E4_CAPACITY_EST_GEN2:
          battery_capacity_my19plus = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case POLL_7E4_SOC_DISPLAY:
          battery_SOC_display = ((rx_frame.data.u8[4] * 100) / 255);
          break;
        case POLL_7E4_SOC_RAW_HIGHPREC:
          battery_SOC_raw_highprec = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 100) / 65535);
          break;
        case POLL_7E4_MAX_TEMPERATURE:
          battery_max_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E4_MIN_TEMPERATURE:
          battery_min_temperature = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E4_MIN_CELL_V:
          battery_min_cell_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 1666;
          break;
        case POLL_7E4_MAX_CELL_V:
          battery_max_cell_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 1666;
          break;
        case POLL_7E4_INTERNAL_RES:
          battery_internal_resistance = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 2;
          break;
        case POLL_7E4_LOWEST_CELL_NUMBER:
          battery_lowest_cell = rx_frame.data.u8[4];
          break;
        case POLL_7E4_HIGHEST_CELL_NUMBER:
          battery_highest_cell = rx_frame.data.u8[4];
          break;
        case POLL_7E4_VOLTAGE:
          battery_voltage_polled = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 0.52);
          break;
        case POLL_7E4_VEHICLE_ISOLATION:
          battery_vehicle_isolation = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case POLL_7E4_ISOLATION_TEST_KOHM:
          battery_isolation_kohm = (rx_frame.data.u8[4] * 25);
          break;
        case POLL_7E4_HV_LOCKED_OUT:
          battery_HV_locked = rx_frame.data.u8[4];
          break;
        case POLL_7E4_CRASH_EVENT:
          battery_crash_event = rx_frame.data.u8[4];
          break;
        case POLL_7E4_HVIL:
          battery_HVIL = rx_frame.data.u8[4];
          break;
        case POLL_7E4_HVIL_STATUS:
          battery_HVIL_status = rx_frame.data.u8[4];
          break;
        case POLL_7E4_CURRENT:
          battery_current_7E4 = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / (-6.675));
          break;
        default:
          break;
      }

      break;
    case 0x7EF:  //When polling 7E7 BMS replies with 7EF

      if (rx_frame.data.u8[0] == 0x10) {  //"PID Header"
        transmit_can_frame(&BOLT_ACK_7E7);
      }

      //Frame 2 & 3 contains reply
      reply_poll_7E7 = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

      switch (reply_poll_7E7) {
        case POLL_7E7_CURRENT:
          battery_current_7E7 = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          break;
        case POLL_7E7_5V_REF:
          battery_5V_ref = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5) / 65535);
          break;
        case POLL_7E7_MODULE_TEMP_1:
          battery_module_temp_1 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_MODULE_TEMP_2:
          battery_module_temp_2 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_MODULE_TEMP_3:
          battery_module_temp_3 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_MODULE_TEMP_4:
          battery_module_temp_4 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_MODULE_TEMP_5:
          battery_module_temp_5 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_MODULE_TEMP_6:
          battery_module_temp_6 = (rx_frame.data.u8[4] - 40);
          break;
        case POLL_7E7_CELL_AVG_VOLTAGE:
          battery_cell_average_voltage = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_AVG_VOLTAGE_2:
          battery_cell_average_voltage_2 = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 8000) * 1000);
          break;
        case POLL_7E7_TERMINAL_VOLTAGE:
          battery_terminal_voltage = rx_frame.data.u8[4] * 2;
          break;
        case POLL_7E7_IGNITION_POWER_MODE:
          battery_ignition_power_mode = rx_frame.data.u8[4];
          break;
        case POLL_7E7_CELL_01:
          battery_cell_voltages[0] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_02:
          battery_cell_voltages[1] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_03:
          battery_cell_voltages[2] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_04:
          battery_cell_voltages[3] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_05:
          battery_cell_voltages[4] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_06:
          battery_cell_voltages[5] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_07:
          battery_cell_voltages[6] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_08:
          battery_cell_voltages[7] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_09:
          battery_cell_voltages[8] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_10:
          battery_cell_voltages[9] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_11:
          battery_cell_voltages[10] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_12:
          battery_cell_voltages[11] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_13:
          battery_cell_voltages[12] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_14:
          battery_cell_voltages[13] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_15:
          battery_cell_voltages[14] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_16:
          battery_cell_voltages[15] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_17:
          battery_cell_voltages[16] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_18:
          battery_cell_voltages[17] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_19:
          battery_cell_voltages[18] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_20:
          battery_cell_voltages[19] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_21:
          battery_cell_voltages[20] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_22:
          battery_cell_voltages[21] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_23:
          battery_cell_voltages[22] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_24:
          battery_cell_voltages[23] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_25:
          battery_cell_voltages[24] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_26:
          battery_cell_voltages[25] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_27:
          battery_cell_voltages[26] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_28:
          battery_cell_voltages[27] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_29:
          battery_cell_voltages[28] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_30:
          battery_cell_voltages[29] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_31:
          battery_cell_voltages[30] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_32:
          battery_cell_voltages[31] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_33:
          battery_cell_voltages[32] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_34:
          battery_cell_voltages[33] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_35:
          battery_cell_voltages[34] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_36:
          battery_cell_voltages[35] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_37:
          battery_cell_voltages[36] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_38:
          battery_cell_voltages[37] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_39:
          battery_cell_voltages[38] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_40:
          battery_cell_voltages[39] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_41:
          battery_cell_voltages[40] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_42:
          battery_cell_voltages[41] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_43:
          battery_cell_voltages[42] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_44:
          battery_cell_voltages[43] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_45:
          battery_cell_voltages[44] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_46:
          battery_cell_voltages[45] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_47:
          battery_cell_voltages[46] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_48:
          battery_cell_voltages[47] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_49:
          battery_cell_voltages[48] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_50:
          battery_cell_voltages[49] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_51:
          battery_cell_voltages[50] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_52:
          battery_cell_voltages[51] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_53:
          battery_cell_voltages[52] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_54:
          battery_cell_voltages[53] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_55:
          battery_cell_voltages[54] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_56:
          battery_cell_voltages[55] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_57:
          battery_cell_voltages[56] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_58:
          battery_cell_voltages[57] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_59:
          battery_cell_voltages[58] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_60:
          battery_cell_voltages[59] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_61:
          battery_cell_voltages[60] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_62:
          battery_cell_voltages[61] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_63:
          battery_cell_voltages[62] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_64:
          battery_cell_voltages[63] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_65:
          battery_cell_voltages[64] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_66:
          battery_cell_voltages[65] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_67:
          battery_cell_voltages[66] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_68:
          battery_cell_voltages[67] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_69:
          battery_cell_voltages[68] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_70:
          battery_cell_voltages[69] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_71:
          battery_cell_voltages[70] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_72:
          battery_cell_voltages[71] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_73:
          battery_cell_voltages[72] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_74:
          battery_cell_voltages[73] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_75:
          battery_cell_voltages[74] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_76:
          battery_cell_voltages[75] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_77:
          battery_cell_voltages[76] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_78:
          battery_cell_voltages[77] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_79:
          battery_cell_voltages[78] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_80:
          battery_cell_voltages[79] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_81:
          battery_cell_voltages[80] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_82:
          battery_cell_voltages[81] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_83:
          battery_cell_voltages[82] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_84:
          battery_cell_voltages[83] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_85:
          battery_cell_voltages[84] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_86:
          battery_cell_voltages[85] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_87:
          battery_cell_voltages[86] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_88:
          battery_cell_voltages[87] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_89:
          battery_cell_voltages[88] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_90:
          battery_cell_voltages[89] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_91:
          battery_cell_voltages[90] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_92:
          battery_cell_voltages[91] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_93:
          battery_cell_voltages[92] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_94:
          battery_cell_voltages[93] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_95:
          battery_cell_voltages[94] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        case POLL_7E7_CELL_96:
          battery_cell_voltages[95] = ((((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) * 5000) / 65535);
          break;
        default:
          break;
      }
    default:
      break;
  }
}

void BoltAmperaBattery::transmit_can(unsigned long currentMillis) {

  //Send 20ms message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;
    transmit_can_frame(&BOLT_778);
  }

  //Send 100ms message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    // Update current poll from the 7E7 array
    currentpoll_7E7 = poll_commands_7E7[poll_index_7E7];
    poll_index_7E7 = (poll_index_7E7 + 1) % 108;

    BOLT_POLL_7E7.data.u8[2] = (uint8_t)((currentpoll_7E7 & 0xFF00) >> 8);
    BOLT_POLL_7E7.data.u8[3] = (uint8_t)(currentpoll_7E7 & 0x00FF);

    transmit_can_frame(&BOLT_POLL_7E7);
  }

  //Send 120ms message
  if (currentMillis - previousMillis120ms >= 120) {
    previousMillis120ms = currentMillis;

    // Update current poll from the 7E4 array
    currentpoll_7E4 = poll_commands_7E4[poll_index_7E4];
    poll_index_7E4 = (poll_index_7E4 + 1) % 19;

    BOLT_POLL_7E4.data.u8[2] = (uint8_t)((currentpoll_7E4 & 0xFF00) >> 8);
    BOLT_POLL_7E4.data.u8[3] = (uint8_t)(currentpoll_7E4 & 0x00FF);

    //transmit_can_frame(&BOLT_POLL_7E4); //TODO: Battery does not seem to reply on this poll
  }
}

void BoltAmperaBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 96;
  datalayer.battery.info.total_capacity_Wh = 64000;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
