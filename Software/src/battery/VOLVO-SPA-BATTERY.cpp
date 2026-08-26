#include "VOLVO-SPA-BATTERY.h"
#include <cstring>  //For unit test
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"  //For "More battery info" webpage
#include "../devboard/utils/common_functions.h"
#include "../devboard/utils/events.h"

void VolvoSpaBattery::
    update_values() {  //This function maps all the values fetched via CAN to the correct parameters used for the inverter

  // Update webserver datalayer
  datalayer_extended.VolvoPolestar.BECMUDynMaxLim = MAX_U;
  datalayer_extended.VolvoPolestar.BECMUDynMinLim = MIN_U;
  datalayer_extended.VolvoPolestar.BECMsupplyVoltage = BECMsupplyVoltage;
  datalayer_extended.VolvoPolestar.HvBattPwrLimDcha1 = HvBattPwrLimDcha1;
  datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSoft = HvBattPwrLimDchaSoft;
  datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSlowAgi = HvBattPwrLimDchaSlowAgi;
  datalayer_extended.VolvoPolestar.HvBattPwrLimChrgSlowAgi = HvBattPwrLimChrgSlowAgi;

  datalayer.battery.status.remaining_capacity_Wh = (datalayer.battery.info.total_capacity_Wh - CHARGE_ENERGY);

  datalayer.battery.status.real_soc = SOC_BMS * 10;   //Add one decimal to make it pptt
  datalayer.battery.status.voltage_dV = BATT_U / 10;  //Remove one decimal
  datalayer.battery.status.current_dA = -BATT_I;      //Invert direction

  datalayer.battery.status.max_discharge_power_W = HvBattPwrLimDchaSlowAgi * 1000;  //kW to W
  datalayer.battery.status.max_charge_power_W = HvBattPwrLimChrgSlowAgi * 1000;     //kW to W
  datalayer.battery.status.temperature_min_dC = BATT_T_MIN;
  datalayer.battery.status.temperature_max_dC = BATT_T_MAX;

  datalayer.battery.status.cell_max_voltage_mV = CELL_U_MAX * 10;  // Use min/max reported from BMS
  datalayer.battery.status.cell_min_voltage_mV = CELL_U_MIN * 10;

  //Map all cell voltages to the global array
  for (int i = 0; i < 108; ++i) {
    datalayer.battery.status.cell_voltages_mV[i] = cell_voltages[i];
  }

  //If we have enough cell values populated (atleast 96 read) AND number_of_cells not initialized yet
  if (cell_voltages[95] > 0 && datalayer.battery.info.number_of_cells == 0) {
    // We can determine whether we have 96S or 108S battery
    if (datalayer.battery.status.cell_voltages_mV[107] > 0) {
      datalayer.battery.info.number_of_cells = 108;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_108S_DV;
      datalayer.battery.info.total_capacity_Wh = 78200;
    } else {
      datalayer.battery.info.number_of_cells = 96;
      datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_96S_DV;
      datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;
      datalayer.battery.info.total_capacity_Wh = 69511;
    }
  }

  //Check safeties
  if (BECMsupplyVoltage < 10700) {  //10.7V,
    //If 12V voltage goes under this, latch battery OFF to prevent contactors from swinging between on/off
    set_event(EVENT_12V_LOW, (BECMsupplyVoltage / 100));
    set_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 0, battery_index);
  }

  // Update diagnostic requests from webserver
  if (UserRequestBECMecuReset) {
    transmit_can_frame(&VOLVO_BECM_ECUreset);  //Send BECM ECU reset command
    UserRequestBECMecuReset = false;
  }

  if (UserRequestDTCreset && !dtc_clear_in_progress) {
    UserRequestDTCreset = false;
    dtc_clear_in_progress = true;
    dtc_clear_millis = millis();
    transmit_can_frame(&VOLVO_DTC_Erase);  //Send DTC clear command
  }

  // Give up waiting for the erase acknowledgement. The previously read list is deliberately left
  // untouched here: an unconfirmed erase is not evidence that the codes are gone.
  if (dtc_clear_in_progress && (millis() - dtc_clear_millis > DTC_TIMEOUT_MS)) {
    dtc_clear_in_progress = false;
  }

  if (UserRequestDTCreadout && !dtc_read_in_progress) {
    UserRequestDTCreadout = false;
    dtc_read_in_progress = true;
    dtc_rx_active = false;
    dtc_rx_len = 0;
    dtc_rx_expected = 0;
    dtc_request_millis = millis();
    datalayer.battery.dtc.dtc_read_failed = false;
    transmit_can_frame(&VOLVO_DTCreadout);  //Send DTC read command
  }

  // Give up if the BMS never completes the reply, so the page stops showing a pending read.
  if (dtc_read_in_progress && (millis() - dtc_request_millis > DTC_TIMEOUT_MS)) {
    dtc_read_in_progress = false;
    dtc_rx_active = false;
    datalayer.battery.dtc.dtc_read_failed = true;
    datalayer.battery.dtc.dtc_last_read_millis = millis();
  }
}

// Parses a reassembled UDS ReadDTCInformation reply out of dtc_buffer: a 4-byte header
// (59 <subfunction> <statusAvailabilityMask> <DTCAndStatusAvailabilityRecord>) followed by
// 4 bytes per DTC, being a 3-byte code plus one status byte. Only the raw codes are stored
// here; the web renderer formats them into the 5-character Volvo strings (P33D7, U1000)
// and looks up their descriptions up in volvo_SPA_dtc.json
// Header length depends on subfunction: 0x02 responses include a statusAvailabilityMask
// byte (3 bytes total: 59 02 <mask>), while 0x03 responses do not (2 bytes: 59 03).
// Followed by 4 bytes per DTC: 3-byte code plus one status byte.
void VolvoSpaBattery::parseDTCResponseVolvo() {
  uint16_t DTC_HEADER_LEN;
  if (dtc_buffer[1] == 0x02) {
    DTC_HEADER_LEN = 3;  // 59 02 <statusMask>
  } else {
    DTC_HEADER_LEN = 2;  // 59 03 (no statusMask)
  }

  dtc_read_in_progress = false;
  dtc_rx_active = false;
  datalayer.battery.dtc.dtc_last_read_millis = millis();

  if (dtc_rx_len < DTC_HEADER_LEN || dtc_buffer[0] != 0x59) {
    datalayer.battery.dtc.dtc_read_failed = true;
    return;
  }

  uint16_t count = (dtc_rx_len - DTC_HEADER_LEN) / 4;
  if (count > DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT) {
    count = DATALAYER_BATTERY_DTC_TYPE::MAX_DTC_COUNT;
  }

  for (uint16_t i = 0; i < count; i++) {
    uint16_t offset = DTC_HEADER_LEN + (i * 4);
    datalayer.battery.dtc.dtc_codes[i] = ((uint32_t)dtc_buffer[offset] << 16) |
                                         ((uint32_t)dtc_buffer[offset + 1] << 8) | (uint32_t)dtc_buffer[offset + 2];
    datalayer.battery.dtc.dtc_status[i] = dtc_buffer[offset + 3];
  }

  datalayer.battery.dtc.dtc_count = count;
  datalayer.battery.dtc.dtc_read_failed = false;
}

void VolvoSpaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x3A:
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      if ((rx_frame.data.u8[6] & 0x80) == 0x80) {
        BATT_I = ((((rx_frame.data.u8[6] & 0x7F) << 8) | rx_frame.data.u8[7]) - 16380);
      }

      if ((rx_frame.data.u8[2] & 0x08) == 0x08) {
        MAX_U = ((((rx_frame.data.u8[2] & 0x07) << 8) | rx_frame.data.u8[3]) / 4);
      }

      if ((rx_frame.data.u8[4] & 0x08) == 0x08) {
        MIN_U = ((((rx_frame.data.u8[4] & 0x07) << 8) | rx_frame.data.u8[5]) / 4);
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08) {
        BATT_U = ((((rx_frame.data.u8[0] & 0x07) << 8) | rx_frame.data.u8[1]) * 25);
      }

      if ((rx_frame.data.u8[0] & 0x40) == 0x40)
        datalayer_extended.VolvoPolestar.HVSysRlySts = ((rx_frame.data.u8[0] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysRlySts = 0xFF;

      if ((rx_frame.data.u8[2] & 0x40) == 0x40)
        datalayer_extended.VolvoPolestar.HVSysDCRlySts1 = ((rx_frame.data.u8[2] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysDCRlySts1 = 0xFF;
      if ((rx_frame.data.u8[2] & 0x80) == 0x80)
        datalayer_extended.VolvoPolestar.HVSysDCRlySts2 = ((rx_frame.data.u8[4] & 0x30) >> 4);
      else
        datalayer_extended.VolvoPolestar.HVSysDCRlySts2 = 0xFF;
      if ((rx_frame.data.u8[0] & 0x80) == 0x80)
        datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts = ((rx_frame.data.u8[4] & 0xC0) >> 6);
      else
        datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts = 0xFF;

      break;
    case 0x1A1:
      if ((rx_frame.data.u8[4] & 0x10) == 0x10)
        CHARGE_ENERGY = ((((rx_frame.data.u8[4] & 0x0F) * 256.0 + rx_frame.data.u8[5]) * 50) - 500);
      else {
        CHARGE_ENERGY = 0;
        set_event(EVENT_KWH_PLAUSIBILITY_ERROR, CHARGE_ENERGY);
      }
      break;
    case 0x413:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80) {
        BATT_ERR_INDICATION = ((rx_frame.data.u8[0] & 0x40) >> 6);  //TODO, do something with this value?
      }
      if ((rx_frame.data.u8[0] & 0x20) == 0x20) {
        BATT_T_MAX = sign_extend_to_int16((((rx_frame.data.u8[2] & 0x1F) << 8) | rx_frame.data.u8[3]), 13);
        BATT_T_MIN = sign_extend_to_int16((((rx_frame.data.u8[4] & 0x1F) << 8) | rx_frame.data.u8[5]), 13);
        BATT_T_AVG = sign_extend_to_int16((((rx_frame.data.u8[0] & 0x1F) << 8) | rx_frame.data.u8[1]), 13);
      }
      break;
    case 0x369:
      if ((rx_frame.data.u8[0] & 0x80) == 0x80) {
        HvBattPwrLimDchaSoft = (((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[6]) >> 2);
      }
      break;
    case 0x175:
      if ((rx_frame.data.u8[4] & 0x80) == 0x80) {
        HvBattPwrLimDcha1 = (((rx_frame.data.u8[2] & 0x07) * 256 + rx_frame.data.u8[3]) >> 2);
      } else {
        HvBattPwrLimDcha1 = 0;
      }
      break;
    case 0x177:
      if ((rx_frame.data.u8[4] & 0x08) == 0x08) {
        HvBattPwrLimDchaSlowAgi = (((rx_frame.data.u8[4] & 0x07) * 256 + rx_frame.data.u8[5]) >> 2);
      } else {
        HvBattPwrLimDchaSlowAgi = 0;
      }
      if ((rx_frame.data.u8[2] & 0x08) == 0x08) {
        HvBattPwrLimChrgSlowAgi = (((rx_frame.data.u8[2] & 0x07) * 256 + rx_frame.data.u8[3]) >> 2);
      } else {
        HvBattPwrLimChrgSlowAgi = 0;
      }
      break;
    case 0x37D:
      if ((rx_frame.data.u8[0] & 0x40) == 0x40) {
        SOC_BMS = ((rx_frame.data.u8[6] & 0x03) * 256 + rx_frame.data.u8[7]);
      }

      if ((rx_frame.data.u8[0] & 0x04) == 0x04) {
        CELL_U_MAX = ((rx_frame.data.u8[2] & 0x01) * 256 + rx_frame.data.u8[3]);
      }

      if ((rx_frame.data.u8[0] & 0x02) == 0x02) {
        CELL_U_MIN = ((rx_frame.data.u8[0] & 0x01) * 256.0 + rx_frame.data.u8[1]);
      }

      if ((rx_frame.data.u8[0] & 0x08) == 0x08) {
        CELL_ID_U_MAX = ((rx_frame.data.u8[4] & 0x01) * 256.0 + rx_frame.data.u8[5]);
      }
      break;
    case 0x635:  // Diag request response

      if (cell_voltage_read_in_progress) {
        if ((rx_frame.data.u8[0] == 0x10) && (rx_frame.data.u8[1] == 0x0B) && (rx_frame.data.u8[2] == 0x62) &&
            (rx_frame.data.u8[3] == 0x4B))  // First response frame of cell voltages
        {
          cell_voltages[battery_request_idx++] = ((rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6]);
          cell_voltages[battery_request_idx] = (rx_frame.data.u8[7] << 8);
          rxConsecutiveFrames = true;
        } else if ((rx_frame.data.u8[0] == 0x21) && (rxConsecutiveFrames)) {
          cell_voltages[battery_request_idx] |= rx_frame.data.u8[1];
          battery_request_idx++;
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
          cell_voltages[battery_request_idx++] = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];

          if (batteryModuleNumber <= 0x2A)  // Run until last pack is read
          {
            VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
            transmit_can_frame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for next module
          } else {                                  //Last pack read, calculate min/max cell voltage
            cell_voltage_read_in_progress = false;
            min_max_voltage[0] = 9999;
            min_max_voltage[1] = 0;
            for (cellcounter = 0; cellcounter < 108; cellcounter++) {
              if (min_max_voltage[0] > cell_voltages[cellcounter])
                min_max_voltage[0] = cell_voltages[cellcounter];
              if (min_max_voltage[1] < cell_voltages[cellcounter])
                min_max_voltage[1] = cell_voltages[cellcounter];
            }
          }
          rxConsecutiveFrames = false;
        }
      }

      // ClearDiagnosticInformation is acknowledged with a single-frame 54. Only then are the stored
      // codes known to be gone. The read timestamp is reset too, so the page goes back to
      // "not read yet": an erase says nothing about what the BMS will report from here on.
      if (dtc_clear_in_progress && rx_frame.data.u8[0] == 0x01 && rx_frame.data.u8[1] == 0x54) {
        dtc_clear_in_progress = false;
        datalayer.battery.dtc.dtc_count = 0;
        datalayer.battery.dtc.dtc_read_failed = false;
        datalayer.battery.dtc.dtc_last_read_millis = 0;
        break;
      }

      // A DTC readout answers on 0x635 just like the polling below, and its first frame would
      // otherwise be mistaken for polling data. Intercept it while a read is in
      // flight. The 0x59 service reply byte is what tells the two apart
      if (dtc_read_in_progress) {
        uint8_t pci = rx_frame.data.u8[0] & 0xF0;

        if (pci == 0x00 && rx_frame.data.u8[1] == 0x59) {  //Single frame: reply fits in one message
          dtc_rx_len = rx_frame.data.u8[0] & 0x0F;
          if (dtc_rx_len > 7) {
            dtc_rx_len = 7;
          }
          for (uint8_t i = 0; i < dtc_rx_len; i++) {
            dtc_buffer[i] = rx_frame.data.u8[1 + i];
          }
          parseDTCResponseVolvo();
          break;
        }

        if (pci == 0x10 && rx_frame.data.u8[2] == 0x59) {  //First frame of a multi-frame reply
          dtc_rx_expected = ((rx_frame.data.u8[0] & 0x0F) << 8) | rx_frame.data.u8[1];
          if (dtc_rx_expected > DTC_BUFFER_SIZE) {
            dtc_rx_expected = DTC_BUFFER_SIZE;  //More codes than we can store, keep the first ones
          }
          dtc_rx_len = 0;
          for (uint8_t i = 2; i < 8 && dtc_rx_len < dtc_rx_expected; i++) {
            dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
          }
          dtc_rx_active = true;
          transmit_can_frame(&VOLVO_FlowControl);  //Flow control, ask for the rest
          break;
        }

        if (dtc_rx_active && pci == 0x20) {  //Consecutive frame
          for (uint8_t i = 1; i < 8 && dtc_rx_len < dtc_rx_expected; i++) {
            dtc_buffer[dtc_rx_len++] = rx_frame.data.u8[i];
          }
          if (dtc_rx_len >= dtc_rx_expected) {
            parseDTCResponseVolvo();
          } else {
            transmit_can_frame(&VOLVO_FlowControl);
          }
          break;
        }

        if (rx_frame.data.u8[1] == 0x7F && rx_frame.data.u8[2] == 0x19) {  //Request rejected by BMS
          dtc_read_in_progress = false;
          dtc_rx_active = false;
          datalayer.battery.dtc.dtc_read_failed = true;
          datalayer.battery.dtc.dtc_last_read_millis = millis();
          break;
        }
      }

      //Periodic polling of battery below

      if (rx_frame.data.u8[0] < 0x10) {  //One line response
        incoming_poll = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];
      }

      if (rx_frame.data.u8[0] == 0x10) {  //Multiframe response, send ACK
        transmit_can_frame(&VOLVO_FlowControl);
        incoming_poll = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
      }

      switch (incoming_poll) {
        case PID_POLL_SOH:
          datalayer.battery.status.soh_pptt = ((rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7]);
          break;
        case PID_POLL_BECM_SUPPLY_VOLTAGE:
          BECMsupplyVoltage = ((rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5]);
          break;
        case PID_POLL_HVIL:
          datalayer_extended.VolvoPolestar.HVILstatusBits = (rx_frame.data.u8[4]);
          break;
        case PID_POLL_CELL_VOLTAGES:
          //Handled at the top of the 0x635 handler
          break;
        default:  //Unknown poll, ignore
          break;
      }
      break;
    default:
      break;
  }
}

void VolvoSpaBattery::readCellVoltages() {
  cell_voltage_read_in_progress = true;
  battery_request_idx = 0;
  batteryModuleNumber = 0x10;
  rxConsecutiveFrames = false;
  VOLVO_CELL_U_Req.data.u8[3] = batteryModuleNumber++;
  transmit_can_frame(&VOLVO_CELL_U_Req);  //Send cell voltage read request for first module
}

void VolvoSpaBattery::transmit_can(unsigned long currentMillis) {
  // Send 100ms CAN Message
  if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
    previousMillis100 = currentMillis;

    transmit_can_frame(&VOLVO_536);  //Send 0x536 Network managing frame to keep BMS alive
    transmit_can_frame(&VOLVO_372);  //Send 0x372 ECMAmbientTempCalculated

    if ((datalayer.system.status.system_status == ACTIVE) && startedUp) {
      datalayer.system.status.battery_allows_contactor_closing = true;
      transmit_can_frame(&VOLVO_140_CLOSE);  //Send 0x140 Close contactors message
    } else {  //datalayer.battery.status.bms_status == FAULT , OR inverter requested opening contactors, OR system not started yet
      datalayer.system.status.battery_allows_contactor_closing = false;
      transmit_can_frame(&VOLVO_140_OPEN);  //Send 0x140 Open contactors message
    }
  }
  // Send 500ms CAN Message
  if (currentMillis - previousMillis500 >= INTERVAL_500_MS) {
    previousMillis500 = currentMillis;

    // Update current poll from the array
    currentpoll = poll_commands[poll_index];
    poll_index = (poll_index + 1) % 3;

    VOLVO_Poll_frame.data.u8[2] = (uint8_t)((currentpoll & 0xFF00) >> 8);
    VOLVO_Poll_frame.data.u8[3] = (uint8_t)(currentpoll & 0x00FF);

    if (!dtc_read_in_progress &&
        !cell_voltage_read_in_progress) {  // Only send poll if not already reading DTCs or cellvoltages
      transmit_can_frame(&VOLVO_Poll_frame);
    }
  }
  if (currentMillis - previousMillis1s >= INTERVAL_1_S) {
    previousMillis1s = currentMillis;

    if (!startedUp) {
      transmit_can_frame(&VOLVO_DTC_Erase);  //Erase any DTCs preventing startup
      DTC_reset_counter++;
      if (DTC_reset_counter > 1) {  // Performed twice before starting
        startedUp = true;
      }
    }
  }
  if (currentMillis - previousMillis60s >= INTERVAL_60_S) {
    previousMillis60s = currentMillis;

    if (cell_voltage_read_in_progress) {
      //If the last cellvoltage read was not completed, yield for next 60s to allow for normal PID polls too go thru
      cell_voltage_read_in_progress = false;
    } else if (!dtc_read_in_progress) {
      readCellVoltages();
    }
  }
}

void VolvoSpaBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 0;        // Initializes when all cells have been read
  datalayer.battery.info.total_capacity_Wh = 78200;  //Startout in 78kWh mode (This value used for SOC calc)
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_108S_DV;  //Startout with max allowed range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_96S_DV;   //Startout with min allowed range
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}
