#include "JAGUAR-IPACE-BATTERY.h"
#include <cstring>  //for unit tests
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void JaguarIpaceBattery::update_values() {

  datalayer.battery.status.real_soc = HVBattAvgSOC * 100;  //Add two decimals

  datalayer.battery.status.soh_pptt = 9900;  //TODO: Map

  datalayer.battery.status.voltage_dV = HVBattVoltageExt * 10;  //TODO: This value OK?

  datalayer.battery.status.current_dA = HVBattCurrentTR * 10;  //TODO: This value OK?

  datalayer.battery.info.total_capacity_Wh = HVBattEnergyUsableMax * 100;  // kWh+1 to Wh

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.cell_max_voltage_mV = HVBattCellVoltageMaxMv;

  datalayer.battery.status.cell_min_voltage_mV = HVBattCellVoltageMinMv;

  datalayer.battery.status.temperature_min_dC = HVBattCellTempColdest * 10;  // C to dC

  datalayer.battery.status.temperature_max_dC = HVBattCellTempHottest * 10;  // C to dC

  datalayer.battery.status.max_discharge_power_W =
      HVBattDischargeContiniousPowerLimit * 10;  // kWh+2 to W (TODO: Check that scaling is right way)

  datalayer.battery.status.max_charge_power_W =
      HVBattChargeContiniousPowerLimit * 10;  // kWh+2 to W (TODO: Check that scaling is right way)

  if (HVBattHVILError) {  // Alert user incase the high voltage interlock is not OK
    set_event(EVENT_HVIL_FAILURE, 0, battery_index);
  } else {
    clear_event(EVENT_HVIL_FAILURE, battery_index);
  }

  if (HVILBattIsolationError) {  // Alert user incase battery reports isolation error
    set_event(EVENT_BATTERY_ISOLATION, 0, battery_index);
  } else {
    clear_event(EVENT_BATTERY_ISOLATION, battery_index);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}

String JaguarIpaceBattery::get_uds_info_html() {
  String content;
  content.reserve(600);
  // clang-format off
  content << "<h4>Positive Contactors B Voltage: " << String(positive_contactors_b_voltage) << "</h4>"
             "<h4>Positive Contactors A Voltage: " << String(positive_contactors_a_voltage) << "</h4>"
             "<h4>Isolation Resistance: " << String(isolation_resistance) << "</h4>"
             "<h4>Coolant Outlet Temperature: " << String(coolant_outlet_temperature) << "</h4>"
             "<h4>Positive Contactors C Voltage: " << String(positive_contactors_c_voltage)<< "</h4>"
             "<h4>Rear Inverter Fuse Voltage: " << String(rear_inverter_fuse_voltage) << "</h4>"
             "<h4>Current Sensor A: " << String(current_sensor_a) << "</h4>"
             "<h4>Front Inverter Fuse Voltage: " << String(front_inverter_fuse_voltage) << "</h4>"
             "<h4>Ancillary Fuse Voltage: " << String(ancillary_fuse_voltage) << "</h4>"
             "<h4>Max Regen Power: " << String(max_regen_power) << " W</h4>"
             "<h4>Negative Contactors Temperature: " << String(negative_contactors_temperature) << "</h4>"
             "<h4>Positive Contactors Temperature: " << String(positive_contactors_temperature) << "</h4>";
  // clang-format on

  return content;
}

void JaguarIpaceBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  // UDS frames (0x7EC PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  switch (rx_frame.ID) {  // These messages are periodically transmitted by the battery
    case 0x080:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBatteryContactorStatus = ((rx_frame.data.u8[0] & 0x80) >> 7);
      HVBattHVILError = ((rx_frame.data.u8[0] & 0x40) >> 6);
      HVILBattIsolationError = ((rx_frame.data.u8[0] & 0x20) >> 5);
      break;
    case 0x100:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattDischargeContiniousPowerLimit =
          ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);                             // 0x3269 = 12905 = 129.05kW
      HVBattDischargePowerLimitExt = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);  // 0x7BD5 = 31701 = 317.01kW
      HVBattDischargeVoltageLimit =
          ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]);  // Lowest voltage the pack can go to
      break;
    case 0x102:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattPwrGpCounter = ((rx_frame.data.u8[1] & 0x3C) >> 2);  // Loops 0-F-0
      HVBattPwerGPCS = rx_frame.data.u8[0];                      // SAE J1850 CRC8 Checksum.
      //TODO: Add function that checks if CRC is correct. We can use this to detect corrupted CAN messages
      //HVBattCurrentExt = //Used only on 2018+
      HVBattVoltageExt = (((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[2]);
      break;
    case 0x104:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBatteryContactorStatusT = ((rx_frame.data.u8[2] & 0x80) >> 7);
      HVIsolationTestStatus = ((rx_frame.data.u8[2] & 0x10) >> 4);
      HVBatteryVoltageOC = (((rx_frame.data.u8[2] & 0x03) << 8) | rx_frame.data.u8[3]);
      HVBatteryChgCurrentLimit = (((rx_frame.data.u8[6] & 0x03) << 8) | rx_frame.data.u8[7]);
      break;
    case 0x10A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      HVBattChargeContiniousPowerLimit = ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]);
      break;
    case 0x198:
      break;
    case 0x1C4:
      HVBattCurrentTR = rx_frame.data.u8[0];  //TODO: scaling?
      //HVBattPwrExtGPCounter = (rx_frame.data.u8[2] & 0xF0) >> 4; // not needed
      //HVBattPwrExtGPCS = rx_frame.data.u8[1]; // Checksum, not needed
      //HVBattVoltageBusTF Frame 3/4/5?
      //HVBattVoltageBusTR Frame 3/4/5?
      break;
    case 0x220:
      break;
    case 0x222:
      HVBattAvgSOC = rx_frame.data.u8[4];
      HVBattAverageTemperature = (rx_frame.data.u8[5] / 2) - 40;
      HVBattFastChgCounter = rx_frame.data.u8[7];
      //HVBattLogEvent
      HVBattTempColdCellID = rx_frame.data.u8[0];
      HVBatTempHotCellID = rx_frame.data.u8[1];
      HVBattVoltMaxCellID = rx_frame.data.u8[2];
      HVBattVoltMinCellID = rx_frame.data.u8[3];
      break;
    case 0x248:
      break;
    case 0x308:
      break;
    case 0x424:
      HVBattCellVoltageMaxMv = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
      HVBattCellVoltageMinMv = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
      //HVBattHeatPowerGenChg // kW
      //HVBattHeatPowerGenDcg // kW
      //HVBattWarmupRateChg // degC/minute
      //HVBattWarmupRateDcg // degC/minute
      break;
    case 0x448:
      HVBattCellTempAverage = (rx_frame.data.u8[0] / 2) - 40;
      HVBattCellTempColdest = (rx_frame.data.u8[1] / 2) - 40;
      HVBattCellTempHottest = (rx_frame.data.u8[2] / 2) - 40;
      //HVBattCIntPumpDiagStat // Pump OK / NOK
      //HVBattCIntPumpDiagStat_UB // True/False
      //HVBattCoolantLevel // Coolant level OK / NOK
      //HVBattHeaterCtrlStat // Off / On
      HVBattInletCoolantTemp = (rx_frame.data.u8[5] / 2) - 40;
      //HVBattInlentCoolantTemp_UB // True/False
      //HVBattMILRequested // True/False
      //HVBattThermalOvercheck // OK / NOK
      break;
    case 0x449:
      break;
    case 0x464:
      HVBattEnergyAvailable =
          ((rx_frame.data.u8[0] << 8) | rx_frame.data.u8[1]) / 2;  // 0x0198 = 408 / 2 = 204 = 20.4kWh
      HVBattEnergyUsableMax =
          ((rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3]) / 2;  // 0x06EA = 1770 / 2 = 885 = 88.5kWh
      HVBattTotalCapacityWhenNew = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);  //0x0395 = 917 = 91.7kWh
      break;
    case 0x522:
      break;
    default:
      break;
  }
}

void JaguarIpaceBattery::transmit_can(unsigned long currentMillis) {
  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);

  /* Send keep-alive every 200ms */
  if (currentMillis - previousMillisKeepAlive >= INTERVAL_200_MS) {
    previousMillisKeepAlive = currentMillis;
    transmit_can_frame(&ipace_keep_alive);
  }
}

uint16_t JaguarIpaceBattery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case POLL_ON_DEMAND_SELF_TEST:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_01:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_02:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_03:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_04:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_05:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_06:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_07:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_08:
      battery_vin = value;
      break;
    case POLL_CONTROL_MODULE_INTERNAL_DATA_09:
      battery_vin = value;
      break;
    case POLL_INTERNAL_FW_VERSION:
      battery_vin = value;
      break;
    case POLL_POSITIVE_CONTACTOR_B_VOLTAGE:
      positive_contactors_b_voltage = value;
      break;
    case POLL_POSITIVE_CONTACTOR_A_VOLTAGE:
      positive_contactors_a_voltage = value;
      break;
    case POLL_ISOLATION_RESISTANCE:
      isolation_resistance = value;
      break;
    case POLL_MAX_CELL_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_MIN_CELL_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_MAX_CELL_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_MIN_CELL_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_TEMPERATURE_UNKNOWN:
      battery_vin = value;
      break;
    case POLL_COOLALNT_OUTLET_TEMPERATURE:
      coolant_outlet_temperature = value;
      break;
    case POLL_POSITIVE_CONTACTOR_C_VOLTAGE:
      positive_contactors_c_voltage = value;
      break;
    case POLL_TEMPERATURE_UNKNOWN_2:
      battery_vin = value;
      break;
    case POLL_REAR_INVERTER_FUSE_VOLTAGE:
      rear_inverter_fuse_voltage = value;
      break;
    case POLL_CURRENT_SENSOR_A:
      current_sensor_a = value;
      break;
    case POLL_FRONT_INVERTER_FUSE_VOLTAGE:
      front_inverter_fuse_voltage = value;
      break;
    case POLL_ANCILLARY_FUSE_VOLTAGE:
      ancillary_fuse_voltage = value;
      break;
    case POLL_BATTERY_PACK_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_AVERAGE_BATTERY_SOC:
      battery_vin = value;
      break;
    case POLL_MIN_BATTERY_SOC:
      battery_vin = value;
      break;
    case POLL_BATTERY_CURRENT_UNKNOWN:
      battery_vin = value;
      break;
    case POLL_MAX_REGEN_POWER:
      max_regen_power = value;
      break;
    case POLL_MAX_BATTERY_SOC:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4915:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4916:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4917:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4918:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4919:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_491A:
      battery_vin = value;
      break;
    case POLL_NEGATIVE_CONTACTOR_TEMPERATURE:
      negative_contactors_temperature = value;
      break;
    case POLL_POSITIVE_CONTACTOR_TEMPERATURE:
      positive_contactors_temperature = value;
      break;
    case POLL_UNKNOWN_491D:
      battery_vin = value;
      break;
    case POLL_ANCILLARY_CONTACTOR_DIFF_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_TRACTION_CONTACTOR_DIFF_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_VOLTAGE_4920:
      battery_vin = value;
      break;
    case POLL_NEGATIVE_CONTACTOR_DIFF_VOLTAGE:
      battery_vin = value;
      break;
    case POLL_FLAGS_4923:
      battery_vin = value;
      break;
    case POLL_CSC_1_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_CSC_2_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_CSC_3_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_CSC_4_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_CSC_5_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_CSC_6_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4931:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4933:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4934:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4935:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4936:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4937:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4938:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4939:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4941:
      battery_vin = value;
      break;
    case POLL_FLAGS_4944:
      battery_vin = value;
      break;
    case POLL_FLAGS_4945:
      battery_vin = value;
      break;
    case POLL_REAR_INVERTER_CURRENT:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4970:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_ASCII_4971:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_VOLTAGE_497A:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_VOLTAGE_497B:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_VOLTAGE_497C:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_497E:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_497F:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_4980:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_VOLTAGE_4981:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_498C:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D015:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D018:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D019:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D020:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D021:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D05B:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D100:
      battery_vin = value;
      break;
    case POLL_FLAGS_D10E:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_D14D:
      battery_vin = value;
      break;
    case POLL_LIVE_DATA_FAIL_CODE:
      battery_vin = value;
      break;
    case POLL_GLOBAL_REAL_TIME:
      battery_vin = value;
      break;
    case POLL_TOTAL_DISTANCE:
      battery_vin = value;
      break;
    case POLL_MAIN_ECU_VOLTAGE_SUPPLY:
      battery_vin = value;
      break;
    case POLL_IN_CAR_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_OUTSIDE_TEMPERATURE:
      battery_vin = value;
      break;
    case POLL_POWER_MODE:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_DD08:
      battery_vin = value;
      break;
    case POLL_VEHICLE_SPEED:
      battery_vin = value;
      break;
    case POLL_PRIMARY_BOOTLOADER_CONFIGURATION:
      battery_vin = value;
      break;
    case POLL_ECU_NETWORK_SIGNAL_CALIBRATION_NUMBER:
      battery_vin = value;
      break;
    case POLL_PRIMARY_BOOT_LOADER_PART_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_SERIAL_NUMBER:
      battery_vin = value;
      break;
    case POLL_VIN:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_FD01:
      battery_vin = value;
      break;
    case POLL_ECU_CORE_ASSEMBLY_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_DELIVERY_ASSEMBLY_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_SOFTWARE_2_PART_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_SOFTWARE_3_PART_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_CALIBRATION_DATA_1_NUMBER:
      battery_vin = value;
      break;
    case POLL_ECU_CALIBRATION_DATA_2_NUMBER:
      battery_vin = value;
      break;
    case POLL_SOFTWARE_DOWNLOAD_SPEC_VERSION:
      battery_vin = value;
      break;
    case POLL_UNKNOWN_F163:
      battery_vin = value;
      break;
    case POLL_JLR_CONFIG_DATA_STORAGE_SPEC_VERSION:
      battery_vin = value;
      break;
    case POLL_NETWORK_CONFIGURATION_IDENTIFIER:
      battery_vin = value;
      break;
    case POLL_BOOT_SOFTWARE_IDENTIFICATION:
      battery_vin = value;
      break;
    case POLL_ACTIVE_DIAGNOSIS_SESSION:
      battery_vin = value;
      break;
    case POLL_ECU_SOFTWARE_NUMBER:
      battery_vin = value;
      break;
    case POLL_ODX_FILE_DATA_IDENTIFIER:
      battery_vin = value;
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void JaguarIpaceBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;

  // UDS: send requests to 0x7E4, accept replies from the BMS on 0x7EC
  setup_uds(0x7E4, 0x7EC);
  static const uint16_t pid_scan_list[] = {
      POLL_ON_DEMAND_SELF_TEST,
      POLL_CONTROL_MODULE_INTERNAL_DATA_01,
      POLL_CONTROL_MODULE_INTERNAL_DATA_02,
      POLL_CONTROL_MODULE_INTERNAL_DATA_03,
      POLL_CONTROL_MODULE_INTERNAL_DATA_04,
      POLL_CONTROL_MODULE_INTERNAL_DATA_05,
      POLL_CONTROL_MODULE_INTERNAL_DATA_06,
      POLL_CONTROL_MODULE_INTERNAL_DATA_07,
      POLL_CONTROL_MODULE_INTERNAL_DATA_08,
      POLL_CONTROL_MODULE_INTERNAL_DATA_09,
      POLL_INTERNAL_FW_VERSION,
      POLL_POSITIVE_CONTACTOR_B_VOLTAGE,
      POLL_POSITIVE_CONTACTOR_A_VOLTAGE,
      POLL_ISOLATION_RESISTANCE,
      POLL_MAX_CELL_VOLTAGE,
      POLL_MIN_CELL_VOLTAGE,
      POLL_MAX_CELL_TEMPERATURE,
      POLL_MIN_CELL_TEMPERATURE,
      POLL_TEMPERATURE_UNKNOWN,
      POLL_COOLALNT_OUTLET_TEMPERATURE,
      POLL_POSITIVE_CONTACTOR_C_VOLTAGE,
      POLL_TEMPERATURE_UNKNOWN_2,
      POLL_REAR_INVERTER_FUSE_VOLTAGE,
      POLL_CURRENT_SENSOR_A,
      POLL_FRONT_INVERTER_FUSE_VOLTAGE,
      POLL_ANCILLARY_FUSE_VOLTAGE,
      POLL_BATTERY_PACK_VOLTAGE,
      POLL_AVERAGE_BATTERY_SOC,
      POLL_MIN_BATTERY_SOC,
      POLL_BATTERY_CURRENT_UNKNOWN,
      POLL_MAX_REGEN_POWER,
      POLL_MAX_BATTERY_SOC,
      POLL_UNKNOWN_4915,
      POLL_UNKNOWN_4916,
      POLL_UNKNOWN_4917,
      POLL_UNKNOWN_4918,
      POLL_UNKNOWN_4919,
      POLL_UNKNOWN_491A,
      POLL_NEGATIVE_CONTACTOR_TEMPERATURE,
      POLL_POSITIVE_CONTACTOR_TEMPERATURE,
      POLL_UNKNOWN_491D,
      POLL_ANCILLARY_CONTACTOR_DIFF_VOLTAGE,
      POLL_TRACTION_CONTACTOR_DIFF_VOLTAGE,
      POLL_UNKNOWN_VOLTAGE_4920,
      POLL_NEGATIVE_CONTACTOR_DIFF_VOLTAGE,
      POLL_FLAGS_4923,
      POLL_CSC_1_TEMPERATURE,
      POLL_CSC_2_TEMPERATURE,
      POLL_CSC_3_TEMPERATURE,
      POLL_CSC_4_TEMPERATURE,
      POLL_CSC_5_TEMPERATURE,
      POLL_CSC_6_TEMPERATURE,
      POLL_UNKNOWN_4931,
      POLL_UNKNOWN_4933,
      POLL_UNKNOWN_4934,
      POLL_UNKNOWN_4935,
      POLL_UNKNOWN_4936,
      POLL_UNKNOWN_4937,
      POLL_UNKNOWN_4938,
      POLL_UNKNOWN_4939,
      POLL_UNKNOWN_4941,
      POLL_FLAGS_4944,
      POLL_FLAGS_4945,
      POLL_REAR_INVERTER_CURRENT,
      POLL_UNKNOWN_4970,
      POLL_UNKNOWN_ASCII_4971,
      POLL_UNKNOWN_VOLTAGE_497A,
      POLL_UNKNOWN_VOLTAGE_497B,
      POLL_UNKNOWN_VOLTAGE_497C,
      POLL_UNKNOWN_497E,
      POLL_UNKNOWN_497F,
      POLL_UNKNOWN_4980,
      POLL_UNKNOWN_VOLTAGE_4981,
      POLL_UNKNOWN_498C,
      POLL_UNKNOWN_D015,
      POLL_UNKNOWN_D018,
      POLL_UNKNOWN_D019,
      POLL_UNKNOWN_D020,
      POLL_UNKNOWN_D021,
      POLL_UNKNOWN_D05B,
      POLL_UNKNOWN_D100,
      POLL_FLAGS_D10E,
      POLL_UNKNOWN_D14D,
      POLL_LIVE_DATA_FAIL_CODE,
      POLL_GLOBAL_REAL_TIME,
      POLL_TOTAL_DISTANCE,
      POLL_MAIN_ECU_VOLTAGE_SUPPLY,
      POLL_IN_CAR_TEMPERATURE,
      POLL_OUTSIDE_TEMPERATURE,
      POLL_POWER_MODE,
      POLL_UNKNOWN_DD08,
      POLL_VEHICLE_SPEED,
      POLL_PRIMARY_BOOTLOADER_CONFIGURATION,
      POLL_ECU_NETWORK_SIGNAL_CALIBRATION_NUMBER,
      POLL_PRIMARY_BOOT_LOADER_PART_NUMBER,
      POLL_ECU_SERIAL_NUMBER,
      POLL_VIN,
      POLL_UNKNOWN_FD01,
      POLL_ECU_CORE_ASSEMBLY_NUMBER,
      POLL_ECU_DELIVERY_ASSEMBLY_NUMBER,
      POLL_ECU_SOFTWARE_2_PART_NUMBER,
      POLL_ECU_SOFTWARE_3_PART_NUMBER,
      POLL_ECU_CALIBRATION_DATA_1_NUMBER,
      POLL_ECU_CALIBRATION_DATA_2_NUMBER,
      POLL_SOFTWARE_DOWNLOAD_SPEC_VERSION,
      POLL_UNKNOWN_F163,
      POLL_JLR_CONFIG_DATA_STORAGE_SPEC_VERSION,
      POLL_NETWORK_CONFIGURATION_IDENTIFIER,
      POLL_BOOT_SOFTWARE_IDENTIFICATION,
      POLL_ACTIVE_DIAGNOSIS_SESSION,
      POLL_ECU_SOFTWARE_NUMBER,
      POLL_ODX_FILE_DATA_IDENTIFIER,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
