#include "MEB-BATTERY.h"
#include <Arduino.h>
#include <algorithm>  // For std::min and std::max
#include <cstring>    //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/safety/safety.h"        //For emulator pause status and battery pause
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "../lib/uds_isotp/uds.h"  // UDS service IDs and negative-response codes

/*
TODO list
- Check all TODO:s in the code
- Investigate why opening and then closing contactors from webpage does not always work
- remaining_capacity_Wh is based on a lower limit of 5% soc. This means that at 5% soc, remaining_capacity_Wh returns 0.
*/

const int RX_KN_Hybrid_01 = 0x0001;
const int RX_BMS_21 = 0x0002;
const int RX_BMS_22 = 0x0004;
const int RX_BMS_23 = 0x0008;
const int RX_BMS_24 = 0x0010;
const int RX_BMS_25 = 0x0020;
const int RX_BMS_31 = 0x0040;
const int RX_BMS_11 = 0x0080;
const int RX_BMS_26 = 0x0100;
const int RX_BMS_28 = 0x0200;
const int RX_BMS_04 = 0x0400;
const int RX_BMS_07 = 0x0800;
const int RX_BMS_20 = 0x1000;
const int RX_DEFAULT = 0xE000;

// VAG PDU constants tables
static const uint8_t Airbag_01_PDU_CONST[16] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                                                0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
static const uint8_t EM1_01_PDU_CONST[16] = {0x2f, 0x44, 0x72, 0xd3, 0x07, 0xf2, 0x39, 0x09,
                                             0x8d, 0x6f, 0x57, 0x20, 0x37, 0xf9, 0x9b, 0xfa};
static const uint8_t BMS_20_PDU_CONST[16] = {0xee, 0x80, 0x6e, 0x4e, 0x29, 0xc6, 0x92, 0xc0,
                                             0x65, 0xaa, 0x3a, 0xa1, 0x8f, 0xcd, 0xe6, 0x90};
static const uint8_t ESC_51_Auth_PDU_CONST[16] = {0x77, 0x5c, 0xa0, 0x89, 0x4b, 0x7c, 0xbb, 0xd6,
                                                  0x1f, 0x6c, 0x4f, 0xf6, 0x20, 0x2b, 0x43, 0xdd};
static const uint8_t ESP_21_PDU_CONST[16] = {0xb4, 0xef, 0xf8, 0x49, 0x1e, 0xe5, 0xc2, 0xc0,
                                             0x97, 0x19, 0x3c, 0xc9, 0xf1, 0x98, 0xd6, 0x61};
static const uint8_t BMS_HYB_02_PDU_CONST[16] = {0x3C, 0x54, 0xCF, 0xA3, 0x81, 0x93, 0x0B, 0xC7,
                                                 0x3E, 0xDF, 0x1C, 0xB0, 0xA7, 0x25, 0xD3, 0xD8};
static const uint8_t DCDC_04_PDU_CONST[16] = {0x5F, 0xA0, 0x44, 0xD0, 0x63, 0x59, 0x5B, 0xA2,
                                              0x68, 0x04, 0x90, 0x87, 0x52, 0x12, 0xB4, 0x9E};
static const uint8_t BMS_HYB_04_PDU_CONST[16] = {0x12, 0x7E, 0x34, 0x16, 0x25, 0x8F, 0x8E, 0x35,
                                                 0xBA, 0x7F, 0xEA, 0x59, 0x4C, 0xF0, 0x88, 0x15};
static const uint8_t MSG_HYB_30_PDU_CONST[16] = {0x03, 0x13, 0x23, 0x7a, 0x40, 0x51, 0x68, 0xba,
                                                 0xa8, 0xbe, 0x55, 0x02, 0x11, 0x31, 0x76, 0xec};
static const uint8_t Motor_54_PDU_CONST[16] = {0x16, 0x35, 0x59, 0x15, 0x9a, 0x2a, 0x97, 0xb8,
                                               0x0e, 0x4e, 0x30, 0xcc, 0xb3, 0x07, 0x01, 0xad};
static const uint8_t Motor_EV_01_PDU_CONST[16] = {0x7F, 0xED, 0x17, 0xC2, 0x7C, 0xEB, 0x44, 0x21,
                                                  0x01, 0xFA, 0xDB, 0x15, 0x4A, 0x6B, 0x23, 0x05};
static const uint8_t MSG_HYB_01_PDU_CONST[16] = {0xB6, 0x1C, 0xC1, 0x23, 0x6D, 0x8B, 0x0C, 0x51,
                                                 0x38, 0x32, 0x24, 0xA8, 0x3F, 0x3A, 0xA4, 0x02};
static const uint8_t DC_HYB_02_PDU_CONST[16] = {0x94, 0x6A, 0xB5, 0x38, 0x8A, 0xB4, 0xAB, 0x27,
                                                0xCB, 0x22, 0x88, 0xEF, 0xA3, 0xE1, 0xD0, 0xBB};
static const uint8_t Motor_14_PDU_CONST[16] = {0x1f, 0x28, 0xc6, 0x85, 0xe6, 0xf8, 0xb0, 0x19,
                                               0x5b, 0x64, 0x35, 0x21, 0xe4, 0xf7, 0x9c, 0x24};
static const uint8_t Klemmen_Status_01_PDU_CONST[16] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                                                        0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3};
static const uint8_t HVK_01_PDU_CONST[16] = {0xed, 0xd6, 0x96, 0x63, 0xa5, 0x12, 0xd5, 0x9a,
                                             0x1e, 0x0d, 0x24, 0xcd, 0x8c, 0xa6, 0x2f, 0x41};
static const uint8_t BMS_DC_01_PDU_CONST[16] = {0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
                                                0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48};
static const uint8_t BMS_04_PDU_CONST[16] = {0xeb, 0x4c, 0x44, 0xaf, 0x21, 0x8d, 0x01, 0x58,
                                             0xfa, 0x93, 0xdb, 0x89, 0x15, 0x10, 0x4a, 0x61};
static const uint8_t BMS_07_PDU_CONST[16] = {0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
                                             0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43};
static const uint8_t Motor_Code_01_PDU_CONST[16] = {0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
                                                    0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47};
static const uint8_t BMS_HYB_06_PDU_CONST[16] = {0xC1, 0x8B, 0x38, 0xA8, 0xA4, 0x27, 0xEB, 0xC8,
                                                 0xEF, 0x05, 0x9A, 0xBB, 0x39, 0xF7, 0x80, 0xA7};
static const uint8_t EM_HYB_05_PDU_CONST[16] = {0xC7, 0xD8, 0xF1, 0xC4, 0xE3, 0x5E, 0x9A, 0xE2,
                                                0xA1, 0xCB, 0x02, 0x4F, 0x57, 0x4E, 0x8E, 0xE4};
static const uint8_t BMS_11_PDU_CONST[16] = {0x79, 0xB9, 0x67, 0xAD, 0xD5, 0xF7, 0x70, 0xAA,
                                             0x44, 0x61, 0x5A, 0xDC, 0x26, 0xB4, 0xD2, 0xC3};

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
uint8_t MebBattery::vw_crc_calc(const uint8_t* inputBytes, uint8_t length, uint32_t address) {

  constexpr uint8_t poly = 0x2F;
  constexpr uint8_t xor_output = 0xFF;

  // Basic validation: need at least two bytes to read the counter
  if (inputBytes == nullptr || length < 2) {
    logging.println("vw_crc_calc: invalid input");
    return 0x00;
  }

  uint8_t crc = 0xFF;
  uint8_t magicByte = 0x00;
  uint8_t counter = inputBytes[1] & 0x0F;  // only the low nibble of the counter is relevant

  switch (address) {
    case Airbag_01:  // Airbag (0x40)
      magicByte = Airbag_01_PDU_CONST[counter];
      break;
    case EM1_01:  // Electric motor (0xC0)
      magicByte = EM1_01_PDU_CONST[counter];
      break;
    case BMS_20:  // BMS_20 (0xCF)
      magicByte = BMS_20_PDU_CONST[counter];
      break;
    case ESC_51_Auth:  // (0xFC)
      magicByte = ESC_51_Auth_PDU_CONST[counter];
      break;
    case ESP_21:  // ESP_21 (0xFD)
      magicByte = ESP_21_PDU_CONST[counter];
      break;
    case DCDC_04:  // DCDC (0xF7)
      magicByte = DCDC_04_PDU_CONST[counter];
      break;
    case BMS_HYB_02:  // BMS_HYB_02 (0x97)
      magicByte = BMS_HYB_02_PDU_CONST[counter];
      break;
    case BMS_HYB_04:  // BMS_HYB_04 (0x124)
      magicByte = BMS_HYB_04_PDU_CONST[counter];
      break;
    case Motor_54:  // Motor_54 (0x14C)
      magicByte = Motor_54_PDU_CONST[counter];
      break;
    case MSG_HYB_30:  // HYB30 (0x153)
      magicByte = MSG_HYB_30_PDU_CONST[counter];
      break;
    case Motor_EV_01:  // Motor_EV_01 (0x187)
      magicByte = Motor_EV_01_PDU_CONST[counter];
      break;
    case MSG_HYB_01:  // MSG_HYB_01 (0x3A6)
      magicByte = MSG_HYB_01_PDU_CONST[counter];
      break;
    case DC_HYB_02:  // DC_HYB_02 (0x3AF)
      magicByte = DC_HYB_02_PDU_CONST[counter];
      break;
    case Motor_14:  // Motor_14 (0x3BE)
      magicByte = Motor_14_PDU_CONST[counter];
      break;
    case Klemmen_Status_01:  // Klemmen status (0x3C0)
      magicByte = Klemmen_Status_01_PDU_CONST[counter];
      break;
    case HVK_01:  // HVK (0x503)
      magicByte = HVK_01_PDU_CONST[counter];
      break;
    case BMS_DC_01:  // BMS DC (0x578)
      magicByte = BMS_DC_01_PDU_CONST[counter];
      break;
    case BMS_04:  // BMS (0x5A2)
      magicByte = BMS_04_PDU_CONST[counter];
      break;
    case BMS_07:  // BMS (0x5CA)
      magicByte = BMS_07_PDU_CONST[counter];
      break;
    case Motor_Code_01:  // Motor (0x641)
      magicByte = Motor_Code_01_PDU_CONST[counter];
      break;
    case BMS_HYB_06:  // BMS_HYB_06 (0x6A3)
      magicByte = BMS_HYB_06_PDU_CONST[counter];
      break;
    case EM_HYB_05:  // EM_HYB_05 (0x6A4)
      magicByte = EM_HYB_05_PDU_CONST[counter];
      break;
    case BMS_11:
      magicByte = BMS_11_PDU_CONST[counter];
      break;
    default:  // this won't lead to correct CRC checksums
      logging.println("Checksum request unknown");
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

void MebBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for modbus

  datalayer_battery->status.real_soc = battery_SOC * 5;  //*0.05*100

  datalayer_battery->status.voltage_dV = BMS_voltage * 2.5f;  // *0.25*10

  datalayer_battery->status.current_dA = (BMS_current - 16300);  // 0.1 * 10

  if (nof_cells_determined) {
    datalayer_battery->info.total_capacity_Wh =
        ((float)datalayer_battery->info.number_of_cells) * 3.67f * ((float)BMS_capacity_ah) * 0.2f * 1.02564f;
    // The factor 1.02564 = 1/0.975 is to correct for bottom 2.5% which is reported by the remaining_capacity_Wh,
    // but which is not actually usable, but if we do not include it, the remaining_capacity_Wh can be larger than
    // the total_capacity_Wh.
    // 0.935 and 0.9025 are the different conversions for different battery sizes to go from design capacity to
    // total_capacity_Wh calculated above.

    if (battery_soh_polled > 0) {
      datalayer_battery->status.soh_pptt = battery_soh_polled;
    } else {
      int Wh_max = 61832 * 0.935f;  // 108 cells
      if (datalayer_battery->info.number_of_cells <= 84)
        Wh_max = 48091 * 0.9025f;
      else if (datalayer_battery->info.number_of_cells <= 96)
        Wh_max = 82442 * 0.9025f;
      if (BMS_capacity_ah > 0)
        datalayer_battery->status.soh_pptt = 10000 * datalayer_battery->info.total_capacity_Wh / (Wh_max * 1.02564f);
    }
  }

  datalayer_battery->status.remaining_capacity_Wh = usable_energy_amount_Wh * 5;

  datalayer_battery->status.max_charge_power_W = (max_charge_power_watt * 100);

  datalayer_battery->status.max_discharge_power_W = (max_discharge_power_watt * 100);

  //Power in watts, Negative = charging batt
  datalayer_battery->status.active_power_W =
      ((datalayer_battery->status.voltage_dV * datalayer_battery->status.current_dA) / 100);

  if ((battery_min_temp != 600) && (battery_max_temp != 600)) {  //Only update when both values have been read
    // datalayer.battery.status.temperature_min_dC = actual_temperature_lowest_C*5 -400;  // We use the value below, because it has better accuracy
    datalayer_battery->status.temperature_min_dC = (battery_min_temp * 10) / 64;
    // datalayer.battery.status.temperature_max_dC = actual_temperature_highest_C*5 -400;  // We use the value below, because it has better accuracy
    datalayer_battery->status.temperature_max_dC = (battery_max_temp * 10) / 64;
  }

  //Map all cell voltages to the global array
  memcpy(datalayer_battery->status.cell_voltages_mV, cellvoltages_polled, 108 * sizeof(uint16_t));

  if (service_disconnect_switch_missing) {
    set_event(EVENT_HVIL_FAILURE, 1);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }
  if (pilotline_open || BMS_HVIL_status == 2) {
    set_event(EVENT_HVIL_FAILURE, 2);
  } else {
    clear_event(EVENT_HVIL_FAILURE);
  }

  // Update webserver datalayer for "More battery info" page
  datalayer_extended.meb.SDSW = service_disconnect_switch_missing;
  datalayer_extended.meb.pilotline = pilotline_open;
  datalayer_extended.meb.transportmode = transportation_mode_active;
  datalayer_extended.meb.componentprotection = component_protection_active;
  datalayer_extended.meb.shutdown_active = shutdown_active;
  datalayer_extended.meb.HVIL = BMS_HVIL_status;
  datalayer_extended.meb.BMS_mode = BMS_mode;
  datalayer_extended.meb.battery_diagnostic = battery_diagnostic;
  datalayer_extended.meb.status_HV_PTC_line = status_HV_PTC_line;
  datalayer_extended.meb.BMS_fault_performance = BMS_fault_performance;
  datalayer_extended.meb.BMS_fault_emergency_shutdown_crash = BMS_fault_emergency_shutdown_crash;
  datalayer_extended.meb.BMS_error_shutdown_request = BMS_error_shutdown_request;
  datalayer_extended.meb.BMS_error_shutdown = BMS_error_shutdown;
  datalayer_extended.meb.BMS_welded_contactors_status = BMS_welded_contactors_status;

  datalayer_extended.meb.warning_support = warning_support;
  datalayer_extended.meb.BMS_status_voltage_free = BMS_status_voltage_free;
  datalayer_extended.meb.BMS_OBD_MIL = BMS_OBD_MIL;
  datalayer_extended.meb.BMS_error_status = BMS_error_status;
  datalayer_extended.meb.BMS_error_lamp_req = BMS_error_lamp_req;
  datalayer_extended.meb.BMS_warning_lamp_req = BMS_warning_lamp_req;
  datalayer_extended.meb.BMS_Kl30c_Status = BMS_Kl30c_Status;
  datalayer_extended.meb.BMS_voltage_intermediate_dV = (BMS_voltage_intermediate - 2000) * 10 / 2;
  datalayer_extended.meb.BMS_voltage_dV = BMS_voltage * 10 / 4;
  datalayer_extended.meb.isolation_resistance = isolation_resistance_kOhm * 5;
  datalayer_extended.meb.battery_heating = battery_heating_active;
  datalayer_extended.meb.rt_overcurrent = realtime_overcurrent_monitor;
  datalayer_extended.meb.rt_CAN_fault = realtime_CAN_communication_fault;
  datalayer_extended.meb.rt_overcharge = realtime_overcharge_warning;
  datalayer_extended.meb.rt_SOC_high = realtime_SOC_too_high;
  datalayer_extended.meb.rt_SOC_low = realtime_SOC_too_low;
  datalayer_extended.meb.rt_SOC_jumping = realtime_SOC_jumping_warning;
  datalayer_extended.meb.rt_temp_difference = realtime_temperature_difference_warning;
  datalayer_extended.meb.rt_cell_overtemp = realtime_cell_overtemperature_warning;
  datalayer_extended.meb.rt_cell_undertemp = realtime_cell_undertemperature_warning;
  datalayer_extended.meb.rt_battery_overvolt = realtime_battery_overvoltage_warning;
  datalayer_extended.meb.rt_battery_undervol = realtime_battery_undervoltage_warning;
  datalayer_extended.meb.rt_cell_overvolt = realtime_cell_overvoltage_warning;
  datalayer_extended.meb.rt_cell_undervol = realtime_cell_undervoltage_warning;
  datalayer_extended.meb.rt_cell_imbalance = realtime_cell_imbalance_warning;
  datalayer_extended.meb.rt_battery_unathorized = realtime_warning_battery_unathorized;
  if (balancing_active == 1 && datalayer_extended.meb.balancing_active != 1) {
    datalayer_battery->status.balancing_status = BALANCING_STATUS_ACTIVE;
    set_event_latched(EVENT_BALANCING_START, 0);
  }
  if (balancing_active == 2 && datalayer_extended.meb.balancing_active == 1) {
    datalayer_battery->status.balancing_status = BALANCING_STATUS_READY;
    set_event(EVENT_BALANCING_END, 0);
  }
  datalayer_extended.meb.balancing_active = balancing_active;
  datalayer_extended.meb.balancing_request = balancing_request;
  datalayer_extended.meb.charging_active = charging_active;
}

void MebBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  last_can_msg_timestamp = millis();
  if (first_can_msg_timestamp == 0) {
    logging.printf("MEB: First CAN msg received\n");
    first_can_msg_timestamp = last_can_msg_timestamp;
  }

  /* CRC check on messages with CRC */
  switch (rx_frame.ID) {
    case BMS_20:
    case BMS_DC_01:
    case BMS_04:
    case BMS_07:
    case BMS_11:
      if (rx_frame.data.u8[0] !=
          vw_crc_calc(rx_frame.data.u8, rx_frame.DLC, rx_frame.ID)) {  //If CRC does not match calc
        datalayer.battery.status.CAN_error_counter++;
        logging.printf("MEB: Msg 0x%04X CRC error\n", rx_frame.ID);
        return;
      }
    default:
      break;
  }

  switch (rx_frame.ID) {
    case KN_Hybrid_01:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_KN_Hybrid_01;
      component_protection_active = (rx_frame.data.u8[0] & 0x01);
      shutdown_active = ((rx_frame.data.u8[0] & 0x02) >> 1);
      transportation_mode_active = ((rx_frame.data.u8[0] & 0x04) >> 2);
      KL15_mode = ((rx_frame.data.u8[0] & 0xF0) >> 4);
      //0 = communication only when terminal 15 = ON (no run-on, cannot be woken up)
      //1 = communication after terminal 15 = OFF (run-on, cannot be woken up)
      //2 = communication when terminal 15 = OFF (run-on, can be woken up)
      bus_knockout_timer = rx_frame.data.u8[5];
      hybrid_wakeup_reason = rx_frame.data.u8[6];  //(if several active, lowest wins)
      //0 = wakeup cause not known 1 = Bus wakeup2 = KL15 HW 3 = TPA active
      break;
    case ISO_Hybrid_01_Resp:  // BMS - Offboard tester diag response
      break;
    case NMH_Hybrid_01:  // BMS - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      wakeup_type =
          ((rx_frame.data.u8[1] & 0x10) >> 4);  //0 passive, SG has not woken up, 1 active, SG has woken up the network
      instrument_cluster_request = ((rx_frame.data.u8[1] & 0x40) >> 6);  //True/false
      break;
    case BMS_21:  // BMS Limits 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_21;
      max_discharge_power_watt =
          ((rx_frame.data.u8[6] & 0x07) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[4] & 0xC0) >> 6;  //*100
      max_discharge_current_amp =
          ((rx_frame.data.u8[3] & 0x01) << 12) | (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*0.2
      max_charge_power_watt = (rx_frame.data.u8[7] << 5) | (rx_frame.data.u8[6] >> 3);                     //*100
      max_charge_current_amp = ((rx_frame.data.u8[4] & 0x3F) << 7) | (rx_frame.data.u8[3] >> 1);           //*0.2
      break;
    case BMS_22:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_22;
      if (rx_frame.data.u8[6] != 0xFE || rx_frame.data.u8[7] != 0xFF) {  // Init state, values below invalid
        battery_SOC = ((rx_frame.data.u8[3] & 0x0F) << 7) | (rx_frame.data.u8[2] >> 1);               //*0.05
        usable_energy_amount_Wh = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];                   //*5
        power_discharge_percentage = ((rx_frame.data.u8[4] & 0x3F) << 4) | rx_frame.data.u8[3] >> 4;  //*0.2
        power_charge_percentage = (rx_frame.data.u8[5] << 2) | rx_frame.data.u8[4] >> 6;              //*0.2
      }
      status_HV_PTC_line = ((rx_frame.data.u8[2] & 0x01) << 1) | rx_frame.data.u8[1] >> 7;
      warning_support = (rx_frame.data.u8[1] & 0x70) >> 4;
      break;
    case BMS_23:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_23;
      battery_heating_active = (rx_frame.data.u8[4] & 0x40) >> 6;
      heating_request = (rx_frame.data.u8[5] & 0xE0) >> 5;
      cooling_request = (rx_frame.data.u8[5] & 0x1C) >> 2;
      power_battery_heating_watt = rx_frame.data.u8[6];
      power_battery_heating_req_watt = rx_frame.data.u8[7];
      break;
    case BMS_24:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_24;
      balancing_active = (rx_frame.data.u8[1] & 0xC0) >> 6;
      charging_active = (rx_frame.data.u8[2] & 0x01);
      max_energy_Wh = ((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[5];                     //*40
      max_charge_percent = ((rx_frame.data.u8[7] << 3) | rx_frame.data.u8[6] >> 5);                  //*0.05
      min_charge_percent = ((rx_frame.data.u8[4] << 3) | rx_frame.data.u8[3] >> 5);                  //*0.05
      isolation_resistance_kOhm = (((rx_frame.data.u8[3] & 0x1F) << 7) | rx_frame.data.u8[2] >> 1);  //*5
      datalayer_battery->status.insulation_resistance_kOhm = isolation_resistance_kOhm * 5;
      datalayer_battery->status.insulation_resistance_available = true;
      break;
    case BMS_25:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_25;
      battery_heating_installed = (rx_frame.data.u8[1] & 0x20) >> 5;
      error_NT_circuit = (rx_frame.data.u8[1] & 0x40) >> 6;
      pump_1_control = rx_frame.data.u8[2] & 0x0F;         //*10, percent
      pump_2_control = (rx_frame.data.u8[2] & 0xF0) >> 4;  //*10, percent
      status_valve_1 = (rx_frame.data.u8[3] & 0x1C) >> 2;
      status_valve_2 = (rx_frame.data.u8[3] & 0xE0) >> 5;
      temperature_request = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      datalayer_extended.meb.battery_temperature_dC = rx_frame.data.u8[5] * 5 - 400;  //*0,5 -40
      target_flow_temperature_C = rx_frame.data.u8[6];                                //*0,5 -40
      return_temperature_C = rx_frame.data.u8[7];                                     //*0,5 -40
      break;
    case BMS_31:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_31;
      performance_index_discharge_peak_temperature_percentage =
          (((rx_frame.data.u8[3] & 0x07) << 6) | rx_frame.data.u8[2] >> 2);  //*0.2
      performance_index_charge_peak_temperature_percentage =
          (((rx_frame.data.u8[4] & 0x3F) << 3) | rx_frame.data.u8[3] >> 5);  //*0.2
      temperature_status_discharge = (rx_frame.data.u8[1] & 0x70) >> 4;
      temperature_status_charge = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      break;
    case BMS_11:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_BMS_11;
      BMS_11_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      isolation_fault = (rx_frame.data.u8[2] & 0xE0) >> 5;
      isolation_status = (rx_frame.data.u8[2] & 0x1E) >> 1;
      if (isolation_fault != 0) {
        actual_temperature_highest_C = rx_frame.data.u8[3];  //*0,5 -40
        actual_temperature_lowest_C = rx_frame.data.u8[4];   //*0,5 -40
        actual_cellvoltage_highest_mV = (((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5]);
        actual_cellvoltage_lowest_mV = ((rx_frame.data.u8[7] << 4) | rx_frame.data.u8[6] >> 4);
        datalayer.battery.status.cell_min_voltage_mV = actual_cellvoltage_lowest_mV + 1000;
        datalayer.battery.status.cell_max_voltage_mV = actual_cellvoltage_highest_mV + 1000;
      }
      break;
    case BMS_29:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      predicted_power_dyn_standard_watt = ((rx_frame.data.u8[6] << 1) | rx_frame.data.u8[5] >> 7);  //*50
      predicted_time_dyn_standard_minutes = rx_frame.data.u8[7];
      break;
    case BMS_CMC_04:  // BMS Temperature and cellvoltages - 180ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & 0x0F);
      switch (mux) {
        case 0:  // Temperatures 1-56. Value is 0xFD if sensor not present
          for (uint8_t i = 0; i < 56; i++) {
            datalayer_extended.meb.celltemperature_dC[i] = ((int16_t)rx_frame.data.u8[i + 1] * 5) - 400;
          }
          break;
        /*
        Broadcast cellvoltages are currently disabled, since they're not in use.

        The polled cellvoltages are being used instead.
        ----
        case 1:  // Cellvoltages 1-42
          cellvoltages[0] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[1] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[2] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[3] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[4] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[5] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[6] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[7] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[8] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[9] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[10] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[11] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[12] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[13] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[14] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[15] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[16] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[17] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[18] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[19] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[20] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[21] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[22] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[23] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[24] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[25] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[26] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[27] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[28] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[29] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[30] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[31] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[32] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[33] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[34] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[35] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[36] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[37] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[38] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[39] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[40] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[41] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 2:  // Cellvoltages 43-84
          cellvoltages[42] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[43] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[44] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[45] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[46] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[47] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[48] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[49] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[50] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[51] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[52] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[53] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[54] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[55] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[56] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[57] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[58] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[59] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[60] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[61] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[62] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[63] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[64] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[65] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[66] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[67] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[68] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[69] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[70] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[71] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[72] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[73] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[74] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[75] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[76] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[77] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[78] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[79] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[80] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[81] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[82] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[83] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 3:  // Cellvoltages 85-126
          cellvoltages[84] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[85] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[86] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[87] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[88] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[89] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[90] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[91] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[92] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[93] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[94] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[95] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[96] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[97] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[98] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[99] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[100] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[101] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[102] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[103] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[104] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[105] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[106] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[107] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[108] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[109] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[110] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[111] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[112] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[113] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[114] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[115] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[116] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[117] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          cellvoltages[118] = (((rx_frame.data.u8[53] & 0x0F) << 8) | rx_frame.data.u8[52]) + 1000;
          cellvoltages[119] = ((rx_frame.data.u8[54] << 4) | (rx_frame.data.u8[53] >> 4)) + 1000;
          cellvoltages[120] = (((rx_frame.data.u8[56] & 0x0F) << 8) | rx_frame.data.u8[55]) + 1000;
          cellvoltages[121] = ((rx_frame.data.u8[57] << 4) | (rx_frame.data.u8[56] >> 4)) + 1000;
          cellvoltages[122] = (((rx_frame.data.u8[59] & 0x0F) << 8) | rx_frame.data.u8[58]) + 1000;
          cellvoltages[123] = ((rx_frame.data.u8[60] << 4) | (rx_frame.data.u8[59] >> 4)) + 1000;
          cellvoltages[124] = (((rx_frame.data.u8[62] & 0x0F) << 8) | rx_frame.data.u8[61]) + 1000;
          cellvoltages[125] = ((rx_frame.data.u8[63] << 4) | (rx_frame.data.u8[62] >> 4)) + 1000;
          break;
        case 4:  //Cellvoltages 127-160
          cellvoltages[126] = (((rx_frame.data.u8[2] & 0x0F) << 8) | rx_frame.data.u8[1]) + 1000;
          cellvoltages[127] = ((rx_frame.data.u8[3] << 4) | (rx_frame.data.u8[2] >> 4)) + 1000;
          cellvoltages[128] = (((rx_frame.data.u8[5] & 0x0F) << 8) | rx_frame.data.u8[4]) + 1000;
          cellvoltages[129] = ((rx_frame.data.u8[6] << 4) | (rx_frame.data.u8[5] >> 4)) + 1000;
          cellvoltages[130] = (((rx_frame.data.u8[8] & 0x0F) << 8) | rx_frame.data.u8[7]) + 1000;
          cellvoltages[131] = ((rx_frame.data.u8[9] << 4) | (rx_frame.data.u8[8] >> 4)) + 1000;
          cellvoltages[132] = (((rx_frame.data.u8[11] & 0x0F) << 8) | rx_frame.data.u8[10]) + 1000;
          cellvoltages[133] = ((rx_frame.data.u8[12] << 4) | (rx_frame.data.u8[11] >> 4)) + 1000;
          cellvoltages[134] = (((rx_frame.data.u8[14] & 0x0F) << 8) | rx_frame.data.u8[13]) + 1000;
          cellvoltages[135] = ((rx_frame.data.u8[15] << 4) | (rx_frame.data.u8[14] >> 4)) + 1000;
          cellvoltages[136] = (((rx_frame.data.u8[17] & 0x0F) << 8) | rx_frame.data.u8[16]) + 1000;
          cellvoltages[137] = ((rx_frame.data.u8[18] << 4) | (rx_frame.data.u8[17] >> 4)) + 1000;
          cellvoltages[138] = (((rx_frame.data.u8[20] & 0x0F) << 8) | rx_frame.data.u8[19]) + 1000;
          cellvoltages[139] = ((rx_frame.data.u8[21] << 4) | (rx_frame.data.u8[20] >> 4)) + 1000;
          cellvoltages[140] = (((rx_frame.data.u8[23] & 0x0F) << 8) | rx_frame.data.u8[22]) + 1000;
          cellvoltages[141] = ((rx_frame.data.u8[24] << 4) | (rx_frame.data.u8[23] >> 4)) + 1000;
          cellvoltages[142] = (((rx_frame.data.u8[26] & 0x0F) << 8) | rx_frame.data.u8[25]) + 1000;
          cellvoltages[143] = ((rx_frame.data.u8[27] << 4) | (rx_frame.data.u8[26] >> 4)) + 1000;
          cellvoltages[144] = (((rx_frame.data.u8[29] & 0x0F) << 8) | rx_frame.data.u8[28]) + 1000;
          cellvoltages[145] = ((rx_frame.data.u8[30] << 4) | (rx_frame.data.u8[29] >> 4)) + 1000;
          cellvoltages[146] = (((rx_frame.data.u8[32] & 0x0F) << 8) | rx_frame.data.u8[31]) + 1000;
          cellvoltages[147] = ((rx_frame.data.u8[33] << 4) | (rx_frame.data.u8[32] >> 4)) + 1000;
          cellvoltages[148] = (((rx_frame.data.u8[35] & 0x0F) << 8) | rx_frame.data.u8[34]) + 1000;
          cellvoltages[149] = ((rx_frame.data.u8[36] << 4) | (rx_frame.data.u8[35] >> 4)) + 1000;
          cellvoltages[150] = (((rx_frame.data.u8[38] & 0x0F) << 8) | rx_frame.data.u8[37]) + 1000;
          cellvoltages[151] = ((rx_frame.data.u8[39] << 4) | (rx_frame.data.u8[38] >> 4)) + 1000;
          cellvoltages[152] = (((rx_frame.data.u8[41] & 0x0F) << 8) | rx_frame.data.u8[40]) + 1000;
          cellvoltages[153] = ((rx_frame.data.u8[42] << 4) | (rx_frame.data.u8[41] >> 4)) + 1000;
          cellvoltages[154] = (((rx_frame.data.u8[44] & 0x0F) << 8) | rx_frame.data.u8[43]) + 1000;
          cellvoltages[155] = ((rx_frame.data.u8[45] << 4) | (rx_frame.data.u8[44] >> 4)) + 1000;
          cellvoltages[156] = (((rx_frame.data.u8[47] & 0x0F) << 8) | rx_frame.data.u8[46]) + 1000;
          cellvoltages[157] = ((rx_frame.data.u8[48] << 4) | (rx_frame.data.u8[47] >> 4)) + 1000;
          cellvoltages[158] = (((rx_frame.data.u8[50] & 0x0F) << 8) | rx_frame.data.u8[49]) + 1000;
          cellvoltages[159] = ((rx_frame.data.u8[51] << 4) | (rx_frame.data.u8[50] >> 4)) + 1000;
          break;
        */
        default:  //Invalid mux
          //TODO: Add corrupted CAN message counter tick?
          break;
      }
      break;
    case 0x1C42017B:  // BMS - Non-Cyclic, TP_ISO
      //hybrid_01_response_fd_data (Whole frame)
      break;
    case BMS_26:  // BMS 1000ms cyclic
      can_msg_received |= RX_BMS_26;
      duration_discharge_power_watt = ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5];
      duration_charge_power_watt = (rx_frame.data.u8[7] << 4) | rx_frame.data.u8[6] >> 4;
      maximum_voltage = ((rx_frame.data.u8[3] & 0x3F) << 4) | rx_frame.data.u8[2] >> 4;
      minimum_voltage = (rx_frame.data.u8[4] << 2) | rx_frame.data.u8[3] >> 6;
      break;
    case BMS_28:  // BMS 1000ms cyclic
      can_msg_received |= RX_BMS_28;
      // All realtime_ have same enumeration, 0 = no fault, 1 = error level 1, 2 error level 2, 3 error level 3
      realtime_overcurrent_monitor = ((rx_frame.data.u8[3] & 0x01) << 2) | rx_frame.data.u8[2] >> 6;
      realtime_CAN_communication_fault = (rx_frame.data.u8[3] & 0x0E) >> 1;
      realtime_overcharge_warning = (rx_frame.data.u8[3] & 0x70) >> 4;
      realtime_SOC_too_high = ((rx_frame.data.u8[4] & 0x03) << 1) | rx_frame.data.u8[3] >> 7;
      realtime_SOC_too_low = (rx_frame.data.u8[4] & 0x1C) >> 2;
      realtime_SOC_jumping_warning = (rx_frame.data.u8[4] & 0xE0) >> 5;
      realtime_temperature_difference_warning = rx_frame.data.u8[5] & 0x07;
      realtime_cell_overtemperature_warning = (rx_frame.data.u8[5] & 0x38) >> 3;
      realtime_cell_undertemperature_warning = ((rx_frame.data.u8[6] & 0x01) << 2) | rx_frame.data.u8[5] >> 6;
      realtime_battery_overvoltage_warning = (rx_frame.data.u8[6] & 0x0E) >> 1;
      realtime_battery_undervoltage_warning = (rx_frame.data.u8[6] & 0x70) >> 4;
      realtime_cell_overvoltage_warning = ((rx_frame.data.u8[7] & 0x03) << 1) | rx_frame.data.u8[6] >> 7;
      realtime_cell_undervoltage_warning = (rx_frame.data.u8[7] & 0x1C) >> 2;
      realtime_cell_imbalance_warning = (rx_frame.data.u8[7] & 0xE0) >> 5;
      for (uint8_t i = 0; i < 26; i++) {  // Frame 9 to 34 is S/N for battery
        battery_serialnumber[i] = rx_frame.data.u8[i + 9];
      }
      realtime_warning_battery_unathorized = (rx_frame.data.u8[40] & 0x07);
      break;
    case BMS_05:  // BMS 50ms
      actual_battery_voltage =
          ((rx_frame.data.u8[1] & 0x3F) << 8) | rx_frame.data.u8[0];  //*0.0625 // Seems to be 0.125 in logging
      regen_battery = ((rx_frame.data.u8[5] & 0x7F) << 8) | rx_frame.data.u8[4];
      energy_extracted_from_battery = ((rx_frame.data.u8[7] & 0x7F) << 8) | rx_frame.data.u8[6];
      break;
    case BMS_DC_01:                                      // BMS 100ms
      BMS_DC_01_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      BMS_Status_DCLS = ((rx_frame.data.u8[1] & 0x30) >> 4);
      DC_voltage_DCLS = (rx_frame.data.u8[2] << 6) | (rx_frame.data.u8[1] >> 6);
      max_fastcharging_current_amp = ((rx_frame.data.u8[4] & 0x01) << 8) | rx_frame.data.u8[3];
      DC_voltage_chargeport = (rx_frame.data.u8[7] << 4) | (rx_frame.data.u8[6] >> 4);
      break;
    case BMS_04:  // BMS 500ms normal, 100ms fast
      can_msg_received |= RX_BMS_04;
      BMS_04_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      service_disconnect_switch_missing = (rx_frame.data.u8[1] & 0x20) >> 5;
      pilotline_open = (rx_frame.data.u8[1] & 0x10) >> 4;
      BMS_status_voltage_free = (rx_frame.data.u8[1] & 0xC0) >> 6;
      BMS_OBD_MIL = (rx_frame.data.u8[2] & 0x01);
      BMS_error_status = (rx_frame.data.u8[2] & 0x70) >> 4;
      BMS_error_lamp_req = (rx_frame.data.u8[4] & 0x04) >> 2;
      BMS_warning_lamp_req = (rx_frame.data.u8[4] & 0x08) >> 3;
      BMS_Kl30c_Status = (rx_frame.data.u8[4] & 0x30) >> 4;
      if (BMS_Kl30c_Status != 0) {  // init state
        BMS_capacity_ah = ((rx_frame.data.u8[4] & 0x03) << 9) | (rx_frame.data.u8[3] << 1) | (rx_frame.data.u8[2] >> 7);
      }
      break;
    case BMS_07:  // BMS 500ms
      can_msg_received |= RX_BMS_07;
      BMS_07_counter = (rx_frame.data.u8[1] & 0x0F);
      balancing_request = (rx_frame.data.u8[5] & 0x08) >> 3;
      // balancing_request: BMS requests a low current end charge to support balancing, maybe unused.
      battery_diagnostic = (rx_frame.data.u8[3] & 0x07);
      battery_Wh_left =
          (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*50  ! Not usable, seems to always contain 0x7F0
      battery_potential_status =
          (rx_frame.data.u8[5] & 0x30) >> 4;  //0 = function not enabled, 1= no potential, 2 = potential on, 3 = fault
      battery_temperature_warning =
          (rx_frame.data.u8[7] & 0x0C) >> 2;  // 0 = no warning, 1 = temp level 1, 2=temp level 2
      battery_Wh_max =
          ((rx_frame.data.u8[5] & 0x07) << 8) | rx_frame.data.u8[4];  //*50  ! Not usable, seems to always contain 0x7F0
      break;
    case BMS_20:  //BMS 10ms
      can_msg_received |= RX_BMS_20;
      BMS_20_counter = (rx_frame.data.u8[1] & 0x0F);
      BMS_welded_contactors_status = (rx_frame.data.u8[1] & 0x60) >> 5;
      BMS_ext_limits_active = (rx_frame.data.u8[1] & 0x80) >> 7;
      BMS_mode = (rx_frame.data.u8[2] & 0x07);
      // Force a clean sleep/re-init cycle via the reset state machine so it comes back up
      // through Init with KL_15 properly gated.
      if (!startup_bms_checked) {
        startup_bms_checked = true;
        if (BMS_mode != BMS_TARGET_INIT) {
          //logging.println("MEB: BMS already awake at boot (emulator reboot) - triggering BMS reset");
          datalayer_meb->UserRequestBMSReset = true;
        }
      }
      switch (BMS_mode) {
        case 1:  // HV_ACTIVE
        case 3:  // EXTERN CHARGING
        case 4:  // AC_CHARGING
        case 6:  // DC_CHARGING
          if (!datalayer.system.status.battery_allows_contactor_closing)
            logging.printf("MEB: Contactors closed\n");
          if (datalayer.battery.status.real_bms_status != BMS_FAULT)
            datalayer.battery.status.real_bms_status = BMS_ACTIVE;
          datalayer.system.status.battery_allows_contactor_closing = true;
          hv_requested = false;
          break;
        case 5:  // Error
          if (datalayer.system.status.battery_allows_contactor_closing)
            logging.printf("MEB: Contactors opened\n");
          datalayer.battery.status.real_bms_status = BMS_FAULT;
          datalayer.system.status.battery_allows_contactor_closing = false;
          hv_requested = false;
          break;
        case 7:  // Init
          if (datalayer.system.status.battery_allows_contactor_closing)
            logging.printf("MEB: Contactors opened\n");
          if (datalayer.battery.status.real_bms_status != BMS_FAULT)
            datalayer.battery.status.real_bms_status = BMS_STANDBY;
          datalayer.system.status.battery_allows_contactor_closing = false;
          hv_requested = false;
          break;
        case 2:  // BALANCING
        default:
          if (datalayer.system.status.battery_allows_contactor_closing)
            logging.printf("MEB: Contactors opened\n");
          if (datalayer.battery.status.real_bms_status != BMS_FAULT)
            datalayer.battery.status.real_bms_status = BMS_STANDBY;
          datalayer.system.status.battery_allows_contactor_closing = false;
      }
      // Pack-internal contactors: BMS_mode 1/3/4/6 are the HV-active states (same set that
      // logs "Contactors closed" above). Guarded so the GPIO state machine wins when enabled.
      if (!contactor_control_enabled) {
        datalayer.system.status.dc_bus_live = (BMS_mode == 1) || (BMS_mode == 3) || (BMS_mode == 4) || (BMS_mode == 6);
      }
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x18) >> 3;
      BMS_error_shutdown = (rx_frame.data.u8[2] & 0x20) >> 5;
      BMS_error_shutdown_request = (rx_frame.data.u8[2] & 0x40) >> 6;
      BMS_fault_performance = (rx_frame.data.u8[2] & 0x80) >> 7;
      BMS_fault_emergency_shutdown_crash = (rx_frame.data.u8[4] & 0x80) >> 7;
      if (BMS_mode != BMS_TARGET_INIT) {  // Init state, values below are invalid
        BMS_current = ((rx_frame.data.u8[4] & 0x7F) << 8) | rx_frame.data.u8[3];
        BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5]));
        BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4));
      }
      break;
    case ISO_Hybrid_01_Resp_FD:  // Diag reply from battery — feed into the ISO-TP state machine.
      // Multi-frame reassembly and flow control are handled by IsoTp; the assembled UDS message
      // is delivered to on_isotp_rx_complete() -> uds_response_handler().
      isotp_receive(rx_frame.data.u8, rx_frame.DLC, ISOTP_TATYPE_PHYSICAL);
      break;
    default:
      break;
  }
  if (can_msg_received == 0xFFFF && nof_cells_determined) {
    if (datalayer.battery.status.real_bms_status == BMS_DISCONNECTED)
      datalayer.battery.status.real_bms_status = BMS_STANDBY;
  }
}

void MebBattery::transmit_can(unsigned long currentMillis) {
  // Drive the ISO-TP timers/retransmissions. Should be called ~every 1 ms.
  IsoTp::isotp_poll();

  // If no UDS response arrived within the timeout window, allow the next request.
  if (uds_request_pending && (currentMillis - uds_request_timestamp > UDS_REQUEST_TIMEOUT_MS)) {
    uds_request_pending = false;
  }

  // Drive the BMS reset state machine. While it holds the bus silent, skip all periodic
  // CAN transmits so the BMS sleeps. ISO-TP polling and the UDS timeout above still run.
  handle_bms_reset(currentMillis);
  if (bms_reset_tx_suppressed) {
    return;
  }

  // DTC readout requested via WebUI: UDS ReadDTCInformation (0x19), report-type 0x02
  // (reportDTCByStatusMask) with status mask 0x09 to read only active/confirmed DTCs.
  if (!uds_request_pending && datalayer_extended.meb.UserRequestDTCreadout) {
    uint8_t payload[3] = {ReadDTCInformation, 0x02, 0x09};
    isotp_send(payload, sizeof(payload));
    uds_request_pending = true;
    uds_request_timestamp = currentMillis;
    datalayer_extended.meb.UserRequestDTCreadout = false;  // consume the request
    datalayer_extended.meb.dtc_read_in_progress = true;
    datalayer_battery->dtc.dtc_read_failed = false;
  }

  // DTC clear requested via WebUI: OBD service 0x04 (ClearDiagnosticInformation) sent to the
  // functional address. Response is handled in uds_response_handler().
  if (!uds_request_pending && datalayer_extended.meb.UserRequestDTCreset) {
    transmit_can_frame(&OBD_CLEAR_DTC);
    uds_request_pending = true;
    uds_request_timestamp = currentMillis;
    datalayer_extended.meb.UserRequestDTCreset = false;  // consume the request
    datalayer_extended.meb.dtc_read_in_progress = true;
    datalayer_battery->dtc.dtc_read_failed = false;
  }

  if (currentMillis - last_can_msg_timestamp > 500) {
    if (first_can_msg_timestamp)
      logging.printf("MEB: No CAN msg received for 500ms\n");
    can_msg_received = RX_DEFAULT;
    first_can_msg_timestamp = 0;
    if (datalayer.battery.status.real_bms_status != BMS_FAULT) {
      datalayer.battery.status.real_bms_status = BMS_DISCONNECTED;
      datalayer.system.status.battery_allows_contactor_closing = false;

      // Set the link voltage back to 0, so that when the BMS comes back, it
      // doesn't immediately skip the precharge.
      BMS_voltage_intermediate = 2000;
      datalayer_extended.meb.BMS_voltage_intermediate_dV = 0;

      // Reset the HV requested state so that we don't skip the precharge.
      hv_requested = false;
    }
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    ESC_51_Auth_frame.data.u8[1] = ((ESC_51_Auth_frame.data.u8[1] & 0xF0) | counter_10ms);
    ESC_51_Auth_frame.data.u8[0] = vw_crc_calc(ESC_51_Auth_frame.data.u8, ESC_51_Auth_frame.DLC, ESC_51_Auth_frame.ID);

    counter_10ms = (counter_10ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&ESC_51_Auth_frame);  // Required for contactor closing
  }
  // Send 20ms CAN Message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;

    ESP_21_frame.data.u8[1] = ((ESP_21_frame.data.u8[1] & 0xF0) | counter_20ms);
    ESP_21_frame.data.u8[0] = vw_crc_calc(ESP_21_frame.data.u8, ESP_21_frame.DLC, ESP_21_frame.ID);

    counter_20ms = (counter_20ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&ESP_21_frame);  // Required for contactor closing
  }
  // Send 40ms CAN Message
  if (currentMillis - previousMillis40ms >= INTERVAL_40_MS) {
    previousMillis40ms = currentMillis;

    /* Handle content for 0x040 message */
    /* Airbag message, needed for BMS to function */
    Airbag_01_frame.data.u8[7] = counter_040;
    Airbag_01_frame.data.u8[1] = ((Airbag_01_frame.data.u8[1] & 0xF0) | counter_40ms);
    Airbag_01_frame.data.u8[0] = vw_crc_calc(Airbag_01_frame.data.u8, Airbag_01_frame.DLC, Airbag_01_frame.ID);
    counter_40ms = (counter_40ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    if (toggle) {
      counter_040 = (counter_040 + 1) % 256;  // Increment only on every other pass
    }
    toggle = !toggle;  // Flip the toggle each time the code block is executed

    transmit_can_frame(&Airbag_01_frame);  // Airbag message - Needed for contactor closing
  }
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {
    previousMillis50ms = currentMillis;

    /* Handle content for 0x0C0 message */
    /* BMS needs to see this EM1 message. Content located in frame5&6 especially (can be static?)*/
    /* Also the voltage seen externally to battery is in frame 7&8. At least for the 62kWh ID3 version does not seem to matter, but we send it anyway. */
    EM1_01_frame.data.u8[1] = ((EM1_01_frame.data.u8[1] & 0xF0) | counter_50ms);
    EM1_01_frame.data.u8[7] = ((datalayer.battery.status.voltage_dV / 10) * 4) & 0x00FF;
    EM1_01_frame.data.u8[8] =
        ((EM1_01_frame.data.u8[8] & 0xF0) | ((((datalayer.battery.status.voltage_dV / 10) * 4) >> 8) & 0x0F));
    EM1_01_frame.data.u8[0] = vw_crc_calc(EM1_01_frame.data.u8, EM1_01_frame.DLC, EM1_01_frame.ID);
    counter_50ms = (counter_50ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&EM1_01_frame);  //  Needed for contactor closing
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    //HV request and DC/DC control lies in 0x503

    if ((!datalayer.system.info.equipment_stop_active) && datalayer.system.status.inverter_allows_contactor_closing &&
        datalayer.battery.status.real_bms_status != BMS_FAULT &&
        (datalayer.battery.status.real_bms_status == BMS_ACTIVE ||
         (datalayer.battery.status.real_bms_status == BMS_STANDBY &&
          (hv_requested ||
           (datalayer.battery.status.voltage_dV > 200 && datalayer_extended.meb.BMS_voltage_intermediate_dV > 0 &&
            labs(((int32_t)datalayer.battery.status.voltage_dV) -
                 ((int32_t)datalayer_extended.meb.BMS_voltage_intermediate_dV)) < 200))))) {
      // We are either:
      //  - Equipment stop is not active, and the inverter allows contactor closing, and the BMS is not in FAULT state, and either:
      //  - in BMS_ACTIVE state (contactors closed, normal operation)
      //  - or in BMS_STANDBY state, ready to request HV from the battery (our precharge is within 20V)
      //  - or in BMS_STANDBY state, having already requested HV (hv_requested = true)

      if (datalayer.battery.status.real_bms_status != BMS_ACTIVE) {
        // We're still awaiting contactor closure, so record that we've
        // requested HV, so that we keep doing so even if the precharge voltage
        // wavers.
        hv_requested = true;
      }

      // We can stop precharging now.
      datalayer.system.info.start_precharging = false;

      if (HVK_01_frame.data.u8[3] == BMS_TARGET_HV_OFF) {
        logging.printf("MEB: Requesting HV\n");
      }
      if ((HVK_01_frame.data.u8[1] & 0x80) !=
          (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00)) {
        if (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING) {
          logging.printf("MEB: Precharge bit set to active\n");
        } else {
          logging.printf("MEB: Precharge bit set to inactive\n");
        }
      }
      HVK_01_frame.data.u8[1] =
          0x30 | (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00);
      HVK_01_frame.data.u8[3] = BMS_TARGET_AC_CHARGING;
      HVK_01_frame.data.u8[5] = 0x82;  // Bordnetz Active
      HVK_01_frame.data.u8[6] = 0xE0;  // Request emergency shutdown HV system == 0, false
    } else if ((first_can_msg_timestamp > 0 && currentMillis - first_can_msg_timestamp > 1000 &&
                BMS_mode != BMS_TARGET_INIT) ||
               datalayer.system.info.equipment_stop_active ||
               !datalayer.system.status.inverter_allows_contactor_closing) {  //FAULT STATE, open contactors

      if (datalayer.system.status.system_status != FAULT && datalayer.battery.status.real_bms_status == BMS_STANDBY &&
          !datalayer.system.info.equipment_stop_active) {
        datalayer.system.info.start_precharging = true;
      }

      if (HVK_01_frame.data.u8[3] != BMS_TARGET_HV_OFF) {
        logging.printf("MEB: Requesting HV_OFF\n");
      }
      if ((HVK_01_frame.data.u8[1] & 0x80) !=
          (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00)) {
        if (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING) {
          logging.printf("MEB: Precharge bit set to active\n");
        } else {
          logging.printf("MEB: Precharge bit set to inactive\n");
        }
      }
      HVK_01_frame.data.u8[1] =
          0x10 | (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00);
      HVK_01_frame.data.u8[3] = BMS_TARGET_HV_OFF;
      HVK_01_frame.data.u8[5] = 0x80;  // Bordnetz Inactive
      HVK_01_frame.data.u8[6] =
          0xE3;  // Request emergency shutdown HV system == init (3) (not sure if we dare activate this, this is done with 0xE1)
    } else {
      HVK_01_frame.data.u8[3] = 0;
      HVK_01_frame.data.u8[5] = 0x80;  // Bordnetz Inactive
    }
    HVK_01_frame.data.u8[1] = ((HVK_01_frame.data.u8[1] & 0xF0) | counter_100ms);
    HVK_01_frame.data.u8[0] = vw_crc_calc(HVK_01_frame.data.u8, HVK_01_frame.DLC, HVK_01_frame.ID);

    //Bidirectional charging message
    HVLM_14_frame.data.u8[1] =
        0x00;  //0x80;  // Bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    HVLM_14_frame.data.u8[2] = 0x00;
    //0x01;  // High load bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    HVLM_14_frame.data.u8[5] = DC_FASTCHARGE_NO_START_REQUEST;  //DC_FASTCHARGE_VEHICLE;  //DC charging

    //Klemmen status
    bool kl15_on = !bms_reset_active && BMS_mode != BMS_TARGET_INIT &&
                   datalayer.battery.status.real_bms_status != BMS_DISCONNECTED;
    Klemmen_Status_01_frame.data.u8[2] = kl15_on ? 0x02 : 0x00;  //bit to signal KL_15 is (0x02 = ON, 0x00 = OFF)
    Klemmen_Status_01_frame.data.u8[1] = ((Klemmen_Status_01_frame.data.u8[1] & 0xF0) | counter_100ms);
    Klemmen_Status_01_frame.data.u8[0] =
        vw_crc_calc(Klemmen_Status_01_frame.data.u8, Klemmen_Status_01_frame.DLC, Klemmen_Status_01_frame.ID);

    Motor_14_frame.data.u8[1] = ((Motor_14_frame.data.u8[1] & 0xF0) | counter_100ms);
    Motor_14_frame.data.u8[0] = vw_crc_calc(Motor_14_frame.data.u8, Motor_14_frame.DLC, Motor_14_frame.ID);

    Motor_54_frame.data.u8[1] = ((Motor_54_frame.data.u8[1] & 0xF0) | counter_100ms);
    Motor_54_frame.data.u8[0] = vw_crc_calc(Motor_54_frame.data.u8, Motor_54_frame.DLC, Motor_54_frame.ID);

    counter_100ms = (counter_100ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can_frame(&HVK_01_frame);
    transmit_can_frame(&HVLM_14_frame);
    transmit_can_frame(&HVLM_13_frame);
    transmit_can_frame(&Klemmen_Status_01_frame);
    transmit_can_frame(&Motor_14_frame);
    transmit_can_frame(&Motor_54_frame);
    transmit_can_frame(&Klima_EV_07_frame);  //PTC / EKK voltage free or not
  }
  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    // MSG_HYB_30_frame does not need CRC even though it has it. Empty in some logs as well.
    transmit_can_frame(&Klima_Sensor_02_frame);
    transmit_can_frame(&MSG_HYB_30_frame);
    transmit_can_frame(&NMH_DCDC_NV_frame);
    transmit_can_frame(&NMH_Klima_frame);
    // based on KL15 state set the network management to bus sleep
    if (Klemmen_Status_01_frame.data.u8[2] & 0x02) {
      NMH_Gateway_frame.data.u64 = 0x00B0F71951045000;
    } else {
      NMH_Gateway_frame.data.u64 = 0x0000020100045000;
    }
    transmit_can_frame(&NMH_Gateway_frame);

    // Snapshot the PID to send, then advance poll_pid to the next in sequence.
    // The actual request byte(s) are built from current_pid by uds_read_data_by_id().
    uint16_t current_pid = poll_pid;
    switch (poll_pid) {
      case PID_SOC:
        poll_pid = PID_VOLTAGE;
        break;
      case PID_VOLTAGE:
        poll_pid = PID_CURRENT;
        break;
      case PID_CURRENT:
        poll_pid = PID_MAX_TEMP;
        break;
      case PID_MAX_TEMP:
        poll_pid = PID_MIN_TEMP;
        break;
      case PID_MIN_TEMP:
        poll_pid = PID_TEMP_POINT_1;
        break;
      case PID_TEMP_POINT_1:
      case PID_TEMP_POINT_2:
      case PID_TEMP_POINT_3:
      case PID_TEMP_POINT_4:
      case PID_TEMP_POINT_5:
      case PID_TEMP_POINT_6:
      case PID_TEMP_POINT_7:
      case PID_TEMP_POINT_8:
      case PID_TEMP_POINT_9:
      case PID_TEMP_POINT_10:
      case PID_TEMP_POINT_11:
      case PID_TEMP_POINT_12:
      case PID_TEMP_POINT_13:
      case PID_TEMP_POINT_14:
      case PID_TEMP_POINT_15:
      case PID_TEMP_POINT_16:
      case PID_TEMP_POINT_17:
        poll_pid = poll_pid + 1;
        break;
      case PID_TEMP_POINT_18:
        poll_pid = PID_MAX_CHARGE_VOLTAGE;
        break;
      case PID_MAX_CHARGE_VOLTAGE:
        poll_pid = PID_MIN_DISCHARGE_VOLTAGE;
        break;
      case PID_MIN_DISCHARGE_VOLTAGE:
        poll_pid = PID_ENERGY_COUNTERS;
        break;
      case PID_ENERGY_COUNTERS:
        poll_pid = PID_ALLOWED_CHARGE_POWER;
        break;
      case PID_ALLOWED_CHARGE_POWER:
        poll_pid = PID_ALLOWED_DISCHARGE_POWER;
        break;
      case PID_ALLOWED_DISCHARGE_POWER:
        poll_pid = PID_SOH;
        break;
      case PID_SOH:
        poll_pid = PID_CELLVOLTAGE_CELL_1;  // Start polling cell voltages
        break;
      // Cell Voltage Cases.
      // Most of these are handled in the default case.
      case PID_CELLVOLTAGE_CELL_1:
        poll_pid = PID_CELLVOLTAGE_CELL_2;
        break;
      case PID_CELLVOLTAGE_CELL_84:
        if (datalayer.battery.info.number_of_cells > 84) {
          if (nof_cells_determined) {
            poll_pid = PID_CELLVOLTAGE_CELL_85;
          } else {
            poll_pid = PID_CELLVOLTAGE_CELL_97;
          }
        } else {
          poll_pid = PID_SOC;
        }
        break;
      case PID_CELLVOLTAGE_CELL_96:
        if (datalayer.battery.info.number_of_cells > 96)
          poll_pid = PID_CELLVOLTAGE_CELL_97;
        else
          poll_pid = PID_SOC;
        break;
      case PID_CELLVOLTAGE_CELL_108:
        poll_pid = PID_SOC;
        break;
      default:
        if (poll_pid >= PID_CELLVOLTAGE_CELL_1 && poll_pid <= PID_CELLVOLTAGE_CELL_108) {
          // The general case for cell voltages (some specific cases handled above)
          poll_pid = poll_pid + 1;
        } else {
          // Out-of-range PID, go back to the start
          poll_pid = PID_SOC;
        }
        break;
    }
    // Send the UDS request only after ≥1 s of CAN activity and when no UDS transaction is
    // pending (ISO-TP is serial — one request/response at a time).
    if (first_can_msg_timestamp > 0 && currentMillis - first_can_msg_timestamp > 1000 && !uds_request_pending) {
      uds_read_data_by_id(current_pid, currentMillis);
    } else {
      // if we could not send the request, don't advance to the next PID.
      poll_pid = current_pid;
    }
  }

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&eTM_01_frame);         //eTM, Cooling valves and pumps for BMS
    transmit_can_frame(&HVEM_04_frame);        // Battery heating requests
    transmit_can_frame(&Klima_EV_06_frame);    //Climate, heatpump and priorities
    transmit_can_frame(&ORU_01_frame);         //ORU, OTA update message for reserving battery
    transmit_can_frame(&Standklima_01_frame);  //Climate, request to BMS for starting preconditioning
  }

  //Send 1s CANFD message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    Motor_Code_01_frame.data.u8[1] = ((Motor_Code_01_frame.data.u8[1] & 0xF0) | counter_1000ms);
    Motor_Code_01_frame.data.u8[0] =
        vw_crc_calc(Motor_Code_01_frame.data.u8, Motor_Code_01_frame.DLC, Motor_Code_01_frame.ID);

    Temperaturen_01_frame.data.u8[2] = 0x7F;  //Outside temperature, factor 0.5, offset -50

    Diagnose_01_frame.data.u8[0] =  //driving cycle counter, 0-254 wrap around. 255 = invalid value
        //Diagnose_01_frame.data.u8[1-2-3b0-4] // Odometer, km (20 bits long)
        Diagnose_01_frame.data.u8[3] = (uint8_t)((TIME_YEAR - 2000) << 4) | Diagnose_01_frame.data.u8[3];
    Diagnose_01_frame.data.u8[4] = (uint8_t)((TIME_DAY & 0x01) << 7 | TIME_MONTH << 3 | (TIME_YEAR - 2000) >> 4);
    Diagnose_01_frame.data.u8[5] = (uint8_t)((TIME_HOUR & 0x0F) << 4 | TIME_DAY >> 1);
    Diagnose_01_frame.data.u8[6] = (uint8_t)((seconds & 0x01) << 7 | TIME_MINUTE << 1 | TIME_HOUR >> 4);
    Diagnose_01_frame.data.u8[7] = (uint8_t)((seconds & 0x3E) >> 1);
    seconds = (seconds + 1) % 60;

    counter_1000ms = (counter_1000ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can_frame(&Diagnose_01_frame);      // Diagnostics - Needed for contactor closing
    transmit_can_frame(&Motor_Code_01_frame);    // Motor - OBD
    transmit_can_frame(&Reichweite_01_frame);    // Loading profile
    transmit_can_frame(&Systeminfo_01_frame);    // Systeminfo
    transmit_can_frame(&Temperaturen_01_frame);  // Temperature QBit
  }

  static auto last_real_bms_status = datalayer.battery.status.real_bms_status;
  static auto last_start_precharging = datalayer.system.info.start_precharging;
  static auto last_hv_requested = hv_requested;
  static auto last_voltage_dV = datalayer.battery.status.voltage_dV;
  static auto last_BMS_voltage_intermediate_dV = datalayer_extended.meb.BMS_voltage_intermediate_dV;
  static auto BMS_mode = datalayer_extended.meb.BMS_mode;

  if (last_real_bms_status != datalayer.battery.status.real_bms_status) {
    logging.printf("MEB: BMS status %d -> %d\n", last_real_bms_status, datalayer.battery.status.real_bms_status);
    last_real_bms_status = datalayer.battery.status.real_bms_status;
  }

  if (last_start_precharging != datalayer.system.info.start_precharging) {
    logging.printf("MEB: Start precharging %d -> %d\n", last_start_precharging,
                   datalayer.system.info.start_precharging);
    last_start_precharging = datalayer.system.info.start_precharging;
  }

  if (last_hv_requested != hv_requested) {
    logging.printf("MEB: HV requested %d -> %d\n", last_hv_requested, hv_requested);
    last_hv_requested = hv_requested;
  }

  if (last_voltage_dV != datalayer.battery.status.voltage_dV) {
    logging.printf("MEB: Voltage dV %d -> %d\n", last_voltage_dV, datalayer.battery.status.voltage_dV);
    last_voltage_dV = datalayer.battery.status.voltage_dV;
  }

  if (last_BMS_voltage_intermediate_dV != datalayer_extended.meb.BMS_voltage_intermediate_dV) {
    logging.printf("MEB: BMS Voltage intermediate dV %d -> %d\n", last_BMS_voltage_intermediate_dV,
                   datalayer_extended.meb.BMS_voltage_intermediate_dV);
    last_BMS_voltage_intermediate_dV = datalayer_extended.meb.BMS_voltage_intermediate_dV;
  }

  if (BMS_mode != datalayer_extended.meb.BMS_mode) {
    logging.printf("MEB: BMS mode %d -> %d\n", BMS_mode, datalayer_extended.meb.BMS_mode);
    BMS_mode = datalayer_extended.meb.BMS_mode;
  }
}

void MebBattery::handle_bms_reset(unsigned long currentMillis) {
  switch (bms_reset_state) {
    case BmsResetState::IDLE:
      // Only start if no diagnostic session is in flight.
      if (datalayer_meb->UserRequestBMSReset && !uds_request_pending) {
        datalayer_meb->UserRequestBMSReset = false;
        // Step 0: reduce power. Zero charge/discharge so the inverter winds current down.
        // Keep CAN sending (pause_CAN = false) so we can still send a graceful HV_OFF.
        setBatteryPause(true, false, UNCHANGED, false);
        bms_reset_state = BmsResetState::WAIT_FOR_PAUSE;
        bms_reset_ms = currentMillis;
        //logging.println("MEB: BMS reset: pausing battery, waiting for current to drop");
      }
      break;

    case BmsResetState::WAIT_FOR_PAUSE:
      // Wait until current has actually dropped before cutting HV (avoids arcing under load).
      if (emulator_pause_status == PAUSED) {
        bms_reset_active = true;  // KL15 OFF + HV_OFF asserted in periodic TX
        bms_reset_state = BmsResetState::REQUEST_HV_OFF;
        bms_reset_ms = currentMillis;
        //logging.println("MEB: BMS reset: current low, requesting HV_OFF / KL15 off");
      } else if (currentMillis - bms_reset_ms > BMS_RESET_PAUSE_TIMEOUT_MS) {
        // Inverter never honoured the 0-power request; abort instead of cutting HV under load.
        setBatteryPause(false, false, UNCHANGED, false);
        bms_reset_state = BmsResetState::IDLE;
        //logging.println("MEB: BMS reset: aborting, battery still under load");
      }
      break;

    case BmsResetState::REQUEST_HV_OFF:
      // Keep transmitting HV_OFF until the BMS reports HV off, or timeout.
      if (BMS_mode == BMS_TARGET_HV_OFF || currentMillis - bms_reset_ms > BMS_RESET_HV_OFF_TIMEOUT_MS) {
        bms_reset_tx_suppressed = true;  // go silent so the BMS sleeps
        bms_reset_state = BmsResetState::SILENCE;
        bms_reset_ms = currentMillis;
        //logging.println("MEB: BMS reset: bus silent, waiting for BMS to sleep");
      }
      break;

    case BmsResetState::SILENCE:
      // BMS considered asleep once it stops sending for BMS_RESET_BMS_SILENT_MS.
      if (currentMillis - last_can_msg_timestamp > BMS_RESET_BMS_SILENT_MS ||
          currentMillis - bms_reset_ms > BMS_RESET_SILENCE_TIMEOUT_MS) {
        bms_reset_state = BmsResetState::SLEEP_WAIT;
        bms_reset_ms = currentMillis;
        //logging.println("MEB: BMS reset: BMS asleep, holding bus quiet");
      }
      break;

    case BmsResetState::SLEEP_WAIT:
      if (currentMillis - bms_reset_ms >= BMS_RESET_SLEEP_MS) {
        // Restart CAN from a clean state so precharge/init runs again.
        bms_reset_tx_suppressed = false;
        bms_reset_active = false;
        first_can_msg_timestamp = 0;
        last_can_msg_timestamp = currentMillis;  // avoid instant "disconnected"
        can_msg_received = RX_DEFAULT;
        BMS_voltage_intermediate = 2000;
        datalayer_meb->BMS_voltage_intermediate_dV = 0;
        hv_requested = false;
        // Clearing the fault is the whole point of the reset: BMS_FAULT is latched (every BMS_20
        // branch is guarded with "!= BMS_FAULT"), so drop it here and let the live BMS_mode
        // repopulate real_bms_status once messages resume.
        datalayer.battery.status.real_bms_status = BMS_DISCONNECTED;
        // Step 6: resume charge/discharge.
        setBatteryPause(false, false, UNCHANGED, false);
        // The BMS is still waking on the bus, so the first frames won't be ACKed. Ignore
        // the battery interface's buffer-full / bus-error events for a while so those
        // transient wakeup errors never reach the event log.
        ignore_can_errors_for(can_interface, BMS_CAN_ERR_IGNORE_MS);
        bms_reset_state = BmsResetState::IDLE;
        //logging.println("MEB: BMS reset: resuming CAN communication");
      }
      break;
  }
}

void MebBattery::uds_read_data_by_id(uint16_t did, unsigned long currentMillis) {
  // Send a UDS ReadDataByIdentifier (0x22) request for the given DID via ISO-TP.
  uint8_t payload[3] = {ReadDataByIdentifier, (uint8_t)(did >> 8), (uint8_t)(did & 0xFF)};
  isotp_send(payload, sizeof(payload));
  uds_request_pending = true;
  uds_request_timestamp = currentMillis;
}

void MebBattery::on_isotp_can_tx(uint32_t can_id, uint8_t* can_data, uint8_t can_dlc) {
  // Called by the ISO-TP layer to emit a raw CAN-FD frame.
  CAN_frame frame;
  frame.FD = true;
  frame.ext_ID = true;
  frame.ID = can_id;
  frame.DLC = can_dlc;
  memcpy(frame.data.u8, can_data, can_dlc);
  transmit_can_frame(&frame);
}

void MebBattery::on_isotp_rx_complete(uint8_t* data, int len, isotp_tatype tatype) {
  // A complete UDS message has been assembled by the ISO-TP layer.
  uds_response_handler(data, len, tatype);
}

void MebBattery::uds_response_handler(uint8_t* data, int len, enum isotp_tatype type) {
  if (len < 1)
    return;
  uint8_t response_service_id = data[0];
  switch (response_service_id) {
    case (UDS_RESPONSE_SID_OF(ReadDataByIdentifier)):  // Read Data by Identifier positive response
      uds_request_pending = false;
      // After ISO-TP assembly the PCI bytes are stripped. Layout:
      //   data[0] = 0x62, data[1] = PID high, data[2] = PID low, data[3..] = value bytes
      if (len < 3)
        break;
      pid_reply = ((uint32_t)data[1] << 8) | data[2];
      switch (pid_reply) {
        case PID_SOC:
          if (len < 4)
            break;
          battery_soc_polled = data[3] * 4;  // 135*4 = 54.0%
          break;
        case PID_SOH:
          if (len < 5)
            break;
          battery_soh_polled = ((data[3] << 8) | data[4]);
          break;
        case PID_VOLTAGE:
          if (len < 5)
            break;
          battery_voltage_polled = ((data[3] << 8) | data[4]);
          break;
        case PID_CURRENT:  // IDLE 0A: 00 08 62 1E 3D 00 02 49 F0 39 AA AA
          if (len < 5)
            break;
          battery_current_polled = ((data[3] << 8) | data[4]);
          break;
        case PID_MAX_TEMP:
          if (len < 5)
            break;
          battery_max_temp = ((data[3] << 8) | data[4]);
          break;
        case PID_MIN_TEMP:
          if (len < 5)
            break;
          battery_min_temp = ((data[3] << 8) | data[4]);
          break;
        // Note: PID_TEMP_POINT_1 to PID_TEMP_POINT_18 are handled in the default case.
        case PID_MAX_CHARGE_VOLTAGE:
          if (len < 5)
            break;
          battery_max_charge_voltage = ((data[3] << 8) | data[4]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          if (len < 5)
            break;
          battery_min_discharge_voltage = ((data[3] << 8) | data[4]);
          break;
        case PID_ENERGY_COUNTERS:
          if (len < 19)
            break;
          // int32_t ah_discharge = ((data[3] << 24) | (data[4] << 16) | (data[5] << 8) | data[6]);
          // int32_t ah_charge    = ((data[7] << 24) | (data[8] << 16) | (data[9] << 8) | data[10]);
          kwh_charge = ((data[11] << 24) | (data[12] << 16) | (data[13] << 8) | data[14]);
          kwh_discharge = ((data[15] << 24) | (data[16] << 16) | (data[17] << 8) | data[18]);
          datalayer.battery.status.total_discharged_battery_Wh = kwh_discharge * 0.11650853;
          datalayer.battery.status.total_charged_battery_Wh = kwh_charge * 0.11650853;
          break;
        case PID_ALLOWED_CHARGE_POWER:
          if (len < 5)
            break;
          battery_allowed_charge_power = ((data[3] << 8) | data[4]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          if (len < 5)
            break;
          battery_allowed_discharge_power = ((data[3] << 8) | data[4]);
          break;
        // Note: most PID_CELLVOLTAGE_CELL_* responses are handled in the default case.
        // Certain specific cases are handled here as they are used to establish the number of cells.
        case PID_CELLVOLTAGE_CELL_85:
          if (len < 5)
            break;
          tempval = ((data[3] << 8) | data[4]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[84] = (tempval + 1000);
          } else {  // Cell 85 unavailable. We have a 84S battery (48kWh)
            datalayer.battery.info.number_of_cells = 84;
            nof_cells_determined = true;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_84S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;
          }
          break;
        case PID_CELLVOLTAGE_CELL_97:
          if (len < 5)
            break;
          tempval = ((data[3] << 8) | data[4]);
          if (tempval != 0xFFE) {
            cellvoltages_polled[96] = (tempval + 1000);
          } else {  // Cell 97 unavailable. We have a 96S battery (55kWh) (Unless already specified as 84S)
            if (datalayer.battery.info.number_of_cells == 84) {
              // Do nothing, we already identified it as 84S
            } else {
              datalayer.battery.info.number_of_cells = 96;
              nof_cells_determined = true;
              datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
              datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
            }
          }
          break;
        case PID_CELLVOLTAGE_CELL_108:
          if (len < 5)
            break;
          tempval = ((data[3] << 8) | data[4]);
          nof_cells_determined = true;  // This is placed outside of the if, to make
          // sure we only take the shortcuts to determine the number of cells once.
          if (tempval != 0xFFE) {
            cellvoltages_polled[107] = (tempval + 1000);
            datalayer.battery.info.number_of_cells = 108;
            datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
            datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
          }
          break;
        default:
          if (len < 5)
            break;
          if (pid_reply >= PID_TEMP_POINT_1 && pid_reply <= PID_TEMP_POINT_18) {
            datalayer_extended.meb.temp_points[pid_reply - PID_TEMP_POINT_1] = (((data[3] << 8) | data[4]) / 8.f) - 40;
          } else if (pid_reply >= PID_CELLVOLTAGE_CELL_1 && pid_reply <= PID_CELLVOLTAGE_CELL_108) {
            // The general case for cell voltages (some specific cases handled above)
            tempval = ((data[3] << 8) | data[4]);
            if (tempval != 0xFFE) {
              cellvoltages_polled[pid_reply - PID_CELLVOLTAGE_CELL_1] = (tempval + 1000);
            }
          }
          break;
      }
      break;
    case (0x04 + kPositiveResponseOffset):  // clear DTCs (OBD service 0x04) positive response
      uds_request_pending = false;
      datalayer_battery->dtc.dtc_read_failed = false;
      datalayer_battery->dtc.dtc_count = 0;  // Clear any existing DTCs after a successful erase
      datalayer_battery->dtc.dtc_last_read_millis = 0;
      datalayer_extended.meb.dtc_read_in_progress = false;
      break;
    case (UDS_RESPONSE_SID_OF(ReadDTCInformation)):  // DTC read positive response (0x59)
      if (data[1] != 0x02) {
        // Unexpected report type — treat as a failed readout.
        datalayer_battery->dtc.dtc_read_failed = true;
      } else {
        datalayer_battery->dtc.dtc_read_failed = false;
        int dtcStartIndex = 3;  // Skip 59 02 <statusAvailabilityMask>
        int availableBytes = len - dtcStartIndex;
        int maxDtcCount = availableBytes / 4;

        if (maxDtcCount > MAX_DTC_COUNT) {
          maxDtcCount = MAX_DTC_COUNT;
          logging.println("DTC count exceeds buffer, truncating");
        }
        if (maxDtcCount < 0)
          maxDtcCount = 0;

        for (int i = 0; i < maxDtcCount; i++) {
          int offset = dtcStartIndex + (i * 4);
          // Bounds check to ensure we don't read beyond the buffer
          if (offset + 3 > len)
            break;
          // Combine 3 bytes into a single uint32
          uint32_t dtcCode =
              ((uint32_t)data[offset] << 16) | ((uint32_t)data[offset + 1] << 8) | (uint32_t)data[offset + 2];
          uint8_t dtcStatus = data[offset + 3];
          datalayer_battery->dtc.dtc_codes[i] = dtcCode;
          datalayer_battery->dtc.dtc_status[i] = dtcStatus;
        }
        datalayer_battery->dtc.dtc_count = maxDtcCount;
      }
      uds_request_pending = false;
      datalayer_battery->dtc.dtc_last_read_millis = millis();
      datalayer_extended.meb.dtc_read_in_progress = false;
      break;
    case (ServiceNotSupportedInActiveSession):  // Negative response (0x7F)
      // data[1] = original request service id, data[2] = NRC
      if (len >= 3 && data[2] == RequestCorrectlyReceived_ResponsePending) {
        // NRC 0x78: requestCorrectlyReceived-ResponsePending — the BMS is still processing.
        // Reset the timeout so we keep waiting instead of retrying.
        uds_request_timestamp = millis();
      } else if (len >= 3 && data[1] == ReadDTCInformation) {
        // DTC read was rejected — the transaction is complete, allow the next request.
        uds_request_pending = false;
        datalayer_extended.meb.dtc_read_in_progress = false;
        datalayer_battery->dtc.dtc_read_failed = true;
      } else {
        // Any other NRC: the transaction is complete (rejected), allow the next request.
        uds_request_pending = false;
      }
      break;
    default:
      break;
  }
}

void MebBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 108;  //Startup in 108S mode. We figure out the actual count later.
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Defined later to correct pack size
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_84S_DV;   //Defined later to correct pack size
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  memset(cellvoltages_polled, 0, sizeof(cellvoltages_polled));

  // ISO-TP transport for UDS diagnostics (PID polling, DTC read). Replies arrive on
  // ISO_Hybrid_01_Resp_FD and are fed to isotp_receive() in handle_incoming_can_frame().
  isotp_init(ISO_Hybrid_01_Req_FD);

  // The BMS may still be asleep at power-on, so our first frames won't be ACKed. Ignore
  // this interface's transient CAN errors for a while so they don't clutter the event log.
  ignore_can_errors_for(can_interface, BMS_CAN_ERR_IGNORE_MS);
}
