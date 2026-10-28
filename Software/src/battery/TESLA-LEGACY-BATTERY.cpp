#include "TESLA-LEGACY-BATTERY.h"
#include <cstdint>
#include <cstring>  //For unit test
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "TESLA-BATTERY.h"

/* `index` is the BMS state code, `battery` the pack this driver instance drives: the
   contactor events it raises have to name the pack, and a free function has no
   battery_index of its own. */
inline const char* getBMSState(uint8_t index, uint8_t battery) {
  switch (index) {
    case 0:
      clear_event(EVENT_CONTACTOR_WELDED, battery);
      clear_event(EVENT_CONTACTOR_OPEN, battery);
      return "STANDBY";
    case 1:
      clear_event(EVENT_CONTACTOR_WELDED, battery);
      clear_event(EVENT_CONTACTOR_OPEN, battery);
      return "DRIVE";
    case 2:
      return "SUPPORT";
    case 3:
      return "CHARGE";
    case 4:
      return "FASTCHARGE";
    case 5:
      return "CHARGERVOLTAGE";
    case 6:
      return "CLEAR_FAULT";
    case 7:
      set_event(EVENT_CONTACTOR_OPEN, 0, battery);
      return "FAULT";
    case 8:
      set_event(EVENT_CONTACTOR_WELDED, 0, battery);
      return "WELD";
    case 15:
      return "SNA";
    default:
      return "UNKNOWN";
  }
}

void TeslaLegacyBattery::update_values() {

  switch (battery_hwID) {
    case 0:  //Not read yet
      break;
    case 27:  //60kWh
    case 39:
    case 42:
    case 50:
      datalayer.battery.info.total_capacity_Wh = 60000;
      break;
    case 61:  //70kWh
    case 65:
    case 67:
      datalayer.battery.info.total_capacity_Wh = 70000;
      break;
    case 74:  //75kWh
    case 80:
    case 84:
      datalayer.battery.info.total_capacity_Wh = 75000;
      break;
    case 24:  //85kWh
    case 31:
    case 41:
    case 46:
    case 49:
    case 57:
    case 60:
    case 64:
    case 69:
    case 70:
    case 86:
      datalayer.battery.info.total_capacity_Wh = 85000;
      break;
    case 68:  //90kWh
    case 71:
    case 73:
    case 75:
    case 76:
    case 77:
    case 81:
    case 82:
    case 83:
    case 85:
    case 94:
      datalayer.battery.info.total_capacity_Wh = 90000;
      break;
    case 79:  //100kWh
    case 89:
      datalayer.battery.info.total_capacity_Wh = 100000;
      break;
    default:  //Unknown hwID. Raise event
      set_event(EVENT_BATTERY_VALUE_UNAVAILABLE, battery_hwID, battery_index);
      break;
  }

  datalayer.battery.status.real_soc = (battery_soc_ui * 10);  //increase SOC range from 0-100.0 -> 100.00

  // Seconds without a valid UI SOC, counted while the pack is on the bus. EVENT_SOC_UNAVAILABLE is a WARNING.
  if (soc_ui_fresh) {
    soc_ui_fresh = false;
    seconds_without_soc = 0;
    clear_event(EVENT_SOC_UNAVAILABLE, battery_index);
  } else if (datalayer.battery.status.CAN_battery_still_alive > 0) {
    if (seconds_without_soc < INT16_MAX) {
      seconds_without_soc++;
    }
    if (seconds_without_soc >= SocUnavailableDelay_s) {
      set_event(EVENT_SOC_UNAVAILABLE, seconds_without_soc, battery_index);
    }
  }

  datalayer.battery.status.voltage_dV = (battery_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery.status.current_dA = (battery_amps * 10);  //13.0A

  TeslaBattery::hold_energy(nominal_full_hold, nominal_full_x10_received,
                            TeslaBattery::energy_x10_plausible_10bit(nominal_full_x10_received));
  datalayer_extended.tesla.nominal_full_held_x10 = nominal_full_hold.value;

  // Nominal energy remaining in Wh, SNA (0x7FF) and 0 (not received yet) are 0 = no data
  const uint32_t received_remaining_Wh =
      nominal_remaining_x10_received >= 0x7FF ? 0 : nominal_remaining_x10_received * 100UL;
  TeslaBattery::hold_energy(nominal_remaining_hold, received_remaining_Wh,
                            TeslaBattery::nominal_remaining_plausible(received_remaining_Wh, nominal_full_hold.value));
  datalayer.battery.status.remaining_capacity_Wh =
      TeslaBattery::remaining_Wh(datalayer.battery.status.real_soc, nominal_full_hold.value,
                                 nominal_remaining_hold.value, datalayer.battery.info.total_capacity_Wh);

  // SOH is the estimate: nominal full pack energy (0x382) over the configured capacity, the Model 3/Y calculation.
  // The reference is the capacity configured for the hardware ID, NOT the real capacity when new (the S/X does not send
  // it). A new pack whose real capacity is below the configured one shows a higher SOH than the true one, for example a
  // new 85 kWh pack reading 81 kWh nominal shows 95 %. TODO: read the capacity when new from the pack (a frame with it
  // is not known yet), the main page shows this configured value as "Total capacity when new".
  datalayer_extended.tesla.battery_beginning_of_life = datalayer.battery.info.total_capacity_Wh / 100;  // 0.1 kWh
  datalayer_extended.tesla.soh_estimate_known = TeslaBattery::apply_soh_estimate(
      datalayer.battery.status, nominal_full_hold.value, datalayer_extended.tesla.battery_beginning_of_life);

  datalayer.battery.status.max_charge_power_W = (datalayer.battery.status.voltage_dV * battery_max_charge_current) / 10;

  datalayer.battery.status.max_discharge_power_W =
      (datalayer.battery.status.voltage_dV * battery_max_discharge_current) / 10;

  datalayer.battery.status.temperature_min_dC = battery_BrickModelTMin;

  datalayer.battery.status.temperature_max_dC = battery_BrickModelTMax;

  datalayer.battery.status.cell_max_voltage_mV = battery_BrickVoltageMax;

  datalayer.battery.status.cell_min_voltage_mV = battery_BrickVoltageMin;

  //Check safeties
  if (battery_BMS_isolationResistance < 100) {  //TODO: Exact limit unknown. Working pack reports 165kOhm
    set_event(EVENT_BATTERY_ISOLATION, battery_BMS_isolationResistance);
  } else {
    clear_event(EVENT_BATTERY_ISOLATION);
  }
}

void TeslaLegacyBattery::handle_incoming_can_frame(CAN_frame rx_frame) {
  static uint8_t mux = 0;
  switch (rx_frame.ID) {
    case 0x212:  // 530 BMS_status: 5
      battery_BMS_rapidDCLinkDchgRequest = (rx_frame.data.u8[0] & 0x01U);
      battery_BMS_chargingActiveOrTrans = ((rx_frame.data.u8[0] >> 2) & 0x01U);
      battery_BMS_dcdcEnableOn = ((rx_frame.data.u8[0] >> 3) & 0x01U);
      battery_BMS_hvacPowerRequest = ((rx_frame.data.u8[0] >> 4) & 0x01U);
      battery_BMS_notEnoughPowerForDrive = ((rx_frame.data.u8[0] >> 5) & 0x01U);
      battery_BMS_hvilStatus = ((rx_frame.data.u8[0] >> 6) & 0x01U);
      battery_BMS_activeHeatingWorthwhile = ((rx_frame.data.u8[0] >> 7) & 0x01U);

      battery_BMS_highestFaultCategory = (rx_frame.data.u8[1] & 0x07U);
      battery_BMS_hvilOn = ((rx_frame.data.u8[1] >> 3) & 0x01U);

      battery_BMS_contactorState = ((rx_frame.data.u8[2] >> 0) & 0x0FU);
      battery_BMS_state = ((rx_frame.data.u8[2] >> 4) & 0x0FU);

      battery_BMS_isolationResistance = (rx_frame.data.u8[3] * 20);

      battery_BMS_contactorStateFC = ((rx_frame.data.u8[4] >> 0) & 0x03U);
      battery_BMS_cpChargeStatus = ((rx_frame.data.u8[4] >> 2) & 0x07U);
      battery_BMS_okToShipByAir = ((rx_frame.data.u8[4] >> 6) & 0x01U);
      battery_BMS_okToShipByLand = ((rx_frame.data.u8[4] >> 7) & 0x01U);
      break;
    case 0x102:  // BMS_hvBusStatus
    {
      battery_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  // 0|16@1+ (0.01,0) [0|430] "V"
      int16_t raw_amps = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      battery_amps = raw_amps * 0.1;  // 16|16@1- (0.1,0) [-3276.7|3276.7] "A"
    } break;
    case 0x7E2:  // CAC (page 0) and brick SOC (page 7), web page only
    {
      if (rx_frame.data.u8[0] == 0) {
        // CAC avg (bit 8), min (bit 20), max (bit 32), 12 bits in 0.1 Ah. The detail page shows them raw, the main page
        // uses them when plausible: 20-400 Ah, min <= avg <= max.
        const uint16_t cac_avg_x10 = tesla_le_bits(rx_frame.data.u8, 8, 12);
        const uint16_t cac_min_x10 = tesla_le_bits(rx_frame.data.u8, 20, 12);
        const uint16_t cac_max_x10 = tesla_le_bits(rx_frame.data.u8, 32, 12);
        datalayer_extended.tesla.cac_raw_seen[0] = true;
        datalayer_extended.tesla.cac_raw_avg_x10[0] = cac_avg_x10;
        datalayer_extended.tesla.cac_raw_min_x10[0] = cac_min_x10;
        datalayer_extended.tesla.cac_raw_max_x10[0] = cac_max_x10;
        if (cac_min_x10 >= 200 && cac_min_x10 <= cac_avg_x10 && cac_avg_x10 <= cac_max_x10 && cac_max_x10 <= 4000) {
          datalayer_extended.tesla.cac_min_x10 = cac_min_x10;
          datalayer_extended.tesla.cac_avg_x10 = cac_avg_x10;
          datalayer_extended.tesla.cac_max_x10 = cac_max_x10;
          datalayer_extended.tesla.cac_valid = true;
        }
      }
      if (rx_frame.data.u8[0] == 7) {
        // Page 7: brick SOC max (bit 8) and min (bit 18), 10 bits, 0.1 %, 1023 = n/a. Same fields as the 3/Y, raw
        // (the main page validates them).
        datalayer_extended.tesla.battery_soc_max = tesla_le_bits(rx_frame.data.u8, 8, 10);
        datalayer_extended.tesla.battery_soc_min = tesla_le_bits(rx_frame.data.u8, 18, 10);
      }
    } break;
    case 0x3D2:  //TotalChargeDischarge:
      battery_total_discharge = ((rx_frame.data.u8[3] << 24) | (rx_frame.data.u8[2] << 16) |
                                 (rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]);
      //0|32@1+ (0.001,0) [0|4294970] "kWh"
      battery_total_charge = ((rx_frame.data.u8[7] << 24) | (rx_frame.data.u8[6] << 16) | (rx_frame.data.u8[5] << 8) |
                              rx_frame.data.u8[4]);
      //32|32@1+ (0.001,0) [0|4294970] "kWh"
      break;
    case 0x332:  //min/max hist values //BattBrickMinMax:
      cellvoltagesRead = true;
      datalayer_extended.tesla.temperature_seen = true;  // the main page shows "no data" until this frame arrives

      battery_BrickVoltageMax = (((rx_frame.data.u8[1] & (0x0F)) << 8) | (rx_frame.data.u8[0])) * 2;
      battery_BrickVoltageMin = (((rx_frame.data.u8[5] & (0x0F)) << 8) | (rx_frame.data.u8[4])) * 2;
      battery_BrickModelTMax = rx_frame.data.u8[3] * 5 - 400;  // 0.5 C per count, offset -40 C, stored in 0.1 C
      battery_BrickModelTMin = rx_frame.data.u8[7] * 5 - 400;
      break;
    case 0x5D2:
      if (rx_frame.data.u8[0] == 0x0A) {
        battery_hwID = rx_frame.data.u8[4] + rx_frame.data.u8[5];
      }
      break;
    case 0x6F2:
      uint16_t volts;

      mux = rx_frame.data.u8[0];

      if (mux <= 23) {
        volts = (((rx_frame.data.u8[2] & 0x3F) << 8) | rx_frame.data.u8[1]) * 0.30517578125;
        datalayer.battery.status.cell_voltages_mV[mux * 4] = volts;

        volts =
            (((rx_frame.data.u8[4] & 0x0F) << 10) | (rx_frame.data.u8[3] << 2) | (rx_frame.data.u8[2] & 0xC0) >> 6) *
            0.30517578125;
        datalayer.battery.status.cell_voltages_mV[mux * 4 + 1] = volts;

        volts =
            (((rx_frame.data.u8[6] & 0x03) << 12) | (rx_frame.data.u8[5] << 4) | (rx_frame.data.u8[4] & 0xF0) >> 4) *
            0.30517578125;
        datalayer.battery.status.cell_voltages_mV[mux * 4 + 2] = volts;

        volts = ((rx_frame.data.u8[7] << 6) | (rx_frame.data.u8[6] & 0xFC) >> 2) * 0.30517578125;
        datalayer.battery.status.cell_voltages_mV[mux * 4 + 3] = volts;

        mux_max = (mux > mux_max) ? mux : mux_max;
        if (mux_zero_counter < 2 && mux == 0u) {
          mux_zero_counter++;
          if (mux_zero_counter == 2u) {
            // One full mux cycle seen: the series brick count is the last slot that
            // reported a voltage (a pack whose count isn't a multiple of 4, e.g. 83S,
            // leaves the tail of the last frame at 0). Fall back to the whole frame span.
            uint16_t n = (4 * mux_max) + 4;
            while (n > 0 && datalayer.battery.status.cell_voltages_mV[n - 1] == 0) {
              n--;
            }
            if (n == 0) {
              n = (4 * mux_max) + 4;
            }
            apply_series_limits(n);
            mux_zero_counter++;
          }
        }
      }
      break;
    case 0x202:  // BMS_driveLimits (CAN-ID 0x202) - Legacy batteries
      battery_bms_min_voltage =
          ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  // 0|16@1+ (0.01,0) [0|430] "V"
      battery_bms_max_voltage =
          ((rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2]) * 0.01;  // 16|16@1+ (0.01,0) [0|430] "V"
      battery_max_charge_current =
          (((rx_frame.data.u8[5] & 0x3F) << 8) | rx_frame.data.u8[4]) * 0.1;  // 32|14@1+ (0.1,0) [0|1638.2] "A"
      battery_max_discharge_current =
          (((rx_frame.data.u8[7] & 0x3F) << 8) | rx_frame.data.u8[6]) * 0.128;  // 48|14@1+ (0.128,0) [0|2096.896] "A"
      break;

    case 0x302:                                                            // BMS_socStatus (Nieuwe CAN-ID)
      datalayer.battery.status.CAN_battery_still_alive = CAN_STILL_ALIVE;  //We are getting CAN messages from the BMS
      {
        // 10 bit fields in 0.1 %: SOC min (bit 0), SOC UI (bit 10). The pack SOC is the UI SOC (like the 3/Y). 1023 =
        // n/a: the last SOC is held (the pack sends 1023 for about 5 s). SOC min is only the initial value, until a
        // valid UI SOC has been seen: 0 would zero the discharge limit (battery empty) at startup.
        const uint16_t soc_min_x10 = tesla_le_bits(rx_frame.data.u8, 0, 10);
        const uint16_t soc_ui_x10 = tesla_le_bits(rx_frame.data.u8, 10, 10);
        datalayer_extended.tesla.battery_soc_min_0x302 = soc_min_x10;  // raw, detail page
        datalayer_extended.tesla.battery_soc_ui = soc_ui_x10;          // raw (live), the held one is battery_soc_ui
        if (soc_ui_x10 <= 1000) {
          battery_soc_ui = soc_ui_x10;
          soc_ui_seen = true;
          soc_ui_fresh = true;
        } else if (!soc_ui_seen && soc_min_x10 <= 1000) {
          battery_soc_ui = soc_min_x10;
        }
      }
      break;
    case 0x382:  // Nominal full pack energy, bits 0-9 in 0.1 kWh, the SOH estimate input (3/Y field)
    {
      // 38.0 to 150.0 kWh is plausible. The pack sends 1023 for about 5 s while it is not available: update_values()
      // holds the last plausible value for 30 s.
      nominal_full_x10_received = tesla_le_bits(rx_frame.data.u8, 0, 10);
      datalayer_extended.tesla.nominal_0x382_seen = true;
      datalayer_extended.tesla.battery_nominal_full_pack_energy = nominal_full_x10_received;  // raw, detail page
      // Nominal energy remaining, bit 11, 11 bits, 0.1 kWh (the remaining capacity uses it, see update_values).
      // This is the layout of the 2012-2020 pack that was measured, the energy fields are 11 bits wide like the
      // pre-2021 3/Y 0x352. The public 10 bit layout (wk057) differs and is not detected.
      nominal_remaining_x10_received = tesla_le_bits(rx_frame.data.u8, 11, 11);
      datalayer_extended.tesla.battery_nominal_energy_remaining = nominal_remaining_x10_received;  // raw, detail page
    } break;
    case 0x612: {
      uint8_t service = rx_frame.data.u8[1];
      bms_uds_response = service;
      bms_uds_response_received = true;
      if (service == 0x7F) {
        uint8_t rejected = rx_frame.data.u8[2];
        uint8_t error = rx_frame.data.u8[3];
        logging.printf("WARN: BMS UDS REJECTED svc=0x%02X err=0x%02X\n", rejected, error);
      } else {
        logging.printf("INFO: BMS UDS OK svc=0x%02X\n", service);
      }
    } break;
    default:
      break;
  }
}

void TeslaLegacyBattery::transmit_can(unsigned long currentMillis) {

  //Send 1000ms message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    transmit_can_frame(&TESLA_408);
    logging.println(getBMSState(battery_BMS_state, battery_index));
  }

  if (user_requests_bms_reset) {
    stateMachineBMSReset = 0;
    user_requests_bms_reset = false;
    bms_uds_response_received = false;
    bms_uds_response = 0;
    bms_uds_timeout = currentMillis;
    logging.println("INFO: Starting BMS reset sequence (Legacy)");
  }

  if (stateMachineBMSReset != 0xFF) {
    static unsigned long lastUDS = 0;

    if (bms_uds_response_received) {
      if (bms_uds_response == 0x7F) {
        logging.printf("WARN: UDS step %d rejected (0x7F), aborting\n", stateMachineBMSReset);
        stateMachineBMSReset = 0xFF;
        bms_uds_response_received = false;
        return;
      }
      bms_uds_response_received = false;
      lastUDS = currentMillis + 80;
      bms_uds_timeout = currentMillis;
    } else if (currentMillis - lastUDS < 130) {
      return;
    } else if (currentMillis - bms_uds_timeout > 3000) {
      logging.printf("WARN: UDS step %d timeout, aborting\n", stateMachineBMSReset);
      stateMachineBMSReset = 0xFF;
      return;
    }

    lastUDS = currentMillis;

    switch (stateMachineBMSReset) {
      case 0:
        TESLA_602.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[0]: Diagnostic Session");
        stateMachineBMSReset = 1;
        break;
      case 1:
        TESLA_602.data = {0x02, 0x27, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[1]: Security Access seed request");
        stateMachineBMSReset = 2;
        break;
      case 2:
        TESLA_602.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[2]: Flow Control");
        stateMachineBMSReset = 3;
        break;
      case 3:
        TESLA_602.data = {0x10, 0x12, 0x27, 0x06, 0x35, 0x34, 0x37, 0x36};
        transmit_can_frame(&TESLA_602);
        stateMachineBMSReset = 4;
        break;
      case 4:
        TESLA_602.data = {0x21, 0x31, 0x30, 0x33, 0x32, 0x3D, 0x3C, 0x3F};
        transmit_can_frame(&TESLA_602);
        stateMachineBMSReset = 5;
        break;
      case 5:
        TESLA_602.data = {0x22, 0x3E, 0x39, 0x38, 0x3B, 0x3A, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[5]: Security Access key sent");
        stateMachineBMSReset = 6;
        break;
      case 6:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x01, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[6]: BMS_w026 reset");
        stateMachineBMSReset = 7;
        break;
      case 7:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x0A, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[7]: BMS_f026 reset");
        stateMachineBMSReset = 8;
        break;
      case 8:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x02, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[8]: BMS_f023 reset");
        stateMachineBMSReset = 9;
        break;
      case 9:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x04, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[9]: BMS_f152 reset");
        stateMachineBMSReset = 10;
        break;
      case 10:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x0D, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[10]: BMS_f107 reset");
        stateMachineBMSReset = 11;
        break;
      case 11:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x06, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[11]: BMS_w161 stress reset");
        stateMachineBMSReset = 12;
        break;
      case 12:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x05, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[12]: BMS_f153 reset");
        stateMachineBMSReset = 13;
        break;
      case 13:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x02, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[13]: BMS_f163 reset");
        stateMachineBMSReset = 14;
        break;
      case 14:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x07, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[14]: BMS WOT reset");
        stateMachineBMSReset = 15;
        break;
      case 15:
        TESLA_602.data = {0x04, 0x31, 0x01, 0x04, 0x0C, 0x00, 0x00, 0x00};
        transmit_can_frame(&TESLA_602);
        logging.println("UDS[15]: BMS_u029 reset");
        stateMachineBMSReset = 0xFF;
        logging.println("INFO: BMS reset sequence complete (Legacy)");
        break;
      default:
        stateMachineBMSReset = 0xFF;
        break;
    }
    return;
  }

  if (!cellvoltagesRead) {
    return;  //All cellvoltages not read yet, do not proceed with contactor closing
  }

  if ((datalayer.system.status.inverter_allows_contactor_closing) && (datalayer.system.status.system_status != FAULT)) {
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      transmit_can_frame(&TESLA_21C);
      transmit_can_frame(&TESLA_25C);
      transmit_can_frame(&TESLA_2C8);
      transmit_can_frame(&TESLA_20E);
    }
  }
}

// Pack design limits follow the series brick count actually reported by the BMS
// (96S full packs, 84S/83S on early/smaller packs) instead of a fixed 300-500 V,
// which blocks discharge far too early on an 84S/83S pack and never trips on a
// full 96S one (its real ceiling is 403.2 V).
void TeslaLegacyBattery::apply_series_limits(uint16_t series_cells) {
  datalayer.battery.info.number_of_cells = series_cells;
  datalayer.battery.info.min_design_voltage_dV = (uint32_t)series_cells * MIN_CELL_VOLTAGE_MV / 100;
  datalayer.battery.info.max_design_voltage_dV = (uint32_t)series_cells * MAX_CELL_VOLTAGE_MV / 100;
  logging.printf("Tesla legacy: %u series bricks -> pack limits %u.%u-%u.%u V\n", series_cells,
                 datalayer.battery.info.min_design_voltage_dV / 10, datalayer.battery.info.min_design_voltage_dV % 10,
                 datalayer.battery.info.max_design_voltage_dV / 10, datalayer.battery.info.max_design_voltage_dV % 10);
}

void TeslaLegacyBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 14;
  // Defaults until the brick count is known: apply_series_limits() replaces them
  // after the first full 0x6F2 cycle (a few seconds). Until then a pack below
  // 300 V simply can't discharge yet; cell limits apply throughout.
  datalayer.battery.info.max_design_voltage_dV = STARTUP_MAX_PACK_VOLTAGE_DV;
  datalayer.battery.info.min_design_voltage_dV = STARTUP_MIN_PACK_VOLTAGE_DV;
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
  // Raw SOC fields: 1023 = not received yet (the detail page shows N/A)
  datalayer_extended.tesla.battery_soc_ui = 1023;
  datalayer_extended.tesla.battery_soc_min_0x302 = 1023;
  datalayer_extended.tesla.battery_soc_min = 1023;
  datalayer_extended.tesla.battery_soc_max = 1023;
}
