#include "Sunwoda-ESS.h"
#include <string.h>
#include "../battery/BATTERIES.h"
#include "../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

/*
Based on reverse engineering of the Sunwoda BCMU CAN variable map together with a real
CAN capture (tools/pcanOut.txt). See Sunwoda-ESS.h for how CAN IDs map to the vendor's
address scheme. Fields not listed in the switch below have not been decoded yet.
*/

void SunwodaBattery::setup(void) {  // Performs one time setup at startup
  if (is_primary()) {
    strncpy(datalayer.system.info.battery_protocol, Name, 63);
    datalayer.system.info.battery_protocol[63] = '\0';
  }

  datalayer_battery->info.chemistry = battery_chemistry_enum::LFP;

  if (user_selected_max_pack_voltage_dV > 0) {
    datalayer_battery->info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  }
  if (user_selected_min_pack_voltage_dV > 0) {
    datalayer_battery->info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  }
  if (user_selected_max_cell_voltage_mV > 0) {
    datalayer_battery->info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  }
  if (user_selected_min_cell_voltage_mV > 0) {
    datalayer_battery->info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  }

  datalayer_battery->info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
}

void SunwodaBattery::update_values() {
  datalayer_battery->status.real_soc = (uint16_t)(soc * 100);  // 98.3% -> 9830

  datalayer_battery->status.soh_pptt = (uint16_t)(soh * 100);  // 100.0% -> 10000

  datalayer_battery->status.voltage_dV = (uint16_t)(packVoltage * 10);  // 622.1V -> 6221

  datalayer_battery->status.current_dA = packCurrent_dA;  // positive = charging
  datalayer_battery->status.active_power_W =
      (int32_t)datalayer_battery->status.voltage_dV * datalayer_battery->status.current_dA / 100;

  datalayer_battery->status.cell_max_voltage_mV = maxCellVoltage;
  datalayer_battery->status.cell_min_voltage_mV = minCellVoltage;

  datalayer_battery->status.temperature_min_dC = minTemperature * 10;  // whole degrees C -> deci-degrees
  datalayer_battery->status.temperature_max_dC = maxTemperature * 10;

  if (actual_cell_count > 0) {
    datalayer_battery->info.number_of_cells = actual_cell_count;
  }

  datalayer_battery->status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer_battery->status.real_soc) / 10000) * datalayer_battery->info.total_capacity_Wh);

  // BCMU limits from gCurrLimit_90 (0x0C50FF5A), verified 2026-10-03 against the vendor sheet
  // and live data (HWv1 running: 201/201/61/61; HWv2 at SOC 0: 201/0/57/0).
  datalayer_battery->status.max_charge_current_dA = bmsLimits[0];
  datalayer_battery->status.max_discharge_current_dA = bmsLimits[1];
  datalayer_battery->status.max_charge_power_W = (uint32_t)bmsLimits[2] * 100;
  datalayer_battery->status.max_discharge_power_W = (uint32_t)bmsLimits[3] * 100;

  // Recovery charge: after a manual close in Debugging mode (BCMU in Stop) the BCMU reports zero
  // limits, so offer a small fixed charge (never discharge) until the highest cell nears full.
  // Latched with hysteresis: once the highest cell reaches the cut-off, stay off until it has relaxed well
  // below it, so a brief spike does not toggle the offer 0/5 A (the SH15T does not resume after a 0).
  if (maxCellVoltage >= RECOVERY_CHARGE_MAX_CELL_MV) {
    recovery_charge_blocked = true;
  } else if (maxCellVoltage < RECOVERY_CHARGE_RESUME_CELL_MV) {
    recovery_charge_blocked = false;
  }
  if (extended_data.operatingMode == 8 && extended_data.main_contactor_closed && !extended_data.faultActive &&
      bmsLimits[0] == 0 && maxCellVoltage > 0 && !recovery_charge_blocked) {
    datalayer_battery->status.max_charge_current_dA = RECOVERY_CHARGE_CURRENT_DA;
    datalayer_battery->status.max_charge_power_W =
        (uint32_t)RECOVERY_CHARGE_CURRENT_DA * datalayer_battery->status.voltage_dV / 100;
  }

  apply_charge_taper(millis());

  /*
  Contactor closing policy:

  The BCMU owns its own negative/main/precharge contactors and closes them autonomously
  (observed in tools/pcanOut.txt: the negative contactor bit comes up on its own a few
  hundred ms after power-up, with no request frame from the host at all). So unlike
  batteries whose BMS waits for an explicit "close contactors" CAN command from us, there
  is nothing to transmit here - we only need to *observe* what the BCMU already decided
  and mirror that into the datalayer so battery-emulator's own contactor/precharge state
  machine (precharge_control.cpp) knows it is safe to connect the DC bus to the inverter.

  main_contactor_closed comes from ID_SWITCH_STATUS (gIoSwhInfo_51) and is the direct
  feedback of the BCMU's own main contactor, so it is a stronger signal than the
  "sleeping" bit used for the same purpose on Growatt HV Ark (which lacks contactor
  feedback). faultActive comes from ID_FAULT_INFO; alarmActive (caution-level) is
  intentionally not included here, matching the ERROR vs INFO split already used above.
  */
  datalayer_battery->status.real_bms_status =
      extended_data.faultActive ? BMS_FAULT : (extended_data.main_contactor_closed ? BMS_ACTIVE : BMS_STANDBY);

  if (allows_contactor_closing) {
    *allows_contactor_closing = extended_data.main_contactor_closed && !extended_data.faultActive;
  }

  // Pack-internal contactors: the BCMU is the only thing that ever closes them (see comment
  // above), so it is also the only thing that knows the DC bus is actually energized. Guarded
  // so the GPIO contactor state machine (comm_contactorcontrol.cpp) stays the single writer
  // if the user also has emulator-driven relays enabled for some other part of the topology.
  if (!contactor_control_enabled && is_primary()) {
    datalayer.system.status.dc_bus_live = extended_data.main_contactor_closed && !extended_data.faultActive;
  }

  // Backstop for the per-frame check in the alarm/fault handlers.
  check_auto_open();
}

void SunwodaBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  // Any frame whose source node is the BCMU proves the current CAN speed is right.
  // Only this instance's BCMU. A frame is 0x0C <src> <dst> <object>: status objects are broadcast
  // to 0xFF, the version replies are addressed to the BSMU (0x6E). Dispatch is on the object byte
  // because the source address is a runtime value and switch labels must be constants.
  if (((rx_frame.ID >> 16) & 0xFF) != bcmu_addr) {
    return;
  }
  last_bcmu_rx_ms = millis();

  const uint8_t dst = (rx_frame.ID >> 8) & 0xFF;
  if (dst != 0xFF && dst != NODE_ADDR_BSMU) {
    return;  // write acks / read replies come back addressed to us; nothing here decodes them
  }

  switch (rx_frame.ID & 0xFF) {
    case OBJ_MAIN_INFO:  // 0x0C50FF50 - Total voltage / current / power / SOC / SOH / avg temp

      if (rx_frame.DLC < 8) {
        break;
      }

      datalayer_battery->status.CAN_battery_still_alive = CAN_STILL_ALIVE;

      /*
      Example:
      43 00 4D 18 00 00 00 00
      0x184D = 6221 -> 622.1V
      */
      if (rx_frame.data.u8[0] == 0x43) {
        packVoltage = (float)u16(&rx_frame.data.u8[2]) / 10.0f;
        // Subindex 1: pack current, S16 0.1 A, negative while charging (verified 2026-10-03:
        // -3.3 A while the inverter reported ~1 kW into the pack). The datalayer wants the opposite.
        packCurrent_dA = -(int16_t)u16(&rx_frame.data.u8[4]);
      }

      /*
      Example:
      83 03 D7 03 E8 03 16 00
      SOC = 983 = 98.3%
      SOH = 1000 = 100%
      */
      if (rx_frame.data.u8[0] == 0x83) {
        soc = (float)u16(&rx_frame.data.u8[2]) / 10.0f;
        soh = (float)u16(&rx_frame.data.u8[4]) / 10.0f;
      }

      break;

    case OBJ_VOLT_CHARA:  // 0x0C50FF51 - Voltage characteristics (lowest/highest cell)

      if (rx_frame.DLC < 8) {
        break;
      }

      // 43 frame: lowest cell number + lowest voltage
      if (rx_frame.data.u8[0] == 0x43) {
        minCellNumber = u16(&rx_frame.data.u8[2]);
        minCellVoltage = u16(&rx_frame.data.u8[4]);
      }

      // 83 frame: highest cell number + highest voltage
      if (rx_frame.data.u8[0] == 0x83) {
        maxCellNumber = u16(&rx_frame.data.u8[2]);
        maxCellVoltage = u16(&rx_frame.data.u8[4]);
      }

      break;

    case OBJ_CURRENT_LIMIT: {  // 0x0C50FF5A - Charge/discharge current and power limits
      // Mux byte: low nibble = word count, data[1] = first subindex, then u16 LE words.
      const uint8_t count = rx_frame.data.u8[0] & 0x0F;
      const uint8_t first = rx_frame.data.u8[1];
      for (uint8_t i = 0; i < count; i++) {
        const uint8_t sub = first + i;
        if (sub < 4 && 2 + 2 * i + 1 < rx_frame.DLC) {
          bmsLimits[sub] = u16(&rx_frame.data.u8[2 + 2 * i]);
        }
      }
      break;
    }

    case OBJ_TEMP_CHARA:  // 0x0C50FF52 - Temperature characteristics (lowest/highest)

      if (rx_frame.DLC < 8) {
        break;
      }

      if (rx_frame.data.u8[0] == 0x43) {  // Lowest temperature
        minTemperature = (int16_t)u16(&rx_frame.data.u8[4]);
      }

      if (rx_frame.data.u8[0] == 0x83) {  // Highest temperature
        maxTemperature = (int16_t)u16(&rx_frame.data.u8[4]);
      }

      break;

    case OBJ_SWITCH_STATUS: {  // 0x0C50FF33 - Contactor / IO switch status (gIoSwhInfo_51)

      if (rx_frame.DLC < 8) {
        break;
      }

      /*
      Example: C3 00 00 00 01 00 03 00
      byte0: mux (ignored)
      byte1: reserved
      [2:4]: output switch I/O status - bit0 negative contactor, bit1 main contactor,
             bit2 precharge contactor, bit8 hard contact 1, bit9 hard contact 2
      [4:6]: input switch I/O status (raw, meaning of individual bits not confirmed)
      [6:8]: safety switch stable state - bit0 disconnecting switch, bit1 surge protector
      */
      uint16_t output_switch_status = u16(&rx_frame.data.u8[2]);
      uint16_t input_switch_status = u16(&rx_frame.data.u8[4]);
      uint16_t safety_switch_status = u16(&rx_frame.data.u8[6]);

      extended_data.raw_output_switch_status = output_switch_status;
      extended_data.raw_input_switch_status = input_switch_status;

      extended_data.negative_contactor_closed = (output_switch_status & 0x0001) != 0;
      extended_data.main_contactor_closed = (output_switch_status & 0x0002) != 0;
      extended_data.precharge_contactor_closed = (output_switch_status & 0x0004) != 0;
      extended_data.hard_contact_1_closed = (output_switch_status & 0x0100) != 0;
      extended_data.hard_contact_2_closed = (output_switch_status & 0x0200) != 0;

      extended_data.disconnect_switch_closed = (safety_switch_status & 0x0001) != 0;
      extended_data.surge_protector_closed = (safety_switch_status & 0x0002) != 0;

    } break;

    case OBJ_SYSTEM_STATUS: {  // 0x0C50FF32 - System status (gStateInfo_50)

      /*
      Same subindex-multiplexed layout as ID_ALARM_INFO/ID_FAULT_INFO: byte0 is a mux marker
      (ignored), byte1 is the subindex of the first word in this frame. gStateInfo_50 has 5
      subindices, confirmed from the vendor's BCMU_APP CAN variable map ("Current status" sheet):
      0 Battery protection status, 1 Battery pack operating status, 2 charge/discharge status,
      3 operating mode, 4 control mode. They arrive as one DLC8 frame (subindex 0-2) followed by
      one DLC6 frame (subindex 3-4) - e.g. "43 00 03 00 01 00 00 00" then "82 03 00 00 00 00".

      operatingStatus (subindex 1) is the key diagnostic: the BCMU only closes its contactors once
      it reaches Running (3). Reaching Running requires the host to send a Start command (see
      OBJ_SYSTEM_CONTROL / transmit_can() below) - without it the pack sits at Stop (1) forever,
      which matches a real capture (tools/pcanOut.txt) showing operatingStatus stuck at 1 and the
      output switch status (0x0C50FF33) stuck at all-open for the whole log.
      */
      if (rx_frame.DLC < 4) {
        break;
      }

      uint8_t start_subindex = rx_frame.data.u8[1];
      uint8_t words_in_frame = (rx_frame.DLC - 2) / 2;
      for (uint8_t i = 0; i < words_in_frame; i++) {
        uint8_t subindex = start_subindex + i;
        uint16_t value = u16(&rx_frame.data.u8[2 + i * 2]);
        switch (subindex) {
          case 0:
            extended_data.batteryProtectionStatus = (uint8_t)value;
            break;
          case 1:
            extended_data.operatingStatus = (uint8_t)value;
            break;
          case 2:
            extended_data.chargeDischargeStatus = (uint8_t)value;
            break;
          case 3:
            extended_data.operatingMode = (uint8_t)value;
            break;
          case 4:
            extended_data.controlMode = (uint8_t)value;
            break;
          default:
            break;
        }
      }

    } break;

    case OBJ_CLUSTER_INFO:  // 0x0C50FF36 - Contactor self-test status (gClusterInfo_54)

      if (rx_frame.DLC < 4) {
        break;
      }

      extended_data.contactor_selftest_status = u16(&rx_frame.data.u8[2]);

      break;

    case OBJ_ALARM_INFO: {  // 0x0C50FF34 - Alarm information (gAlarmInfo_52)

      /*
      Multiplexed the same way as ID_SWITCH_STATUS/ID_VOLT_CHARA: byte0 is a mux marker (ignored),
      byte1 is the subindex of the first word carried in this frame, and each subsequent u16 is one
      more consecutive subindex. gAlarmInfo_52 has 4 subindices (external alarm 0/1, internal alarm
      0/1), confirmed from the vendor's BCMU_APP CAN variable map (tools/H102025_P02_BCMU_APP_V1.16_
      FerroAMP_20210420_4BMU translated.xlsx, "Current status" sheet). They normally arrive as one
      DLC8 frame (subindex 0-2) followed by one DLC4 frame (subindex 3 only) - e.g. the header bytes
      themselves (0x43/0x00 or 0x81/0x03) must NOT be treated as alarm data, otherwise the mux/
      subindex bytes falsely look like an active alarm even when the real bits are all zero.
      */
      if (rx_frame.DLC < 4) {
        break;
      }

      uint8_t start_subindex = rx_frame.data.u8[1];
      uint8_t words_in_frame = (rx_frame.DLC - 2) / 2;
      for (uint8_t i = 0; i < words_in_frame; i++) {
        uint8_t subindex = start_subindex + i;
        if (subindex >= 4) {
          continue;
        }
        extended_data.alarmWords[subindex] = u16(&rx_frame.data.u8[2 + i * 2]);
      }

      extended_data.alarmActive = (extended_data.alarmWords[0] | extended_data.alarmWords[1] |
                                   extended_data.alarmWords[2] | extended_data.alarmWords[3]) != 0;

      // Surface an unspecified BMS alarm on the events page, same as other drivers do for a
      // generic "caution" bit whose exact meaning isn't broken out (e.g. Nissan Leaf case 4).
      if (extended_data.alarmActive) {
        set_event(EVENT_BATTERY_CAUTION, 0);
      } else {
        clear_event(EVENT_BATTERY_CAUTION);
      }
      check_auto_open();

    } break;

    case OBJ_FAULT_INFO: {  // 0x0C50FF35 - Fault information (gFaultInfo_53)

      // Same subindex-multiplexed layout as ID_ALARM_INFO above (4 subindices: external fault
      // 0/1, internal fault 0/1), confirmed from the vendor's BCMU_APP CAN variable map.
      if (rx_frame.DLC < 4) {
        break;
      }

      uint8_t start_subindex = rx_frame.data.u8[1];
      uint8_t words_in_frame = (rx_frame.DLC - 2) / 2;
      for (uint8_t i = 0; i < words_in_frame; i++) {
        uint8_t subindex = start_subindex + i;
        if (subindex >= 4) {
          continue;
        }
        extended_data.faultWords[subindex] = u16(&rx_frame.data.u8[2 + i * 2]);
      }

      extended_data.faultActive = (extended_data.faultWords[0] | extended_data.faultWords[1] |
                                   extended_data.faultWords[2] | extended_data.faultWords[3]) != 0;

      // A Fault frame is assumed to be more severe than an Alarm one, so it is mapped to the
      // generic ERROR-level "stop charge/discharge" event rather than the INFO-level caution
      // used for Alarm above. Same pattern as Nissan Leaf's worst-case failsafe status (case 7).
      if (extended_data.faultActive) {
        set_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ, 0);
      } else {
        clear_event(EVENT_BATTERY_CHG_DISCHG_STOP_REQ);
      }
      check_auto_open();

    } break;

    case OBJ_CELL_VOLTAGE_0: {  // 0x0C50FF55 - Individual cell voltages, 3 cells per frame

      if (rx_frame.DLC < 8) {
        break;
      }

      uint8_t start_index = rx_frame.data.u8[1];  // 0-based cell index of the first cell in this frame

      for (uint8_t i = 0; i < 3; i++) {
        uint8_t cell_index = start_index + i;
        if (cell_index >= MAX_AMOUNT_CELLS) {
          continue;
        }
        uint16_t cell_mV = u16(&rx_frame.data.u8[2 + i * 2]);
        if (cell_mV == 0) {
          continue;
        }
        datalayer_battery->status.cell_voltages_mV[cell_index] = cell_mV;
        if ((uint8_t)(cell_index + 1) > actual_cell_count) {
          actual_cell_count = cell_index + 1;
        }
      }

    } break;

    case OBJ_TEMP_ARRAY_0: {  // 0x0C50FF57 - Individual temperature sensors, 3 per frame, 0.1 degC

      if (rx_frame.DLC < 8) {
        break;
      }

      uint8_t start_index = rx_frame.data.u8[1];  // 0-based sensor index of the first sensor in this frame

      for (uint8_t i = 0; i < 3; i++) {
        uint8_t sensor_index = start_index + i;
        if (sensor_index >= SunwodaExtendedData::MAX_TEMP_SENSORS) {
          continue;
        }
        int16_t temp_dC = (int16_t)u16(&rx_frame.data.u8[2 + i * 2]);
        extended_data.temperatures_dC[sensor_index] = temp_dC;
        if ((uint8_t)(sensor_index + 1) > extended_data.temp_sensor_count) {
          extended_data.temp_sensor_count = sensor_index + 1;
        }
      }

    } break;

    case OBJ_SOFTWARE_VERSION:  // 0x0C506E07 - Software version number (see comment in Sunwoda-ESS.h)

      if (rx_frame.DLC < 4) {
        break;
      }

      extended_data.softwareVersion = u16(&rx_frame.data.u8[2]);

      break;

    case OBJ_HARDWARE_VERSION:  // 0x0C506E1D - Hardware version number (see comment in Sunwoda-ESS.h)

      if (rx_frame.DLC < 4) {
        break;
      }

      extended_data.hardwareVersion = u16(&rx_frame.data.u8[2]);

      break;

    case OBJ_CELL_BALANCE_0: {  // 0x0C50FF3C - Cell balancing status, 6 cells per frame

      if (rx_frame.DLC < 8) {
        break;
      }

      uint8_t start_index = rx_frame.data.u8[1];  // 0-based cell index of the first cell in this frame

      for (uint8_t i = 0; i < 6; i++) {
        uint8_t cell_index = start_index + i;
        if (cell_index >= MAX_AMOUNT_CELLS) {
          continue;
        }
        // 0: Idle, 1: Equalize charge, 2: Equalize discharge
        datalayer_battery->status.cell_balancing_status[cell_index] = (rx_frame.data.u8[2 + i] != 0);
      }

    } break;

    default:
      break;
  }
}

/*
Manual contactor control buttons - see the MANUAL CONTACTOR CONTROL comment in Sunwoda-ESS.h.
The identifiers here must match the BatteryCommand identifiers in advanced_battery_html.cpp - that
string is both the HTTP route and the key looked up here.
*/
const SunwodaBattery::VendorCommandDef SunwodaBattery::VENDOR_COMMANDS[] = {
    {"swNegativeClose", OBJ_CONTACTOR_CONTROL, 2, CONTACTOR_ENGAGE},
    {"swNegativeOpen", OBJ_CONTACTOR_CONTROL, 2, CONTACTOR_DISENGAGE},
    {"swPrechargeClose", OBJ_CONTACTOR_CONTROL, 1, CONTACTOR_ENGAGE},
    {"swPrechargeOpen", OBJ_CONTACTOR_CONTROL, 1, CONTACTOR_DISENGAGE},
    {"swMainClose", OBJ_CONTACTOR_CONTROL, 0, CONTACTOR_ENGAGE},
    {"swMainOpen", OBJ_CONTACTOR_CONTROL, 0, CONTACTOR_DISENGAGE},
};

const uint8_t SunwodaBattery::VENDOR_COMMAND_COUNT =
    sizeof(SunwodaBattery::VENDOR_COMMANDS) / sizeof(SunwodaBattery::VENDOR_COMMANDS[0]);

const SunwodaBattery::VendorCommandDef SunwodaBattery::CMD_DEBUG_MODE = {nullptr, OBJ_SYSTEM_CONTROL, 1, 8};
const SunwodaBattery::VendorCommandDef SunwodaBattery::CMD_MAIN_OPEN = {nullptr, OBJ_CONTACTOR_CONTROL, 0,
                                                                        CONTACTOR_DISENGAGE};
const SunwodaBattery::VendorCommandDef SunwodaBattery::CMD_PRECHARGE_OPEN = {nullptr, OBJ_CONTACTOR_CONTROL, 1,
                                                                             CONTACTOR_DISENGAGE};
const SunwodaBattery::VendorCommandDef SunwodaBattery::CMD_NEGATIVE_OPEN = {nullptr, OBJ_CONTACTOR_CONTROL, 2,
                                                                            CONTACTOR_DISENGAGE};

const SunwodaBattery::VendorCommandDef* SunwodaBattery::find_vendor_command(const char* identifier) {
  if (!identifier) {
    return nullptr;
  }
  for (uint8_t i = 0; i < VENDOR_COMMAND_COUNT; i++) {
    if (strcmp(VENDOR_COMMANDS[i].identifier, identifier) == 0) {
      return &VENDOR_COMMANDS[i];
    }
  }
  return nullptr;
}

bool SunwodaBattery::enqueue_vendor_command(const VendorCommandDef* cmd) {
  if (vendor_command_queue_count >= VENDOR_COMMAND_QUEUE_SIZE) {
    return false;
  }
  vendor_command_queue[vendor_command_queue_count++] = cmd;
  return true;
}

void SunwodaBattery::request_vendor_command(const char* identifier) {
  if (identifier && strcmp(identifier, CMD_OPEN_ALL) == 0) {
    open_all_contactors(true);  // the button: keep them open, the BCMU would otherwise restart itself
    return;
  }
  const VendorCommandDef* cmd = find_vendor_command(identifier);
  if (!cmd) {
    return;
  }
  if (cmd->value == CONTACTOR_ENGAGE) {
    // Never close into an active alarm or fault - check_auto_open() would only open it again.
    if (extended_data.alarmActive || extended_data.faultActive) {
      return;
    }
    // Contactor writes are only honoured in Debugging work mode, so switch to it first.
    if (vendor_command_queue_count + 2 > VENDOR_COMMAND_QUEUE_SIZE) {
      return;
    }
    enqueue_vendor_command(&CMD_DEBUG_MODE);
    manual_control = true;
  }
  enqueue_vendor_command(cmd);
}

void SunwodaBattery::open_all_contactors(bool hold_open) {
  vendor_command_queue_count = 0;  // drop anything pending, including closes
  if (hold_open) {
    // Debugging work mode leaves the BCMU in Stop; in normal mode it runs its start sequence again within
    // seconds. A power cycle returns it to normal mode.
    enqueue_vendor_command(&CMD_DEBUG_MODE);
  }
  enqueue_vendor_command(&CMD_MAIN_OPEN);
  enqueue_vendor_command(&CMD_PRECHARGE_OPEN);
  enqueue_vendor_command(&CMD_NEGATIVE_OPEN);
}

void SunwodaBattery::apply_charge_taper(unsigned long now) {
  // Some cell readings include ~20-35 mOhm of link resistance (module position 17), so they jump about
  // 30 mV per amp the moment charge current flows. Above ~7 A that crosses the BCMU's 3.60 V severe
  // limit within seconds and it opens; it then recovers and the cycle repeats. So the offered charge
  // current is regulated on the highest cell reading: start low, creep up while comfortably below the
  // target, back off quickly above it. Near full this also tapers the current like a CV phase.
  if (!extended_data.main_contactor_closed || extended_data.faultActive) {
    taper_cap_dA = TAPER_START_DA;  // next charge starts low again
    return;
  }
  if (packCurrent_dA < TAPER_CHARGING_DA) {
    taper_cap_dA = TAPER_START_DA;  // idle or discharging: do not let the cap creep up unobserved
    last_taper_step_ms = now;
  } else if (maxCellVoltage >= TAPER_TARGET_MV + TAPER_HARD_MARGIN_MV) {
    if (now - last_taper_cut_ms >= TAPER_CUT_INTERVAL_MS) {
      taper_cap_dA = taper_cap_dA / 2;
      last_taper_cut_ms = now;
    }
    last_taper_step_ms = now;
  } else if (maxCellVoltage > TAPER_TARGET_MV) {
    if (now - last_taper_cut_ms >= TAPER_CUT_INTERVAL_MS) {
      taper_cap_dA = taper_cap_dA > TAPER_CUT_DA ? taper_cap_dA - TAPER_CUT_DA : 0;
      last_taper_cut_ms = now;
    }
    last_taper_step_ms = now;
  } else if (maxCellVoltage + TAPER_STEP_MARGIN_MV < TAPER_TARGET_MV && now - last_taper_step_ms >= TAPER_STEP_MS) {
    if (taper_cap_dA < TAPER_MAX_DA) {
      taper_cap_dA += TAPER_STEP_DA;
    }
    last_taper_step_ms = now;
  }
  if (datalayer_battery->status.max_charge_current_dA > taper_cap_dA) {
    datalayer_battery->status.max_charge_current_dA = taper_cap_dA;
    datalayer_battery->status.max_charge_power_W = (uint32_t)taper_cap_dA * datalayer_battery->status.voltage_dV / 100;
  }
  extended_data.charge_taper_dA = taper_cap_dA;
}

void SunwodaBattery::check_auto_open() {
  bool any_closed = extended_data.negative_contactor_closed || extended_data.main_contactor_closed ||
                    extended_data.precharge_contactor_closed;
  if (!any_closed) {
    manual_control = false;  // everything open again: back to BCMU-managed operation
  }
  if (!manual_control || !(extended_data.alarmActive || extended_data.faultActive) || !any_closed) {
    return;
  }
  unsigned long now = millis();
  if (last_auto_open_ms != 0 && now - last_auto_open_ms < AUTO_OPEN_INTERVAL_MS) {
    return;
  }
  last_auto_open_ms = now;
  open_all_contactors();
}

void SunwodaBattery::autodetect_can_speed(unsigned long currentMillis) {
  if (!CAN_SPEED_AUTODETECT) {
    return;
  }
  if (last_bcmu_rx_ms != 0 && currentMillis - last_bcmu_rx_ms < CAN_SPEED_PROBE_MS) {
    return;  // BCMU heard recently at this speed
  }
  if (currentMillis - last_speed_switch_ms < CAN_SPEED_PROBE_MS) {
    return;  // give the current speed a full probe period
  }
  can_at_250k = !can_at_250k;
  CAN_Speed speed = can_at_250k ? CAN_Speed::CAN_SPEED_250KBPS : CAN_Speed::CAN_SPEED_500KBPS;
  change_can_speed(speed);
  last_speed_switch_ms = currentMillis;
  extended_data.can_speed_kbps = (uint16_t)speed;
  logging.printf("Sunwoda: no BCMU frames, trying %d kbit/s\n", (int)speed);
}

void SunwodaBattery::transmit_can(unsigned long currentMillis) {
  autodetect_can_speed(currentMillis);
  if (!bcmu_link_up(currentMillis)) {
    return;  // keep queued writes until the BCMU is heard at the current speed
  }

  // Queued object writes, one per call: ID 04 <us> 50 <object>, data C1 <subindex> <lo> <hi>.
  if (vendor_command_queue_count > 0) {
    const VendorCommandDef* cmd = vendor_command_queue[0];
    for (uint8_t i = 1; i < vendor_command_queue_count; i++) {
      vendor_command_queue[i - 1] = vendor_command_queue[i];
    }
    vendor_command_queue_count--;

    const uint8_t object = cmd->object;
    CAN_frame frame = {.FD = false,
                       .ext_ID = true,
                       .DLC = 4,
                       .ID = write_id(object),
                       .data = {(uint8_t)(MUX_FIRST | MUX_LAST | 1), cmd->subindex, (uint8_t)(cmd->value & 0xFF),
                                (uint8_t)(cmd->value >> 8), 0x00, 0x00, 0x00, 0x00}};
    transmit_can_frame(&frame);
  }

  // RTC time sync broadcast, one frame per button press. See ID_TIME_SYNC in Sunwoda-ESS.h.
  if (time_sync_requested) {
    time_sync_requested = false;
    transmit_can_frame(&SUNWODA_TIME_SYNC);
  }
}

void SunwodaBattery::request_time_sync(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                                       uint16_t ms_in_minute) {
  SUNWODA_TIME_SYNC.data.u8[0] = year & 0xFF;
  SUNWODA_TIME_SYNC.data.u8[1] = year >> 8;
  SUNWODA_TIME_SYNC.data.u8[2] = month;
  SUNWODA_TIME_SYNC.data.u8[3] = day;
  SUNWODA_TIME_SYNC.data.u8[4] = hour;
  SUNWODA_TIME_SYNC.data.u8[5] = minute;
  SUNWODA_TIME_SYNC.data.u8[6] = ms_in_minute & 0xFF;
  SUNWODA_TIME_SYNC.data.u8[7] = ms_in_minute >> 8;
  time_sync_requested = true;
}
