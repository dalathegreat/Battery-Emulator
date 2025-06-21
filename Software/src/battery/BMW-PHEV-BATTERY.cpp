#include "BMW-PHEV-BATTERY.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../include.h"

const unsigned char crc8_table[256] =
    {  // CRC8_SAE_J1850_ZER0 formula,0x1D Poly,initial value 0x3F,Final XOR value varies
        0x00, 0x1D, 0x3A, 0x27, 0x74, 0x69, 0x4E, 0x53, 0xE8, 0xF5, 0xD2, 0xCF, 0x9C, 0x81, 0xA6, 0xBB, 0xCD, 0xD0,
        0xF7, 0xEA, 0xB9, 0xA4, 0x83, 0x9E, 0x25, 0x38, 0x1F, 0x02, 0x51, 0x4C, 0x6B, 0x76, 0x87, 0x9A, 0xBD, 0xA0,
        0xF3, 0xEE, 0xC9, 0xD4, 0x6F, 0x72, 0x55, 0x48, 0x1B, 0x06, 0x21, 0x3C, 0x4A, 0x57, 0x70, 0x6D, 0x3E, 0x23,
        0x04, 0x19, 0xA2, 0xBF, 0x98, 0x85, 0xD6, 0xCB, 0xEC, 0xF1, 0x13, 0x0E, 0x29, 0x34, 0x67, 0x7A, 0x5D, 0x40,
        0xFB, 0xE6, 0xC1, 0xDC, 0x8F, 0x92, 0xB5, 0xA8, 0xDE, 0xC3, 0xE4, 0xF9, 0xAA, 0xB7, 0x90, 0x8D, 0x36, 0x2B,
        0x0C, 0x11, 0x42, 0x5F, 0x78, 0x65, 0x94, 0x89, 0xAE, 0xB3, 0xE0, 0xFD, 0xDA, 0xC7, 0x7C, 0x61, 0x46, 0x5B,
        0x08, 0x15, 0x32, 0x2F, 0x59, 0x44, 0x63, 0x7E, 0x2D, 0x30, 0x17, 0x0A, 0xB1, 0xAC, 0x8B, 0x96, 0xC5, 0xD8,
        0xFF, 0xE2, 0x26, 0x3B, 0x1C, 0x01, 0x52, 0x4F, 0x68, 0x75, 0xCE, 0xD3, 0xF4, 0xE9, 0xBA, 0xA7, 0x80, 0x9D,
        0xEB, 0xF6, 0xD1, 0xCC, 0x9F, 0x82, 0xA5, 0xB8, 0x03, 0x1E, 0x39, 0x24, 0x77, 0x6A, 0x4D, 0x50, 0xA1, 0xBC,
        0x9B, 0x86, 0xD5, 0xC8, 0xEF, 0xF2, 0x49, 0x54, 0x73, 0x6E, 0x3D, 0x20, 0x07, 0x1A, 0x6C, 0x71, 0x56, 0x4B,
        0x18, 0x05, 0x22, 0x3F, 0x84, 0x99, 0xBE, 0xA3, 0xF0, 0xED, 0xCA, 0xD7, 0x35, 0x28, 0x0F, 0x12, 0x41, 0x5C,
        0x7B, 0x66, 0xDD, 0xC0, 0xE7, 0xFA, 0xA9, 0xB4, 0x93, 0x8E, 0xF8, 0xE5, 0xC2, 0xDF, 0x8C, 0x91, 0xB6, 0xAB,
        0x10, 0x0D, 0x2A, 0x37, 0x64, 0x79, 0x5E, 0x43, 0xB2, 0xAF, 0x88, 0x95, 0xC6, 0xDB, 0xFC, 0xE1, 0x5A, 0x47,
        0x60, 0x7D, 0x2E, 0x33, 0x14, 0x09, 0x7F, 0x62, 0x45, 0x58, 0x0B, 0x16, 0x31, 0x2C, 0x97, 0x8A, 0xAD, 0xB0,
        0xE3, 0xFE, 0xD9, 0xC4};

/*
INFO

V0.1  very basic implementation reading Gen3/4 BMW PHEV SME. 
Supported:
-Pre/Post contactor voltage
-Min/ Cell Temp
-SOC

VEHICLE CANBUS
i3 messages don't affect contact close block except 0x380 which gives an error on PHEV (Possibly VIN number) 0x56, 0x5A, 0x37, 0x39, 0x34, 0x34, 0x34

BROADCAST MAP
0x112 20ms Status Of High-Voltage Battery 2
0x1F1 1000ms Status Of High-Voltage Battery 1
0x239 200ms predicted charge condition and predicted target
0x295 1000ms ? [1] Alive Counter 50-5F?
0x2A5 200ms ?
0x2F5 100ms High-Voltage Battery Charge/Discharge Limitations
0x33e 5000ms?
0x40D 1000ms ?
0x430 1000ms Charging status of high-voltage battery - 2
0x431 200ms Data High-Voltage Battery Unit
0x432 200ms SOC% info

UDS MAP
22 D6 CF - CSC Temps
22 DD C0 - Min Max temps
22 DF A5 - All Cell voltages
22 E5 EA - Alternate all cell voltages
22 DE 7E - Voltage limits.   62 DD 73 9D 5A 69 26 = 269.18V - 402.82V
22 DD 7D - Current limits 62 DD 7D 08 20 0B EA = 305A max discharge 208A max charge
22 E5 E9 DD 7D - Individual cell SOC
22 DD 69 - Current in Amps 62 DD 69 00 00 00 00 = 0 Amps
22 DD 7B - SOH  62 DD 7B 62 = 98%
22 DD 62 - HVIL Status 62 DD 64 01 = OK/Closed
22 DD BF - Cell min max alt - f18 ZELLSPANNUNGEN_MIN_MAX < doesn't work
22 DD 6A - Isolation values  62 DD 6A 07 D0 07 D0 07 D0 01 01 01 = in operation plausible/2000kOhm, in follow up plausible/2000kohm, internal iso open contactors (measured on request) pluasible/2000kohm
31 03 AD 61 - Isolation measurement status  71 03 AD 61 00 FF = Nmeasurement status - not successful / fault satate - not defined
22 DF A0 - Cell voltage and temps summary including min/max/average, Ah,  (ZUSTAND_SPEICHER)


TODO:
 BMWPHEV_6F1_REQUEST_LAST_ISO_READING - add results to advanced
 BMWPHEV_6F1_REQUEST_PACK_INFO - add cell count reading
 Find current measurement reading

*/

// Function to check if a value has gone stale over a specified time period
bool BmwPhevBattery::isStale(int16_t currentValue, uint16_t& lastValue, unsigned long& lastChangeTime) {
  unsigned long currentTime = millis();

  // Check if the value has changed
  if (currentValue != lastValue) {
    // Update the last change time and value
    lastChangeTime = currentTime;
    lastValue = currentValue;
    return false;  // Value is fresh because it has changed
  }

  // Check if the value has stayed the same for the specified staleness period
  return (currentTime - lastChangeTime >= STALE_PERIOD);
}

static uint8_t calculateCRC(CAN_frame rx_frame, uint8_t length, uint8_t initial_value) {
  uint8_t crc = initial_value;
  for (uint8_t j = 1; j < length; j++) {  //start at 1, since 0 is the CRC
    crc = crc8_table[(crc ^ static_cast<uint8_t>(rx_frame.data.u8[j])) % 256];
  }
  return crc;
}

static uint8_t increment_uds_req_id_counter(uint8_t index, int numReqs) {
  index++;
  if (index >= numReqs) {
    index = 0;
  }
  return index;
}

/* --------------------------------------------------------------------------
   UDS Multi-Frame Helpers
   -------------------------------------------------------------------------- */

void BmwPhevBattery::startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID) {
  gUDSContext.UDS_inProgress = true;
  gUDSContext.UDS_expectedLength = totalLength;
  gUDSContext.UDS_bytesReceived = 0;
  gUDSContext.UDS_moduleID = moduleID;
  memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
  gUDSContext.UDS_lastFrameMillis = millis();  // if you want to track timeouts
}

bool BmwPhevBattery::storeUDSPayload(const uint8_t* payload, uint8_t length) {
  if (gUDSContext.UDS_bytesReceived + length > sizeof(gUDSContext.UDS_buffer)) {
    // Overflow => abort
    gUDSContext.UDS_inProgress = false;
#ifdef DEBUG_LOG
    logging.println("UDS Payload Overflow");
#endif  // DEBUG_LOG
    return false;
  }
  memcpy(&gUDSContext.UDS_buffer[gUDSContext.UDS_bytesReceived], payload, length);
  gUDSContext.UDS_bytesReceived += length;
  gUDSContext.UDS_lastFrameMillis = millis();

  // If we’ve reached or exceeded the expected length, mark complete
  if (gUDSContext.UDS_bytesReceived >= gUDSContext.UDS_expectedLength) {
    gUDSContext.UDS_inProgress = false;
    // #ifdef DEBUG_LOG
    //     logging.println("Recived all expected UDS bytes");
    // #endif  // DEBUG_LOG
  }
  return true;
}

bool BmwPhevBattery::isUDSMessageComplete() {
  return (!gUDSContext.UDS_inProgress && gUDSContext.UDS_bytesReceived > 0);
}

uint8_t BmwPhevBattery::increment_alive_counter(uint8_t counter) {
  counter++;
  if (counter > ALIVE_MAX_VALUE) {
    counter = 0;
  }
  return counter;
}

static byte increment_0C0_counter(byte counter) {
  counter++;
  // Reset to 0xF0 if it exceeds 0xFE
  if (counter > 0xFE) {
    counter = 0xF0;
  }
  return counter;
}

void BmwPhevBattery::processCellVoltages() {
  const int startByte = 3;     // Start reading at byte 3
  const int numVoltages = 96;  // Number of cell voltage values to process
  int voltage_index = 0;       // Starting index for the destination array

  // Loop through 96 voltage values
  for (int i = 0; i < numVoltages; i++) {
    // Calculate the index of the first and second bytes in the input array
    int byteIndex = startByte + (i * 2);

    // Combine two bytes to form a 16-bit value
    uint16_t voltageRaw = (gUDSContext.UDS_buffer[byteIndex] << 8) | gUDSContext.UDS_buffer[byteIndex + 1];

    // Store the result in the destination array
    datalayer.battery.status.cell_voltages_mV[voltage_index] = voltageRaw;

    // Increment the destination array index
    voltage_index++;
  }
}

void BmwPhevBattery::wake_battery_via_canbus() {
  //TJA1055 transceiver remote wake requires pulses on the bus of
  // Dominant for at least ~7 µs (min) and at most ~38 µs (max)
  // Followed by a Recessive interval of at least ~3 µs (min) and at most ~10 µs (max)
  // Then a second dominant pulse of similar timing.

  CAN_cfg.speed = CAN_SPEED_100KBPS;  //Slow down canbus to achieve wakeup timings
  ESP32Can.CANInit();                 // ReInit native CAN module at new speed
  transmit_can_frame(&BMW_PHEV_BUS_WAKEUP_REQUEST, can_config.battery);
  transmit_can_frame(&BMW_PHEV_BUS_WAKEUP_REQUEST, can_config.battery);
  CAN_cfg.speed = CAN_SPEED_500KBPS;  //Resume fullspeed
  ESP32Can.CANInit();                 // ReInit native CAN module at new speed

#ifdef DEBUG_LOG
  logging.println("Sent magic wakeup packet to SME at 100kbps...");
#endif
}

void BmwPhevBattery::update_values() {  //This function maps all the values fetched via CAN to the battery datalayer

  datalayer.battery.status.real_soc = avg_soc_state;

  datalayer.battery.status.voltage_dV = battery_voltage;

  datalayer.battery.status.current_dA = battery_current;

  datalayer.battery.info.total_capacity_Wh = (battery_energy_content_maximum_kWh * 1000);  // Convert kWh to Wh

  datalayer.battery.status.remaining_capacity_Wh = battery_predicted_energy_charge_condition;

  datalayer.battery.status.soh_pptt = min_soh_state;

  datalayer.battery.status.max_discharge_power_W = battery_BEV_available_power_longterm_discharge;

  //datalayer.battery.status.max_charge_power_W = 3200; //10000; //Aux HV Port has 100A Fuse  Moved to Ramping

  // Charge power is set in .h file
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        battery_BEV_available_power_longterm_charge *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = battery_BEV_available_power_longterm_charge;
  }

  datalayer.battery.status.temperature_min_dC = battery_temperature_min * 10;  // Add a decimal

  datalayer.battery.status.temperature_max_dC = battery_temperature_max * 10;  // Add a decimal

  //Check stale values. As values dont change much during idle only consider stale if both parts of this message freeze.
  bool isMinCellVoltageStale =
      isStale(min_cell_voltage, datalayer.battery.status.cell_min_voltage_mV, min_cell_voltage_lastchanged);
  bool isMaxCellVoltageStale =
      isStale(max_cell_voltage, datalayer.battery.status.cell_max_voltage_mV, max_cell_voltage_lastchanged);

  if (isMinCellVoltageStale && isMaxCellVoltageStale &&
      battery_current != 0) {                             //Ignore stale values if there is no current flowing
    datalayer.battery.status.cell_min_voltage_mV = 9999;  //Stale values force stop
    datalayer.battery.status.cell_max_voltage_mV = 9999;  //Stale values force stop
    set_event(EVENT_STALE_VALUE, 0);
#ifdef DEBUG_LOG
    logging.println("Stale Min/Max voltage values detected during charge/discharge sending - 9999mV...");
#endif  // DEBUG_LOG
  } else {

    datalayer.battery.status.cell_min_voltage_mV = min_cell_voltage;  //Value is alive
    datalayer.battery.status.cell_max_voltage_mV = max_cell_voltage;  //Value is alive
  }

  datalayer.battery.info.max_design_voltage_dV = max_design_voltage;

  datalayer.battery.info.min_design_voltage_dV = min_design_voltage;

  datalayer.battery.info.number_of_cells = detected_number_of_cells;

  datalayer_extended.bmwphev.min_cell_voltage_data_age = (millis() - min_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.max_cell_voltage_data_age = (millis() - max_cell_voltage_lastchanged);

  datalayer_extended.bmwphev.T30_Voltage = terminal30_12v_voltage;

  datalayer_extended.bmwphev.hvil_status = hvil_status;

  datalayer_extended.bmwphev.allowable_charge_amps = allowable_charge_amps;

  datalayer_extended.bmwphev.allowable_discharge_amps = allowable_discharge_amps;

  datalayer_extended.bmwphev.balancing_status = balancing_status;

  datalayer_extended.bmwphev.battery_voltage_after_contactor = battery_voltage_after_contactor;

  // Update webserver datalayer

  datalayer_extended.bmwphev.ST_iso_ext = battery_status_error_isolation_external_Bordnetz;
  datalayer_extended.bmwphev.ST_iso_int = battery_status_error_isolation_internal_Bordnetz;
  datalayer_extended.bmwphev.ST_valve_cooling = battery_status_valve_cooling;
  datalayer_extended.bmwphev.ST_interlock = battery_status_error_locking;
  datalayer_extended.bmwphev.ST_precharge = battery_status_precharge_locked;
  datalayer_extended.bmwphev.ST_DCSW = battery_status_disconnecting_switch;
  datalayer_extended.bmwphev.ST_EMG = battery_status_emergency_mode;
  datalayer_extended.bmwphev.ST_WELD = battery_status_error_disconnecting_switch;
  datalayer_extended.bmwphev.ST_isolation = battery_status_warning_isolation;
  datalayer_extended.bmwphev.ST_cold_shutoff_valve = battery_status_cold_shutoff_valve;
  datalayer_extended.bmwphev.iso_safety_int_kohm = iso_safety_int_kohm;
  datalayer_extended.bmwphev.iso_safety_ext_kohm = iso_safety_ext_kohm;
  datalayer_extended.bmwphev.iso_safety_trg_kohm = iso_safety_trg_kohm;
  datalayer_extended.bmwphev.iso_safety_ext_plausible = iso_safety_ext_plausible;
  datalayer_extended.bmwphev.iso_safety_int_plausible = iso_safety_int_plausible;
  datalayer_extended.bmwphev.iso_safety_kohm = iso_safety_kohm;
  datalayer_extended.bmwphev.iso_safety_kohm_quality = iso_safety_kohm_quality;

  if (pack_limit_info_available) {
    // If we have pack limit data from battery - override the defaults to suit
    datalayer.battery.info.max_design_voltage_dV = max_design_voltage;
    datalayer.battery.info.min_design_voltage_dV = min_design_voltage;
  }
  if (cell_limit_info_available) {
    // If we have cell limit data from battery - override the defaults to suit
    datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
    datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  }
}
void BmwPhevBattery::handle_incoming_can_frame(CAN_frame rx_frame) {

  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x112:
      battery_awake = true;
      datalayer.battery.status.CAN_battery_still_alive =
          CAN_STILL_ALIVE;  //This message is only sent if 30C (Wakeup pin on battery) is energized with 12V
      break;
    case 0x2F5:  //BMS [100ms] High-Voltage Battery Charge/Discharge Limitations
      battery_max_charge_voltage = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_max_charge_amperage = (((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) - 819.2);
      battery_min_discharge_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_max_discharge_amperage = (((rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6]) - 819.2);
      break;
    case 0x239:                                                                                      //BMS [200ms]
      battery_predicted_energy_charge_condition = (rx_frame.data.u8[2] << 8 | rx_frame.data.u8[1]);  //Wh
      battery_predicted_energy_charging_target = ((rx_frame.data.u8[4] << 8 | rx_frame.data.u8[3]) * 0.02);  //kWh
      break;
    case 0x40D:  //BMS [1s] Charging status of high-voltage storage - 1
      battery_BEV_available_power_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]) * 3;
      battery_BEV_available_power_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]) * 3;
      battery_BEV_available_power_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]) * 3;
      battery_BEV_available_power_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]) * 3;
      break;
    case 0x430:  //BMS [1s] - Charging status of high-voltage battery - 2
      battery_prediction_voltage_shortterm_charge = (rx_frame.data.u8[1] << 8 | rx_frame.data.u8[0]);
      battery_prediction_voltage_shortterm_discharge = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_voltage_longterm_charge = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[4]);
      battery_prediction_voltage_longterm_discharge = (rx_frame.data.u8[7] << 8 | rx_frame.data.u8[6]);
      break;
    case 0x431:  //BMS [200ms] Data High-Voltage Battery Unit
      battery_status_service_disconnection_plug = (rx_frame.data.u8[0] & 0x0F);
      battery_status_measurement_isolation = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_abort_charging = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_prediction_duration_charging_minutes = (rx_frame.data.u8[3] << 8 | rx_frame.data.u8[2]);
      battery_prediction_time_end_of_charging_minutes = rx_frame.data.u8[4];
      battery_energy_content_maximum_kWh = (((rx_frame.data.u8[6] & 0x0F) << 8 | rx_frame.data.u8[5])) / 50;
      break;
    case 0x432:  //BMS [200ms] SOC% info
      battery_request_operating_mode = (rx_frame.data.u8[0] & 0x03);
      battery_target_voltage_in_CV_mode = ((rx_frame.data.u8[1] << 4 | rx_frame.data.u8[0] >> 4)) / 10;
      battery_request_charging_condition_minimum = (rx_frame.data.u8[2] / 2);
      battery_request_charging_condition_maximum = (rx_frame.data.u8[3] / 2);
      battery_display_SOC = rx_frame.data.u8[4];
      break;
    case 0x607:  //SME responds to UDS requests on 0x607
    {
      // UDS Multi Frame vars - Top nibble indicates Frame Type: SF (0), FF (1), CF (2), FC (3)
      // Extended addressing => data[0] is ext address, data[1] is PCI
      uint8_t extAddr = rx_frame.data.u8[0];  // e.g., 0xF1
      uint8_t pciByte = rx_frame.data.u8[1];  // e.g., 0x10, 0x21, etc.
      uint8_t pciType = pciByte >> 4;         // top nibble => 0=SF,1=FF,2=CF,3=FC
      uint8_t pciLower = pciByte & 0x0F;      // bottom nibble => length nibble or sequence

      switch (pciType) {
        case 0x0: {
          // Single Frame reponse
          // SF payload length is in pciLower
          uint8_t sfLength = pciLower;
          uint8_t moduleID = rx_frame.data.u8[5];

          if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xDD && rx_frame.data.u8[4] == 0xC4) {  // SOC%
            avg_soc_state = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);
          }
          if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xDD && rx_frame.data.u8[4] == 0x7B) {  // SOH%
            min_soh_state = (rx_frame.data.u8[5]) * 100;
          }

          if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xD6 && rx_frame.data.u8[4] == 0xD9) {  // Isolation Reading 2
            iso_safety_kohm = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]);  //STAT_R_ISO_ROH_01_WERT
            iso_safety_kohm_quality =
                (rx_frame.data.u8[7]);  //STAT_R_ISO_ROH_QAL_01_INFO Quality of measurement 0-21 (higher better)
          }

          if (rx_frame.DLC = 8 && rx_frame.data.u8[3] == 0xDD &&
                             rx_frame.data.u8[4] == 0xB4) {  //Main Battery Voltage (Pre Contactor)
            battery_voltage = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
          }

          if (rx_frame.DLC = 7 && rx_frame.data.u8[3] == 0xDD &&
                             rx_frame.data.u8[4] == 0x66) {  //Main Battery Voltage (Post Contactor)
            battery_voltage_after_contactor = (rx_frame.data.u8[5] << 8 | rx_frame.data.u8[6]) / 10;
          }

          if (rx_frame.DLC = 7 && rx_frame.data.u8[1] == 0x05 && rx_frame.data.u8[2] == 0x71 &&
                             rx_frame.data.u8[3] == 0x03 &&
                             rx_frame.data.u8[4] ==
                                 0xAD) {  //Balancing Status  01 Active 03 Not Active    7DLC F1 05 71 03 AD 6B 01
            balancing_status = (rx_frame.data.u8[6]);
            // #ifdef DEBUG_LOG

            //             logging.println("Balancing Status received");

            // #endif  // DEBUG_LOG
          }

          break;
        }
        case 0x1: {
          // total length = (pciLower << 8) + data[2]
          uint16_t totalLength = ((uint16_t)pciLower << 8) | rx_frame.data.u8[2];
          uint8_t moduleID = rx_frame.data.u8[5];
#if defined(DEBUG_LOG) && defined(UDS_LOG)
          logging.print("FF arrived! moduleID=0x");
          logging.print(moduleID, HEX);
          logging.print(", totalLength=");
          logging.println(totalLength);
#endif  // DEBUG_LOG && UDS_LOG

          // Start the multi-frame
          startUDSMultiFrameReception(totalLength, moduleID);
          gUDSContext.receivedInBatch = 0;  // Reset batch count
          // The FF payload is at data[3..7] (5 bytes) for an 8-byte CAN frame in extended addressing
          const uint8_t* ffPayload = &rx_frame.data.u8[3];
          uint8_t ffPayloadSize = 5;
          storeUDSPayload(ffPayload, ffPayloadSize);

#if defined(DEBUG_LOG) && defined(UDS_LOG)
          logging.print("After FF, UDS_bytesReceived=");
          logging.println(gUDSContext.UDS_bytesReceived);
#endif  // DEBUG_LOG && UDS_LOG
#if defined(DEBUG_LOG) && defined(UDS_LOG)
          logging.println("Requesting continue frame...");
#endif  // DEBUG_LOG && UDS_LOG
          transmit_can_frame(&BMW_6F1_REQUEST_CONTINUE_MULTIFRAME, can_config.battery);
          break;
        }

        case 0x2: {
          // The sequence number is in (data[0] & 0x0F), but we often don’t need it if frames are in order.
          // Make sure we *are* in progress
          if (!gUDSContext.UDS_inProgress) {
// Unexpected CF. Possibly ignore or reset.
#if defined(DEBUG_LOG) && defined(UDS_LOG)
            uint8_t seq = pciByte & 0x0F;
            logging.print("Unexpected CF --- seq=0x");
            logging.print(seq, HEX);
            logging.print(" for moduleID=0x");
            logging.println(gUDSContext.UDS_moduleID, HEX);
#endif  // DEBUG_LOG && UDS_LOG
            return;
          }
#if defined(DEBUG_LOG) && defined(UDS_LOG)
          uint8_t seq = pciByte & 0x0F;
          logging.print("CF seq=0x");
          logging.print(seq, HEX);
          logging.print("CF pcibyte=0x");
          logging.print(pciByte, HEX);
          logging.print(" for moduleID=0x");
          logging.println(gUDSContext.UDS_moduleID, HEX);
#endif  // DEBUG_LOG && UDS_LOG

          storeUDSPayload(&rx_frame.data.u8[2], 6);
          // Increment batch counter
          gUDSContext.receivedInBatch++;
#if defined(DEBUG_LOG) && defined(UDS_LOG)
          logging.print("After CF seq=0x");
          logging.print(seq, HEX);
          logging.print(", moduleID=0x");
          logging.print(gUDSContext.UDS_moduleID, HEX);
          logging.print(", UDS_bytesReceived=");
          logging.println(gUDSContext.UDS_bytesReceived);
#endif  // DEBUG_LOG && UDS_LOG

          // Check if the batch is complete
          if (gUDSContext.receivedInBatch >= 3) {  //BMW PHEV Using batch size of 3 in continue message
                                                   // Send the next Flow Control
#if defined(DEBUG_LOG) && defined(UDS_LOG)
            logging.println("Batch Complete - Requesting continue frame...");
#endif  // DEBUG_LOG && UDS_LOG
            transmit_can_frame(&BMW_6F1_REQUEST_CONTINUE_MULTIFRAME, can_config.battery);
            gUDSContext.receivedInBatch = 0;  // Reset batch count
            Serial.println("Sent FC for next batch of 3 frames.");
          }

          break;
        }

        case 0x3: {
          // Flow Control Frame from ECU -> Tester (rare in a typical request/response flow)
          // Typically we only *send* FC. If the ECU sends one, parse or ignore here.
          break;
        }
      }

      // Optionally, check if message is complete
      if (isUDSMessageComplete()) {
        // We have a complete UDS/ISO-TP response in gUDSContext.UDS_buffer
#if defined(DEBUG_LOG) && defined(UDS_LOG)
        logging.print("UDS message complete for module ID 0x");
        logging.println(gUDSContext.UDS_moduleID, HEX);

        logging.print("Total bytes: ");
        logging.println(gUDSContext.UDS_bytesReceived);

        logging.print("Received data: ");
        for (uint16_t i = 0; i < gUDSContext.UDS_bytesReceived; i++) {
          // Optional leading zero for single-digit hex
          if (gUDSContext.UDS_buffer[i] < 0x10) {
            logging.print("0");
          }
          logging.print(gUDSContext.UDS_buffer[i], HEX);
          logging.print(" ");
        }
        logging.println();  // new line at the end
#endif                      // DEBUG_LOG

        //Cell Voltages
        if (gUDSContext.UDS_moduleID == 0xA5) {  //We have a complete set of cell voltages - pass to data layer
          processCellVoltages();
        }
        //Current measurement
        if (gUDSContext.UDS_moduleID == 0x69) {  //Current (32bit mA?  negative = discharge)
          battery_current = ((int32_t)((gUDSContext.UDS_buffer[3] << 24) | (gUDSContext.UDS_buffer[4] << 16) |
                                       (gUDSContext.UDS_buffer[5] << 8) | gUDSContext.UDS_buffer[6])) *
                            0.1;
#ifdef DEBUG_LOG
          logging.print("Received current/amps measurement data: ");
          logging.print(battery_current);
          logging.print(" - ");
          for (uint16_t i = 0; i < gUDSContext.UDS_bytesReceived; i++) {
            // Optional leading zero for single-digit hex
            if (gUDSContext.UDS_buffer[i] < 0x10) {
              logging.print("0");
            }
            logging.print(gUDSContext.UDS_buffer[i], HEX);
            logging.print(" ");
          }
          logging.println();  // new line at the end
#endif                        // DEBUG_LOG
        }

        //Cell Min/Max
        if (gUDSContext.UDS_moduleID ==
            0xA0) {  //We have a complete frame for cell min max - pass to data layer UNCONFIRMED IF THESE ARE CORRECT BYTES

          //Check values are valid
          if (gUDSContext.UDS_buffer[9] != 0xFF && gUDSContext.UDS_buffer[10] != 0xFF &&
              gUDSContext.UDS_buffer[11] != 0xFF && gUDSContext.UDS_buffer[12] != 0xFF &&
              gUDSContext.UDS_buffer[9] != 0x00 && gUDSContext.UDS_buffer[10] != 0x00 &&
              gUDSContext.UDS_buffer[11] != 0x00 && gUDSContext.UDS_buffer[12] != 0x00) {
            min_cell_voltage = (gUDSContext.UDS_buffer[9] << 8 | gUDSContext.UDS_buffer[10]) / 10;
            max_cell_voltage = (gUDSContext.UDS_buffer[11] << 8 | gUDSContext.UDS_buffer[12]) / 10;
          } else {
#ifdef DEBUG_LOG
            logging.println("Cell Min Max Invalid 65535 or 0...");
            logging.print("Received data: ");
            for (uint16_t i = 0; i < gUDSContext.UDS_bytesReceived; i++) {
              // Optional leading zero for single-digit hex
              if (gUDSContext.UDS_buffer[i] < 0x10) {
                logging.print("0");
              }
              logging.print(gUDSContext.UDS_buffer[i], HEX);
              logging.print(" ");
            }
            logging.println();  // new line at the end
#endif                          // DEBUG_LOG
          }
        }

        if (gUDSContext.UDS_moduleID == 0x7E) {  // Voltage Limits
          max_design_voltage = (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]) / 10;
          min_design_voltage = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]) / 10;
          pack_limit_info_available = true;
        }

        if (gUDSContext.UDS_moduleID == 0x7D) {  // Current Limits
          allowable_charge_amps = (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]) / 10;
          allowable_discharge_amps = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]) / 10;
        }

        if (gUDSContext.UDS_moduleID == 0x90) {  // Paired VIN
          for (int i = 0; i < 17; i++) {
            paired_vin[i] = gUDSContext.UDS_buffer[4 + i];
          }
        }
        if (gUDSContext.UDS_moduleID == 0x6A) {  // Iso Reading 1
          iso_safety_int_kohm =
              (gUDSContext.UDS_buffer[7] << 8 | gUDSContext.UDS_buffer[8]);  //STAT_ISOWIDERSTAND_INT_WERT
          iso_safety_ext_kohm =
              (gUDSContext.UDS_buffer[3] << 8 | gUDSContext.UDS_buffer[4]);  //STAT_ISOWIDERSTAND_EXT_STD_WERT
          iso_safety_trg_kohm = (gUDSContext.UDS_buffer[5] << 8 | gUDSContext.UDS_buffer[6]);
          iso_safety_ext_plausible = gUDSContext.UDS_buffer[9];   //STAT_ISOWIDERSTAND_EXT_TRG_PLAUS
          iso_safety_trg_plausible = gUDSContext.UDS_buffer[10];  //STAT_ISOWIDERSTAND_EXT_TRG_WERT
          iso_safety_int_plausible = gUDSContext.UDS_buffer[11];  //STAT_ISOWIDERSTAND_EXT_TRG_WERT
        }
      }

      break;
    }
    case 0x1FA:  //BMS [1000ms] Status Of High-Voltage Battery - 1
      battery_status_error_isolation_external_Bordnetz = (rx_frame.data.u8[0] & 0x03);
      battery_status_error_isolation_internal_Bordnetz = (rx_frame.data.u8[0] & 0x0C) >> 2;
      battery_request_cooling = (rx_frame.data.u8[0] & 0x30) >> 4;
      battery_status_valve_cooling = (rx_frame.data.u8[0] & 0xC0) >> 6;
      battery_status_error_locking = (rx_frame.data.u8[1] & 0x03);
      battery_status_precharge_locked = (rx_frame.data.u8[1] & 0x0C) >> 2;
      battery_status_disconnecting_switch = (rx_frame.data.u8[1] & 0x30) >> 4;
      battery_status_emergency_mode = (rx_frame.data.u8[1] & 0xC0) >> 6;
      battery_request_service = (rx_frame.data.u8[2] & 0x03);
      battery_error_emergency_mode = (rx_frame.data.u8[2] & 0x0C) >> 2;
      battery_status_error_disconnecting_switch = (rx_frame.data.u8[2] & 0x30) >> 4;
      battery_status_warning_isolation = (rx_frame.data.u8[2] & 0xC0) >> 6;
      battery_status_cold_shutoff_valve = (rx_frame.data.u8[3] & 0x0F);
      battery_temperature_HV = (rx_frame.data.u8[4] - 50);
      battery_temperature_heat_exchanger = (rx_frame.data.u8[5] - 50);
      if (rx_frame.data.u8[6] > 0 && rx_frame.data.u8[6] < 255) {
        battery_temperature_min = (rx_frame.data.u8[6] - 50);
      } else {
#ifdef DEBUG_LOG
        logging.println("Pre parsed Cell Temp Min is Invalid ");
#endif
      }
      if (rx_frame.data.u8[7] > 0 && rx_frame.data.u8[7] < 255) {
        battery_temperature_max = (rx_frame.data.u8[7] - 50);
      } else {
#ifdef DEBUG_LOG
        logging.println("Pre parsed Cell Temp Max is Invalid ");
#endif
      }

      break;
    default:
      break;
  }
}

void BmwPhevBattery::transmit_can(unsigned long currentMillis) {

  //if (battery_awake) { //We can always send CAN as the PHEV BMS will wake up on vehicle comms

  if (currentMillis - previousMillis20 >= INTERVAL_20_MS) {
    previousMillis20 = currentMillis;

    if (startup_counter_contactor < 160) {
      startup_counter_contactor++;
    } else {                      //After 160 messages, turn on the request
      BMW_10B.data.u8[1] = 0x10;  // Close contactors
    }

    BMW_10B.data.u8[1] = ((BMW_10B.data.u8[1] & 0xF0) + alive_counter_20ms);
    BMW_10B.data.u8[0] = calculateCRC(BMW_10B, 3, 0x3F);

    alive_counter_20ms = increment_alive_counter(alive_counter_20ms);

    BMW_13E_counter++;
    BMW_13E.data.u8[4] = BMW_13E_counter;

    //if (datalayer.battery.status.bms_status == FAULT) {  //ALLOW ANY TIME - TEST ONLY
    //}  //If battery is not in Fault mode, allow contactor to close by sending 10B
    //else {
    transmit_can_frame(&BMW_10B, can_config.battery);
    //}
  }

  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;
  }
  // Send 200ms CAN Message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;
    uds_fast_req_id_counter = increment_uds_req_id_counter(
        uds_fast_req_id_counter, numFastUDSreqs);  //Loop through and send a different UDS request each cycle
    transmit_can_frame(UDS_REQUESTS_FAST[uds_fast_req_id_counter], can_config.battery);
  }
  // Send 1000ms CAN Message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    uds_slow_req_id_counter = increment_uds_req_id_counter(
        uds_slow_req_id_counter, numSlowUDSreqs);  //Loop through and send a different UDS request each cycle
    transmit_can_frame(UDS_REQUESTS_SLOW[uds_slow_req_id_counter], can_config.battery);
  }
  // Send 5000ms CAN Message
  if (currentMillis - previousMillis5000 >= INTERVAL_5_S) {
    previousMillis5000 = currentMillis;

    // transmit_can_frame(&BMWPHEV_6F1_REQUEST_CONTACTORS_CLOSE,
    //                    can_config.battery);  // Attempt contactor close - experimental
  }
  // Send 10000ms CAN Message
  if (currentMillis - previousMillis10000 >= INTERVAL_10_S) {
    previousMillis10000 = currentMillis;
    transmit_can_frame(&BMWPHEV_6F1_REQUEST_BALANCING_START,
                       can_config.battery);  // Enable Balancing
  }
}

void BmwPhevBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, "BMW PHEV Battery", 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  //Wakeup the SME
  wake_battery_via_canbus();

  transmit_can_frame(&BMWPHEV_6F1_REQUEST_ISOLATION_TEST,
                     can_config.battery);  // Run Internal Isolation Test at startup

  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
