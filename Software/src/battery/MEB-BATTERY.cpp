#include "../include.h"
#ifdef MEB_BATTERY
#include <algorithm>  // For std::min and std::max
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "MEB-BATTERY.h"

/*
TODO list
- Get contactors closing
- What CAN messages needs to be sent towards the battery to keep it alive
- Check value mappings on the PID polls
- Check value mappings on the constantly broadcasted messages
- Check all TODO:s in the code
*/

/* Do not change code below unless you are sure what you are doing */

static unsigned long previousMillis200 = 0;    // will store last time a 200ms CAN Message was send
static unsigned long previousMillis10 = 0;     // will store last time a 10ms CAN Message was send
static unsigned long previousMillis1s = 0;     // will store last time a 1s CAN Message was send
static unsigned long previousMillis100ms = 0;  // will store last time a 100ms CAN Message was send
static unsigned long previousMillis70ms = 0;   // will store last time a 70ms CAN Message was send
static unsigned long previousMillis40ms = 0;   // will store last time a 40ms CAN Message was send
static bool battery_awake = false;
static bool toggle = false;
static uint8_t counter_40ms = 0;
static uint8_t counter_10ms = 0;
static uint8_t counter_040 = 0;
static uint8_t counter_0F7 = 0;
static uint8_t counter_3b5 = 0;

static uint32_t poll_pid = 0;
static uint32_t pid_reply = 0;
static uint16_t battery_soc_polled = 0;
static uint16_t battery_voltage_polled = 0;
static int16_t battery_current_polled = 0;
static int16_t battery_max_temp = 600;
static int16_t battery_min_temp = 600;
static uint16_t battery_max_charge_voltage = 0;
static uint16_t battery_min_discharge_voltage = 0;
static uint16_t battery_allowed_charge_power = 0;
static uint16_t battery_allowed_discharge_power = 0;
static uint16_t cellvoltages[108];
static uint16_t tempval = 0;
static uint8_t BMS_5A2_CRC = 0;
static uint8_t BMS_5CA_CRC = 0;
static uint8_t BMS_0CF_CRC = 0;
static uint8_t BMS_578_CRC = 0;
static uint8_t BMS_5A2_counter = 0;
static uint8_t BMS_5CA_counter = 0;
static uint8_t BMS_0CF_counter = 0;
static uint8_t BMS_578_counter = 0;
static bool BMS_fault_status_contactor = false;
static bool BMS_exp_limits_active = 0;
static uint8_t BMS_mode = 0x07;
static bool BMS_HVIL_status = 0;
static bool BMS_fault_HVbatt_shutdown = 0;
static bool BMS_fault_HVbatt_shutdown_req = 0;
static bool BMS_fault_performance = 0;
static uint16_t BMS_current = 16300;
static bool BMS_fault_emergency_shutdown_crash = 0;
static uint32_t BMS_voltage_intermediate = 0;
static uint32_t BMS_voltage = 0;
static bool service_disconnect_switch_missing = false;
static bool pilotline_open = false;
static bool balancing_request = false;
static uint8_t battery_diagnostic = 0;
static uint16_t battery_Wh_left = 0;
static uint16_t battery_Wh_max = 0;
static uint8_t battery_potential_status = 0;
static uint8_t battery_temperature_warning = 0;
static uint16_t max_discharge_power_watt = 0;
static uint16_t max_discharge_current_amp = 0;
static uint16_t max_charge_power_watt = 0;
static uint16_t max_charge_current_amp = 0;
static uint16_t battery_SOC = 0;
static uint16_t usable_energy_amount_Wh = 0;
static uint8_t status_HV_line = 0;
static uint8_t warning_support = 0;
static bool battery_heating_active = false;
static uint16_t power_discharge_percentage = 0;
static uint16_t power_charge_percentage = 0;
static uint16_t actual_battery_voltage = 0;
static uint16_t regen_battery = 0;
static uint16_t energy_extracted_from_battery = 0;
static uint16_t max_fastcharging_current_amp = 0;
static uint16_t DC_voltage = 0;
static uint16_t DC_voltage_chargeport = 0;
static uint8_t BMS_welded_contactors_status = 0;
static uint8_t BMS_error_shutdown_request = 0;
static uint8_t BMS_error_shutdown = 0;
static uint16_t power_battery_heating_watt = 0;
static uint16_t power_battery_heating_req_watt = 0;
static uint8_t cooling_request =
    0;  //0 = No cooling, 1 = Light cooling, cabin prio, 2= higher cooling, 3 = immediate cooling, 4 = emergency cooling
static uint8_t heating_request = 0;       //0 = init, 1= maintain temp, 2=higher demand, 3 = immediate heat demand
static uint8_t balancing_active = false;  //0 = init, 1 active, 2 not active
static bool charging_active = false;
static uint16_t max_energy_Wh = 0;
static uint16_t max_charge_percent = 0;
static uint16_t min_charge_percent = 0;
static uint16_t isolation_resistance_kOhm = 0;
static bool battery_heating_installed = false;
static bool error_NT_circuit = false;
static uint8_t pump_1_control = 0;  //0x0D not installed, 0x0E init, 0x0F fault
static uint8_t pump_2_control = 0;  //0x0D not installed, 0x0E init, 0x0F fault
static uint8_t target_flow_temperature_C = 0;
static uint8_t return_temperature_C = 0;
static uint8_t status_valve_1 = 0;  //0 not active, 1 active, 5 not installed, 6 init, 7 fault
static uint8_t status_valve_2 = 0;  //0 not active, 1 active, 5 not installed, 6 init, 7 fault
static uint8_t battery_temperature = 0;
static uint8_t temperature_request =
    0;  //0 high cooling, 1 medium cooling, 2 low cooling, 3 no temp requirement init, 4 low heating , 5 medium heating, 6 high heating, 7 circulation

CAN_frame MEB_POLLING_FRAME = {.FD = true,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x1C40007B,  // SOC 02 8C
                               .data = {0x03, 0x22, 0x02, 0x8C, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_ACK_FRAME = {.FD = true,
                           .ext_ID = true,
                           .DLC = 8,
                           .ID = 0x1C40007B,  // Ack
                           .data = {0x30, 0x00, 0x00, 0x55, 0x55, 0x55, 0x55, 0x55}};
//Messages that might be needed for contactor closing
CAN_frame MEB_040 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x040,
                     .data = {0x7E, 0x83, 0x00, 0x01, 0x00, 0x00, 0x15, 0x00}};
CAN_frame MEB_0F7 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x0F7,
                     .data = {0x73, 0x00, 0x00, 0x00, 0x20, 0xF0, 0x1F, 0x64}};
CAN_frame MEB_3B5 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x3B5,
                     .data = {0x00, 0xFE, 0x00, 0x00, 0x0C, 0x00, 0x20, 0x00}};
CAN_frame MEB_3E9 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x3E9,
                     .data = {0x04, 0x3F, 0xE0, 0x03, 0x00, 0x00, 0xF0, 0xC7}};
CAN_frame MEB_530 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x530,
                     .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame MEB_5E7 = {.FD = true,
                     .ext_ID = true,
                     .DLC = 8,
                     .ID = 0x5E7,
                     .data = {0xFF, 0xFF, 0x0, 0x07, 0x03, 0x0, 0x0, 0x0}};
CAN_frame MEB_17FC007B_poll = {.FD = true,
                               .ext_ID = true,
                               .DLC = 8,
                               .ID = 0x17FC007B,
                               .data = {0x03, 0x22, 0x1E, 0x3D, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_17FC007B_reply = {.FD = true,
                                .ext_ID = true,
                                .DLC = 8,
                                .ID = 0x17FC007B,
                                .data = {0x30, 0x00, 0x01, 0x55, 0x55, 0x55, 0x55, 0x55}};
CAN_frame MEB_1A555564 = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x1A555564,
                          .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame MEB_12DD5513 = {.FD = true,
                          .ext_ID = true,
                          .DLC = 8,
                          .ID = 0x12DD5513,
                          .data = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
CAN_frame MEB_16A954FA = {
    .FD = true,
    .ext_ID = true,
    .DLC = 16,
    .ID = 0x16A954FA,
    .data = {0x00, 0x00, 0xD0, 0x01, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00}};

/** Calculate the CRC checksum for VAG CAN Messages
 *
 * The method used is described in Chapter "7.2.1.2 8-bit 0x2F polynomial CRC Calculation".
 * CRC Parameters:
 *     0x2F - Polynomial
 *     0xFF - Initial Value
 *     0xFF - XOR Output
 * 
 * @see https://github.com/crasbe/VW-OnBoard-Charger
 * @see https://github.com/colinoflynn/crcbeagle for CRC hacking :)
 * @see https://github.com/commaai/opendbc/blob/master/can/common.cc#L110
 * @see https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
 * @see https://web.archive.org/web/20221105210302/https://www.autosar.org/fileadmin/user_upload/standards/classic/4-3/AUTOSAR_SWS_CRCLibrary.pdf
 */
uint8_t vw_crc_calc(uint8_t* inputBytes, uint8_t length, uint16_t address) {

  const uint8_t poly = 0x2F;
  const uint8_t xor_output = 0xFF;
  // VAG Magic Bytes
  const uint8_t MB0040[16] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                              0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
  const uint8_t MB0097[16] = {0x3C, 0x54, 0xCF, 0xA3, 0x81, 0x93, 0x0B, 0xC7,
                              0x3E, 0xDF, 0x1C, 0xB0, 0xA7, 0x25, 0xD3, 0xD8};
  const uint8_t MB00F7[16] = {0x5F, 0xA0, 0x44, 0xD0, 0x63, 0x59, 0x5B, 0xA2,
                              0x68, 0x04, 0x90, 0x87, 0x52, 0x12, 0xB4, 0x9E};
  const uint8_t MB0124[16] = {0x12, 0x7E, 0x34, 0x16, 0x25, 0x8F, 0x8E, 0x35,
                              0xBA, 0x7F, 0xEA, 0x59, 0x4C, 0xF0, 0x88, 0x15};
  const uint8_t MB0187[16] = {0x7F, 0xED, 0x17, 0xC2, 0x7C, 0xEB, 0x44, 0x21,
                              0x01, 0xFA, 0xDB, 0x15, 0x4A, 0x6B, 0x23, 0x05};
  const uint8_t MB03A6[16] = {0xB6, 0x1C, 0xC1, 0x23, 0x6D, 0x8B, 0x0C, 0x51,
                              0x38, 0x32, 0x24, 0xA8, 0x3F, 0x3A, 0xA4, 0x02};
  const uint8_t MB03AF[16] = {0x94, 0x6A, 0xB5, 0x38, 0x8A, 0xB4, 0xAB, 0x27,
                              0xCB, 0x22, 0x88, 0xEF, 0xA3, 0xE1, 0xD0, 0xBB};
  const uint8_t MB0578[16] = {0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
                              0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48};
  const uint8_t MB05CA[16] = {0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
                              0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43};
  const uint8_t MB06A3[16] = {0xC1, 0x8B, 0x38, 0xA8, 0xA4, 0x27, 0xEB, 0xC8,
                              0xEF, 0x05, 0x9A, 0xBB, 0x39, 0xF7, 0x80, 0xA7};
  const uint8_t MB06A4[16] = {0xC7, 0xD8, 0xF1, 0xC4, 0xE3, 0x5E, 0x9A, 0xE2,
                              0xA1, 0xCB, 0x02, 0x4F, 0x57, 0x4E, 0x8E, 0xE4};

  uint8_t crc = 0xFF;
  uint8_t magicByte = 0x00;
  uint8_t counter = inputBytes[1] & 0x0F;  // only the low byte of the couner is relevant

  switch (address) {
    case 0x0040:  // Airbag
      magicByte = MB0040[counter];
      break;
    case 0x0097:  // ??
      magicByte = MB0097[counter];
      break;
    case 0x00F7:  // ??
      magicByte = MB00F7[counter];
      break;
    case 0x0124:  // ??
      magicByte = MB0124[counter];
      break;
    case 0x0187:  // EV_Gearshift "Gear" selection data for EVs with no gearbox
      magicByte = MB0187[counter];
      break;
    case 0x03A6:  // ??
      magicByte = MB03A6[counter];
      break;
    case 0x03AF:  // ??
      magicByte = MB03AF[counter];
      break;
    case 0x0578:  // BMS DC
      magicByte = MB0578[counter];
      break;
    case 0x05CA:  // BMS
      magicByte = MB05CA[counter];
      break;
    case 0x06A3:  // ??
      magicByte = MB06A3[counter];
      break;
    case 0x06A4:  // ??
      magicByte = MB06A4[counter];
      break;
    default:  // this won't lead to correct CRC checksums
      magicByte = 0x00;
      break;
  }

  for (uint8_t i = 1; i < length + 1; i++) {
    // We skip the empty CRC position and start at the timer
    // The last element is the VAG magic byte for address 0x187 depending on the counter value.
    if (i < length)
      crc ^= inputBytes[i];
    else
      crc ^= magicByte;

    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x80)
        crc = (crc << 1) ^ poly;
      else
        crc = (crc << 1);
    }
  }

  crc ^= xor_output;

  return crc;
}

void update_values_battery() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer.battery.status.real_soc = battery_soc_polled * 10;
  //Alternatively use battery_SOC for more precision

  datalayer.battery.status.soh_pptt;

  datalayer.battery.status.voltage_dV = battery_voltage_polled * 2.5;

  datalayer.battery.status.current_dA = (BMS_current / 10) - 1630;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);
  //Alternatively use battery_Wh_left

  datalayer.battery.status.max_charge_power_W = (max_charge_power_watt * 100);

  datalayer.battery.status.max_discharge_power_W = (max_discharge_power_watt * 100);

  //Power in watts, Negative = charging batt
  datalayer.battery.status.active_power_W =
      ((datalayer.battery.status.voltage_dV * datalayer.battery.status.current_dA) / 100);

  datalayer.battery.status.temperature_min_dC = ((battery_min_temp / 2) - 350);

  datalayer.battery.status.temperature_max_dC = ((battery_max_temp / 2) - 350);

  //Map all cell voltages to the global array
  memcpy(datalayer.battery.status.cell_voltages_mV, cellvoltages, 108 * sizeof(uint16_t));

  // Initialize min and max, lets find which cells are min and max!
  uint16_t min_cell_mv_value = std::numeric_limits<uint16_t>::max();
  uint16_t max_cell_mv_value = 0;
  // Loop to find the min and max while ignoring zero values
  for (uint8_t i = 0; i < 108; ++i) {
    uint16_t voltage_mV = datalayer.battery.status.cell_voltages_mV[i];
    if (voltage_mV != 0) {  // Skip unread values (0)
      min_cell_mv_value = std::min(min_cell_mv_value, voltage_mV);
      max_cell_mv_value = std::max(max_cell_mv_value, voltage_mV);
    }
  }
  // If all array values are 0, reset min/max to 3700
  if (min_cell_mv_value == std::numeric_limits<uint16_t>::max()) {
    min_cell_mv_value = 3700;
    max_cell_mv_value = 3700;
  }

  datalayer.battery.status.cell_min_voltage_mV = min_cell_mv_value;
  datalayer.battery.status.cell_max_voltage_mV = max_cell_mv_value;

  if (service_disconnect_switch_missing) {
    set_event(EVENT_HVIL_FAILURE, 1);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (pilotline_open) {
    set_event(EVENT_HVIL_FAILURE, 2);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  // Update webserver datalayer for "More battery info" page
  datalayer_extended.meb.SDSW = service_disconnect_switch_missing;
  datalayer_extended.meb.pilotline = pilotline_open;
  datalayer_extended.meb.HVIL = BMS_HVIL_status;
  datalayer_extended.meb.BMS_mode = BMS_mode;
  datalayer_extended.meb.battery_diagnostic = battery_diagnostic;
  datalayer_extended.meb.status_HV_line = status_HV_line;
  datalayer_extended.meb.warning_support = warning_support;
}

void receive_can_battery(CAN_frame rx_frame) {
  battery_awake = true;
  switch (rx_frame.ID) {
    case 0x17F0007B:  // Suspected to be from BMS
      break;
    case 0x17FE007B:  // BMS - Handshake?
      //If the message is (10 08 62 1E 3D 00 02 49)
      //We reply with
      //17FC007B  [08]  30 00 01 55 55 55 55 55
      //If the message is (21 F0 34 AA AA AA AA AA)
      //We do not need to send anything
      if (rx_frame.data.u8[7] == 0xAA) {
        // Do nothing
      } else {
        transmit_can(&MEB_17FC007B_reply, can_config.battery);
      }
      break;
    case 0x1B00007B:  // Suspected to be from BMS
      break;
    case 0x12DD54D0:  // BMS Limits 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      max_discharge_power_watt =
          ((rx_frame.data.u8[6] & 0x07) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[4] & 0xC0) >> 6;  //*100
      max_discharge_current_amp =
          ((rx_frame.data.u8[3] & 0x01) << 12) | (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*0.2
      max_charge_power_watt = (rx_frame.data.u8[7] << 5) | (rx_frame.data.u8[6] >> 3);                     //*100
      max_charge_current_amp = ((rx_frame.data.u8[4] & 0x3F) << 7) | (rx_frame.data.u8[3] >> 1);           //*0.2
      break;
    case 0x12DD54D1:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_SOC = ((rx_frame.data.u8[3] & 0x0F) << 7) | (rx_frame.data.u8[2] >> 1);               //*0.05
      usable_energy_amount_Wh = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];                   //*5
      power_discharge_percentage = ((rx_frame.data.u8[4] & 0x3F) << 4) | rx_frame.data.u8[3] >> 4;  //*0.2
      power_charge_percentage = (rx_frame.data.u8[5] << 2) | rx_frame.data.u8[4] >> 6;              //*0.2
      status_HV_line = ((rx_frame.data.u8[2] & 0x01) << 2) | rx_frame.data.u8[1] >> 7;
      warning_support = (rx_frame.data.u8[1] & 0x70) >> 4;
      break;
    case 0x12DD54D2:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      battery_heating_active = (rx_frame.data.u8[4] & 0x40) >> 6;
      heating_request = (rx_frame.data.u8[5] & 0xE0) >> 5;
      cooling_request = (rx_frame.data.u8[5] & 0x1C) >> 2;
      power_battery_heating_watt = rx_frame.data.u8[6];
      power_battery_heating_req_watt = rx_frame.data.u8[7];
      break;
    case 0x1A555550:  // BMS 500ms
      balancing_active = (rx_frame.data.u8[1] & 0xC0) >> 6;
      charging_active = (rx_frame.data.u8[2] & 0x01);
      max_energy_Wh = ((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[5];                     //*40
      max_charge_percent = ((rx_frame.data.u8[7] << 3) | rx_frame.data.u8[6] >> 5);                  //*0.05
      min_charge_percent = ((rx_frame.data.u8[4] << 3) | rx_frame.data.u8[3] >> 5);                  //*0.05
      isolation_resistance_kOhm = (((rx_frame.data.u8[3] & 0x1F) << 7) | rx_frame.data.u8[2] >> 1);  //*5
      break;
    case 0x1A555551:  // BMS 500ms
      battery_heating_installed = (rx_frame.data.u8[1] & 0x20) >> 5;
      error_NT_circuit = (rx_frame.data.u8[1] & 0x40) >> 6;
      pump_1_control = rx_frame.data.u8[2] & 0x0F;         //*10, percent
      pump_2_control = (rx_frame.data.u8[2] & 0xF0) >> 4;  //*10, percent
      status_valve_1 = (rx_frame.data.u8[3] & 0x1C) >> 2;
      status_valve_2 = (rx_frame.data.u8[3] & 0xE0) >> 5;
      temperature_request = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      battery_temperature = rx_frame.data.u8[5];        //*0,5 -40
      target_flow_temperature_C = rx_frame.data.u8[6];  //*0,5 -40
      return_temperature_C = rx_frame.data.u8[7];       //*0,5 -40
      break;
    case 0x1A5555B2:  // BMS
      break;
    case 0x16A954A6:  // BMS
      break;
    case 0x16A954F8:  // BMS
      break;
    case 0x16A954E8:  // BMS
      break;
    case 0x1C42017B:  // BMS
      break;
    case 0x1A5555B0:  // BMS
      break;
    case 0x1A5555B1:  // BMS 1000ms
      break;
    case 0x2AF:                                                                            // BMS 50ms
      actual_battery_voltage = ((rx_frame.data.u8[1] & 0x3F) << 8) | rx_frame.data.u8[0];  //*0.0625
      regen_battery = ((rx_frame.data.u8[5] & 0x7F) << 8) | rx_frame.data.u8[4];
      energy_extracted_from_battery = ((rx_frame.data.u8[7] & 0x7F) << 8) | rx_frame.data.u8[6];
      break;
    case 0x578:                                        // BMS 100ms
      BMS_578_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_578_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      max_fastcharging_current_amp = ((rx_frame.data.u8[4] & 0x01) << 8) | rx_frame.data.u8[3];
      DC_voltage = (rx_frame.data.u8[4] << 6) | (rx_frame.data.u8[1] >> 6);
      DC_voltage_chargeport = (rx_frame.data.u8[7] << 4) | (rx_frame.data.u8[6] >> 4);
      break;
    case 0x5A2:                                        // BMS 500ms normal, 100ms fast
      BMS_5A2_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_5A2_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      service_disconnect_switch_missing = (rx_frame.data.u8[1] & 0x20) >> 5;
      pilotline_open = (rx_frame.data.u8[1] & 0x10) >> 4;
      break;
    case 0x5CA:                                               // BMS 500ms
      BMS_5CA_CRC = rx_frame.data.u8[0];                      // Can be used to check CAN signal integrity later on
      BMS_5CA_counter = (rx_frame.data.u8[1] & 0x0F);         // Can be used to check CAN signal integrity later on
      balancing_request = (rx_frame.data.u8[5] & 0x08) >> 3;  //True/False
      battery_diagnostic = (rx_frame.data.u8[3] & 0x07);
      battery_Wh_left = (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*50
      battery_potential_status =
          (rx_frame.data.u8[5] & 0x30) >> 4;  //0 = function not enabled, 1= no potential, 2 = potential on, 3 = fault
      battery_temperature_warning =
          (rx_frame.data.u8[7] & 0x0C) >> 2;  // 0 = no warning, 1 = temp level 1, 2=temp level 2
      battery_Wh_max = ((rx_frame.data.u8[5] & 0x07) << 8) | rx_frame.data.u8[4];  //*50
      break;
    case 0x0CF:                                        //BMS 10ms
      BMS_0CF_CRC = rx_frame.data.u8[0];               // Can be used to check CAN signal integrity later on
      BMS_0CF_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      BMS_fault_status_contactor = (rx_frame.data.u8[1] & 0x20) >> 5;
      BMS_welded_contactors_status = (rx_frame.data.u8[1] & 0x60) >> 5;
      BMS_error_shutdown_request = (rx_frame.data.u8[2] & 0x40) >> 6;
      BMS_error_shutdown = (rx_frame.data.u8[2] & 0x20) >> 5;
      BMS_mode = (rx_frame.data.u8[2] & 0x07);
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x08) >> 3;
      //BMS_exp_limits_active
      //BMS_fault_performance
      //BMS_fault_emergency_shutdown_crash
      BMS_current = ((rx_frame.data.u8[4] & 0x7F) << 8) | rx_frame.data.u8[3];
      BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5]));
      BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4));
      break;
    case 0x1C42007B:                      // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can(&MEB_ACK_FRAME, can_config.battery);
      }
      if (rx_frame.DLC == 8) {
        pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      } else {  //12 or 24bit message has reply in other location
        pid_reply = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
      }

      switch (pid_reply) {
        case PID_SOC:
          battery_soc_polled = rx_frame.data.u8[4] * 4;  // 135*4 = 54.0%
        case PID_VOLTAGE:
          battery_voltage_polled = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CURRENT:  // IDLE 0A: 00 08 62 1E 3D (00 02) 49 F0 39 AA AA
          battery_current_polled = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);  //TODO: right bits?
          break;
        case PID_MAX_TEMP:
          battery_max_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_TEMP:
          battery_min_temp = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MAX_CHARGE_VOLTAGE:
          battery_max_charge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          battery_min_discharge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_CHARGE_POWER:
          battery_allowed_charge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          battery_allowed_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_CELLVOLTAGE_CELL_1:
          cellvoltages[0] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_2:
          cellvoltages[1] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_3:
          cellvoltages[2] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_4:
          cellvoltages[3] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_5:
          cellvoltages[4] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_6:
          cellvoltages[5] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_7:
          cellvoltages[6] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_8:
          cellvoltages[7] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_9:
          cellvoltages[8] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_10:
          cellvoltages[9] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_11:
          cellvoltages[10] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_12:
          cellvoltages[11] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_13:
          cellvoltages[12] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_14:
          cellvoltages[13] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_15:
          cellvoltages[14] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_16:
          cellvoltages[15] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_17:
          cellvoltages[16] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_18:
          cellvoltages[17] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_19:
          cellvoltages[18] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_20:
          cellvoltages[19] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_21:
          cellvoltages[20] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_22:
          cellvoltages[21] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_23:
          cellvoltages[22] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_24:
          cellvoltages[23] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_25:
          cellvoltages[24] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_26:
          cellvoltages[25] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_27:
          cellvoltages[26] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_28:
          cellvoltages[27] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_29:
          cellvoltages[28] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_30:
          cellvoltages[29] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_31:
          cellvoltages[30] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_32:
          cellvoltages[31] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_33:
          cellvoltages[32] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_34:
          cellvoltages[33] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_35:
          cellvoltages[34] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_36:
          cellvoltages[35] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_37:
          cellvoltages[36] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_38:
          cellvoltages[37] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_39:
          cellvoltages[38] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_40:
          cellvoltages[39] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_41:
          cellvoltages[40] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_42:
          cellvoltages[41] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_43:
          cellvoltages[42] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_44:
          cellvoltages[43] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_45:
          cellvoltages[44] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_46:
          cellvoltages[45] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_47:
          cellvoltages[46] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_48:
          cellvoltages[47] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_49:
          cellvoltages[48] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_50:
          cellvoltages[49] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_51:
          cellvoltages[50] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_52:
          cellvoltages[51] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_53:
          cellvoltages[52] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_54:
          cellvoltages[53] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_55:
          cellvoltages[54] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_56:
          cellvoltages[55] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_57:
          cellvoltages[56] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_58:
          cellvoltages[57] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_59:
          cellvoltages[58] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_60:
          cellvoltages[59] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_61:
          cellvoltages[60] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_62:
          cellvoltages[61] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_63:
          cellvoltages[62] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_64:
          cellvoltages[63] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_65:
          cellvoltages[64] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_66:
          cellvoltages[65] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_67:
          cellvoltages[66] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_68:
          cellvoltages[67] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_69:
          cellvoltages[68] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_70:
          cellvoltages[69] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_71:
          cellvoltages[70] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_72:
          cellvoltages[71] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_73:
          cellvoltages[72] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_74:
          cellvoltages[73] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_75:
          cellvoltages[74] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_76:
          cellvoltages[75] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_77:
          cellvoltages[76] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_78:
          cellvoltages[77] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_79:
          cellvoltages[78] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_80:
          cellvoltages[79] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_81:
          cellvoltages[80] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_82:
          cellvoltages[81] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_83:
          cellvoltages[82] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_84:
          cellvoltages[83] = (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) + 1000);
          break;
        case PID_CELLVOLTAGE_CELL_85:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[84] = (tempval + 1000);
          } else {  // Cell 85 unavailable. We have a 84S battery (48kWh)
            datalayer.battery.info.number_of_cells = 84;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_84S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;
          }
          break;
        case PID_CELLVOLTAGE_CELL_86:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[85] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_87:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[86] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_88:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[87] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_89:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[88] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_90:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[89] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_91:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[90] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_92:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[91] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_93:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[92] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_94:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[93] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_95:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[94] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_96:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[95] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_97:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[96] = (tempval + 1000);
          } else {  // Cell 97 unavailable. We have a 96S battery (55kWh) (Unless already specified as 84S)
            if (datalayer.battery.info.number_of_cells == 84) {
              // Do nothing, we already identified it as 84S
            } else {
              datalayer.battery.info.number_of_cells = 96;
              datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
              datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
            }
          }
          break;
        case PID_CELLVOLTAGE_CELL_98:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[97] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_99:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[98] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_100:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[99] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_101:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[100] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_102:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[101] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_103:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[102] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_104:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[103] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_105:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[104] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_106:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[105] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_107:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[106] = (tempval + 1000);
          }
          break;
        case PID_CELLVOLTAGE_CELL_108:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          if (tempval != 0xFFE) {
            cellvoltages[107] = (tempval + 1000);
            datalayer.battery.info.number_of_cells = 108;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
          }
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
}

void send_can_battery() {
  if (!battery_awake) {
    return;
  }
  unsigned long currentMillis = millis();
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10 >= INTERVAL_10_MS) {
    // Check if sending of CAN messages has been delayed too much.
    if ((currentMillis - previousMillis10 >= INTERVAL_10_MS_DELAYED) && (currentMillis > BOOTUP_TIME)) {
      set_event(EVENT_CAN_OVERRUN, (currentMillis - previousMillis10));
    } else {
      clear_event(EVENT_CAN_OVERRUN);
    }
    previousMillis10 = currentMillis;

    /* Handle content for 0x0F7 message */
    if (counter_0F7 < 250) {
      counter_0F7++;
    }
    if (counter_0F7 > 40) {
      MEB_0F7.data.u8[4] = 0x40;
      MEB_0F7.data.u8[5] = 0xD2;
    }
    if (counter_0F7 > 50) {
      MEB_0F7.data.u8[4] = 0x44;
      MEB_0F7.data.u8[5] = 0xF2;
    }
    if (counter_0F7 > 70) {
      MEB_0F7.data.u8[4] = 0xC0;
      MEB_0F7.data.u8[5] = 0xF4;
    }
    MEB_0F7.data.u8[1] = ((MEB_0F7.data.u8[1] & 0xF0) | counter_10ms);
    MEB_0F7.data.u8[0] = vw_crc_calc(MEB_0F7.data.u8, MEB_0F7.DLC, MEB_0F7.ID);
    counter_10ms = (counter_10ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can(&MEB_0F7, can_config.battery);
  }
  // Send 40ms CAN Message
  if (currentMillis - previousMillis40ms >= INTERVAL_40_MS) {
    previousMillis40ms = currentMillis;

    /* Handle content for 0x040 message*/
    MEB_040.data.u8[7] = counter_040;
    MEB_040.data.u8[1] = ((MEB_040.data.u8[1] & 0xF0) | counter_40ms);
    MEB_040.data.u8[0] = vw_crc_calc(MEB_040.data.u8, MEB_040.DLC, MEB_040.ID);
    counter_40ms = (counter_40ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    if (toggle) {
      counter_040 = (counter_040 + 1) % 256;  // Increment only on every other pass
    }
    toggle = !toggle;  // Flip the toggle each time the code block is executed

    transmit_can(&MEB_040, can_config.battery);  // Airbag message
  }
  // Send 70ms CAN Message
  if (currentMillis - previousMillis70ms >= INTERVAL_70_MS) {
    previousMillis70ms = currentMillis;

    transmit_can(&MEB_17FC007B_poll, can_config.battery);
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    if (counter_3b5 < 250) {
      counter_3b5++;
    }
    if (counter_3b5 > 4) {
      MEB_3B5.data.u8[2] = 0x12;

      MEB_3E9.data.u8[0] = 0x00;
      MEB_3E9.data.u8[1] = 0x01;
      MEB_3E9.data.u8[2] = 0xC0;
      MEB_3E9.data.u8[3] = 0x03;
    }

    transmit_can(&MEB_3B5, can_config.battery);
    transmit_can(&MEB_3E9, can_config.battery);
    transmit_can(&MEB_12DD5513, can_config.battery);
  }
  //Send 200ms message
  if (currentMillis - previousMillis200 >= INTERVAL_200_MS) {
    previousMillis200 = currentMillis;
    transmit_can(&MEB_530, can_config.battery);
    transmit_can(&MEB_16A954FA, can_config.battery);
    transmit_can(&MEB_5E7, can_config.battery);

    switch (poll_pid) {
      case PID_SOC:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_SOC >> 8);  // High byte
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_SOC;         // Low byte
        poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_VOLTAGE;
        poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CURRENT >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CURRENT;
        poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_TEMP;
        poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_TEMP >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_TEMP;
        poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MAX_CHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MAX_CHARGE_VOLTAGE;
        poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_MIN_DISCHARGE_VOLTAGE >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_MIN_DISCHARGE_VOLTAGE;
        poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_CHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_CHARGE_POWER;
        poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ALLOWED_DISCHARGE_POWER >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ALLOWED_DISCHARGE_POWER;
        poll_pid = PID_CELLVOLTAGE_CELL_1;  // Start polling cell voltages
        break;
      // Cell Voltage Cases
      case PID_CELLVOLTAGE_CELL_1:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CELLVOLTAGE_CELL_1 >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_1;
        poll_pid = PID_CELLVOLTAGE_CELL_2;
        break;
      case PID_CELLVOLTAGE_CELL_2:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_2;
        poll_pid = PID_CELLVOLTAGE_CELL_3;
        break;
      case PID_CELLVOLTAGE_CELL_3:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_3;
        poll_pid = PID_CELLVOLTAGE_CELL_4;
        break;
      case PID_CELLVOLTAGE_CELL_4:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_4;
        poll_pid = PID_CELLVOLTAGE_CELL_5;
        break;
      case PID_CELLVOLTAGE_CELL_5:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_5;
        poll_pid = PID_CELLVOLTAGE_CELL_6;
        break;
      case PID_CELLVOLTAGE_CELL_6:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_6;
        poll_pid = PID_CELLVOLTAGE_CELL_7;
        break;
      case PID_CELLVOLTAGE_CELL_7:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_7;
        poll_pid = PID_CELLVOLTAGE_CELL_8;
        break;
      case PID_CELLVOLTAGE_CELL_8:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_8;
        poll_pid = PID_CELLVOLTAGE_CELL_9;
        break;
      case PID_CELLVOLTAGE_CELL_9:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_9;
        poll_pid = PID_CELLVOLTAGE_CELL_10;
        break;
      case PID_CELLVOLTAGE_CELL_10:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_10;
        poll_pid = PID_CELLVOLTAGE_CELL_11;
        break;
      case PID_CELLVOLTAGE_CELL_11:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_11;
        poll_pid = PID_CELLVOLTAGE_CELL_12;
        break;
      case PID_CELLVOLTAGE_CELL_12:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_12;
        poll_pid = PID_CELLVOLTAGE_CELL_13;
        break;
      case PID_CELLVOLTAGE_CELL_13:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_13;
        poll_pid = PID_CELLVOLTAGE_CELL_14;
        break;
      case PID_CELLVOLTAGE_CELL_14:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_14;
        poll_pid = PID_CELLVOLTAGE_CELL_15;
        break;
      case PID_CELLVOLTAGE_CELL_15:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_15;
        poll_pid = PID_CELLVOLTAGE_CELL_16;
        break;
      case PID_CELLVOLTAGE_CELL_16:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_16;
        poll_pid = PID_CELLVOLTAGE_CELL_17;
        break;
      case PID_CELLVOLTAGE_CELL_17:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_17;
        poll_pid = PID_CELLVOLTAGE_CELL_18;
        break;
      case PID_CELLVOLTAGE_CELL_18:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_18;
        poll_pid = PID_CELLVOLTAGE_CELL_19;
        break;
      case PID_CELLVOLTAGE_CELL_19:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_19;
        poll_pid = PID_CELLVOLTAGE_CELL_20;
        break;
      case PID_CELLVOLTAGE_CELL_20:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_20;
        poll_pid = PID_CELLVOLTAGE_CELL_21;
        break;
      case PID_CELLVOLTAGE_CELL_21:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_21;
        poll_pid = PID_CELLVOLTAGE_CELL_22;
        break;
      case PID_CELLVOLTAGE_CELL_22:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_22;
        poll_pid = PID_CELLVOLTAGE_CELL_23;
        break;
      case PID_CELLVOLTAGE_CELL_23:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_23;
        poll_pid = PID_CELLVOLTAGE_CELL_24;
        break;
      case PID_CELLVOLTAGE_CELL_24:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_24;
        poll_pid = PID_CELLVOLTAGE_CELL_25;
        break;
      case PID_CELLVOLTAGE_CELL_25:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_25;
        poll_pid = PID_CELLVOLTAGE_CELL_26;
        break;
      case PID_CELLVOLTAGE_CELL_26:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_26;
        poll_pid = PID_CELLVOLTAGE_CELL_27;
        break;
      case PID_CELLVOLTAGE_CELL_27:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_27;
        poll_pid = PID_CELLVOLTAGE_CELL_28;
        break;
      case PID_CELLVOLTAGE_CELL_28:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_28;
        poll_pid = PID_CELLVOLTAGE_CELL_29;
        break;
      case PID_CELLVOLTAGE_CELL_29:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_29;
        poll_pid = PID_CELLVOLTAGE_CELL_30;
        break;
      case PID_CELLVOLTAGE_CELL_30:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_30;
        poll_pid = PID_CELLVOLTAGE_CELL_31;
        break;
      case PID_CELLVOLTAGE_CELL_31:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_31;
        poll_pid = PID_CELLVOLTAGE_CELL_32;
        break;
      case PID_CELLVOLTAGE_CELL_32:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_32;
        poll_pid = PID_CELLVOLTAGE_CELL_33;
        break;
      case PID_CELLVOLTAGE_CELL_33:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_33;
        poll_pid = PID_CELLVOLTAGE_CELL_34;
        break;
      case PID_CELLVOLTAGE_CELL_34:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_34;
        poll_pid = PID_CELLVOLTAGE_CELL_35;
        break;
      case PID_CELLVOLTAGE_CELL_35:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_35;
        poll_pid = PID_CELLVOLTAGE_CELL_36;
        break;
      case PID_CELLVOLTAGE_CELL_36:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_36;
        poll_pid = PID_CELLVOLTAGE_CELL_37;
        break;
      case PID_CELLVOLTAGE_CELL_37:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_37;
        poll_pid = PID_CELLVOLTAGE_CELL_38;
        break;
      case PID_CELLVOLTAGE_CELL_38:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_38;
        poll_pid = PID_CELLVOLTAGE_CELL_39;
        break;
      case PID_CELLVOLTAGE_CELL_39:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_39;
        poll_pid = PID_CELLVOLTAGE_CELL_40;
        break;
      case PID_CELLVOLTAGE_CELL_40:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_40;
        poll_pid = PID_CELLVOLTAGE_CELL_41;
        break;
      case PID_CELLVOLTAGE_CELL_41:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_41;
        poll_pid = PID_CELLVOLTAGE_CELL_42;
        break;
      case PID_CELLVOLTAGE_CELL_42:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_42;
        poll_pid = PID_CELLVOLTAGE_CELL_43;
        break;
      case PID_CELLVOLTAGE_CELL_43:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_43;
        poll_pid = PID_CELLVOLTAGE_CELL_44;
        break;
      case PID_CELLVOLTAGE_CELL_44:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_44;
        poll_pid = PID_CELLVOLTAGE_CELL_45;
        break;
      case PID_CELLVOLTAGE_CELL_45:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_45;
        poll_pid = PID_CELLVOLTAGE_CELL_46;
        break;
      case PID_CELLVOLTAGE_CELL_46:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_46;
        poll_pid = PID_CELLVOLTAGE_CELL_47;
        break;
      case PID_CELLVOLTAGE_CELL_47:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_47;
        poll_pid = PID_CELLVOLTAGE_CELL_48;
        break;
      case PID_CELLVOLTAGE_CELL_48:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_48;
        poll_pid = PID_CELLVOLTAGE_CELL_49;
        break;
      case PID_CELLVOLTAGE_CELL_49:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_49;
        poll_pid = PID_CELLVOLTAGE_CELL_50;
        break;
      case PID_CELLVOLTAGE_CELL_50:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_50;
        poll_pid = PID_CELLVOLTAGE_CELL_51;
        break;
      case PID_CELLVOLTAGE_CELL_51:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_51;
        poll_pid = PID_CELLVOLTAGE_CELL_52;
        break;
      case PID_CELLVOLTAGE_CELL_52:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_52;
        poll_pid = PID_CELLVOLTAGE_CELL_53;
        break;
      case PID_CELLVOLTAGE_CELL_53:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_53;
        poll_pid = PID_CELLVOLTAGE_CELL_54;
        break;
      case PID_CELLVOLTAGE_CELL_54:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_54;
        poll_pid = PID_CELLVOLTAGE_CELL_55;
        break;
      case PID_CELLVOLTAGE_CELL_55:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_55;
        poll_pid = PID_CELLVOLTAGE_CELL_56;
        break;
      case PID_CELLVOLTAGE_CELL_56:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_56;
        poll_pid = PID_CELLVOLTAGE_CELL_57;
        break;
      case PID_CELLVOLTAGE_CELL_57:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_57;
        poll_pid = PID_CELLVOLTAGE_CELL_58;
        break;
      case PID_CELLVOLTAGE_CELL_58:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_58;
        poll_pid = PID_CELLVOLTAGE_CELL_59;
        break;
      case PID_CELLVOLTAGE_CELL_59:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_59;
        poll_pid = PID_CELLVOLTAGE_CELL_60;
        break;
      case PID_CELLVOLTAGE_CELL_60:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_60;
        poll_pid = PID_CELLVOLTAGE_CELL_61;
        break;
      case PID_CELLVOLTAGE_CELL_61:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_61;
        poll_pid = PID_CELLVOLTAGE_CELL_62;
        break;
      case PID_CELLVOLTAGE_CELL_62:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_62;
        poll_pid = PID_CELLVOLTAGE_CELL_63;
        break;
      case PID_CELLVOLTAGE_CELL_63:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_63;
        poll_pid = PID_CELLVOLTAGE_CELL_64;
        break;
      case PID_CELLVOLTAGE_CELL_64:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_64;
        poll_pid = PID_CELLVOLTAGE_CELL_65;
        break;
      case PID_CELLVOLTAGE_CELL_65:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_65;
        poll_pid = PID_CELLVOLTAGE_CELL_66;
        break;
      case PID_CELLVOLTAGE_CELL_66:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_66;
        poll_pid = PID_CELLVOLTAGE_CELL_67;
        break;
      case PID_CELLVOLTAGE_CELL_67:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_67;
        poll_pid = PID_CELLVOLTAGE_CELL_68;
        break;
      case PID_CELLVOLTAGE_CELL_68:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_68;
        poll_pid = PID_CELLVOLTAGE_CELL_69;
        break;
      case PID_CELLVOLTAGE_CELL_69:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_69;
        poll_pid = PID_CELLVOLTAGE_CELL_70;
        break;
      case PID_CELLVOLTAGE_CELL_70:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_70;
        poll_pid = PID_CELLVOLTAGE_CELL_71;
        break;
      case PID_CELLVOLTAGE_CELL_71:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_71;
        poll_pid = PID_CELLVOLTAGE_CELL_72;
        break;
      case PID_CELLVOLTAGE_CELL_72:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_72;
        poll_pid = PID_CELLVOLTAGE_CELL_73;
        break;
      case PID_CELLVOLTAGE_CELL_73:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_73;
        poll_pid = PID_CELLVOLTAGE_CELL_74;
        break;
      case PID_CELLVOLTAGE_CELL_74:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_74;
        poll_pid = PID_CELLVOLTAGE_CELL_75;
        break;
      case PID_CELLVOLTAGE_CELL_75:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_75;
        poll_pid = PID_CELLVOLTAGE_CELL_76;
        break;
      case PID_CELLVOLTAGE_CELL_76:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_76;
        poll_pid = PID_CELLVOLTAGE_CELL_77;
        break;
      case PID_CELLVOLTAGE_CELL_77:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_77;
        poll_pid = PID_CELLVOLTAGE_CELL_78;
        break;
      case PID_CELLVOLTAGE_CELL_78:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_78;
        poll_pid = PID_CELLVOLTAGE_CELL_79;
        break;
      case PID_CELLVOLTAGE_CELL_79:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_79;
        poll_pid = PID_CELLVOLTAGE_CELL_80;
        break;
      case PID_CELLVOLTAGE_CELL_80:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_80;
        poll_pid = PID_CELLVOLTAGE_CELL_81;
        break;
      case PID_CELLVOLTAGE_CELL_81:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_81;
        poll_pid = PID_CELLVOLTAGE_CELL_82;
        break;
      case PID_CELLVOLTAGE_CELL_82:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_82;
        poll_pid = PID_CELLVOLTAGE_CELL_83;
        break;
      case PID_CELLVOLTAGE_CELL_83:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_83;
        poll_pid = PID_CELLVOLTAGE_CELL_84;
        break;
      case PID_CELLVOLTAGE_CELL_84:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_84;
        poll_pid = PID_CELLVOLTAGE_CELL_85;
        break;
      case PID_CELLVOLTAGE_CELL_85:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_85;
        poll_pid = PID_CELLVOLTAGE_CELL_86;
        break;
      case PID_CELLVOLTAGE_CELL_86:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_86;
        poll_pid = PID_CELLVOLTAGE_CELL_87;
        break;
      case PID_CELLVOLTAGE_CELL_87:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_87;
        poll_pid = PID_CELLVOLTAGE_CELL_88;
        break;
      case PID_CELLVOLTAGE_CELL_88:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_88;
        poll_pid = PID_CELLVOLTAGE_CELL_89;
        break;
      case PID_CELLVOLTAGE_CELL_89:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_89;
        poll_pid = PID_CELLVOLTAGE_CELL_90;
        break;
      case PID_CELLVOLTAGE_CELL_90:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_90;
        poll_pid = PID_CELLVOLTAGE_CELL_91;
        break;
      case PID_CELLVOLTAGE_CELL_91:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_91;
        poll_pid = PID_CELLVOLTAGE_CELL_92;
        break;
      case PID_CELLVOLTAGE_CELL_92:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_92;
        poll_pid = PID_CELLVOLTAGE_CELL_93;
        break;
      case PID_CELLVOLTAGE_CELL_93:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_93;
        poll_pid = PID_CELLVOLTAGE_CELL_94;
        break;
      case PID_CELLVOLTAGE_CELL_94:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_94;
        poll_pid = PID_CELLVOLTAGE_CELL_95;
        break;
      case PID_CELLVOLTAGE_CELL_95:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_95;
        poll_pid = PID_CELLVOLTAGE_CELL_96;
        break;
      case PID_CELLVOLTAGE_CELL_96:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_96;
        poll_pid = PID_CELLVOLTAGE_CELL_97;
        break;
      case PID_CELLVOLTAGE_CELL_97:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_97;
        poll_pid = PID_CELLVOLTAGE_CELL_98;
        break;
      case PID_CELLVOLTAGE_CELL_98:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_98;
        poll_pid = PID_CELLVOLTAGE_CELL_99;
        break;
      case PID_CELLVOLTAGE_CELL_99:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_99;
        poll_pid = PID_CELLVOLTAGE_CELL_100;
        break;
      case PID_CELLVOLTAGE_CELL_100:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_100;
        poll_pid = PID_CELLVOLTAGE_CELL_101;
        break;
      case PID_CELLVOLTAGE_CELL_101:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_101;
        poll_pid = PID_CELLVOLTAGE_CELL_102;
        break;
      case PID_CELLVOLTAGE_CELL_102:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_102;
        poll_pid = PID_CELLVOLTAGE_CELL_103;
        break;
      case PID_CELLVOLTAGE_CELL_103:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_103;
        poll_pid = PID_CELLVOLTAGE_CELL_104;
        break;
      case PID_CELLVOLTAGE_CELL_104:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_104;
        poll_pid = PID_CELLVOLTAGE_CELL_105;
        break;
      case PID_CELLVOLTAGE_CELL_105:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_105;
        poll_pid = PID_CELLVOLTAGE_CELL_106;
        break;
      case PID_CELLVOLTAGE_CELL_106:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_106;
        poll_pid = PID_CELLVOLTAGE_CELL_107;
        break;
      case PID_CELLVOLTAGE_CELL_107:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_107;
        poll_pid = PID_CELLVOLTAGE_CELL_108;
        break;
      case PID_CELLVOLTAGE_CELL_108:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_108;
        poll_pid = PID_SOC;
        break;
      default:
        poll_pid = PID_SOC;
        break;
    }

    transmit_can(&MEB_POLLING_FRAME, can_config.battery);
  }

  //Send 1s CANFD message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;
    transmit_can(&MEB_1A555564, can_config.battery);
  }
}

void setup_battery(void) {  // Performs one time setup at startup
#ifdef DEBUG_VIA_USB
  Serial.println("Volkswagen Group MEB platform battery selected");
#endif
  datalayer.battery.info.number_of_cells = 108;  //Startup in 108S mode. We figure out the actual count later.
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Defined later to correct pack size
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;   //Defined later to correct pack size
}

#endif
