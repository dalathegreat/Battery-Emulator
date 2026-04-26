#include "UdsCanBattery.h"

#include <Arduino.h>
#include "../devboard/utils/logging.h"

void UdsCanBattery::transmit_uds_can(unsigned long currentMillis) {
  if (currentMillis - previousUdsMillis200 >= INTERVAL_200_MS) {
    previousUdsMillis200 = currentMillis;

    if (uds_busy_timeout > 0) {
      // Still busy, do not send new requests
      uds_busy_timeout--;
      return;
    }

    if (user_request_clear_dtc) {
      UDS_CLEAR_DTCs.ID = uds_address;
      transmit_can_frame(&UDS_CLEAR_DTCs);

      uds_busy_timeout = 25;  // 2.5s
      return;
    }

    if (user_request_read_dtc) {
      UDS_RQ_DTCs.ID = uds_address;
      transmit_can_frame(&UDS_RQ_DTCs);

      uds_busy_timeout = 10;  // 1s
      return;
    }

    if (next_pid == 0 && first_pid != 0) {
      // Reset PID cycle
      next_pid = first_pid;
    }

    if (next_pid) {
      // Request the next PID
      UDS_PID_REQUEST.ID = uds_address;
      UDS_PID_REQUEST.data.u8[2] = (next_pid >> 8) & 0xFF;
      UDS_PID_REQUEST.data.u8[3] = next_pid & 0xFF;
      transmit_can_frame(&UDS_PID_REQUEST);

      uds_busy_timeout = 2;  // 0.2s
    }
  }
}

void UdsCanBattery::startUDSMultiFrameReception(uint16_t totalLength, uint8_t moduleID) {
  gUDSContext.UDS_inProgress = true;
  gUDSContext.UDS_expectedLength = totalLength;
  gUDSContext.UDS_bytesReceived = 0;
  gUDSContext.UDS_moduleID = moduleID;
  gUDSContext.receivedInBatch = 0;
  memset(gUDSContext.UDS_buffer, 0, sizeof(gUDSContext.UDS_buffer));
  gUDSContext.UDS_lastFrameMillis = millis();  // if you want to track timeouts
  uds_busy_timeout = 5;                        // 0.5s
}

bool UdsCanBattery::storeUDSPayload(const uint8_t* payload, uint8_t length) {
  if (gUDSContext.UDS_bytesReceived + length > sizeof(gUDSContext.UDS_buffer)) {
    // We have received too many bytes, abort
    gUDSContext.UDS_inProgress = false;
#ifdef DEBUG_LOG
    logging.println("UDS Payload Overflow");
#endif  // DEBUG_LOG
    return false;
  }
  memcpy(&gUDSContext.UDS_buffer[gUDSContext.UDS_bytesReceived], payload, length);
  gUDSContext.UDS_bytesReceived += length;
  gUDSContext.UDS_lastFrameMillis = millis();

  // If we’ve reached or exceeded the expected length, mark complete
  if (gUDSContext.UDS_bytesReceived >= gUDSContext.UDS_expectedLength) {
    gUDSContext.UDS_inProgress = false;
  }
  return true;
}

bool UdsCanBattery::isUDSMessageComplete() {
  return (!gUDSContext.UDS_inProgress && gUDSContext.UDS_bytesReceived > 0);
}

void UdsCanBattery::print_formatted_dtc(uint32_t dtc24, uint8_t status) {
  // DTC bytes: A B C (24 bits). SAE letter from top 2 bits of A.
  uint8_t A = (dtc24 >> 16) & 0xFF;
  uint8_t B = (dtc24 >> 8) & 0xFF;
  // uint8_t C =  dtc24        & 0xFF; // often a failure-type byte; keep if you need it

  const char sysMap[4] = {'P', 'C', 'B', 'U'};
  char sys = sysMap[(A & 0xC0) >> 6];

  // Four digits: D1 D2 D3 D4 from the remaining nibbles of A and B
  uint8_t d1 = (A & 0x30) >> 4;
  uint8_t d2 = (A & 0x0F);
  uint8_t d3 = (B & 0xF0) >> 4;
  uint8_t d4 = (B & 0x0F);

  logging.printf("DTC %c%X%X%X%X  status=0x%X [", sys, d1, d2, d3, d4, status);

  const struct {
    uint8_t bit;
    const char* label;
  } statusFlags[] = {
      {0x08, "Confirmed"}, {0x04, "Pending"},           {0x20, "FailSinceClear"},
      {0x01, "Fail"},      {0x10, "NotCompSinceClear"}, {0x40, "NotCompThisCycle"},
      {0x80, "MIL"},       {0x02, "FailThisCycle"},
  };

  bool first = true;
  for (size_t i = 0; i < sizeof(statusFlags) / sizeof(statusFlags[0]); i++) {
    if (status & statusFlags[i].bit) {
      if (!first) {
        logging.print(", ");
      }
      first = false;
      logging.print(statusFlags[i].label);
    }
  }

  if (first)
    logging.print("NoFlags");
  logging.println("]");
}

bool UdsCanBattery::handle_incoming_uds_can_frame(CAN_frame rx_frame) {
  if (rx_frame.ID >= MIN_UDS_RESPONSE_ID && rx_frame.ID <= MAX_UDS_RESPONSE_ID) {
    // logging.printf("Got UDS [%03X] %02X %02X %02X %02X %02X %02X %02X %02X\n", rx_frame.ID,
    //                rx_frame.data.u8[0], rx_frame.data.u8[1], rx_frame.data.u8[2], rx_frame.data.u8[3],
    //                rx_frame.data.u8[4], rx_frame.data.u8[5], rx_frame.data.u8[6], rx_frame.data.u8[7]);

    uint8_t pciByte = rx_frame.data.u8[0];  // 0x10/0x21/etc.
    uint8_t pciType = pciByte >> 4;         // 0=SF,1=FF,2=CF,3=FC
    uint8_t pciLower = pciByte & 0x0F;      // length nibble or sequence
    switch (pciType) {
      case 0x0: {  // Single Frame (SF)
        uint8_t sid = rx_frame.data.u8[1];

        // Positive response to Diagnostic Session Control (0x10 -> 0x50)
        if (sid == 0x50) {
          uint8_t sub = rx_frame.data.u8[2];  // 0x01=Default, 0x03=Extended, etc.

          if (sub == 0x03 || sub == 0x01) {
            // logging.print("entered ");
            // logging.println(sub == 0x03 ? "extended diagnostic session" : "default session");

            // This UDS transaction is done
            uds_busy_timeout = 0;
          }
          break;
        }

        // Positive response to clear DTC request
        if (sid == 0x54) {
          //  [0] PCI
          //  [1] SID (0x54)
          //  [2] DTC high byte  (0x02)
          //  [3] DTC mid byte   (0x93)
          //  [4] DTC low byte   (0x00)
          uint8_t b2 = rx_frame.data.u8[2];
          uint8_t b3 = rx_frame.data.u8[3];
          uint8_t b4 = rx_frame.data.u8[4];

          bool allDtcCleared = ((b2 == 0xFF && b3 == 0xFF && b4 == 0xFF) || (b2 == 0xAA && b3 == 0xAA && b4 == 0xAA));

          if (allDtcCleared) {
            logging.println("UDS: positive response, ALL DTCs cleared");
            user_request_clear_dtc = false;
          } else {
            logging.printf("UDS: positive ClearDTC response, group/DTC = %02X %02X %02X\n", b2, b3, b4);
          }

          uds_busy_timeout = 0;
          break;
        }

        // Negative response
        if (sid == 0x7F) {
          uint8_t origSid = rx_frame.data.u8[2];
          uint8_t nrc = rx_frame.data.u8[3];

          logging.printf("UDS negative response to 0x%02X: NRC=0x%02X\n", origSid, nrc);

          if (nrc != 0x78) {  // 0x78 = Response Pending; otherwise we’re done with this tx
            uds_busy_timeout = 0;
          }
          break;
        }

        if (sid == 0x62) {  // ReadDataByIdentifier response
          uint16_t did = (rx_frame.data.u8[2] << 8) | rx_frame.data.u8[3];

          // logging.printf("UDS [%03X] DID 0x%04X: %02X %02X %02X %02X\n", rx_frame.ID, did, rx_frame.data.u8[4],
          //                rx_frame.data.u8[5], rx_frame.data.u8[6], rx_frame.data.u8[7]);

          // The UDS transaction is now complete
          uds_busy_timeout = 0;

          // Value is 1-4 bytes (big endian), fit it into a uint32_t
          uint32_t value = 0;
          if (pciLower == 4) {
            value = rx_frame.data.u8[4];
          } else if (pciLower == 5) {
            value = (rx_frame.data.u8[4] << 8) | rx_frame.data.u8[5];
          } else if (pciLower == 6) {
            value = (rx_frame.data.u8[4] << 16) | (rx_frame.data.u8[5] << 8) | rx_frame.data.u8[6];
          } else if (pciLower == 7) {
            value = (rx_frame.data.u8[4] << 24) | (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) |
                    rx_frame.data.u8[7];
          }

          next_pid = handle_pid(did, value, &rx_frame.data.u8[4], pciLower - 3, UdsStatus::OK);
        }
        break;
      }

      case 0x1: {  // First Frame (FF)
        const bool allow_multiframe_pids = (next_pid & SHORT_PID) == 0;

        uint16_t totalLength = (uint16_t(pciLower) << 8) | rx_frame.data.u8[1];
        if (rx_frame.DLC == 8 && rx_frame.data.u8[2] == 0x62 && !allow_multiframe_pids) {
          // Multi-frame PID, but we'll only look at the first frame and ignore the rest.
          // This avoids tying up the BMS with regular multi-frame queries.

          uint16_t did = (rx_frame.data.u8[3] << 8) | rx_frame.data.u8[4];
          uint32_t value = (rx_frame.data.u8[5] << 16) | (rx_frame.data.u8[6] << 8) | rx_frame.data.u8[7];
          next_pid = handle_pid(did, value, &rx_frame.data.u8[5], 3, UdsStatus::OK_SHORT);
        } else if (rx_frame.DLC >= 4) {  //} && rx_frame.data.u8[2] == 0x59 && rx_frame.data.u8[3] == 0x02) {
          // Probably a multi-frame DTC response. Start storing it.

          startUDSMultiFrameReception(totalLength - 1, rx_frame.data.u8[2]);
          // FF payload starts at data[3]
          uint8_t avail = rx_frame.DLC - 3;
          uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
          uint8_t toStore = uint8_t(remain < avail ? remain : avail);
          //uint8_t toStore =
          if (toStore)
            storeUDSPayload(&rx_frame.data.u8[3], toStore);

          //logging.printf("Requesting next frames\n");

          // Ask for the next frames in the sequence
          UDS_RQ_CONTINUE_MULTIFRAME.ID = uds_address;
          transmit_can_frame(&UDS_RQ_CONTINUE_MULTIFRAME);
          uds_busy_timeout = 5;  // 0.5s
        }

        break;
      }
      case 0x2: {  // Consecutive Frame (CF)
        if (!gUDSContext.UDS_inProgress)
          break;
        uint8_t avail = (rx_frame.DLC > 1) ? (rx_frame.DLC - 1) : 0;  // CF payload at data[1]
        uint16_t remain = gUDSContext.UDS_expectedLength - gUDSContext.UDS_bytesReceived;
        uint8_t toStore = uint8_t(remain < avail ? remain : avail);
        if (toStore)
          storeUDSPayload(&rx_frame.data.u8[1], toStore);

        gUDSContext.receivedInBatch++;
        if (gUDSContext.receivedInBatch == 3) {
          // After 3 CFs, we send a Flow Control frame to keep things moving
          transmit_can_frame(&UDS_RQ_CONTINUE_MULTIFRAME);
          gUDSContext.receivedInBatch = 0;
        }

        if (isUDSMessageComplete()) {

          if (gUDSContext.UDS_moduleID == 0x59) {
            const uint8_t* p = gUDSContext.UDS_buffer;
            uint16_t len = gUDSContext.UDS_bytesReceived;

            // moduleID: SID(0x59), 0: subfunc(0x02), 1: status availability mask
            if (len >= 2 && p[0] == 0x02) {
              uint16_t off = 2;

              logging.printf("UDS DTC list (%d bytes of data)\n", len - off);

              // entries are 3-byte DTC + 1-byte status
              while (off + 4 <= len) {
                uint32_t dtc = (uint32_t(p[off]) << 16) | (uint32_t(p[off + 1]) << 8) | p[off + 2];
                uint8_t status = p[off + 3];

                print_formatted_dtc(dtc, status);
                off += 4;
              }
            }
          } else if (gUDSContext.UDS_moduleID == 0x62) {
            uint16_t did = (gUDSContext.UDS_buffer[0] << 8) | gUDSContext.UDS_buffer[1];
            // We read a multi-frame PID response
            //logging.printf("UDS multi-frame response for DID 0x%04X: %d bytes\n", did, gUDSContext.UDS_bytesReceived - 2);
            next_pid = handle_pid(did,
                                  ((gUDSContext.UDS_buffer[2] << 24) | (gUDSContext.UDS_buffer[3] << 16) |
                                   (gUDSContext.UDS_buffer[4] << 8) | gUDSContext.UDS_buffer[5]),
                                  &gUDSContext.UDS_buffer[2], gUDSContext.UDS_bytesReceived - 2, UdsStatus::OK);
          }

          user_request_read_dtc = false;
          uds_busy_timeout = 0;

          // ready for the next transaction
          gUDSContext.UDS_inProgress = false;
          gUDSContext.UDS_expectedLength = 0;
          gUDSContext.UDS_bytesReceived = 0;
        }
        break;
      }
      case 0x3: {  // Flow Control from ECU (rare)
        // optional: parse / ignore
        break;
      }
    }

    return true;
  }

  return false;
}

void UdsCanBattery::setup_uds(uint16_t uds_address, uint32_t first_pid) {
  this->uds_address = uds_address;
  this->first_pid = first_pid;
  this->next_pid = first_pid;
}

bool UdsCanBattery::supports_read_DTC() {
  return true;
}

bool UdsCanBattery::supports_reset_DTC() {
  return true;
}

void UdsCanBattery::read_DTC() {
  user_request_read_dtc = true;
}

void UdsCanBattery::reset_DTC() {
  user_request_clear_dtc = true;
}
