#include "ENNOID-BMS.h"
#include "../battery/BATTERIES.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"

void EnnoidBms::update_values() {

  datalayer.battery.status.real_soc = SOC;

  datalayer.battery.status.remaining_capacity_Wh = static_cast<uint32_t>(
      (static_cast<double>(datalayer.battery.status.real_soc) / 10000) * datalayer.battery.info.total_capacity_Wh);

  datalayer.battery.status.soh_pptt = SOH;

  datalayer.battery.status.voltage_dV = (uint16_t)lroundf(packVoltage * 10.0f);

  datalayer.battery.status.current_dA = (int16_t)lroundf(packCurrent * 10.0f);

  // Charge power is manually set
  if (datalayer.battery.status.real_soc > 9900) {
    datalayer.battery.status.max_charge_power_W = MAX_CHARGE_POWER_WHEN_TOPBALANCING_W;
  } else if (datalayer.battery.status.real_soc > RAMPDOWN_SOC) {
    // When real SOC is between RAMPDOWN_SOC-99%, ramp the value between Max<->0
    datalayer.battery.status.max_charge_power_W =
        datalayer.battery.status.override_charge_power_W *
        (1 - (datalayer.battery.status.real_soc - RAMPDOWN_SOC) / (10000.0 - RAMPDOWN_SOC));
  } else {  // No limits, max charging power allowed
    datalayer.battery.status.max_charge_power_W = datalayer.battery.status.override_charge_power_W;
  }

  // Discharge power is manually set
  datalayer.battery.status.max_discharge_power_W = datalayer.battery.status.override_discharge_power_W;

  datalayer.battery.status.temperature_min_dC = tBms_cC;

  datalayer.battery.status.temperature_max_dC = tBms_cC - 1;

  datalayer.battery.info.number_of_cells = numberOfCells;  // 1-192S

  datalayer.battery.status.cell_max_voltage_mV = cellVoltageMax_mV;

  datalayer.battery.status.cell_min_voltage_mV = cellVoltageLow_mV;

  memcpy(datalayer.battery.status.cell_voltages_mV, cellVoltages_mV,
         datalayer.battery.info.number_of_cells * sizeof(uint16_t));

  datalayer.battery.status.total_discharged_battery_Wh = (int32_t)lroundf(totalDischargeWh);

  datalayer.battery.status.total_charged_battery_Wh = (int32_t)lroundf(totalChargeWh);
}

static inline uint16_t be_u16(const uint8_t* d) {
  return (uint16_t)((d[0] << 8) | d[1]);
}

static inline int16_t be_i16(const uint8_t* d) {
  return (int16_t)be_u16(d);
}

static inline float be_f32(const uint8_t* d) {
  uint32_t raw = ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) | ((uint32_t)d[2] << 8) | (uint32_t)d[3];
  float f;
  memcpy(&f, &raw, sizeof(f));
  return f;
}

void EnnoidBms::handle_incoming_can_frame(CAN_frame rx_frame) {

  switch (rx_frame.ID) {
    case 0x260a:
    case 0x260:
      packVoltage = be_f32(&rx_frame.data.u8[0]);
      break;

    case 0x270a:
    case 0x270:
      packCurrent = be_f32(&rx_frame.data.u8[0]);
      break;

    case 0x280a:
    case 0x280:
      // All zeros after BMS restart/counter reset until current flows
      dischargeAh = be_f32(&rx_frame.data.u8[0]);
      dischargeWh = be_f32(&rx_frame.data.u8[4]);
      break;

    case 0x290a:
    case 0x290: {
      uint8_t seq = rx_frame.data.u8[0];  // 0, 3, 6, ...
      numberOfCells = rx_frame.data.u8[1];
      for (uint8_t i = 0; i < 3; i++) {
        uint16_t idx = seq + i;
        if (idx < numberOfCells && idx < 180) {
          cellVoltages_mV[idx] = be_u16(&rx_frame.data.u8[2 + i * 2]);
        }
      }
      break;
    }

    case 0x2a0a:
    case 0x2a0:  //Unclear if this frame exists
      numberOfCells = rx_frame.data.u8[0];
      break;

    case 0x2b0a:
    case 0x2b0: {
      uint8_t seq = rx_frame.data.u8[0];
      numberOfTempSensors = rx_frame.data.u8[1];
      for (uint8_t i = 0; i < 3; i++) {
        uint16_t idx = seq + i;
        if (idx < numberOfTempSensors && idx < 50) {
          temperatures_cC[idx] = be_i16(&rx_frame.data.u8[2 + i * 2]);
        }
      }
      break;
    }

    case 0x2c0a:
    case 0x2c0:
      tBms_cC = be_i16(&rx_frame.data.u8[0]);
      unknown2C0 = be_i16(&rx_frame.data.u8[4]);
      break;

    case 0x2d0a:
    case 0x2d0:
      cellVoltageLow_mV = be_u16(&rx_frame.data.u8[0]);
      cellVoltageMax_mV = be_u16(&rx_frame.data.u8[2]);
      SOC = rx_frame.data.u8[4] * 40;
      SOH = rx_frame.data.u8[5] * 40;
      //tBattHi = rx_frame.data.u8[6];
      //BitF = rx_frame.data.u8[7];
      break;

    case 0x350a:
    case 0x350:
      totalChargeAh = be_f32(&rx_frame.data.u8[0]);
      totalChargeWh = be_f32(&rx_frame.data.u8[4]);
      break;

    case 0x360a:
    case 0x360:
      totalDischargeAh = be_f32(&rx_frame.data.u8[0]);
      totalDischargeWh = be_f32(&rx_frame.data.u8[4]);
      break;

    default:
      break;
  }
}

void EnnoidBms::transmit_can(unsigned long currentMillis) {
  //No periodic sending for this protocol
}

void EnnoidBms::setup(void) {  // Performs one time setup at startup
  strncpy(datalayer.system.info.battery_protocol, Name, 63);
  datalayer.system.info.battery_protocol[63] = '\0';
  datalayer.battery.info.max_design_voltage_dV = user_selected_max_pack_voltage_dV;
  datalayer.battery.info.min_design_voltage_dV = user_selected_min_pack_voltage_dV;
  datalayer.battery.info.max_cell_voltage_mV = user_selected_max_cell_voltage_mV;
  datalayer.battery.info.min_cell_voltage_mV = user_selected_min_cell_voltage_mV;
  datalayer.system.status.battery_allows_contactor_closing = true;
}
