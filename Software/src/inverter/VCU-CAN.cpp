#include "VCU-CAN.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../devboard/utils/common_functions.h"  //For CRC table
#include "../inverter/INVERTERS.h"

/*TODO once testing starts:
- Map more optional content into all message
- Add OBD2 responses for 79B /7BB polls
- Populate onboard-charger power limit (0x1DC byte2 low nibble + byte3 top 6 bits, "chargelimit"
in leafbms.cpp). This currently stays at 0 and needs a real value once we know what the VCU
expects here, since leafbms.cpp maps it (in Amps) straight into Param::BMS_ChargeLim.
-*/

static uint8_t calculate_CRC_Nissan(CAN_frame* frame) {
  uint8_t crc = 0;
  for (uint8_t j = 0; j < 7; j++) {
    crc = crctable_nissan_leaf[(crc ^ static_cast<uint8_t>(frame->data.u8[j])) % 256];
  }
  return crc;
}

void VCUInverter::update_values() {  //Called every 1s

  //Fake that we have inverter alive, to avoid fault event (-1 to not trigger detected successfully message)
  datalayer.system.status.CAN_inverter_still_alive = (CAN_STILL_ALIVE - 1);

  // ---- 0x1DC: Discharge / charge power limits ----
  // leafbms.cpp decodes these as 10-bit fields in units of 0.25kW:
  // dislimit = ((byte0<<2) + (byte1>>6)) * 0.25
  // chglimit = (((byte1&0x3F)<<4) + (byte2>>4)) * 0.25
  // so the raw value = power_in_watts / 250.
  uint16_t dislimit_raw = static_cast<uint16_t>(datalayer.battery.status.max_discharge_power_W / 250) & 0x3FF;
  uint16_t chglimit_raw = static_cast<uint16_t>(datalayer.battery.status.max_charge_power_W / 250) & 0x3FF;

  LEAF_1DC.data.u8[0] = (dislimit_raw >> 2) & 0xFF;
  LEAF_1DC.data.u8[1] = static_cast<uint8_t>(((dislimit_raw & 0x03) << 6) | ((chglimit_raw >> 4) & 0x3F));
  LEAF_1DC.data.u8[2] = static_cast<uint8_t>((chglimit_raw & 0x0F) << 4);
  //Note: low nibble of byte2 + top 6 bits of byte3 are the onboard-charger power limit field
  //("chargelimit" in leafbms.cpp -> Param::BMS_ChargeLim). Left at 0 for now, see TODO above.

  // ---- 0x55B: State of charge ----
  // leafbms.cpp decodes: soc = ((byte0<<2) + (byte1>>6)) * 0.1
  // real_soc is stored as integer-percent x100 (9550 = 95.50%), so /10 gives 0.1%-per-bit units.
  uint16_t soc_raw = static_cast<uint16_t>(datalayer.battery.status.real_soc / 10) & 0x3FF;

  LEAF_55B.data.u8[0] = (soc_raw >> 2) & 0xFF;
  LEAF_55B.data.u8[1] = static_cast<uint8_t>((soc_raw & 0x03) << 6);

  // ---- 0x1DB: Pack current & voltage ----
  // leafbms.cpp decodes:
  // cur = (byte0<<3) + (byte1>>5); if (cur>1023) cur -= 2047; BattCur = cur/2
  // udc = (byte2<<2) + (byte3>>6); BattVoltage = udc/2
  // i.e. raw units are 0.5A/bit and 0.5V/bit, with current using an 11-bit field
  // biased by 2047 (not 2048) for negative values - this matches genuine Nissan LEAF traffic.
  int32_t current_raw = (static_cast<int32_t>(datalayer.battery.status.current_dA) * 2) / 10;
  if (current_raw < 0) {
    current_raw += 2047;
  }
  current_raw &= 0x7FF;

  uint16_t voltage_raw =
      static_cast<uint16_t>((static_cast<uint32_t>(datalayer.battery.status.voltage_dV) * 2) / 10) & 0x3FF;

  LEAF_1DB.data.u8[0] = (current_raw >> 3) & 0xFF;
  LEAF_1DB.data.u8[1] = static_cast<uint8_t>((current_raw & 0x07) << 5);
  LEAF_1DB.data.u8[2] = (voltage_raw >> 2) & 0xFF;
  LEAF_1DB.data.u8[3] = static_cast<uint8_t>(((voltage_raw & 0x03) << 6) | 0x2B);  //Lower 6 bits: status flags

  // ---- 0x5BC: Remaining GIDs & SOH ----
  // Not currently consumed by leafbms.cpp (SOH/GIDs decoding is commented out there),
  // but fixed here for correctness / future compatibility.
  remaining_gids = static_cast<uint16_t>((datalayer.battery.status.real_soc / 10000.0) * 281);  //0-281 for 24kWh
  LEAF_5BC.data.u8[0] = (remaining_gids >> 2) & 0xFF;
  LEAF_5BC.data.u8[1] = static_cast<uint8_t>((remaining_gids & 0x03) << 6);
  LEAF_5BC.data.u8[4] = static_cast<uint8_t>((datalayer.battery.status.soh_pptt / 100) << 1);
}

void VCUInverter::map_can_frame_to_variable(CAN_frame rx_frame) {
  switch (rx_frame.ID) {
    case 0x1F2:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x1D4:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x50B:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    case 0x50C:
      datalayer.system.status.CAN_inverter_still_alive = CAN_STILL_ALIVE;
      break;
    default:
      break;
  }
}

void VCUInverter::transmit_can(unsigned long currentMillis) {
  // Send 10ms CAN Message
  if (currentMillis - previousMillis10ms >= INTERVAL_10_MS) {
    previousMillis10ms = currentMillis;

    mprun10 = (mprun10 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    LEAF_1DC.data.u8[6] = mprun10;
    LEAF_1DC.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DC);
    transmit_can_frame(&LEAF_1DC);
    LEAF_1DB.data.u8[6] = mprun10;
    LEAF_1DB.data.u8[7] = calculate_CRC_Nissan(&LEAF_1DB);
    transmit_can_frame(&LEAF_1DB);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis100ms >= INTERVAL_100_MS) {
    previousMillis100ms = currentMillis;

    mprun100 = (mprun100 + 1) % 4;  // mprun10 cycles between 0-1-2-3-0-1...
    counter_55B = (counter_55B + 1) % 10;
    if (counter_55B < 5) {
      LEAF_55B.data.u8[2] = 0xAA;
    } else {
      LEAF_55B.data.u8[2] = 0x55;
    }
    LEAF_55B.data.u8[6] = (1 << 4) | mprun100;  //1 = RefuseToSleep 2 = ReadyToSleep
    LEAF_55B.data.u8[7] = calculate_CRC_Nissan(&LEAF_55B);
    transmit_can_frame(&LEAF_55B);
    transmit_can_frame(&LEAF_5BC);
  }
  // Send 100ms CAN Messages
  if (currentMillis - previousMillis500ms >= INTERVAL_500_MS) {
    previousMillis500ms = currentMillis;

    transmit_can_frame(&LEAF_59E);
    if (LEAF_5C0.data.u8[0] == 0x40) {
      LEAF_5C0.data.u8[0] = 0x80;
    } else if (LEAF_5C0.data.u8[0] == 0x80) {
      LEAF_5C0.data.u8[0] = 0xC0;
    } else if (LEAF_5C0.data.u8[0] == 0xC0) {
      LEAF_5C0.data.u8[0] = 0x40;
    }
    transmit_can_frame(&LEAF_5C0);
  }
}
