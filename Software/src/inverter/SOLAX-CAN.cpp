#include "SOLAX-CAN.h"
#include <Arduino.h>
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "../inverter/INVERTERS.h"

// __builtin_bswap64 needed to convert to ESP32 little endian format
// Byte[4] defines the requested contactor state: 1 = Closed , 0 = Open
#define Contactor_Open_Payload __builtin_bswap64(0x0200010000000000)
#define Contactor_Close_Payload __builtin_bswap64(0x0200010001000000)

// ---------------------------------------------------------------------------------------
// SolaX assumes a fixed number of cells per configured module. Derived from the published
// type table, not guessed: the type-131 row for 4 modules reads 179.2 / 204.8 / 233.6 V
// (min / nominal / max). Per module that is 44.8 / 51.2 / 58.4 V, and per 16 cells it lands
// on exactly 2.800 / 3.200 / 3.650 V - the textbook LFP figures. No other cell count gives
// round numbers. The HS25/HS36 stacks confirm it independently: SolaX lists them as 3-13
// modules spanning 153.6-665.6 V nominal, which is 51.2 V per module either way.
#define SOLAX_CELLS_PER_MODULE 16

// ---------------------------------------------------------------------------------------
// Express cell voltages in terms of the cell count SolaX infers from the module setting,
// rather than the pack's real cell count.
//
// This is not cosmetic. SolaX believes it is talking to LFP cells with a 3.65 V ceiling. An
// AKASOL pack has 180 NMC cells rated to 4.2 V, while type 89 with 13 modules makes SolaX
// assume 208 cells. Sending the real per-cell figure works at low state of charge and then
// fails higher up: the real cells cross 3.65 V at a pack voltage of only 657 V, and reach
// 4.2 V at the pack's 756 V maximum - reported to an inverter that treats anything above
// 3.65 V as a cell overvoltage.
//
// Dividing the reported pack voltage by SolaX's own assumed cell count keeps the per-cell
// figure inside the window it expects across the whole range, while the real measured spread
// between highest and lowest cell is preserved. Set to 0 only if the battery's real cell
// count and chemistry match what the configured type implies.
#define SOLAX_REMAP_CELLS_TO_MODULE_COUNT 1
// ---------------------------------------------------------------------------------------

void SolaxInverter::update_values() {
  // If not receiving any communication from the inverter, open contactors and
  // return to battery announce state
  if (millis() - LastFrameTime >= INTERVAL_2_S &&
      configured_contactor_mode == inverter_contactor_mode_enum::NoWorkaround) {
    datalayer.system.status.inverter_allows_contactor_closing = false;
    if (STATE != BATTERY_ANNOUNCE) {
      logging.println("[Solax] Timeout, opening contactor");
      STATE = BATTERY_ANNOUNCE;
    }
  }
  //Calculate the required values
  temperature_average =
      ((datalayer.battery.status.temperature_max_dC + datalayer.battery.status.temperature_min_dC) / 2);

  //Put the values into the CAN messages
  //BMS_Limits
  SOLAX_1872.data.u8[0] = (uint8_t)datalayer.battery.info.max_design_voltage_dV;
  SOLAX_1872.data.u8[1] = (datalayer.battery.info.max_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[2] = (uint8_t)datalayer.battery.info.min_design_voltage_dV;
  SOLAX_1872.data.u8[3] = (datalayer.battery.info.min_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[4] = (uint8_t)datalayer.battery.status.max_charge_current_dA;
  SOLAX_1872.data.u8[5] = (datalayer.battery.status.max_charge_current_dA >> 8);
  SOLAX_1872.data.u8[6] = (uint8_t)datalayer.battery.status.max_discharge_current_dA;
  SOLAX_1872.data.u8[7] = (datalayer.battery.status.max_discharge_current_dA >> 8);

  //BMS_PackData
  SOLAX_1873.data.u8[0] = (uint8_t)datalayer.battery.status.voltage_dV;  // OK
  SOLAX_1873.data.u8[1] = (datalayer.battery.status.voltage_dV >> 8);
  SOLAX_1873.data.u8[2] =
      (int8_t)datalayer.battery.status.reported_current_dA;  // OK, Signed (Active current in Amps x 10)
  SOLAX_1873.data.u8[3] = (datalayer.battery.status.reported_current_dA >> 8);
  SOLAX_1873.data.u8[4] = (uint8_t)(datalayer.battery.status.reported_soc / 100);  //SOC (100.00%)
  //SOLAX_1873.data.u8[5] = //Seems like this is not required? Or shall we put SOC decimals here?
  // Remaining energy is expressed in 0.1 kWh, matching frame 0x1878 below. Confirmed against
  // a real capture that reads 419 here alongside a total of 944 - 41.9 kWh of 94.4 kWh.
  // Upstream divides by 10 (raw Wh/10), which for a 33 kWh pack transmits 1650 where the
  // inverter reads 165 kWh remaining out of 33 kWh total. Remaining larger than total.
  uint16_t remaining_0p1kWh = (uint16_t)(datalayer.battery.status.reported_remaining_capacity_Wh / 100);
  SOLAX_1873.data.u8[6] = (uint8_t)remaining_0p1kWh;
  SOLAX_1873.data.u8[7] = (remaining_0p1kWh >> 8);

  //BMS_CellData
  SOLAX_1874.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  SOLAX_1874.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  SOLAX_1874.data.u8[2] = (int8_t)datalayer.battery.status.temperature_min_dC;
  SOLAX_1874.data.u8[3] = (datalayer.battery.status.temperature_min_dC >> 8);

  int32_t cell_max_voltage_mV = datalayer.battery.status.cell_max_voltage_mV;
  int32_t cell_min_voltage_mV = datalayer.battery.status.cell_min_voltage_mV;

  // Fake values during startup?
  if (cell_max_voltage_mV == 0) {
    cell_max_voltage_mV = 3300;
  }
  if (cell_min_voltage_mV == 0) {
    cell_min_voltage_mV = 3300;
  }

  // The upstream 3.0->3.5 V rescale is gone. A real capture of a working system shows this
  // frame carrying the ACTUAL cell voltage in 0.1 V units (41 = 4.1 V, 40 = 4.0 V) with no
  // compression, and frame 0x1876 below already sent the real value - so the two frames
  // disagreed about the same cells.

#if SOLAX_REMAP_CELLS_TO_MODULE_COUNT
  // SolaX derives an expected average cell voltage from pack voltage / (modules x cells per
  // module). With 13 modules it assumes 208 cells while an AKASOL pack really has 180, so the
  // raw per-cell figure sits well above the average the inverter computes. Re-express the
  // cell voltages against its own cell count, keeping the real measured spread intact.
  {
    uint16_t assumed_cells = (uint16_t)configured_number_of_modules * SOLAX_CELLS_PER_MODULE;
    if (assumed_cells > 0 && datalayer.battery.status.voltage_dV > 100) {
      int32_t real_spread_mV = cell_max_voltage_mV - cell_min_voltage_mV;
      int32_t implied_avg_mV = ((int32_t)datalayer.battery.status.voltage_dV * 100) / assumed_cells;
      cell_max_voltage_mV = implied_avg_mV + (real_spread_mV / 2);
      cell_min_voltage_mV = implied_avg_mV - (real_spread_mV / 2);
    }
  }
#endif

  uint16_t cell_max_voltage_dV = cell_max_voltage_mV / 100;
  uint16_t cell_min_voltage_dV = cell_min_voltage_mV / 100;

  SOLAX_1874.data.u8[4] = (uint8_t)(cell_max_voltage_dV);
  SOLAX_1874.data.u8[5] = (cell_max_voltage_dV >> 8);
  SOLAX_1874.data.u8[6] = (uint8_t)(cell_min_voltage_dV);
  SOLAX_1874.data.u8[7] = (cell_min_voltage_dV >> 8);

  //BMS_Status
  SOLAX_1875.data.u8[0] = (uint8_t)temperature_average;
  SOLAX_1875.data.u8[1] = (temperature_average >> 8);
  SOLAX_1875.data.u8[2] = (uint8_t)configured_number_of_modules;  // Number of slave batteries
  SOLAX_1875.data.u8[4] = (uint8_t)0;                             // Contactor Status 0=off, 1=on.

  //BMS_PackTemps (strange name, since it has voltages?)
  SOLAX_1876.data.u8[0] = (int8_t)datalayer.battery.status.temperature_max_dC;
  SOLAX_1876.data.u8[1] = (datalayer.battery.status.temperature_max_dC >> 8);
  // Reads the same locals as 0x1874 above, so the two frames always agree about the cells.
  SOLAX_1876.data.u8[2] = (uint8_t)cell_max_voltage_mV;
  SOLAX_1876.data.u8[3] = (cell_max_voltage_mV >> 8);

  SOLAX_1876.data.u8[4] = (int8_t)datalayer.battery.status.temperature_min_dC;
  SOLAX_1876.data.u8[5] = (datalayer.battery.status.temperature_min_dC >> 8);
  SOLAX_1876.data.u8[6] = (uint8_t)cell_min_voltage_mV;
  SOLAX_1876.data.u8[7] = (cell_min_voltage_mV >> 8);

  // Byte 1 is the BMS ALARM REGISTER. Mapped bit by bit against a live X3-Hybrid G4; twelve
  // data points, no exceptions, and the last two were predicted before being measured:
  //     bit 0 -> BE09 CellImbalance      bit 4 -> BE13 BMS_VolSen
  //     bit 1 -> BE10 BMS hardware       bit 5 -> BE14 BMS temp sen
  //     bit 2 -> BE11 BMS circuit        bit 6 -> BE15 BMS cur sen
  //     bit 3 -> BE12 BMS iso fault      bit 7 -> BE16 BMS relay
  // The display shows the LOWEST set bit (0x50 = bits 6+4 shows BE13; 0xFF shows BE09).
  // Anything written here is reported by the inverter as a battery fault, so it stays zero
  // unless real alarms are ever forwarded. Bytes 0, 2 and 3 are blank in both known-good
  // captures. Bytes 6/7 are fixed constants 0x1D/0x10 in both of them; upstream sends
  // 0x22/0x02, which came from a comment that guessed "Firmware version?" with a question mark.
  SOLAX_1877.data.u8[0] = 0;
  SOLAX_1877.data.u8[1] = 0;
  SOLAX_1877.data.u8[2] = 0;
  SOLAX_1877.data.u8[3] = 0;
  SOLAX_1877.data.u8[4] = (uint8_t)configured_battery_type;
  SOLAX_1877.data.u8[5] = 0;
  SOLAX_1877.data.u8[6] = (uint8_t)0x1D;
  SOLAX_1877.data.u8[7] = (uint8_t)0x10;

  //BMS_PackStats
  SOLAX_1878.data.u8[0] = (uint8_t)(datalayer.battery.status.voltage_dV);
  SOLAX_1878.data.u8[1] = ((datalayer.battery.status.voltage_dV) >> 8);

  // Both known-good captures put total capacity in bytes 4-5 only, as 16 bits of 0.1 kWh,
  // with byte 6 a constant 4 and byte 7 zero:
  //     capture A: B0 03 04 00 -> 944 = 94.4 kWh      capture B: B0 00 04 00 -> 176 = 17.6 kWh
  // Upstream spreads raw Wh across bytes 4-7 as a 32-bit value, which for a 33 kWh pack sends
  // 33000 where 330 is expected, and overwrites byte 6 with zero.
  uint16_t capacity_0p1kWh = (uint16_t)(datalayer.battery.info.reported_total_capacity_Wh / 100);
  SOLAX_1878.data.u8[4] = (uint8_t)capacity_0p1kWh;
  SOLAX_1878.data.u8[5] = (capacity_0p1kWh >> 8);
  SOLAX_1878.data.u8[6] = 4;
  SOLAX_1878.data.u8[7] = 0;

  // BMS_Answer
  SOLAX_1801.data.u8[0] = 2;
  SOLAX_1801.data.u8[2] = 1;
  SOLAX_1801.data.u8[4] = 1;

  //Ultra messages
  SOLAX_187E.data.u8[0] = (uint8_t)datalayer.battery.info.reported_total_capacity_Wh;
  SOLAX_187E.data.u8[1] = (datalayer.battery.info.reported_total_capacity_Wh >> 8);
  SOLAX_187E.data.u8[2] = (datalayer.battery.info.reported_total_capacity_Wh >> 16);
  SOLAX_187E.data.u8[3] = (datalayer.battery.info.reported_total_capacity_Wh >> 24);
  SOLAX_187E.data.u8[4] = (uint8_t)(datalayer.battery.status.soh_pptt / 100);
  SOLAX_187E.data.u8[5] = (uint8_t)(datalayer.battery.status.reported_soc / 100);
}

void SolaxInverter::transmit_can(unsigned long currentMillis) {
  // No periodic sending used on this protocol, we react only on incoming CAN messages!
}

// Build the battery serial reported in frames 0x1881 / 0x1882.
//
// The X3-Hybrid G4 manual shows the LCD line as "BatBrand: BAK 6S012345012345", i.e. brand
// plus a 14 character serial that begins "6S". The upstream comments show the same shape from
// a real capture: 0x1881 "6SBMSFA", 0x1882 "23AB052".
//
// The previous generator emitted pure hex derived from the eFuse MAC, which can never produce
// that prefix - 'S' is not a hex digit - so the serial was unparseable and BatBrand read "NA".
// Now: "6S" plus 12 digits from the MAC, so it is well formed, stable across reboots and
// unique per board. 0x1881 carries characters 1-7 and 0x1882 characters 8-14.
void solax_pack_identity_ascii(const uint8_t mac[6], uint8_t slot, uint8_t half, uint8_t out[7]) {
  char serial[14];
  serial[0] = '6';
  serial[1] = 'S';

  uint32_t seed = ((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) | ((uint32_t)mac[2] << 8) |
                  (uint32_t)mac[3];
  seed ^= (((uint32_t)mac[4] << 8) | (uint32_t)mac[5]);
  seed += (uint32_t)slot * 7919u;

  for (int i = 2; i < 14; i++) {
    if (seed == 0) {
      seed = 2166136261u + (uint32_t)mac[i % 6] * 16777619u + (uint32_t)slot + (uint32_t)i;
    }
    serial[i] = (char)('0' + (char)(seed % 10u));
    seed /= 10u;
  }

  const int base = half ? 7 : 0;
  for (int i = 0; i < 7; i++) {
    out[i] = (uint8_t)serial[base + i];
  }
}

void SolaxInverter::map_can_frame_to_variable(CAN_frame rx_frame) {

  if (rx_frame.ID == 0x1871) {
    datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;

    if ((rx_frame.data.u8[0] == (0x01)) || (rx_frame.data.u8[0] == (0x02))) {
      LastFrameTime = millis();

      // AlwaysClosed mode: Bypass state machine, keep contactors always closed
      if (configured_contactor_mode == inverter_contactor_mode_enum::AlwaysClosed) {
        datalayer.system.status.inverter_allows_contactor_closing = true;
        SOLAX_1875.data.u8[4] = (0x01);  // Inform Inverter: Contactor 0=off, 1=on.
        transmit_can_frame(&SOLAX_187E);
        transmit_can_frame(&SOLAX_187A);
        transmit_can_frame(&SOLAX_1872);
        transmit_can_frame(&SOLAX_1873);
        transmit_can_frame(&SOLAX_1874);
        transmit_can_frame(&SOLAX_1875);
        transmit_can_frame(&SOLAX_1876);
        transmit_can_frame(&SOLAX_1877);
        transmit_can_frame(&SOLAX_1878);
        transmit_can_frame(&SOLAX_100A001);
        return;
      }

      const bool print_state = (STATE != PREV_STATE);
      PREV_STATE = STATE;

      // Normal state machine (NoWorkaround and LockAfterFirstClose modes)
      switch (STATE) {
        case (BATTERY_ANNOUNCE):
          if (print_state)
            logging.println("[Solax]: Announce");
          datalayer.system.status.inverter_allows_contactor_closing = false;
          SOLAX_1875.data.u8[4] = (0x00);  // Inform Inverter: Contactor 0=off, 1=on.
          for (uint8_t i = 0; i < number_of_batteries; i++) {
            transmit_can_frame(&SOLAX_187E);
            transmit_can_frame(&SOLAX_187A);
            transmit_can_frame(&SOLAX_1872);
            transmit_can_frame(&SOLAX_1873);
            transmit_can_frame(&SOLAX_1874);
            transmit_can_frame(&SOLAX_1875);
            transmit_can_frame(&SOLAX_1876);
            transmit_can_frame(&SOLAX_1877);
            transmit_can_frame(&SOLAX_1878);
          }
          transmit_can_frame(&SOLAX_100A001);  //BMS Announce
          // Message from the inverter to proceed to contactor closing
          // Byte 4 changes from 0 to 1
          if (rx_frame.data.u64 == Contactor_Close_Payload)
            STATE = WAITING_FOR_CONTACTOR;
          break;

        case (WAITING_FOR_CONTACTOR):
          if (print_state)
            logging.println("[Solax]: Waiting for contactor");
          SOLAX_1875.data.u8[4] = (0x00);  // Inform Inverter: Contactor 0=off, 1=on.
          transmit_can_frame(&SOLAX_187E);
          transmit_can_frame(&SOLAX_187A);
          transmit_can_frame(&SOLAX_1872);
          transmit_can_frame(&SOLAX_1873);
          transmit_can_frame(&SOLAX_1874);
          transmit_can_frame(&SOLAX_1875);
          transmit_can_frame(&SOLAX_1876);
          transmit_can_frame(&SOLAX_1877);
          transmit_can_frame(&SOLAX_1878);
          transmit_can_frame(&SOLAX_1801);  // Announce that the battery will be connected
          STATE = CONTACTOR_CLOSED;         // Jump to Contactor Closed State
          break;

        case (CONTACTOR_CLOSED):
          if (print_state)
            logging.println("[Solax]: Contactor closed");
          datalayer.system.status.inverter_allows_contactor_closing = true;
          SOLAX_1875.data.u8[4] = (0x01);  // Inform Inverter: Contactor 0=off, 1=on.
          transmit_can_frame(&SOLAX_187E);
          transmit_can_frame(&SOLAX_187A);
          transmit_can_frame(&SOLAX_1872);
          transmit_can_frame(&SOLAX_1873);
          transmit_can_frame(&SOLAX_1874);
          transmit_can_frame(&SOLAX_1875);
          transmit_can_frame(&SOLAX_1876);
          transmit_can_frame(&SOLAX_1877);
          transmit_can_frame(&SOLAX_1878);
          // Message from the inverter to open contactor
          // Byte 4 changes from 1 to 0
          // Only process open request in NoWorkaround mode; LockAfterFirstClose mode ignores it
          if (rx_frame.data.u64 == Contactor_Open_Payload &&
              configured_contactor_mode == inverter_contactor_mode_enum::NoWorkaround) {
            set_event(EVENT_INVERTER_OPEN_CONTACTOR, 0);
            STATE = BATTERY_ANNOUNCE;
          }
          break;
      }
    }
  }

  if (rx_frame.ID == 0x1871 && rx_frame.data.u64 == __builtin_bswap64(0x0500010000000000)) {
    uint16_t modules = configured_number_of_modules;
    if (modules > 254) {
      modules = 254;
    }
    int slot_count = (int)modules + 1;

    uint64_t mac64 = ESP.getEfuseMac();
    uint8_t mac[6];
    for (int i = 0; i < 6; i++) {
      mac[i] = (uint8_t)(mac64 >> (i * 8));
    }

    for (int slot = 0; slot < slot_count; slot++) {
      SOLAX_1881.data.u8[0] = (uint8_t)slot;
      solax_pack_identity_ascii(mac, (uint8_t)slot, 0, &SOLAX_1881.data.u8[1]);
      SOLAX_1882.data.u8[0] = (uint8_t)slot;
      solax_pack_identity_ascii(mac, (uint8_t)slot, 1, &SOLAX_1882.data.u8[1]);
      transmit_can_frame(&SOLAX_1881);
      transmit_can_frame(&SOLAX_1882);
    }
  }
  if (rx_frame.ID == 0x1871 && rx_frame.data.u8[0] == (0x03)) {
    // Unused message
  }
}

bool SolaxInverter::setup(void) {  // Performs one time setup at startup
  // Use user selected values if nonzero, otherwise use defaults
  if (user_selected_inverter_modules > 0) {
    configured_number_of_modules = user_selected_inverter_modules;
  } else {
    configured_number_of_modules = DEFAULT_NUMBER_OF_MODULES;
  }

  if (user_selected_inverter_battery_type > 0) {
    configured_battery_type = user_selected_inverter_battery_type;
  } else {
    configured_battery_type = DEFAULT_BATTERY_TYPE;
  }

  configured_contactor_mode = user_selected_inverter_contactor_mode;

  if (configured_contactor_mode != inverter_contactor_mode_enum::AlwaysClosed) {
    datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  }

  return true;
}
