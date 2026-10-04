#include "BOLT-AMPERA-BATTERY.h"
#include <cstring>  //For unit test
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
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
      float t = (cellVoltage - voltage_lookup[i]) / (voltage_lookup[i - 1] - voltage_lookup[i]);
      return SOC[i] + t * (SOC[i - 1] - SOC[i]);
    }
  }
  return 0;  // Default return for safety, should never reach here
}

void BoltAmperaBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer_battery->status.real_soc = estimateSOC(battery_cell_voltage_max_mV);  //TODO, this is bad and barely works

  datalayer_battery->status.voltage_dV = battery_voltage_periodic_dV;

  datalayer_battery->status.current_dA = (sensed_current_sensor_1 * 0.2);  //TODO: Is sensor 1 OK?

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.soh_pptt = 9900;

  // Charge power is set by user (TODO: Remove this estimation when real value has been found)
  // This value gets ramped down by inverter function
  datalayer_battery->status.max_charge_power_W = datalayer_battery->status.override_charge_power_W;

  // Discharge power is also set by user (TODO: Remove this estimation when real value has been found)
  // This value gets ramped down by inverter function
  datalayer_battery->status.max_discharge_power_W = datalayer_battery->status.override_discharge_power_W;

  datalayer_battery->status.temperature_min_dC = temperature_lowest_C * 10;

  datalayer_battery->status.temperature_max_dC = temperature_highest_C * 10;

  //Map all cell voltages to the global array
  memcpy(datalayer_battery->status.cell_voltages_mV, cellblock_voltage, 96 * sizeof(uint16_t));

  datalayer_battery->status.cell_max_voltage_mV = battery_cell_voltage_max_mV;

  datalayer_battery->status.cell_min_voltage_mV = battery_cell_voltage_min_mV;
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String BoltAmperaBattery::get_uds_info_html() {
  String content;
  content.reserve(1600);

  // clang-format off
content << "<h4>7E7 polled values</h4>"
           "<h4>Battery current (7E7): "  << battery_current_7E7            << "</h4>"
           "<h4>5V Reference 0: "         << battery_5V_ref                 << "</h4>"
           "<h4>5V Reference 1: "         << battery_5V_ref_1               << "</h4>"
           "<h4>5V Reference 2: "         << battery_5V_ref_2               << "</h4>"
           "<h4>Module temp (1-6): "      << battery_module_temp_1 << " " << battery_module_temp_2 << " " << battery_module_temp_3 << " " << battery_module_temp_4 << " " << battery_module_temp_5 << " " << battery_module_temp_6 << " " << "</h4>"
           "<h4>Cell average voltage: "   << battery_cell_average_voltage   << "</h4>"
           "<h4>Cell average voltage 2: " << battery_cell_average_voltage_2 << "</h4>"
           "<h4>Terminal voltage: "       << battery_terminal_voltage       << "</h4>"
           "<h4>Ignition power mode: "    << battery_ignition_power_mode    << "</h4>"
           "<h4>GMLAN high speed status: "<< battery_gmlan_high_speed_st    << "</h4>"
           "<h4>Isolation resistance: "   << battery_hv_iso_resist_7E7      << "</h4>"
           "<h4>Bus voltage: "            << battery_bus_volage             << "</h4>"
           "<h4>Cell Balancing ID 1-6: "  << battery_cell_bal_id_1 << " " << battery_cell_bal_id_2 << " " << battery_cell_bal_id_3 << " " << battery_cell_bal_id_4 << " " << battery_cell_bal_id_5 << " " << battery_cell_bal_id_6 << " " << "</h4>"
           "<h4>Cell Balance Status: "    << battery_cell_bal_status        << "</h4>";
           /*
           "<h4>7E4 polled values (Not polled!)</h4>"
           "<h4>Max temp: "               << battery_max_temperature        << "</h4>"
           "<h4>Min temp: "               << battery_min_temperature        << "</h4>"
           "<h4>Capacity MY17-18: "       << battery_capacity_my17_18       << "</h4>"
           "<h4>Capacity MY19+: "         << battery_capacity_my19plus      << "</h4>"
           "<h4>SOC Display: "            << battery_SOC_display            << "</h4>"
           "<h4>SOC Raw highprec: "       << battery_SOC_raw_highprec       << "</h4>"
           "<h4>Cell max mV: "            << battery_max_cell_voltage       << "</h4>"
           "<h4>Cell min mV: "            << battery_min_cell_voltage       << "</h4>"
           "<h4>Lowest cell: "            << battery_lowest_cell            << "</h4>"
           "<h4>Highest cell: "           << battery_highest_cell           << "</h4>"
           "<h4>Internal resistance: "    << battery_internal_resistance    << "</h4>"
           "<h4>Voltage: "                << battery_voltage_polled         << "</h4>"
           "<h4>Isolation Ohm: "          << battery_vehicle_isolation      << "</h4>"
           "<h4>Isolation kOhm: "         << battery_isolation_kohm         << "</h4>"
           "<h4>HV locked: "              << battery_HV_locked              << "</h4>"
           "<h4>Crash event: "            << battery_crash_event            << "</h4>"
           "<h4>HVIL: "                   << battery_HVIL                   << "</h4>"
           "<h4>HVIL status: "            << battery_HVIL_status            << "</h4>"
           "<h4>Current (7E4): "          << battery_current_7E4            << "</h4>";
           */
  // clang-format on

  return content;
}

void BoltAmperaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  uint8_t cellbank_mux = 0;
  uint8_t cellblock_index = 0;
  switch (rx_frame.ID) {
    case 0x200:  //High voltage Battery Cell Voltage Matrix 1
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //Goes from 0-7
      cellblock_index = cellbank_mux * 3;
      cellblock_voltage[cellblock_index] =
          (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 1] =
          (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 2] =
          (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25f;
      break;
    case 0x202:  //High voltage Battery Cell Voltage Matrix 2
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      cellblock_index = 24 + (cellbank_mux * 3);
      cellblock_voltage[cellblock_index] =
          (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 1] =
          (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 2] =
          (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25f;
      break;
    case 0x204:  //High voltage Battery Cell Voltage Matrix 3
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      cellblock_index = 48 + (cellbank_mux * 3);
      cellblock_voltage[cellblock_index] =
          (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 1] =
          (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 2] =
          (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25f;
      break;
    case 0x206:  //High voltage Battery Cell Voltage Matrix 4
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      cellblock_index = 72 + (cellbank_mux * 3);
      cellblock_voltage[cellblock_index] =
          (((rx_frame.data.u8[0] & 0x1F) << 7) | ((rx_frame.data.u8[1] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 1] =
          (((rx_frame.data.u8[2] & 0x1F) << 7) | ((rx_frame.data.u8[3] & 0xFE) >> 1)) * 1.25f;
      cellblock_voltage[cellblock_index + 2] =
          (((rx_frame.data.u8[4]) << 4) | ((rx_frame.data.u8[5] & 0xF0) >> 4)) * 1.25f;
      break;
    case 0x208:  //High voltage Battery Cell Voltage Matrix 5 (Empty on most packs)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      cellbank_mux = ((rx_frame.data.u8[6] & 0xE0) >> 5);  //goes from 0-7
      break;
    case 0x20A:  //VICM Status HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x20C:  //VITM Status HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_isolation_kohm = (rx_frame.data.u8[1] * 25);
      battery_cell_voltage_max_mV = (rx_frame.data.u8[4] * 20);
      battery_cell_voltage_min_mV = (rx_frame.data.u8[5] * 20);
      break;
    case 0x216:  // High voltage battery sensed Output HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      sensed_battery_voltage_mV = (((rx_frame.data.u8[1] & 0x0F) << 4) | rx_frame.data.u8[2]) * 125;  //mV
      sensed_current_sensor_1 = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4]);
      sensed_current_sensor_2 = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
      break;
    case 0x2C7:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_voltage_periodic_dV = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[4] >> 4)) * 1.25;
      /*355V 2C7 [6] 03 20 00 AF A0 00
      360V 2C7 [6] 03 20 00 AD D0 00
      396V 2C7 [6] 03 20 53 C7 30 00*/
      break;
    case 0x260:  //VITM Diagnostic Status 1 HV (Contains which DTCs are active)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x262:  //Battery block voltage diagnostic status
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x270:  //Battery VoltageSensor BalancingSwitches diagnostic status
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x272:  //Battery Cell Voltage Diagnostic Status HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x274:  //Battery Temperature Sensor diagnostic status HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x302:  // High Voltage Battery Temperature Matrix
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
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
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x3E3:  //Min and maximum values
      //Frame0 is cellvoltage min * 20
      //Frame1 is cellvoltage max * 20
      //Frame7 is cellvoltage avg * 20
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      temperature_lowest_C = ((rx_frame.data.u8[2] / 2) - 40);
      temperature_highest_C = ((rx_frame.data.u8[4] / 2) - 40);
      break;
    case 0x460:  //Energy Storage System Temp HV
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      inlet_coolant_temperature = ((((rx_frame.data.u8[0] & 0x03) << 8) | rx_frame.data.u8[1]) / 2) - 40;
      outlet_coolant_temperature = ((((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]) / 2) - 40;
      break;
    case 0x5EF:  //OBD7E7 Unsolicited tester responce (UUDT)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x5EC:  //OBD7E4 Unsolicited tester responce (ECU to tester)
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x7EC:  //When polling 7E4 BMS replies with 7EC (This is not working for some reason)
      //Code left intentionally in incase someone wants to continue experimenting with it
      /*
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
       */
      break;
    case 0x7EF:  //When polling 7E7 BMS replies with 7EF
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      // Hand the reply to the UDS superclass: ISO-TP reassembly, then handle_pid()
      // for PID scan responses and the DTC handlers for the rest.
      handle_incoming_uds_can_frame(rx_frame);
      break;
    default:
      break;
  }
}

uint16_t BoltAmperaBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case POLL_7E7_CURRENT:
      battery_current_7E7 = value;
      break;
    case POLL_7E7_5V_REF:
      battery_5V_ref = ((value * 5) / 65535);
      break;
    case POLL_7E7_MODULE_TEMP_1:
      battery_module_temp_1 = (value - 40);
      break;
    case POLL_7E7_MODULE_TEMP_2:
      battery_module_temp_2 = (value - 40);
      break;
    case POLL_7E7_MODULE_TEMP_3:
      battery_module_temp_3 = (value - 40);
      break;
    case POLL_7E7_MODULE_TEMP_4:
      battery_module_temp_4 = (value - 40);
      break;
    case POLL_7E7_MODULE_TEMP_5:
      battery_module_temp_5 = (value - 40);
      break;
    case POLL_7E7_MODULE_TEMP_6:
      battery_module_temp_6 = (value - 40);
      break;
    case POLL_7E7_CELL_AVG_VOLTAGE:
      battery_cell_average_voltage = ((value * 5000) / 65535);
      break;
    case POLL_7E7_CELL_AVG_VOLTAGE_2:
      battery_cell_average_voltage_2 = ((value / 8000) * 1000);
      break;
    case POLL_7E7_TERMINAL_VOLTAGE:
      battery_terminal_voltage = value * 2;
      break;
    case POLL_7E7_IGNITION_POWER_MODE:
      battery_ignition_power_mode = value;
      break;
    case POLL_7E7_GMLAN_HIGH_SPEED_STATUS:
      battery_gmlan_high_speed_st = value;
      break;
    case POLL_7E7_HV_ISOLATION_RESISTANCE:
      battery_hv_iso_resist_7E7 = value;
      break;
    case POLL_7E7_HV_BUS_VOLTAGE:
      battery_bus_volage = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_1:
      battery_cell_bal_id_1 = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_2:
      battery_cell_bal_id_2 = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_3:
      battery_cell_bal_id_3 = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_4:
      battery_cell_bal_id_4 = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_5:
      battery_cell_bal_id_5 = value;
      break;
    case POLL_7E7_HYBRID_CELL_BALANCING_ID_6:
      battery_cell_bal_id_6 = value;
      break;
    case POLL_7E7_HYBRID_BATTERY_CELL_BALANCE_STATUS:
      battery_cell_bal_status = value;
      break;
    case POLL_7E7_5V_REF_VOLTAGE_1:
      battery_5V_ref_1 = value;
      break;
    case POLL_7E7_5V_REF_VOLTAGE_2:
      battery_5V_ref_2 = value;
      break;
    default:
      // Handle cell voltages in two banks (as they are not contiguous)

      if (pid >= POLL_7E7_CELL_01 && pid <= POLL_7E7_CELL_31) {
        battery_cell_voltages[pid - POLL_7E7_CELL_01] = ((value * 5000) / 65535);
      }

      if (pid >= POLL_7E7_CELL_32 && pid <= POLL_7E7_CELL_96) {
        battery_cell_voltages[pid - POLL_7E7_CELL_32 + 31] = ((value * 5000) / 65535);
      }
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void BoltAmperaBattery::transmit_can(unsigned long currentMillis) {
  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);

  //Send 20ms message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;
    transmit_can_frame(&BOLT_778);
  }
}

void BoltAmperaBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.number_of_cells = 96;
  datalayer_battery->info.total_capacity_Wh = 64000;
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  // UDS: send requests to 0x7E7, accept replies from the BMS on 07EF.
  // This battery should technically have nother BMS (7E4-7EC), but this poll does not work
  setup_uds(0x7E7, 0x7EF);
  static const uint16_t pid_scan_list[] = {
      POLL_7E7_CURRENT,
      POLL_7E7_5V_REF,
      POLL_7E7_MODULE_TEMP_1,
      POLL_7E7_MODULE_TEMP_2,
      POLL_7E7_MODULE_TEMP_3,
      POLL_7E7_MODULE_TEMP_4,
      POLL_7E7_MODULE_TEMP_5,
      POLL_7E7_MODULE_TEMP_6,
      POLL_7E7_CELL_AVG_VOLTAGE,
      POLL_7E7_CELL_AVG_VOLTAGE_2,
      POLL_7E7_TERMINAL_VOLTAGE,
      POLL_7E7_IGNITION_POWER_MODE,
      POLL_7E7_GMLAN_HIGH_SPEED_STATUS,
      POLL_7E7_HV_ISOLATION_RESISTANCE,
      POLL_7E7_HV_BUS_VOLTAGE,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_1,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_2,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_3,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_4,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_5,
      POLL_7E7_HYBRID_CELL_BALANCING_ID_6,
      POLL_7E7_HYBRID_BATTERY_CELL_BALANCE_STATUS,
      POLL_7E7_5V_REF_VOLTAGE_1,
      POLL_7E7_5V_REF_VOLTAGE_2,
      POLL_7E7_CELL_01,
      POLL_7E7_CELL_02,
      POLL_7E7_CELL_03,
      POLL_7E7_CELL_04,
      POLL_7E7_CELL_05,
      POLL_7E7_CELL_06,
      POLL_7E7_CELL_07,
      POLL_7E7_CELL_08,
      POLL_7E7_CELL_09,
      POLL_7E7_CELL_10,
      POLL_7E7_CELL_11,
      POLL_7E7_CELL_12,
      POLL_7E7_CELL_13,
      POLL_7E7_CELL_14,
      POLL_7E7_CELL_15,
      POLL_7E7_CELL_16,
      POLL_7E7_CELL_17,
      POLL_7E7_CELL_18,
      POLL_7E7_CELL_19,
      POLL_7E7_CELL_20,
      POLL_7E7_CELL_21,
      POLL_7E7_CELL_22,
      POLL_7E7_CELL_23,
      POLL_7E7_CELL_24,
      POLL_7E7_CELL_25,
      POLL_7E7_CELL_26,
      POLL_7E7_CELL_27,
      POLL_7E7_CELL_28,
      POLL_7E7_CELL_29,
      POLL_7E7_CELL_30,
      POLL_7E7_CELL_31,
      POLL_7E7_CELL_32,
      POLL_7E7_CELL_33,
      POLL_7E7_CELL_34,
      POLL_7E7_CELL_35,
      POLL_7E7_CELL_36,
      POLL_7E7_CELL_37,
      POLL_7E7_CELL_38,
      POLL_7E7_CELL_39,
      POLL_7E7_CELL_40,
      POLL_7E7_CELL_41,
      POLL_7E7_CELL_42,
      POLL_7E7_CELL_43,
      POLL_7E7_CELL_44,
      POLL_7E7_CELL_45,
      POLL_7E7_CELL_46,
      POLL_7E7_CELL_47,
      POLL_7E7_CELL_48,
      POLL_7E7_CELL_49,
      POLL_7E7_CELL_50,
      POLL_7E7_CELL_51,
      POLL_7E7_CELL_52,
      POLL_7E7_CELL_53,
      POLL_7E7_CELL_54,
      POLL_7E7_CELL_55,
      POLL_7E7_CELL_56,
      POLL_7E7_CELL_57,
      POLL_7E7_CELL_58,
      POLL_7E7_CELL_59,
      POLL_7E7_CELL_60,
      POLL_7E7_CELL_61,
      POLL_7E7_CELL_62,
      POLL_7E7_CELL_63,
      POLL_7E7_CELL_64,
      POLL_7E7_CELL_65,
      POLL_7E7_CELL_66,
      POLL_7E7_CELL_67,
      POLL_7E7_CELL_68,
      POLL_7E7_CELL_69,
      POLL_7E7_CELL_70,
      POLL_7E7_CELL_71,
      POLL_7E7_CELL_72,
      POLL_7E7_CELL_73,
      POLL_7E7_CELL_74,
      POLL_7E7_CELL_75,
      POLL_7E7_CELL_76,
      POLL_7E7_CELL_77,
      POLL_7E7_CELL_78,
      POLL_7E7_CELL_79,
      POLL_7E7_CELL_80,
      POLL_7E7_CELL_81,
      POLL_7E7_CELL_82,
      POLL_7E7_CELL_83,
      POLL_7E7_CELL_84,
      POLL_7E7_CELL_85,
      POLL_7E7_CELL_86,
      POLL_7E7_CELL_87,
      POLL_7E7_CELL_88,
      POLL_7E7_CELL_89,
      POLL_7E7_CELL_90,
      POLL_7E7_CELL_91,
      POLL_7E7_CELL_92,
      POLL_7E7_CELL_93,
      POLL_7E7_CELL_94,
      POLL_7E7_CELL_95,
      POLL_7E7_CELL_96,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
