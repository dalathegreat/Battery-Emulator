#include "BMW-SBOX.h"
#include "../communication/can/comm_can.h"
#include "../datalayer/datalayer.h"
#include "../include.h"

uint8_t reverse_bits(uint8_t byte) {
  uint8_t reversed = 0;
  for (int i = 0; i < 8; i++) {
    reversed = (reversed << 1) | (byte & 1);
    byte >>= 1;
  }
  return reversed;
}

/** CRC8, both inverted, poly 0x31 **/
uint8_t calculateCRC(CAN_frame CAN) {
  uint8_t crc = 0;
  for (size_t i = 0; i < CAN.DLC; i++) {
    uint8_t reversed_byte = reverse_bits(CAN.data.u8[i]);
    crc ^= reversed_byte;
    for (int j = 0; j < 8; j++) {
      if (crc & 0x80) {
        crc = (crc << 1) ^ 0x31;
      } else {
        crc <<= 1;
      }
      crc &= 0xFF;
    }
  }
  crc = reverse_bits(crc);
  return crc;
}

void BmwSbox::handle_incoming_can_frame(CAN_frame rx_frame) {
  unsigned long currentTime = millis();
  if (rx_frame.ID == 0x200) {
    ShuntLastSeen = currentTime;
    datalayer.shunt.measured_amperage_mA =
        ((rx_frame.data.u8[2] << 24) | (rx_frame.data.u8[1] << 16) | (rx_frame.data.u8[0] << 8)) / 256;
    datalayer.shunt.measured_amperage_dA = datalayer.shunt.measured_amperage_mA / 100;

    /** Calculate 1S avg current **/
    if (LastAvgTime + 100 < currentTime) {
      LastAvgTime = currentTime;
      if (k > 9) {
        k = 0;
      }
      avg_mA_array[k] = datalayer.shunt.measured_amperage_mA;
      k++;
      avg_sum = 0;
      for (uint8_t i = 0; i < 10; i++) {
        avg_sum = avg_sum + avg_mA_array[i];
      }
      datalayer.shunt.measured_avg1S_amperage_mA = avg_sum / 10;
    }
  } else if (rx_frame.ID == 0x210)  //SBOX input (battery side) voltage
  {
    ShuntLastSeen = currentTime;
    datalayer.shunt.measured_voltage_mV =
        ((rx_frame.data.u8[2] << 16) | (rx_frame.data.u8[1] << 8) | (rx_frame.data.u8[0]));
  } else if (rx_frame.ID == 0x220)  //SBOX output voltage
  {
    ShuntLastSeen = currentTime;
    datalayer.shunt.measured_outvoltage_mV =
        ((rx_frame.data.u8[2] << 16) | (rx_frame.data.u8[1] << 8) | (rx_frame.data.u8[0]));
    datalayer.shunt.available = true;
  }
}

void BmwSbox::transmit_can(unsigned long currentMillis) {

  /** Shunt can frames seen? **/
  if (ShuntLastSeen + 1000 < currentMillis) {
    datalayer.shunt.available = false;
  } else {
    datalayer.shunt.available = true;
  }
  // Send 20ms CAN Message
  if (currentMillis - LastMsgTime >= INTERVAL_20_MS) {
    LastMsgTime = currentMillis;
    // First check if we have any active errors, incase we do, turn off the battery
    if (datalayer.battery.status.bms_status == FAULT) {
      timeSpentInFaultedMode++;
    } else {
      timeSpentInFaultedMode = 0;
    }

    //handle contactor control SHUTDOWN_REQUESTED
    if (timeSpentInFaultedMode > MAX_ALLOWED_FAULT_TICKS) {
      contactorStatus = SHUTDOWN_REQUESTED;
      SBOX_100.data.u8[0] = 0x55;  // All open
    }

    if (contactorStatus == SHUTDOWN_REQUESTED) {
      datalayer.shunt.contactors_engaged = false;
      return;  // A fault scenario latches the contactor control. It is not possible to recover without a powercycle (and investigation why fault occured)
    }

    // After that, check if we are OK to start turning on the contactors
    if (contactorStatus == DISCONNECTED) {
      datalayer.shunt.contactors_engaged = false;
      SBOX_100.data.u8[0] = 0x55;  // All open

      if (datalayer.system.status.battery_allows_contactor_closing &&
          datalayer.system.status.inverter_allows_contactor_closing &&
          !datalayer.system.settings.equipment_stop_active &&
          (datalayer.shunt.measured_voltage_mV > MINIMUM_INPUT_VOLTAGE * 1000)) {
        contactorStatus = PRECHARGE;
      }
    }
    // In case the inverter requests contactors to open, set the state accordingly
    if (contactorStatus == COMPLETED) {
      //Incase inverter (or estop) requests contactors to open, make state machine jump to Disconnected state (recoverable)
      if (!datalayer.system.status.inverter_allows_contactor_closing ||
          datalayer.system.settings.equipment_stop_active) {
        contactorStatus = DISCONNECTED;
      }
    }
    // Handle actual state machine. This first turns on Precharge, then Negative, then Positive, and finally turns OFF precharge
    switch (contactorStatus) {
      case PRECHARGE:
        SBOX_100.data.u8[0] = 0x86;  // Precharge relay only
        prechargeStartTime = currentMillis;
        contactorStatus = NEGATIVE;
#ifdef DEBUG_VIA_USB
        Serial.println("S-BOX Precharge relay engaged");
#endif
        break;
      case NEGATIVE:
        if (currentMillis - prechargeStartTime >= CONTACTOR_CONTROL_T1) {
          SBOX_100.data.u8[0] = 0xA6;  // Precharge + Negative
          negativeStartTime = currentMillis;
          contactorStatus = POSITIVE;
          datalayer.shunt.precharging = true;
#ifdef DEBUG_VIA_USB
          Serial.println("S-BOX Negative relay engaged");
#endif
        }
        break;
      case POSITIVE:
        if (currentMillis - negativeStartTime >= CONTACTOR_CONTROL_T2 &&
            (datalayer.shunt.measured_voltage_mV * MAX_PRECHARGE_RESISTOR_VOLTAGE_PERCENT <
             datalayer.shunt.measured_outvoltage_mV)) {
          SBOX_100.data.u8[0] = 0xAA;  // Precharge + Negative + Positive
          positiveStartTime = currentMillis;
          contactorStatus = PRECHARGE_OFF;
          datalayer.shunt.precharging = false;
#ifdef DEBUG_VIA_USB
          Serial.println("S-BOX Positive relay engaged");
#endif
        }
        break;
      case PRECHARGE_OFF:
        if (currentMillis - positiveStartTime >= CONTACTOR_CONTROL_T3) {
          SBOX_100.data.u8[0] = 0x6A;  // Negative + Positive
          contactorStatus = COMPLETED;
#ifdef DEBUG_VIA_USB
          Serial.println("S-BOX Precharge relay released");
#endif
          datalayer.shunt.contactors_engaged = true;
        }
        break;
      case COMPLETED:
        SBOX_100.data.u8[0] = 0x6A;  // Negative + Positive
      default:
        break;
    }
    CAN100_cnt++;
    if (CAN100_cnt > 0x0E) {
      CAN100_cnt = 0;
    }
    SBOX_100.data.u8[1] = CAN100_cnt << 4 | 0x01;
    SBOX_100.data.u8[3] = 0x00;
    SBOX_100.data.u8[3] = calculateCRC(SBOX_100);
    transmit_can_frame(&SBOX_100, can_config.shunt);
    transmit_can_frame(&SBOX_300, can_config.shunt);
  }
}

void BmwSbox::setup() {
  strncpy(datalayer.system.info.shunt_protocol, Name, 63);
  datalayer.system.info.shunt_protocol[63] = '\0';
}
