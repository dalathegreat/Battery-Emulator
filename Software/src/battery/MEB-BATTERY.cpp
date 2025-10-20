#include "MEB-BATTERY.h"
#include <Arduino.h>
#include <algorithm>  // For std::min and std::max
#include <cstring>    //For unit test
#include "../communication/can/comm_can.h"
#include "../communication/can/obd.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/*
TODO list
- Check all TODO:s in the code
- Investigate why opening and then closing contactors from webpage does not always work
- remaining_capacity_Wh is based on a lower limit of 5% soc. This means that at 5% soc, remaining_capacity_Wh returns 0.
*/

const int RX_0x17F0007B = 0x0001;
const int RX_0x12DD54D0 = 0x0002;
const int RX_0x12DD54D1 = 0x0004;
const int RX_0x12DD54D2 = 0x0008;
const int RX_0x1A555550 = 0x0010;
const int RX_0x1A555551 = 0x0020;
const int RX_0x1A5555B2 = 0x0040;
const int RX_0x16A954A6 = 0x0080;
const int RX_0x1A5555B0 = 0x0100;
const int RX_0x1A5555B1 = 0x0200;
const int RX_0x5A2 = 0x0400;
const int RX_0x5CA = 0x0800;
const int RX_0x0CF = 0x1000;
const int RX_DEFAULT = 0xE000;

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
uint8_t vw_crc_calc(uint8_t* inputBytes, uint8_t length, uint32_t address) {

  const uint8_t poly = 0x2F;
  const uint8_t xor_output = 0xFF;
  // VAG Magic Bytes
  const uint8_t MB0040[16] = {0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
                              0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40};
  const uint8_t MB00C0[16] = {0x2f, 0x44, 0x72, 0xd3, 0x07, 0xf2, 0x39, 0x09,
                              0x8d, 0x6f, 0x57, 0x20, 0x37, 0xf9, 0x9b, 0xfa};
  const uint8_t MB00CF[16] = {0xee, 0x80, 0x6e, 0x4e, 0x29, 0xc6, 0x92, 0xc0,
                              0x65, 0xaa, 0x3a, 0xa1, 0x8f, 0xcd, 0xe6, 0x90};
  const uint8_t MB00FC[16] = {0x77, 0x5c, 0xa0, 0x89, 0x4b, 0x7c, 0xbb, 0xd6,
                              0x1f, 0x6c, 0x4f, 0xf6, 0x20, 0x2b, 0x43, 0xdd};
  const uint8_t MB00FD[16] = {0xb4, 0xef, 0xf8, 0x49, 0x1e, 0xe5, 0xc2, 0xc0,
                              0x97, 0x19, 0x3c, 0xc9, 0xf1, 0x98, 0xd6, 0x61};
  const uint8_t MB0097[16] = {0x3C, 0x54, 0xCF, 0xA3, 0x81, 0x93, 0x0B, 0xC7,
                              0x3E, 0xDF, 0x1C, 0xB0, 0xA7, 0x25, 0xD3, 0xD8};
  const uint8_t MB00F7[16] = {0x5F, 0xA0, 0x44, 0xD0, 0x63, 0x59, 0x5B, 0xA2,
                              0x68, 0x04, 0x90, 0x87, 0x52, 0x12, 0xB4, 0x9E};
  const uint8_t MB0124[16] = {0x12, 0x7E, 0x34, 0x16, 0x25, 0x8F, 0x8E, 0x35,
                              0xBA, 0x7F, 0xEA, 0x59, 0x4C, 0xF0, 0x88, 0x15};
  const uint8_t MB0153[16] = {0x03, 0x13, 0x23, 0x7a, 0x40, 0x51, 0x68, 0xba,
                              0xa8, 0xbe, 0x55, 0x02, 0x11, 0x31, 0x76, 0xec};
  const uint8_t MB014C[16] = {0x16, 0x35, 0x59, 0x15, 0x9a, 0x2a, 0x97, 0xb8,
                              0x0e, 0x4e, 0x30, 0xcc, 0xb3, 0x07, 0x01, 0xad};
  const uint8_t MB0187[16] = {0x7F, 0xED, 0x17, 0xC2, 0x7C, 0xEB, 0x44, 0x21,
                              0x01, 0xFA, 0xDB, 0x15, 0x4A, 0x6B, 0x23, 0x05};
  const uint8_t MB03A6[16] = {0xB6, 0x1C, 0xC1, 0x23, 0x6D, 0x8B, 0x0C, 0x51,
                              0x38, 0x32, 0x24, 0xA8, 0x3F, 0x3A, 0xA4, 0x02};
  const uint8_t MB03AF[16] = {0x94, 0x6A, 0xB5, 0x38, 0x8A, 0xB4, 0xAB, 0x27,
                              0xCB, 0x22, 0x88, 0xEF, 0xA3, 0xE1, 0xD0, 0xBB};
  const uint8_t MB03BE[16] = {0x1f, 0x28, 0xc6, 0x85, 0xe6, 0xf8, 0xb0, 0x19,
                              0x5b, 0x64, 0x35, 0x21, 0xe4, 0xf7, 0x9c, 0x24};
  const uint8_t MB03C0[16] = {0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3,
                              0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3, 0xc3};
  const uint8_t MB0503[16] = {0xed, 0xd6, 0x96, 0x63, 0xa5, 0x12, 0xd5, 0x9a,
                              0x1e, 0x0d, 0x24, 0xcd, 0x8c, 0xa6, 0x2f, 0x41};
  const uint8_t MB0578[16] = {0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48,
                              0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48, 0x48};
  const uint8_t MB05A2[16] = {0xeb, 0x4c, 0x44, 0xaf, 0x21, 0x8d, 0x01, 0x58,
                              0xfa, 0x93, 0xdb, 0x89, 0x15, 0x10, 0x4a, 0x61};
  const uint8_t MB05CA[16] = {0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43,
                              0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43, 0x43};
  const uint8_t MB0641[16] = {0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47,
                              0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47, 0x47};
  const uint8_t MB06A3[16] = {0xC1, 0x8B, 0x38, 0xA8, 0xA4, 0x27, 0xEB, 0xC8,
                              0xEF, 0x05, 0x9A, 0xBB, 0x39, 0xF7, 0x80, 0xA7};
  const uint8_t MB06A4[16] = {0xC7, 0xD8, 0xF1, 0xC4, 0xE3, 0x5E, 0x9A, 0xE2,
                              0xA1, 0xCB, 0x02, 0x4F, 0x57, 0x4E, 0x8E, 0xE4};
  const uint8_t MB16A954A6[16] = {0x79, 0xB9, 0x67, 0xAD, 0xD5, 0xF7, 0x70, 0xAA,
                                  0x44, 0x61, 0x5A, 0xDC, 0x26, 0xB4, 0xD2, 0xC3};

  uint8_t crc = 0xFF;
  uint8_t magicByte = 0x00;
  uint8_t counter = inputBytes[1] & 0x0F;  // only the low byte of the couner is relevant

  switch (address) {
    case 0x0040:  // Airbag
      magicByte = MB0040[counter];
      break;
    case 0x00C0:  //
      magicByte = MB00C0[counter];
      break;
    case 0x00CF:  //BMS
      magicByte = MB00CF[counter];
      break;
    case 0x00FC:
      magicByte = MB00FC[counter];
      break;
    case 0x00FD:
      magicByte = MB00FD[counter];
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
    case 0x014C:  // Motor
      magicByte = MB014C[counter];
      break;
    case 0x0153:  // HYB30
      magicByte = MB0153[counter];
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
    case 0x03BE:  // Motor
      magicByte = MB03BE[counter];
      break;
    case 0x03C0:  // Klemmen status
      magicByte = MB03C0[counter];
      break;
    case 0x0503:  // HVK
      magicByte = MB0503[counter];
      break;
    case 0x0578:  // BMS DC
      magicByte = MB0578[counter];
      break;
    case 0x05A2:  // BMS
      magicByte = MB05A2[counter];
      break;
    case 0x05CA:  // BMS
      magicByte = MB05CA[counter];
      break;
    case 0x0641:  // Motor
      magicByte = MB0641[counter];
      break;
    case 0x06A3:  // ??
      magicByte = MB06A3[counter];
      break;
    case 0x06A4:  // ??
      magicByte = MB06A4[counter];
      break;
    case 0x16A954A6:
      magicByte = MB16A954A6[counter];
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

  datalayer_battery->status.voltage_dV = BMS_voltage * 2.5;  // *0.25*10

  datalayer_battery->status.current_dA = (BMS_current - 16300);  // 0.1 * 10

  if (nof_cells_determined) {
    datalayer_battery->info.total_capacity_Wh =
        ((float)datalayer_battery->info.number_of_cells) * 3.67 * ((float)BMS_capacity_ah) * 0.2 * 1.02564;
    // The factor 1.02564 = 1/0.975 is to correct for bottom 2.5% which is reported by the remaining_capacity_Wh,
    // but which is not actually usable, but if we do not include it, the remaining_capacity_Wh can be larger than
    // the total_capacity_Wh.
    // 0.935 and 0.9025 are the different conversions for different battery sizes to go from design capacity to
    // total_capacity_Wh calculated above.

    int Wh_max = 61832 * 0.935;  // 108 cells
    if (datalayer_battery->info.number_of_cells <= 84)
      Wh_max = 48091 * 0.9025;
    else if (datalayer_battery->info.number_of_cells <= 96)
      Wh_max = 82442 * 0.9025;
    if (BMS_capacity_ah > 0)
      datalayer_battery->status.soh_pptt = 10000 * datalayer_battery->info.total_capacity_Wh / (Wh_max * 1.02564);
  }

  datalayer_battery->status.remaining_capacity_Wh = usable_energy_amount_Wh * 5;

  datalayer_battery->status.max_charge_power_W = (max_charge_power_watt * 100);

  datalayer_battery->status.max_discharge_power_W = (max_discharge_power_watt * 100);

  //Power in watts, Negative = charging batt
  datalayer_battery->status.active_power_W =
      ((datalayer_battery->status.voltage_dV * datalayer_battery->status.current_dA) / 100);

  // datalayer.battery.status.temperature_min_dC = actual_temperature_lowest_C*5 -400;  // We use the value below, because it has better accuracy
  datalayer_battery->status.temperature_min_dC = (battery_min_temp * 10) / 64;

  // datalayer.battery.status.temperature_max_dC = actual_temperature_highest_C*5 -400;  // We use the value below, because it has better accuracy
  datalayer_battery->status.temperature_max_dC = (battery_max_temp * 10) / 64;

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
  datalayer_extended.meb.status_HV_line = status_HV_line;
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
  if (balancing_active == 1 && datalayer_extended.meb.balancing_active != 1)
    set_event_latched(EVENT_BALANCING_START, 0);
  if (balancing_active == 2 && datalayer_extended.meb.balancing_active == 1)
    set_event(EVENT_BALANCING_END, 0);
  datalayer_extended.meb.balancing_active = balancing_active;
  datalayer_extended.meb.balancing_request = balancing_request;
  datalayer_extended.meb.charging_active = charging_active;
}

void MebBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  last_can_msg_timestamp = millis();
  if (first_can_msg == 0) {
    logging.printf("MEB: First CAN msg received\n");
    first_can_msg = last_can_msg_timestamp;
  }

  /* CRC check on messages with CRC */
  switch (rx_frame.ID) {
    case 0x0CF:
    case 0x578:
    case 0x5A2:
    case 0x5CA:
    case 0x16A954A6:
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
    case 0x17F0007B:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x17F0007B;
      component_protection_active = (rx_frame.data.u8[0] & 0x01);
      shutdown_active = ((rx_frame.data.u8[0] & 0x02) >> 1);
      transportation_mode_active = ((rx_frame.data.u8[0] & 0x02) >> 1);
      KL15_mode = ((rx_frame.data.u8[0] & 0xF0) >> 4);
      //0 = communication only when terminal 15 = ON (no run-on, cannot be woken up)
      //1 = communication after terminal 15 = OFF (run-on, cannot be woken up)
      //2 = communication when terminal 15 = OFF (run-on, can be woken up)
      bus_knockout_timer = rx_frame.data.u8[5];
      hybrid_wakeup_reason = rx_frame.data.u8[6];  //(if several active, lowest wins)
      //0 = wakeup cause not known 1 = Bus wakeup2 = KL15 HW 3 = TPA active
      break;
    case 0x17FE007B:  // BMS - Offboard tester diag response
      break;
    case 0x1B00007B:  // BMS - 200ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      wakeup_type =
          ((rx_frame.data.u8[1] & 0x10) >> 4);  //0 passive, SG has not woken up, 1 active, SG has woken up the network
      instrumentation_cluster_request = ((rx_frame.data.u8[1] & 0x40) >> 6);  //True/false
      break;
    case 0x12DD54D0:  // BMS Limits 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x12DD54D0;
      max_discharge_power_watt =
          ((rx_frame.data.u8[6] & 0x07) << 10) | (rx_frame.data.u8[5] << 2) | (rx_frame.data.u8[4] & 0xC0) >> 6;  //*100
      max_discharge_current_amp =
          ((rx_frame.data.u8[3] & 0x01) << 12) | (rx_frame.data.u8[2] << 4) | (rx_frame.data.u8[1] >> 4);  //*0.2
      max_charge_power_watt = (rx_frame.data.u8[7] << 5) | (rx_frame.data.u8[6] >> 3);                     //*100
      max_charge_current_amp = ((rx_frame.data.u8[4] & 0x3F) << 7) | (rx_frame.data.u8[3] >> 1);           //*0.2
      break;
    case 0x12DD54D1:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x12DD54D1;
      if (rx_frame.data.u8[6] != 0xFE || rx_frame.data.u8[7] != 0xFF) {  // Init state, values below invalid
        battery_SOC = ((rx_frame.data.u8[3] & 0x0F) << 7) | (rx_frame.data.u8[2] >> 1);               //*0.05
        usable_energy_amount_Wh = (rx_frame.data.u8[7] << 8) | rx_frame.data.u8[6];                   //*5
        power_discharge_percentage = ((rx_frame.data.u8[4] & 0x3F) << 4) | rx_frame.data.u8[3] >> 4;  //*0.2
        power_charge_percentage = (rx_frame.data.u8[5] << 2) | rx_frame.data.u8[4] >> 6;              //*0.2
      }
      status_HV_line = ((rx_frame.data.u8[2] & 0x01) << 1) | rx_frame.data.u8[1] >> 7;
      warning_support = (rx_frame.data.u8[1] & 0x70) >> 4;
      break;
    case 0x12DD54D2:  // BMS 100ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x12DD54D2;
      battery_heating_active = (rx_frame.data.u8[4] & 0x40) >> 6;
      heating_request = (rx_frame.data.u8[5] & 0xE0) >> 5;
      cooling_request = (rx_frame.data.u8[5] & 0x1C) >> 2;
      power_battery_heating_watt = rx_frame.data.u8[6];
      power_battery_heating_req_watt = rx_frame.data.u8[7];
      break;
    case 0x1A555550:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x1A555550;
      balancing_active = (rx_frame.data.u8[1] & 0xC0) >> 6;
      charging_active = (rx_frame.data.u8[2] & 0x01);
      max_energy_Wh = ((rx_frame.data.u8[6] & 0x1F) << 8) | rx_frame.data.u8[5];                     //*40
      max_charge_percent = ((rx_frame.data.u8[7] << 3) | rx_frame.data.u8[6] >> 5);                  //*0.05
      min_charge_percent = ((rx_frame.data.u8[4] << 3) | rx_frame.data.u8[3] >> 5);                  //*0.05
      isolation_resistance_kOhm = (((rx_frame.data.u8[3] & 0x1F) << 7) | rx_frame.data.u8[2] >> 1);  //*5
      break;
    case 0x1A555551:  // BMS 500ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x1A555551;
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
    case 0x1A5555B2:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x1A5555B2;
      performance_index_discharge_peak_temperature_percentage =
          (((rx_frame.data.u8[3] & 0x07) << 6) | rx_frame.data.u8[2] >> 2);  //*0.2
      performance_index_charge_peak_temperature_percentage =
          (((rx_frame.data.u8[4] & 0x3F) << 3) | rx_frame.data.u8[3] >> 5);  //*0.2
      temperature_status_discharge = (rx_frame.data.u8[1] & 0x70) >> 4;
      temperature_status_charge = (((rx_frame.data.u8[2] & 0x03) << 1) | rx_frame.data.u8[1] >> 7);
      break;
    case 0x16A954A6:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      can_msg_received |= RX_0x16A954A6;
      BMS_16A954A6_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
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
    case 0x16A954F8:  // BMS
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      predicted_power_dyn_standard_watt = ((rx_frame.data.u8[6] << 1) | rx_frame.data.u8[5] >> 7);  //*50
      predicted_time_dyn_standard_minutes = rx_frame.data.u8[7];
      break;
    case 0x16A954E8:  // BMS Temperature and cellvoltages - 180ms
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;
      mux = (rx_frame.data.u8[0] & 0x0F);
      switch (mux) {
        case 0:  // Temperatures 1-56. Value is 0xFD if sensor not present
          for (uint8_t i = 0; i < 56; i++) {
            datalayer_extended.meb.celltemperature_dC[i] = (rx_frame.data.u8[i + 1] * 5) - 400;
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
    case 0x1A5555B0:  // BMS 1000ms cyclic
      can_msg_received |= RX_0x1A5555B0;
      duration_discharge_power_watt = ((rx_frame.data.u8[6] & 0x0F) << 8) | rx_frame.data.u8[5];
      duration_charge_power_watt = (rx_frame.data.u8[7] << 4) | rx_frame.data.u8[6] >> 4;
      maximum_voltage = ((rx_frame.data.u8[3] & 0x3F) << 4) | rx_frame.data.u8[2] >> 4;
      minimum_voltage = (rx_frame.data.u8[4] << 2) | rx_frame.data.u8[3] >> 6;
      break;
    case 0x1A5555B1:  // BMS 1000ms cyclic
      can_msg_received |= RX_0x1A5555B1;
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
    case 0x2AF:  // BMS 50ms
      actual_battery_voltage =
          ((rx_frame.data.u8[1] & 0x3F) << 8) | rx_frame.data.u8[0];  //*0.0625 // Seems to be 0.125 in logging
      regen_battery = ((rx_frame.data.u8[5] & 0x7F) << 8) | rx_frame.data.u8[4];
      energy_extracted_from_battery = ((rx_frame.data.u8[7] & 0x7F) << 8) | rx_frame.data.u8[6];
      break;
    case 0x578:                                        // BMS 100ms
      BMS_578_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
      BMS_Status_DCLS = ((rx_frame.data.u8[1] & 0x30) >> 4);
      DC_voltage_DCLS = (rx_frame.data.u8[2] << 6) | (rx_frame.data.u8[1] >> 6);
      max_fastcharging_current_amp = ((rx_frame.data.u8[4] & 0x01) << 8) | rx_frame.data.u8[3];
      DC_voltage_chargeport = (rx_frame.data.u8[7] << 4) | (rx_frame.data.u8[6] >> 4);
      break;
    case 0x5A2:  // BMS 500ms normal, 100ms fast
      can_msg_received |= RX_0x5A2;
      BMS_5A2_counter = (rx_frame.data.u8[1] & 0x0F);  // Can be used to check CAN signal integrity later on
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
    case 0x5CA:  // BMS 500ms
      can_msg_received |= RX_0x5CA;
      BMS_5CA_counter = (rx_frame.data.u8[1] & 0x0F);
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
    case 0x0CF:  //BMS 10ms
      can_msg_received |= RX_0x0CF;
      BMS_0CF_counter = (rx_frame.data.u8[1] & 0x0F);
      BMS_welded_contactors_status = (rx_frame.data.u8[1] & 0x60) >> 5;
      BMS_ext_limits_active = (rx_frame.data.u8[1] & 0x80) >> 7;
      BMS_mode = (rx_frame.data.u8[2] & 0x07);
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
      BMS_HVIL_status = (rx_frame.data.u8[2] & 0x18) >> 3;
      BMS_error_shutdown = (rx_frame.data.u8[2] & 0x20) >> 5;
      BMS_error_shutdown_request = (rx_frame.data.u8[2] & 0x40) >> 6;
      BMS_fault_performance = (rx_frame.data.u8[2] & 0x80) >> 7;
      BMS_fault_emergency_shutdown_crash = (rx_frame.data.u8[4] & 0x80) >> 7;
      if (BMS_mode != 7) {  // Init state, values below are invalid
        BMS_current = ((rx_frame.data.u8[4] & 0x7F) << 8) | rx_frame.data.u8[3];
        BMS_voltage_intermediate = (((rx_frame.data.u8[6] & 0x0F) << 8) + (rx_frame.data.u8[5]));
        BMS_voltage = ((rx_frame.data.u8[7] << 4) + ((rx_frame.data.u8[6] & 0xF0) >> 4));
      }
      break;
    case 0x1C42007B:                      // Reply from battery
      if (rx_frame.data.u8[0] == 0x10) {  //PID header
        transmit_can_frame(&MEB_ACK_FRAME);
      }
      if (rx_frame.DLC == 8) {
        pid_reply = (rx_frame.data.u8[2] << 8) + rx_frame.data.u8[3];
      } else {  //12 or 24bit message has reply in other location
        pid_reply = (rx_frame.data.u8[3] << 8) + rx_frame.data.u8[4];
      }

      switch (pid_reply) {
        case PID_SOC:
          battery_soc_polled = rx_frame.data.u8[4] * 4;  // 135*4 = 54.0%
          break;
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
        // Note: PID_TEMP_POINT_1 to PID_TEMP_POINT_18 are handled in the default case.
        case PID_MAX_CHARGE_VOLTAGE:
          battery_max_charge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_MIN_DISCHARGE_VOLTAGE:
          battery_min_discharge_voltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ENERGY_COUNTERS:
          // int32_t ah_discharge = ((rx_frame.data.u8[5] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[7] << 8) |rx_frame.data.u8[8]);
          // int32_t ah_charge = ((rx_frame.data.u8[9] << 24) | (rx_frame.data.u8[10] << 16) | (rx_frame.data.u8[11] << 8) |rx_frame.data.u8[12]);
          kwh_charge = ((rx_frame.data.u8[13] << 24) | (rx_frame.data.u8[14] << 16) | (rx_frame.data.u8[15] << 8) |
                        rx_frame.data.u8[16]);
          kwh_discharge = ((rx_frame.data.u8[17] << 24) | (rx_frame.data.u8[18] << 16) | (rx_frame.data.u8[19] << 8) |
                           rx_frame.data.u8[20]);
          // logging.printf("ah_dis:%.3f ah_ch:%.3f kwh_dis:%.3f kwh_ch:%.3f\n", ah_discharge*0.00182044545, ah_charge*0.00182044545,
          // kwh_discharge*0.00011650853, kwh_charge*0.00011650853);
          datalayer.battery.status.total_discharged_battery_Wh = kwh_discharge * 0.11650853;
          datalayer.battery.status.total_charged_battery_Wh = kwh_charge * 0.11650853;
          break;
        case PID_ALLOWED_CHARGE_POWER:
          battery_allowed_charge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_ALLOWED_DISCHARGE_POWER:
          battery_allowed_discharge_power = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        // Note: most PID_CELLVOLTAGE_CELL_* responses are handled in the default case.
        // Certain specific cases are handled here as they are used to establish the number of cells.
        case PID_CELLVOLTAGE_CELL_85:
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
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
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
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
          tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
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
          if (pid_reply >= PID_TEMP_POINT_1 && pid_reply <= PID_TEMP_POINT_18) {
            datalayer_extended.meb.temp_points[pid_reply - PID_TEMP_POINT_1] =
                (((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]) / 8.f) - 40;
          } else if (pid_reply >= PID_CELLVOLTAGE_CELL_1 && pid_reply <= PID_CELLVOLTAGE_CELL_108) {
            // The general case for cell voltages (some specific cases handled above)
            tempval = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
            if (tempval != 0xFFE) {
              cellvoltages_polled[pid_reply - PID_CELLVOLTAGE_CELL_1] = (tempval + 1000);
            }
          }

          break;
      }

      break;
    case 0x18DAF105:
      handle_obd_frame(rx_frame, can_interface);
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

  if (currentMillis - last_can_msg_timestamp > 500) {
    if (first_can_msg)
      logging.printf("MEB: No CAN msg received for 500ms\n");
    can_msg_received = RX_DEFAULT;
    first_can_msg = 0;
    if (datalayer.battery.status.real_bms_status != BMS_FAULT) {
      datalayer.battery.status.real_bms_status = BMS_DISCONNECTED;
      datalayer.system.status.battery_allows_contactor_closing = false;
    }
  }
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    MEB_0FC.data.u8[1] = ((MEB_0FC.data.u8[1] & 0xF0) | counter_10ms);
    MEB_0FC.data.u8[0] = vw_crc_calc(MEB_0FC.data.u8, MEB_0FC.DLC, MEB_0FC.ID);

    counter_10ms = (counter_10ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&MEB_0FC);  // Required for contactor closing
  }
  // Send 20ms CAN Message
  if (currentMillis - previousMillis20ms >= INTERVAL_20_MS) {
    previousMillis20ms = currentMillis;

    MEB_0FD.data.u8[1] = ((MEB_0FD.data.u8[1] & 0xF0) | counter_20ms);
    MEB_0FD.data.u8[0] = vw_crc_calc(MEB_0FD.data.u8, MEB_0FD.DLC, MEB_0FD.ID);

    counter_20ms = (counter_20ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&MEB_0FD);  // Required for contactor closing
  }
  // Send 40ms CAN Message
  if (currentMillis - previousMillis40ms >= INTERVAL_40_MS) {
    previousMillis40ms = currentMillis;

    /* Handle content for 0x040 message */
    /* Airbag message, needed for BMS to function */
    MEB_040.data.u8[7] = counter_040;
    MEB_040.data.u8[1] = ((MEB_040.data.u8[1] & 0xF0) | counter_40ms);
    MEB_040.data.u8[0] = vw_crc_calc(MEB_040.data.u8, MEB_040.DLC, MEB_040.ID);
    counter_40ms = (counter_40ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    if (toggle) {
      counter_040 = (counter_040 + 1) % 256;  // Increment only on every other pass
    }
    toggle = !toggle;  // Flip the toggle each time the code block is executed

    transmit_can_frame(&MEB_040);  // Airbag message - Needed for contactor closing
  }
  // Send 50ms CAN Message
  if (currentMillis - previousMillis50ms >= INTERVAL_50_MS) {
    previousMillis50ms = currentMillis;

    /* Handle content for 0x0C0 message */
    /* BMS needs to see this EM1 message. Content located in frame5&6 especially (can be static?)*/
    /* Also the voltage seen externally to battery is in frame 7&8. At least for the 62kWh ID3 version does not seem to matter, but we send it anyway. */
    MEB_0C0.data.u8[1] = ((MEB_0C0.data.u8[1] & 0xF0) | counter_50ms);
    MEB_0C0.data.u8[7] = ((datalayer.battery.status.voltage_dV / 10) * 4) & 0x00FF;
    MEB_0C0.data.u8[8] =
        ((MEB_0C0.data.u8[8] & 0xF0) | ((((datalayer.battery.status.voltage_dV / 10) * 4) >> 8) & 0x0F));
    MEB_0C0.data.u8[0] = vw_crc_calc(MEB_0C0.data.u8, MEB_0C0.DLC, MEB_0C0.ID);
    counter_50ms = (counter_50ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..

    transmit_can_frame(&MEB_0C0);  //  Needed for contactor closing
  }
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    //HV request and DC/DC control lies in 0x503

    if ((!datalayer.system.settings.equipment_stop_active) && datalayer.battery.status.real_bms_status != BMS_FAULT &&
        (datalayer.battery.status.real_bms_status == BMS_ACTIVE ||
         (datalayer.battery.status.real_bms_status == BMS_STANDBY &&
          (hv_requested ||
           (datalayer.battery.status.voltage_dV > 200 && datalayer_extended.meb.BMS_voltage_intermediate_dV > 0 &&
            labs(((int32_t)datalayer.battery.status.voltage_dV) -
                 ((int32_t)datalayer_extended.meb.BMS_voltage_intermediate_dV)) < 200))))) {
      hv_requested = true;
      datalayer.system.settings.start_precharging = false;
      if (MEB_503.data.u8[3] == BMS_TARGET_HV_OFF) {
        logging.printf("MEB: Requesting HV\n");
      }
      if ((MEB_503.data.u8[1] & 0x80) !=
          (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00)) {
        if (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING) {
          logging.printf("MEB: Precharge bit set to active\n");
        } else {
          logging.printf("MEB: Precharge bit set to inactive\n");
        }
      }
      MEB_503.data.u8[1] =
          0x30 | (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00);
      MEB_503.data.u8[3] = BMS_TARGET_AC_CHARGING;
      MEB_503.data.u8[5] = 0x82;  // Bordnetz Active
      MEB_503.data.u8[6] = 0xE0;  // Request emergency shutdown HV system == 0, false
    } else if ((first_can_msg > 0 && currentMillis > first_can_msg + 1000 && BMS_mode != 7) ||
               datalayer.system.settings.equipment_stop_active) {  //FAULT STATE, open contactors

      if (datalayer.battery.status.bms_status != FAULT && datalayer.battery.status.real_bms_status == BMS_STANDBY &&
          !datalayer.system.settings.equipment_stop_active) {
        datalayer.system.settings.start_precharging = true;
      }

      if (MEB_503.data.u8[3] != BMS_TARGET_HV_OFF) {
        logging.printf("MEB: Requesting HV_OFF\n");
      }
      if ((MEB_503.data.u8[1] & 0x80) !=
          (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00)) {
        if (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING) {
          logging.printf("MEB: Precharge bit set to active\n");
        } else {
          logging.printf("MEB: Precharge bit set to inactive\n");
        }
      }
      MEB_503.data.u8[1] =
          0x10 | (datalayer.system.status.precharge_status == AUTO_PRECHARGE_PRECHARGING ? 0x80 : 0x00);
      MEB_503.data.u8[3] = BMS_TARGET_HV_OFF;
      MEB_503.data.u8[5] = 0x80;  // Bordnetz Inactive
      MEB_503.data.u8[6] =
          0xE3;  // Request emergency shutdown HV system == init (3) (not sure if we dare activate this, this is done with 0xE1)
    } else {
      MEB_503.data.u8[3] = 0;
      MEB_503.data.u8[5] = 0x80;  // Bordnetz Inactive
    }
    MEB_503.data.u8[1] = ((MEB_503.data.u8[1] & 0xF0) | counter_100ms);
    MEB_503.data.u8[0] = vw_crc_calc(MEB_503.data.u8, MEB_503.DLC, MEB_503.ID);

    //Bidirectional charging message
    MEB_272.data.u8[1] =
        0x00;  //0x80;  // Bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    MEB_272.data.u8[2] = 0x00;
    //0x01;  // High load bidirectional charging active (Set to 0x00 incase no bidirectional charging wanted)
    MEB_272.data.u8[5] = DC_FASTCHARGE_NO_START_REQUEST;  //DC_FASTCHARGE_VEHICLE;  //DC charging

    //Klemmen status
    MEB_3C0.data.u8[2] = 0x02;  //bit to signal that KL_15 is ON // Always 0 in start4.log
    MEB_3C0.data.u8[1] = ((MEB_3C0.data.u8[1] & 0xF0) | counter_100ms);
    MEB_3C0.data.u8[0] = vw_crc_calc(MEB_3C0.data.u8, MEB_3C0.DLC, MEB_3C0.ID);

    MEB_3BE.data.u8[1] = ((MEB_3BE.data.u8[1] & 0xF0) | counter_100ms);
    MEB_3BE.data.u8[0] = vw_crc_calc(MEB_3BE.data.u8, MEB_3BE.DLC, MEB_3BE.ID);

    MEB_14C.data.u8[1] = ((MEB_14C.data.u8[1] & 0xF0) | counter_100ms);
    MEB_14C.data.u8[0] = vw_crc_calc(MEB_14C.data.u8, MEB_14C.DLC, MEB_14C.ID);

    counter_100ms = (counter_100ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can_frame(&MEB_503);
    transmit_can_frame(&MEB_272);
    transmit_can_frame(&MEB_3C0);
    transmit_can_frame(&MEB_3BE);
    transmit_can_frame(&MEB_14C);
  }
  //Send 200ms message
  if (currentMillis - previousMillis200ms >= INTERVAL_200_MS) {
    previousMillis200ms = currentMillis;

    // MEB_153 does not need CRC even though it has it. Empty in some logs as well.

    //TODO: MEB_1B0000B9 & MEB_1B000010 & MEB_1B000046 has CAN sleep commands. May be removed?

    transmit_can_frame(&MEB_5E1);
    transmit_can_frame(&MEB_153);
    transmit_can_frame(&MEB_1B0000B9);
    transmit_can_frame(&MEB_1B000010);
    transmit_can_frame(&MEB_1B000046);

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
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(poll_pid >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)poll_pid;
        poll_pid = poll_pid + 1;
        break;
      case PID_TEMP_POINT_18:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(poll_pid >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)poll_pid;
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
        poll_pid = PID_ENERGY_COUNTERS;
        break;
      case PID_ENERGY_COUNTERS:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_ENERGY_COUNTERS >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_ENERGY_COUNTERS;
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
      // Cell Voltage Cases.
      // Most of these are handled in the default case.
      case PID_CELLVOLTAGE_CELL_1:
        MEB_POLLING_FRAME.data.u8[2] = (uint8_t)(PID_CELLVOLTAGE_CELL_1 >> 8);
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_1;
        poll_pid = PID_CELLVOLTAGE_CELL_2;
        break;
      case PID_CELLVOLTAGE_CELL_84:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_84;
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
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_96;
        if (datalayer.battery.info.number_of_cells > 96)
          poll_pid = PID_CELLVOLTAGE_CELL_97;
        else
          poll_pid = PID_SOC;
        break;
      case PID_CELLVOLTAGE_CELL_108:
        MEB_POLLING_FRAME.data.u8[3] = (uint8_t)PID_CELLVOLTAGE_CELL_108;
        poll_pid = PID_SOC;
        break;
      default:
        if (poll_pid >= PID_CELLVOLTAGE_CELL_1 && poll_pid <= PID_CELLVOLTAGE_CELL_108) {
          // The general case for cell voltages (some specific cases handled above)
          MEB_POLLING_FRAME.data.u8[3] = (uint8_t)poll_pid;
          // Poll the next cell next
          poll_pid = poll_pid + 1;
        } else {
          // Out-of-range PID, go back to the start
          poll_pid = PID_SOC;
        }
        break;
    }
    if (first_can_msg > 0 && currentMillis > first_can_msg + 1000) {
      transmit_can_frame(&MEB_POLLING_FRAME);
    }
  }

  // Send 500ms CAN Message
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&MEB_16A954B4);  //eTM, Cooling valves and pumps for BMS
    transmit_can_frame(&MEB_569);       // Battery heating requests
    transmit_can_frame(&MEB_1A55552B);  //Climate, heatpump and priorities
    transmit_can_frame(&MEB_1A555548);  //ORU, OTA update message for reserving battery
    transmit_can_frame(&MEB_16A954FB);  //Climate, request to BMS for starting preconditioning
  }

  //Send 1s CANFD message
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    MEB_641.data.u8[1] = ((MEB_641.data.u8[1] & 0xF0) | counter_1000ms);
    MEB_641.data.u8[0] = vw_crc_calc(MEB_641.data.u8, MEB_641.DLC, MEB_641.ID);

    MEB_1A5555A6.data.u8[2] = 0x7F;  //Outside temperature, factor 0.5, offset -50

    MEB_6B2.data.u8[0] =  //driving cycle counter, 0-254 wrap around. 255 = invalid value
        //MEB_6B2.data.u8[1-2-3b0-4] // Odometer, km (20 bits long)
        MEB_6B2.data.u8[3] = (uint8_t)((TIME_YEAR - 2000) << 4) | MEB_6B2.data.u8[3];
    MEB_6B2.data.u8[4] = (uint8_t)((TIME_DAY & 0x01) << 7 | TIME_MONTH << 3 | (TIME_YEAR - 2000) >> 4);
    MEB_6B2.data.u8[5] = (uint8_t)((TIME_HOUR & 0x0F) << 4 | TIME_DAY >> 1);
    MEB_6B2.data.u8[6] = (uint8_t)((seconds & 0x01) << 7 | TIME_MINUTE << 1 | TIME_HOUR >> 4);
    MEB_6B2.data.u8[7] = (uint8_t)((seconds & 0x3E) >> 1);
    seconds = (seconds + 1) % 60;

    counter_1000ms = (counter_1000ms + 1) % 16;  //Goes from 0-1-2-3...15-0-1-2-3..
    transmit_can_frame(&MEB_6B2);                // Diagnostics - Needed for contactor closing
    transmit_can_frame(&MEB_641);                // Motor - OBD
    transmit_can_frame(&MEB_5F5);                // Loading profile
    transmit_can_frame(&MEB_585);                // Systeminfo
    transmit_can_frame(&MEB_1A5555A6);           // Temperature QBit

    transmit_obd_can_frame(0x18DA05F1, can_config.battery, true);
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
}
