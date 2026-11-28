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

void SolaxInverter::update_values() {
  // If not receiving any communication from the inverter, open contactors and
  // return to battery announce state
  if (millis() - LastFrameTime >= INTERVAL_5_S &&
      configured_contactor_mode == inverter_contactor_mode_enum::NoWorkaround) {
    datalayer.system.status.inverter_allows_contactor_closing = false;
    if (STATE != BATTERY_ANNOUNCE) {
      logging.println("[Solax] Timeout, opening contactor");
      STATE = BATTERY_ANNOUNCE;
    }
  }
  //Calculate the required values
  temperature_average = ((datalayer.aggregate.temperature_max_dC + datalayer.aggregate.temperature_min_dC) / 2);

  // The inverter won't close contactors for a pack reporting 0A discharge
  uint16_t max_discharge_current_dA = datalayer.aggregate.max_discharge_current_dA;
  if (max_discharge_current_dA == 0 && (STATE != CONTACTOR_CLOSED || !datalayer.system.status.dc_bus_live) &&
      configured_contactor_mode != inverter_contactor_mode_enum::AlwaysClosed &&
      !datalayer.system.info.equipment_stop_active && datalayer.system.status.system_status != FAULT) {
    max_discharge_current_dA = ANNOUNCE_DISCHARGE_dA;
  }

  //Put the values into the CAN messages
  //BMS_Limits
  SOLAX_1872.data.u8[0] = (uint8_t)datalayer.aggregate.max_design_voltage_dV;
  SOLAX_1872.data.u8[1] = (datalayer.aggregate.max_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[2] = (uint8_t)datalayer.aggregate.min_design_voltage_dV;
  SOLAX_1872.data.u8[3] = (datalayer.aggregate.min_design_voltage_dV >> 8);
  SOLAX_1872.data.u8[4] = (uint8_t)datalayer.aggregate.max_charge_current_dA;
  SOLAX_1872.data.u8[5] = (datalayer.aggregate.max_charge_current_dA >> 8);
  SOLAX_1872.data.u8[6] = (uint8_t)max_discharge_current_dA;
  SOLAX_1872.data.u8[7] = (max_discharge_current_dA >> 8);

  //BMS_PackData
  SOLAX_1873.data.u8[0] = (uint8_t)datalayer.aggregate.voltage_dV;  // OK
  SOLAX_1873.data.u8[1] = (datalayer.aggregate.voltage_dV >> 8);
  SOLAX_1873.data.u8[2] = (int8_t)datalayer.aggregate.current_dA;  // OK, Signed (Active current in Amps x 10)
  SOLAX_1873.data.u8[3] = (datalayer.aggregate.current_dA >> 8);
  SOLAX_1873.data.u8[4] = (uint8_t)(datalayer.aggregate.reported_soc / 100);  //SOC (100.00%)
  //SOLAX_1873.data.u8[5] = //Seems like this is not required? Or shall we put SOC decimals here?
  SOLAX_1873.data.u8[6] = (uint8_t)(datalayer.aggregate.reported_remaining_capacity_Wh / 10);
  SOLAX_1873.data.u8[7] = ((datalayer.aggregate.reported_remaining_capacity_Wh / 10) >> 8);

  //BMS_CellData
  SOLAX_1874.data.u8[0] = (int8_t)datalayer.aggregate.temperature_max_dC;
  SOLAX_1874.data.u8[1] = (datalayer.aggregate.temperature_max_dC >> 8);
  SOLAX_1874.data.u8[2] = (int8_t)datalayer.aggregate.temperature_min_dC;
  SOLAX_1874.data.u8[3] = (datalayer.aggregate.temperature_min_dC >> 8);

  int32_t cell_max_voltage_mV = datalayer.aggregate.cell_max_voltage_mV;
  int32_t cell_min_voltage_mV = datalayer.aggregate.cell_min_voltage_mV;

  // Fake values during startup?
  if (cell_max_voltage_mV == 0) {
    cell_max_voltage_mV = 3300;
  }
  if (cell_min_voltage_mV == 0) {
    cell_min_voltage_mV = 3300;
  }

  // Rescale to the range 3.0->3.5V
  cell_max_voltage_mV =
      3000 + ((cell_max_voltage_mV - datalayer.battery.info.min_cell_voltage_mV) * (3500 - 3000)) /
                 (datalayer.battery.info.max_cell_voltage_mV - datalayer.battery.info.min_cell_voltage_mV);
  cell_min_voltage_mV =
      3000 + ((cell_min_voltage_mV - datalayer.battery.info.min_cell_voltage_mV) * (3500 - 3000)) /
                 (datalayer.battery.info.max_cell_voltage_mV - datalayer.battery.info.min_cell_voltage_mV);

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
  SOLAX_1876.data.u8[0] = (int8_t)datalayer.aggregate.temperature_max_dC;
  SOLAX_1876.data.u8[1] = (datalayer.aggregate.temperature_max_dC >> 8);
  SOLAX_1876.data.u8[2] = (uint8_t)datalayer.aggregate.cell_max_voltage_mV;
  SOLAX_1876.data.u8[3] = (datalayer.aggregate.cell_max_voltage_mV >> 8);

  SOLAX_1876.data.u8[4] = (int8_t)datalayer.aggregate.temperature_min_dC;
  SOLAX_1876.data.u8[5] = (datalayer.aggregate.temperature_min_dC >> 8);
  SOLAX_1876.data.u8[6] = (uint8_t)datalayer.aggregate.cell_min_voltage_mV;
  SOLAX_1876.data.u8[7] = (datalayer.aggregate.cell_min_voltage_mV >> 8);

  //Unknown
  SOLAX_1877.data.u8[4] = (uint8_t)configured_battery_type;  // Battery type (Default 0x50)
  SOLAX_1877.data.u8[6] = (uint8_t)0x22;                     // Firmware version?
  SOLAX_1877.data.u8[7] =
      (uint8_t)0x02;  // The above firmware version applies to:02 = Master BMS, 10 = S1, 20 = S2, 30 = S3, 40 = S4

  //BMS_PackStats
  // X1-AC/X1-Fit Gen3 Manager firmware reads 0x1878 byte0 as a field count and, when non-zero,
  // byte1 as the BMS minimum SOC, which the DSP enforces as a floor on Min Capacity. Sending the
  // pack voltage here made that floor voltage>>8 (13/14/15 % at 333/358/384 V). Send a fixed
  // count (7 = keep parsing the capacity in bytes 4-7) and the inverter's own default minimum.
  SOLAX_1878.data.u8[0] = 7;
  SOLAX_1878.data.u8[1] = 10;

  SOLAX_1878.data.u8[4] = (uint8_t)datalayer.aggregate.reported_total_capacity_Wh;
  SOLAX_1878.data.u8[5] = (datalayer.aggregate.reported_total_capacity_Wh >> 8);
  SOLAX_1878.data.u8[6] = (datalayer.aggregate.reported_total_capacity_Wh >> 16);
  SOLAX_1878.data.u8[7] = (datalayer.aggregate.reported_total_capacity_Wh >> 24);

  // BMS_Answer
  SOLAX_1801.data.u8[0] = 2;
  SOLAX_1801.data.u8[2] = 1;
  SOLAX_1801.data.u8[4] = 1;

  //Ultra messages
  SOLAX_187E.data.u8[0] = (uint8_t)datalayer.aggregate.reported_total_capacity_Wh;
  SOLAX_187E.data.u8[1] = (datalayer.aggregate.reported_total_capacity_Wh >> 8);
  SOLAX_187E.data.u8[2] = (datalayer.aggregate.reported_total_capacity_Wh >> 16);
  SOLAX_187E.data.u8[3] = (datalayer.aggregate.reported_total_capacity_Wh >> 24);
  SOLAX_187E.data.u8[4] = (uint8_t)(datalayer.aggregate.soh_pptt / 100);
  SOLAX_187E.data.u8[5] = (uint8_t)(datalayer.aggregate.reported_soc / 100);
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

  uint32_t seed = ((uint32_t)mac[0] << 24) | ((uint32_t)mac[1] << 16) | ((uint32_t)mac[2] << 8) | (uint32_t)mac[3];
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
  if (rx_frame.ID == ALT_INVERTER_POLL_ID) {
    map_can_frame_alt(rx_frame);
    return;
  }

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
          bus_watch = BUS_WAIT_LIVE;        // The pack is not closed yet, whatever the state machine says
          STATE = CONTACTOR_CLOSED;         // Jump to Contactor Closed State
          break;

        case (CONTACTOR_CLOSED):
          if (print_state)
            logging.println("[Solax]: Contactor closed");
          datalayer.system.status.inverter_allows_contactor_closing = true;
          SOLAX_1875.data.u8[4] =
              datalayer.system.status.dc_bus_live ? 0x01 : 0x00;  // Inform Inverter: Contactor 0=off, 1=on.
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
            datalayer.system.status.inverter_allows_contactor_closing = false;
            STATE = BATTERY_ANNOUNCE;
          }
          // Replay the handshake once the battery has closed its contactors.
          if (bus_watch == BUS_WAIT_LIVE && datalayer.system.status.dc_bus_live) {
            bus_watch = BUS_LIVE;
          } else if (bus_watch == BUS_LIVE && !datalayer.system.status.dc_bus_live) {
            bus_watch = BUS_DROPPED;
          } else if (bus_watch == BUS_DROPPED && datalayer.system.status.dc_bus_live) {
            logging.println("[Solax]: Battery reconnected, replaying announce");
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

  // The alternate protocol defaults to HS51 battery type
  alt_battery_type =
      user_selected_inverter_battery_type > 0 ? user_selected_inverter_battery_type : ALT_DEFAULT_BATTERY_TYPE;

  if (configured_contactor_mode != inverter_contactor_mode_enum::AlwaysClosed) {
    datalayer.system.status.inverter_allows_contactor_closing = false;  // The inverter needs to allow first
  }

  return true;
}

// ---------------------------------------------------------------------------------------------------------
// Alternate SolaX protocol (BMS 0x18xxF3F4, inverter 0x1810F4F3 / 0x1812F4F3)
//
// Some batteries (HS51) use an alternate protocol, supported by inverters such
// as X3G4 and AELIO. On the AELIO the original protocol isn't supported, so
// the alternate protocol is used automatically on receipt of the first inverter
// poll.
// ---------------------------------------------------------------------------------------------------------

static void put16(uint8_t* p, uint16_t v) {
  p[0] = (uint8_t)v;
  p[1] = (uint8_t)(v >> 8);
}

static uint8_t temperature_byte(int16_t temperature_dC) {  // degC + 50
  int16_t t = temperature_dC / 10 + 50;
  return (uint8_t)(t < 0 ? 0 : (t > 255 ? 255 : t));
}

// Same 3.0..3.5 V rescaling as the classic 0x1874 frame
static uint16_t rescaled_cell_mV(int32_t cell_mV) {
  if (cell_mV == 0) {
    cell_mV = 3300;
  }
  int32_t span = datalayer.battery.info.max_cell_voltage_mV - datalayer.battery.info.min_cell_voltage_mV;
  if (span <= 0) {
    return (uint16_t)cell_mV;
  }
  return (uint16_t)(3000 + ((cell_mV - datalayer.battery.info.min_cell_voltage_mV) * (3500 - 3000)) / span);
}

// Transmit a CAN frame (to only allocate one on the stack at a time)
void SolaxInverter::transmit_frame(uint32_t id, const uint8_t data[8]) {
  CAN_frame frame = {.FD = false, .ext_ID = true, .DLC = 8, .ID = id, .data = {}};
  memcpy(frame.data.u8, data, 8);
  transmit_can_frame(&frame);
}

void SolaxInverter::map_can_frame_alt(const CAN_frame& rx_frame) {
  datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
  LastFrameTime = millis();
  if (!alt_protocol) {
    logging.println("[Solax] Inverter polls with 0x1810F4F3, using the alternate protocol");
    alt_protocol = true;
  }

  const uint8_t request = rx_frame.data.u8[0] & 0x0F;
  switch (configured_contactor_mode) {
    case inverter_contactor_mode_enum::AlwaysClosed:
      datalayer.system.status.inverter_allows_contactor_closing = true;
      break;
    default:
      if (request == ALT_REQUEST_CLOSE) {
        if (!datalayer.system.status.inverter_allows_contactor_closing) {
          logging.println("[Solax] Alternate protocol: inverter allows closing");
        }
        datalayer.system.status.inverter_allows_contactor_closing = true;
        alt_first_close_seen = true;
      } else if (request == ALT_REQUEST_OPEN && datalayer.system.status.inverter_allows_contactor_closing &&
                 (configured_contactor_mode == inverter_contactor_mode_enum::NoWorkaround || !alt_first_close_seen)) {
        set_event(EVENT_INVERTER_OPEN_CONTACTOR, 0);
        datalayer.system.status.inverter_allows_contactor_closing = false;
      }
      break;
  }

  transmit_frames_alt();
}

void SolaxInverter::transmit_frames_alt() {
  const bool always_closed = configured_contactor_mode == inverter_contactor_mode_enum::AlwaysClosed;
  const bool closed = always_closed || (datalayer.system.status.inverter_allows_contactor_closing &&
                                        datalayer.system.status.dc_bus_live);

  // The inverter won't close contactors for a pack reporting 0A discharge (as in update_values)
  uint16_t max_discharge_current_dA = datalayer.aggregate.max_discharge_current_dA;
  if (max_discharge_current_dA == 0 && !closed && !datalayer.system.info.equipment_stop_active &&
      datalayer.system.status.system_status != FAULT) {
    max_discharge_current_dA = ANNOUNCE_DISCHARGE_dA;
  }
  const int16_t temperature_avg_dC =
      (datalayer.aggregate.temperature_max_dC + datalayer.aggregate.temperature_min_dC) / 2;

  uint8_t d[8];

  // 0x1820: max / min cell voltage (mV), positions of those cells
  memset(d, 0, 8);
  put16(d + 0, rescaled_cell_mV(datalayer.aggregate.cell_max_voltage_mV));
  put16(d + 2, rescaled_cell_mV(datalayer.aggregate.cell_min_voltage_mV));
  transmit_frame(0x1820F3F4, d);

  // 0x1821: max / min temperature (degC + 50), positions
  memset(d, 0, 8);
  d[0] = temperature_byte(datalayer.aggregate.temperature_max_dC);
  d[1] = temperature_byte(datalayer.aggregate.temperature_min_dC);
  transmit_frame(0x1821F3F4, d);

  // 0x1822: SOC (0.1 %), pack voltage (0.1 V), SOH (0.1 %, not used by the X3), current (0.1 A + 10000)
  memset(d, 0, 8);
  put16(d + 0, datalayer.aggregate.reported_soc / 10);
  put16(d + 2, datalayer.aggregate.voltage_dV);
  put16(d + 4, datalayer.aggregate.soh_pptt / 10);
  put16(d + 6, (uint16_t)(datalayer.aggregate.current_dA + 10000));
  transmit_frame(0x1822F3F4, d);

  // 0x1823: max discharge / max charge current (1 A)
  memset(d, 0, 8);
  put16(d + 0, max_discharge_current_dA / 10);
  put16(d + 2, datalayer.aggregate.max_charge_current_dA / 10);
  transmit_frame(0x1823F3F4, d);

  // 0x1824: BMS state (3 = contactor closed, 0 = open)
  memset(d, 0, 8);
  d[0] = closed ? ALT_BMS_STATE_CLOSED : ALT_BMS_STATE_OPEN;
  transmit_frame(0x1824F3F4, d);

  // 0x1825: alarm / protection bits (none)
  memset(d, 0, 8);
  transmit_frame(0x1825F3F4, d);

  // 0x1835: clusters, modules, remaining capacity (0.1 kWh)
  memset(d, 0, 8);
  d[0] = 1;
  d[1] = (uint8_t)configured_number_of_modules;
  put16(d + 4, datalayer.aggregate.reported_remaining_capacity_Wh / 100);
  transmit_frame(0x1835F3F4, d);

  // 0x1836: byte 0 flags (as the classic 0x187A byte 1; 0 = inverter manages the contactor), average temp + 50
  memset(d, 0, 8);
  d[5] = temperature_byte(temperature_avg_dC);
  transmit_frame(0x1836F3F4, d);

  // 0x1837: min / max design voltage (1 V), total capacity (0.1 kWh)
  memset(d, 0, 8);
  put16(d + 0, (datalayer.aggregate.min_design_voltage_dV + 9) / 10);
  put16(d + 2, datalayer.aggregate.max_design_voltage_dV / 10);
  put16(d + 4, datalayer.aggregate.reported_total_capacity_Wh / 100);
  transmit_frame(0x1837F3F4, d);

  // 0x1840: energy counters (not tracked)
  memset(d, 0, 8);
  transmit_frame(0x1840F3F4, d);

  // 0x1841: module 0 versions and battery type; also the inverter's keep-alive
  memset(d, 0, 8);
  put16(d + 5, alt_battery_type);
  transmit_frame(0x1841F3F4, d);
}
