#include "TESLA-LEGACY-BATTERY.h"
#include <cstdint>
#include <cstring>  //For unit test
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"

inline const char* getBMSState(uint8_t index) {
  switch (index) {
    case 0:
      clear_event(EVENT_CONTACTOR_WELDED);
      clear_event(EVENT_CONTACTOR_OPEN);
      return "STANDBY";
    case 1:
      clear_event(EVENT_CONTACTOR_WELDED);
      clear_event(EVENT_CONTACTOR_OPEN);
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
      set_event(EVENT_CONTACTOR_OPEN, 0);
      return "FAULT";
    case 8:
      set_event(EVENT_CONTACTOR_WELDED, 0);
      return "WELD";
    case 15:
      return "SNA";
    default:
      return "UNKNOWN";
  }
}

void TeslaLegacyBattery::update_values() {
  //After values are mapped, we perform some safety checks, and do some serial printouts

  datalayer.battery.status.soh_pptt = BMS_CAC_min / 2316;  // uitgelezen minimale CAC / CAC bij nieuw (231,6 Ah)

  datalayer.battery.status.real_soc = (battery_soc_ui * 10);  //increase SOC range from 0-100.0 -> 100.00

  datalayer.battery.status.voltage_dV = (battery_volts * 10);  //One more decimal needed (370 -> 3700)

  datalayer.battery.status.current_dA = (battery_amps * 10);  //13.0A

  //Calculate the remaining Wh amount from SOC% and max Wh value.
  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  /*
    Following section is useful if we ever need to go back to manually set charge/discharge values

  // Define the allowed discharge power
  datalayer.battery.status.max_discharge_power_W = (battery_max_discharge_current * battery_volts);
  // Cap the allowed discharge power if higher than the maximum discharge power allowed
  if (datalayer.battery.status.max_discharge_power_W > datalayer.battery.status.override_discharge_power_W) {
    datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;
  }

  //The allowed charge power behaves strangely. We instead estimate this value
  if (battery_soc_ui > 990) {
    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
  } else if (battery_soc_ui >
             RAMPDOWN_SOC) {  // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        RAMPDOWNPOWERALLOWED * (1 - (battery_soc_ui - RAMPDOWN_SOC) / (1000.0 - RAMPDOWN_SOC));
    //If the cellvoltages start to reach overvoltage, only allow a small amount of power in
    //if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) {
    //  if (battery_cell_max_v > (MAX_CELL_VOLTAGE_LFP - FLOAT_START_MV)) {
    //    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
    //  }
    //} else {  //NCM/A
    //  if (battery_cell_max_v > (MAX_CELL_VOLTAGE_NCA_NCM - FLOAT_START_MV)) {
    //    datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
    //  }
    if (battery_cell_max_v > (MAX_CELL_VOLTAGE_MV - FLOAT_START_MV)) {
      datalayer.battery.status.max_charge_power_W = FLOAT_MAX_POWER_W;
    }
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }
  */

  datalayer.battery.status.max_charge_power_W = (datalayer.battery.status.voltage_dV * battery_max_charge_current);

  datalayer.battery.status.max_discharge_power_W =
      (datalayer.battery.status.voltage_dV * battery_max_discharge_current);

  datalayer.battery.status.temperature_min_dC = battery_min_temp;

  datalayer.battery.status.temperature_max_dC = battery_max_temp;

  datalayer.battery.status.cell_max_voltage_mV = battery_cell_max_v;

  datalayer.battery.status.cell_min_voltage_mV = battery_cell_min_v;
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
    case 0x252:  //Limit //594 BMS_powerAvailable:
      BMS_maxRegenPower = ((rx_frame.data.u8[1] << 8) |
                           rx_frame.data.u8[0]);  //0|16@1+ (0.01,0) [0|655.35] "kW"  //Example 4715 * 0.01 = 47.15kW
      BMS_maxDischargePower =
          ((rx_frame.data.u8[3] << 8) |
           rx_frame.data.u8[2]);  //16|16@1+ (0.013,0) [0|655.35] "kW"  //Example 2009 * 0.013 = 26.117???
      BMS_maxStationaryHeatPower =
          (((rx_frame.data.u8[5] & 0x03) << 8) |
           rx_frame.data.u8[4]);  //32|10@1+ (0.01,0) [0|10.23] "kW"  //Example 500 * 0.01 = 5kW
      BMS_hvacPowerBudget =
          (((rx_frame.data.u8[7] << 6) |
            ((rx_frame.data.u8[6] & 0xFC) >> 2)));  //50|10@1+ (0.02,0) [0|20.46] "kW"  //Example 1000 * 0.02 = 20kW?
      BMS_notEnoughPowerForHeatPump =
          ((rx_frame.data.u8[5] >> 2) & (0x01U));  //BMS_notEnoughPowerForHeatPump : 42|1@1+ (1,0) [0|1] ""  Receiver
      BMS_powerLimitState =
          (rx_frame.data.u8[6] &
           (0x01U));  //BMS_powerLimitsState : 48|1@1+ (1,0) [0|1] 0 "NOT_CALCULATED_FOR_DRIVE" 1 "CALCULATED_FOR_DRIVE"
      BMS_inverterTQF = ((rx_frame.data.u8[7] >> 4) & (0x03U));
      break;
    case 0x102:  // BMS_hvBusStatus
    {
      battery_volts = ((rx_frame.data.u8[1] << 8) | rx_frame.data.u8[0]) * 0.01;  // 0|16@1+ (0.01,0) [0|430] "V"
      int16_t raw_amps = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[2];
      battery_amps = raw_amps * 0.1;  // 16|16@1- (0.1,0) [-3276.7|3276.7] "A"
    } break;

    case 0x7E2:  // read CACmin to determine SOH
    {
      logging.println(rx_frame.data.u8[0]);
      if (rx_frame.data.u8[0] == 0) {
        BMS_CAC_min = (((rx_frame.data.u8[2] >> 4) & 0x0F) | ((rx_frame.data.u8[3]) << 4)) * 10000;
        logging.print("CACmin: ");
        logging.println(BMS_CAC_min);
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

      battery_BrickVoltageMax =
          (((rx_frame.data.u8[1] & (0x0F)) << 8) | (rx_frame.data.u8[0])) * 2;  //to datalayer_extended
      battery_cell_max_v = battery_BrickVoltageMax;
      battery_BrickVoltageMin =
          (((rx_frame.data.u8[5] & (0x0F)) << 8) | (rx_frame.data.u8[4])) * 2;  //to datalayer_extended
      battery_cell_min_v = battery_BrickVoltageMin;

      battery_BrickModelTMax = ((rx_frame.data.u8[3] * 0.5) - 40);  //to datalayer_extended
      battery_max_temp = battery_BrickModelTMax * 10;
      battery_BrickModelTMin = ((rx_frame.data.u8[7] * 0.5) - 40);
      //to datalayer_extended
      battery_min_temp = battery_BrickModelTMin * 10;
      break;

    case 0x6F2:
      static uint16_t volts;
      static uint8_t mux_zero_counter = 0u;
      static uint8_t mux_max = 0u;

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
            // The max index will be 2 + mux_max * 3 (see above), so "+ 1" for the number of cells
            datalayer.battery.info.number_of_cells = (4 * mux_max) + 4;
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
      battery_soc_ui = ((rx_frame.data.u8[1] & 0x03) << 8) | rx_frame.data.u8[0];  // 0|10@1+ (0.1,0) [0|102.2] "%"
      break;
    default:
      break;
  }
}

void TeslaLegacyBattery::transmit_can(unsigned long currentMillis) {
  //Send 1000ms message
  if (currentMillis - previousMillis1000 >= INTERVAL_1_S) {
    previousMillis1000 = currentMillis;

    transmit_can_frame(&TESLA_408);
    logging.println(getBMSState(battery_BMS_state));
  }

  if (!cellvoltagesRead) {
    return;  //All cellvoltages not read yet, do not proceed with contactor closing
  }

  if ((datalayer.system.status.inverter_allows_contactor_closing) && (datalayer.battery.status.bms_status != FAULT)) {
    if (currentMillis - previousMillis100 >= INTERVAL_100_MS) {
      previousMillis100 = currentMillis;

      transmit_can_frame(&TESLA_21C);
      transmit_can_frame(&TESLA_25C);
      transmit_can_frame(&TESLA_2C8);
      transmit_can_frame(&TESLA_20E);
    }
  }
}

void TeslaLegacyBattery::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.number_of_cells = 14;
  datalayer.battery.info.max_design_voltage_dV = MAX_PACK_VOLTAGE_100_DV;  //Startup in wide range
  datalayer.battery.info.min_design_voltage_dV = MIN_PACK_VOLTAGE_60_DV;   //Autodetect later
  datalayer.battery.info.max_cell_voltage_mV = MAX_CELL_VOLTAGE_MV;
  datalayer.battery.info.min_cell_voltage_mV = MIN_CELL_VOLTAGE_MV;
  datalayer.battery.info.max_cell_voltage_deviation_mV = MAX_CELL_DEVIATION_MV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
