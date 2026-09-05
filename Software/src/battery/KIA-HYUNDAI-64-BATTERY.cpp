#include "KIA-HYUNDAI-64-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

void KiaHyundai64Battery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer_battery->status.real_soc = (SOC_Display * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer_battery->status.soh_pptt = (batterySOH * 10);  //Increase decimals from 100.0% -> 100.00%

  datalayer_battery->status.voltage_dV = batteryVoltage;  //value is *10 (3700 = 370.0)

  datalayer_battery->status.current_dA = -batteryAmps;  //value is *10 (150 = 15.0) , invert the sign

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  datalayer_battery->status.max_charge_power_W = allowedChargePower * 10;

  datalayer_battery->status.max_discharge_power_W = allowedDischargePower * 10;

  datalayer_battery->status.temperature_min_dC = (int8_t)temperatureMin * 10;  //Increase decimals, 17C -> 17.0C

  datalayer_battery->status.temperature_max_dC = (int8_t)temperatureMax * 10;  //Increase decimals, 18C -> 18.0C

  datalayer_battery->status.cell_max_voltage_mV = CellVoltMax_mV;

  datalayer_battery->status.cell_min_voltage_mV = CellVoltMin_mV;

  if (waterleakageSensor == 0) {
    set_event(EVENT_WATER_INGRESS, 0);
  }

  if (leadAcidBatteryVoltage < 110) {
    set_event(EVENT_12V_LOW, leadAcidBatteryVoltage);
  }
}

void KiaHyundai64Battery::update_number_of_cells() {
  // Check if we have 98S or 90S battery. If the 98th cell is valid range, we are on a 98S battery
  if ((datalayer_battery->status.cell_voltages_mV[97] > 2000) &&
      (datalayer_battery->status.cell_voltages_mV[97] < 4500)) {
    datalayer_battery->info.number_of_cells = 98;
    datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;
    datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_98S_DV;
    datalayer_battery->info.total_capacity_Wh = 64000;
  } else {
    datalayer_battery->info.number_of_cells = 90;
    datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_90S_DV;
    datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;
    datalayer_battery->info.total_capacity_Wh = 40000;
  }
}

void KiaHyundai64Battery::set_cell_voltages(uint8_t reading, uint8_t cellNumber) {
  if (reading > 4) {
    datalayer.battery.status.cell_voltages_mV[cellNumber] = (reading * 20);
  }
}

void KiaHyundai64Battery::process_cell_voltage_group(const uint8_t* data, uint8_t baseCell) {
  for (int i = 0; i < 32; i++) {
    set_cell_voltages(data[4 + i], baseCell + i);
  }
}

template <typename T>
inline String& operator<<(String& str, const T& value) {
  str += value;
  return str;
}
String KiaHyundai64Battery::get_uds_info_html() {
  String content;
  content.reserve(3600);

  // Handle serial number conversion
  char readableSerialNumber[17];
  memcpy(readableSerialNumber, ecu_serial_number, sizeof(ecu_serial_number));
  readableSerialNumber[16] = '\0';

  // Handle version number conversion
  char readableVersionNumber[17];
  memcpy(readableVersionNumber, ecu_version_number, sizeof(ecu_version_number));
  readableVersionNumber[16] = '\0';

  // clang-format off
  content << "<h4>BMS serial number: " << String(readableSerialNumber) << "</h4>"
  "<h4>BMS software version: " << String(readableVersionNumber) << "</h4>"
  "<h4>Cells: " << String(datalayer_battery->info.number_of_cells) << " S</h4>"
  "<h4>12V voltage: " << String(leadAcidBatteryVoltage / 10.0f, 1) << " V</h4>"
  "<h4>Waterleakage: ";
  
  if (waterleakageSensor == 0) {
    content << "LEAK DETECTED</h4>";
  } else if (waterleakageSensor == 164) {
    content << "No leakage</h4>";
  } else {
    content << String(waterleakageSensor) << "</h4>";
  }
  
  content << "<h4>Temperature, water inlet: " << String(temperature_water_inlet) << " &deg;C</h4>"
  "<h4>Temperature, power relay: " << String(powerRelayTemperature) << " &deg;C</h4>"
  "<h4>Batterymanagement mode: " << String(batteryManagementMode) << "</h4>"
  "<h4>BMS ignition: " << String(BMS_ign) << "</h4>"
  "<h4>Battery relay: " << String(batteryRelay) << "</h4>"
  "<h4>Inverter voltage: " << String(inverterVoltage) << " V</h4>"
  "<h4>Isolation resistance: " << String(isolation_resistance_kOhm) << " kOhm</h4>"
  "<h4>Power on total time: " << String(powered_on_total_time) << " s</h4>"
  "<h4>Fastcharging sessions: " << String(number_of_fastcharging_sessions) << " x</h4>"
  "<h4>Slowcharging sessions: " << String(number_of_standard_charging_sessions) << " x</h4>"
  "<h4>Normal charged energy amount: " << String(accumulated_normal_charging_energy_kWh) << " kWh</h4>"
  "<h4>Fastcharged energy amount: " << String(accumulated_fastcharging_energy_kWh) << " kWh</h4>"
  "<h4>Total amount charged energy: " << String(cumulative_energy_charged_kWh / 10.0) << " kWh</h4>"
  "<h4>Total amount discharged energy: " << String(cumulative_energy_discharged_kWh / 10.0) << " kWh</h4>"
  "<h4>Cumulative charge current: " << String(cumulative_charge_current_ah / 10.0) << " Ah</h4>"
  "<h4>Cumulative discharge current: " << String(cumulative_discharge_current_ah / 10.0) << " Ah</h4>";
  // clang-format on

  return content;
}

void KiaHyundai64Battery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // UDS frames (0x7EC PID/DTC replies) are handled by the superclass.
  if (handle_incoming_uds_can_frame(rx_frame)) {
    return;
  }

  startedUp = true;
  switch (rx_frame.ID) {
    case 0x4DE:
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x542:  //BMS SOC
      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      SOC_Display = rx_frame.data.u8[0] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x594:
      allowedChargePower = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) / 2;
      allowedDischargePower = ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) / 2;
      SOC_BMS = rx_frame.data.u8[5] * 5;  //100% = 200 ( 200 * 5 = 1000 )
      break;
    case 0x595:
      batteryVoltage = (rx_frame.data.u8[7] << 8) + rx_frame.data.u8[6];
      batteryAmps = (rx_frame.data.u8[5] << 8) + rx_frame.data.u8[4];
      if (counter_200 > 3) {
        KIA_HYUNDAI_524.data.u8[0] = (uint8_t)(batteryVoltage / 10);
        KIA_HYUNDAI_524.data.u8[1] = (uint8_t)((batteryVoltage / 10) >> 8);
      }  //VCU measured voltage sent back to bms
      break;
    case 0x596:
      leadAcidBatteryVoltage = rx_frame.data.u8[1];  //12v Battery Volts
      temperatureMin = rx_frame.data.u8[6];          //Lowest temp in battery
      temperatureMax = rx_frame.data.u8[7];          //Highest temp in battery
      break;
    case 0x598:
      break;
    case 0x5D5:
      waterleakageSensor = rx_frame.data.u8[3];  //Water sensor inside pack, value 164 is no water --> 0 is short
      powerRelayTemperature = rx_frame.data.u8[7];
      break;
    case 0x5D8:
      if (datalayer.system.status.system_status == FAULT) {
        //If we are in fault mode and still have CAN communication, request contactors to open via UDS
        open_state++;
        if (open_state == 1) {  //Enter elevated mode
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[0] = 0x02;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[1] = 0x10;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[2] = 0x03;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[3] = 0x00;
        } else if (open_state == 2) {  //Request negative contactor OFF
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[0] = 0x04;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[1] = 0x2F;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[2] = 0xF0;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[3] = 0x32;
        } else if (open_state == 3) {  //Enter elevated mode
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[0] = 0x02;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[1] = 0x10;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[2] = 0x03;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[3] = 0x00;
        } else if (open_state == 4) {  //Request positive contactor OFF
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[0] = 0x04;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[1] = 0x2F;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[2] = 0xF0;
          KIA64_7E4_OPEN_CONTACTOR_SEQUENCE.data.u8[3] = 0x31;
          open_state = 0;
        }
        transmit_can_frame(&KIA64_7E4_OPEN_CONTACTOR_SEQUENCE);
        set_event(EVENT_CONTACTOR_OPEN, 0);
      }

      break;
    case 0x7EC:  //Data From polled PID group, BigEndian
      //Now handled in UDS superclass
      break;
    default:
      break;
  }
}

void KiaHyundai64Battery::transmit_can(unsigned long currentMillis) {

  if (!startedUp || (datalayer.system.status.system_status == FAULT)) {
    return;  // Don't send any CAN messages towards battery until it has started up. Also stop sending if we are in critical FAULT mode
  }

  // UDS PID polling and DTC handling
  transmit_uds_can(currentMillis);

  //Send 100ms message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    if ((contactor_closing_allowed == nullptr || *contactor_closing_allowed) &&
        datalayer.system.status.inverter_allows_contactor_closing) {
      transmit_can_frame(&KIA64_553);
      transmit_can_frame(&KIA64_57F);
      transmit_can_frame(&KIA64_2A1);
    }
  }

  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    previousMillis10 = currentMillis;

    if ((contactor_closing_allowed == nullptr || *contactor_closing_allowed) &&
        datalayer.system.status.inverter_allows_contactor_closing) {

      switch (counter_200) {
        case 0:
          KIA_HYUNDAI_200.data.u8[5] = 0x17;
          ++counter_200;
          break;
        case 1:
          KIA_HYUNDAI_200.data.u8[5] = 0x57;
          ++counter_200;
          break;
        case 2:
          KIA_HYUNDAI_200.data.u8[5] = 0x97;
          ++counter_200;
          break;
        case 3:
          KIA_HYUNDAI_200.data.u8[5] = 0xD7;
          ++counter_200;
          break;
        case 4:
          KIA_HYUNDAI_200.data.u8[3] = 0x10;
          KIA_HYUNDAI_200.data.u8[5] = 0xFF;
          ++counter_200;
          break;
        case 5:
          KIA_HYUNDAI_200.data.u8[5] = 0x3B;
          ++counter_200;
          break;
        case 6:
          KIA_HYUNDAI_200.data.u8[5] = 0x7B;
          ++counter_200;
          break;
        case 7:
          KIA_HYUNDAI_200.data.u8[5] = 0xBB;
          ++counter_200;
          break;
        case 8:
          KIA_HYUNDAI_200.data.u8[5] = 0xFB;
          counter_200 = 5;
          break;
      }

      transmit_can_frame(&KIA_HYUNDAI_200);
      transmit_can_frame(&KIA_HYUNDAI_523);
      transmit_can_frame(&KIA_HYUNDAI_524);
    }
  }
}

uint16_t KiaHyundai64Battery::handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) {
  // Called by the UDS superclass for every successful PID response. `value` is
  // the big-endian PID value (up to 4 bytes), `data` points at the raw value
  // bytes (without the SID/DID header). Return 0 to continue the scan list.
  switch (pid) {
    case POLL_ECU_SERIAL:
      if (length >= 15) {
        //Loop thru the 15 bytes of the serial number, and store them in the ecu_serial_number array
        for (int i = 0; i < 15; i++) {
          ecu_serial_number[i] = data[i];
        }
      }
      break;
    case POLL_ECU_VERSION:
      if (length >= 15) {
        //Loop thru the 15 bytes of the version number, and store them in the ecu_version_number array
        for (int i = 0; i < 15; i++) {
          ecu_version_number[i] = data[i];
        }
      }
      break;
    case POLL_GROUP_1:  //59 bytes
      // Frame 10 (ff f7 e7)//data[0-2] Most likely unused
      // Frame 21 (ff 32 00 00 00 00 07) data3-9
      //SOC_BMS = data[4] * 5;                               //56
      //allowedChargePower = ((data[5] << 8) + data[6]);     //00 00 (apparently not working)
      //allowedDischargePower = ((data[7] << 8) + data[8]);  //00 00 (apparently not working)
      //Frame 22 (ff c1 0d b5 16 14 14) data10-16
      //batteryAmps = (data[10] << 8) + data[11];
      //batteryVoltage = (data[12] << 8) + data[13];
      //temperatureMax = data[14]; Not required, we read from constantly sent CAN
      //temperatureMin = data[15]; Not required, we read from constantly sent CAN
      //temperatureAvg = data[16]; Not required
      // Frame 23 (14 15 15 00 00 15 b3) data17-23
      temperature_water_inlet = data[22];
      CellVoltMax_mV = (data[23] * 20);
      // Frame 24 (2c b2 01 00 00 78 00) data24-30
      CellVmaxNo = data[24];
      CellVoltMin_mV = (data[25] * 20);
      CellVminNo = data[26];
      leadAcidBatteryVoltage = data[29];
      // Frame 25 (01 98 c7 00 01 97 7e) data31-37
      //cumulativeChargeEnergy = data[31] << 16 | data[32] << 8 | data[33];
      //cumulativeDischargeEnergy = data[35] << 16 | data[36] << 8 | data[37];
      //Frame 26 (00 00 95 ec 00 00 90) data38-44
      //cumulativeChargeEnergy2 = data[39] << 16 | data[40] << 8 | data[41];
      //cumulativeDischargeEnergy2 = data[43] << 16 | data[44] << 8 | data[45]; //Flow over
      //Frame 27 (8b 01 02 1d 12 09 01) data45-51
      powered_on_total_time = data[46] << 24 | data[47] << 16 | data[48] << 8 | data[49];
      BMS_ign = data[50];
      inverterVoltage = ((data[51] << 8) + data[52]);  //Flow over
      //Frame 28 (5e 7f ff 7f ff 00 00) data52-58
      break;
    case POLL_GROUP_2:  //Cellvoltages, Cells 1-32
      process_cell_voltage_group(data, 0);
      break;
    case POLL_GROUP_3:  //Cellvoltages, Cells 33-64
      process_cell_voltage_group(data, 32);
      break;
    case POLL_GROUP_4:  //Cellvoltages, Cells 65-96 (Some batteries have only 90 cells)
      process_cell_voltage_group(data, 64);
      break;
    case POLL_GROUP_5:
      //Frame 0 (10 2e 62 01 05 00 3f ff) //data0-2
      //Frame21 90 00 00 00 00 00 00 //data3-9
      //Frame22 00 00 15 78 5e 01 21 //data10-16
      //Frame23 34 1f 0e 00 01 64 1e //data17-23
      heatertemp = data[23];
      //Frame24 33 03 b6 00 00 5b 00 //data24-30
      batterySOH = (data[25] << 8) | data[26];
      //amountOfCells = data[29];
      //Frame25 33 00 00 b3 b3 01 00 //data31-37
      if (data[34] > 4) {  //Only valid on 98S
        cellvoltages_mv[96] = data[34] * 20;
      }
      if (data[35] > 4) {  //Only valid on 98S
        cellvoltages_mv[97] = data[35] * 20;
      }
      //SOC_Display = data[31] * 5; Not required, we read from constantly sent CAN
      //Frame26 1e 00 00 00 00 aa aa //data38-44
      break;
    case POLL_GROUP_6:
      //Frame 0 (ff ff ff) //data0-2
      //Frame21 ff 15 00 d0 00 00 00 //data3-9
      //Frame22 00 00 00 00 04 00 00 //data10-16
      batteryManagementMode = data[14];
      //Frame23 00 00 08 0f 31 ea 00 //data17-23
      break;
    case POLL_GROUP_11:
      //Frame 0 (f8 00 00) //data0-2
      //Frame21 00 00 00 00 38 00 00 //data3-9
      number_of_standard_charging_sessions = ((data[6] << 8) | data[7]);
      //Frame22 00 27 00 00 03 b5 00 //data10-16
      number_of_fastcharging_sessions = ((data[10] << 8) | data[11]);
      accumulated_normal_charging_energy_kWh = ((data[14] << 8) | data[15]);
      //Frame23 00 05 94 0d 69 aa aa //data17-23
      accumulated_fastcharging_energy_kWh = ((data[18] << 8) | data[19]);
      break;
    default:  //Unknown pid
      break;
  }
  return 0;  //Continue scanning the PID list in order
}

void KiaHyundai64Battery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer_battery->info.max_design_voltage_dV = MAX_PACK_VOLTAGE_98S_DV;  //Start with 98S value. Precised later
  datalayer_battery->info.min_design_voltage_dV = MIN_PACK_VOLTAGE_90S_DV;  //Start with 90S value. Precised later
  datalayer_battery->info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer_battery->info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  if (allows_contactor_closing) {
    *allows_contactor_closing = true;
  }
  // UDS: send requests to 0x7E4, accept replies from the BMS on 0x7EC. Also passing true to isFD
  setup_uds(0x7E4, 0x7EC, true);
  static const uint16_t pid_scan_list[] = {
      POLL_GROUP_1, POLL_GROUP_2,  POLL_GROUP_3,    POLL_GROUP_4,     POLL_GROUP_5,
      POLL_GROUP_6, POLL_GROUP_11, POLL_ECU_SERIAL, POLL_ECU_VERSION,
  };
  set_pid_scan_list(pid_scan_list, sizeof(pid_scan_list) / sizeof(pid_scan_list[0]));
}
