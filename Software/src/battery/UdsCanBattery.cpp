#include "UdsCanBattery.h"

#include <Arduino.h>
#include "../devboard/utils/logging.h"

// Timeouts (to wait for a UDS response) in 100ms ticks
constexpr uint16_t UDS_TIMEOUT_CLEAR_DTC = 25;
constexpr uint16_t UDS_TIMEOUT_READ_DTC = 20;
constexpr uint16_t UDS_TIMEOUT_READ_DID = 2;
constexpr uint16_t UDS_TIMEOUT_CONTINUE = 5;
constexpr uint16_t UDS_TIMEOUT_SESSION_CONTROL = 10;
constexpr uint16_t UDS_TIMEOUT_PAUSE = 10;
constexpr uint16_t UDS_TIMEOUT_RESET = 50;
constexpr uint16_t UDS_DEFAULT_ACTION_COOLDOWN = 10;
constexpr uint16_t UDS_PID_SCAN_INTERVAL = 2;
constexpr uint16_t UDS_PID_MAX_RETRIES = 10;

constexpr uint16_t UDS_POST_BMS_RESET_PAUSE = 5;

// #define UDS_DEBUG 1
//static constexpr bool UDS_DEBUG = false;

void UdsCanBattery::transmit_uds_can(unsigned long currentMillis) {
  // Called during the CAN transmit phase.

  // Poll the underlying ISO-TP layer (at the native ~1KHz rate).
  isotp_poll();

  // Otherwise we do UDS operations every 100ms.
  if (currentMillis - previousUdsMillis100 < INTERVAL_100_MS) {
    return;
  }
  previousUdsMillis100 = currentMillis;

  if (transaction_tick()) {
    // Transaction still active.
    return;
  }

  if (isotp_is_busy()) {
    // ISO-TP transaction in progress, wait for it to finish before sending new requests
    return;
  }

  if (transmit_uds_action()) {
    // There's an active action in progress.
    return;
  } else if (transmit_uds_pid_scan()) {
    // We did a PID scan step.
    return;
  }
}

bool UdsCanBattery::transmit_uds_action() {
  // Called during the CAN transmit phase, if there is no current UDS
  // transaction in progress. Will progress the current action (or end it if it
  // has timed out). Returns true if it did something, false if there is no action to progress.

  if (uds_action_attempts > uds_action_max_retries) {
    logging.println("UDS action exceeded max retries.");
    pending_action = UdsAction::NONE;
    uds_action_attempts = 0;
  }

  switch (pending_action) {
    case UdsAction::READ_DTC:
      uds_send(SID::ReadDTCInformation, (const uint8_t*)"\x02\x09", 2, UDS_TIMEOUT_READ_DTC);
      expected_response_sid = UDS_RESPONSE_SID_OF(SID::ReadDTCInformation);
      uds_action_attempts++;
      return true;
    case UdsAction::CLEAR_DTC:
      uds_send(SID::ClearDiagnosticInformation, (const uint8_t*)"\xFF\xFF\xFF", 3, UDS_TIMEOUT_CLEAR_DTC);
      expected_response_sid = UDS_RESPONSE_SID_OF(SID::ClearDiagnosticInformation);
      uds_action_attempts++;
      return true;
    case UdsAction::RESET_BMS:
      uds_send(SID::ECUReset, (const uint8_t*)"\x01", 1, UDS_TIMEOUT_RESET);
      expected_response_sid = UDS_RESPONSE_SID_OF(SID::ECUReset);
      uds_action_attempts++;
      // Reset the PID scanning here, in case the BMS never responds to this request.
      next_pid = 0;
      uds_started = false;
      return true;
    // case UdsAction::READ_MEMORY: {
    //   uint32_t size = 4;
    //   uint32_t address = 0x40000000;
    //   const uint8_t data[9] = {0x44, (uint8_t)((address >> 24) & 0xFF), (uint8_t)((address >> 16) & 0xFF),
    //                   (uint8_t)((address >> 8) & 0xFF), (uint8_t)(address & 0xFF), (uint8_t)((size >> 24) & 0xFF),
    //                   (uint8_t)((size >> 16) & 0xFF), (uint8_t)((size >> 8) & 0xFF), (uint8_t)(size & 0xFF)};
    //   uds_send(SID::ReadMemoryByAddress,
    //            data, sizeof(data), UDS_TIMEOUT_READ_DID);
    //   expected_response_sid = UDS_RESPONSE_SID_OF(SID::ReadMemoryByAddress);
    //   uds_action_attempts++;
    //   return true;
    //}
    case UdsAction::PAUSE:
      // Do a dummy transaction.
      uds_transaction_timeout = UDS_TIMEOUT_PAUSE;
      uds_action_attempts++;
      return true;
    default:
      break;
  }

  if (uds_action_cooldown > 0) {
    // During post-action cooldown
    uds_action_cooldown--;
  }

  // Subclasses can override this to send their own custom requests
  if (uds_action_cooldown <= 0 && transmit_custom_uds()) {
    return true;
  }

  // Not within an action right now
  return false;
}

bool UdsCanBattery::transaction_tick() {
  if (uds_transaction_timeout > 0) {
    uds_transaction_timeout--;

    // Still busy, do not send new requests
    if (uds_transaction_timeout > 0)
      return true;

    if (pending_action == UdsAction::NONE && pending_pid != 0) {
      on_uds_pid_scan_timeout();
    } else if (pending_action != UdsAction::NONE) {
      logging.println("UDS transaction timed out.");
    }

    uds_current_response_address = 0;
  }
  return false;
}

bool UdsCanBattery::handle_incoming_uds_can_frame(CAN_frame rx) {
  if (uds_current_response_address > 0 && rx.ID != uds_current_response_address) {
    // Not from the address we're mid-transaction with, ignore
    return false;
  } else if (uds_response_address > 0 && rx.ID != uds_response_address) {
    // Not from the address we're expecting responses from, ignore
    return false;
  } else if (rx.ID < MIN_UDS_RESPONSE_ID || rx.ID > MAX_UDS_RESPONSE_ID) {
    // Outside the range of potential UDS response IDs, ignore
    return false;
  }

#ifdef UDS_DEBUG
  logging.printf("UDS RX: ID=0x%03X DLC=%d data=", rx.ID, rx.DLC);
  for (int i = 0; i < rx.DLC; i++) {
    logging.printf("%02X ", rx.data.u8[i]);
  }
  logging.println();
#endif

  // Record the address the current transaction is coming from.
  uds_current_response_address = rx.ID;

  // Pass down to the ISO-TP layer for reassembly.
  isotp_receive(rx.data.u8, rx.DLC, ISOTP_TATYPE_PHYSICAL);

  return true;
}

void UdsCanBattery::on_isotp_can_tx(uint32_t can_id, const uint8_t* can_data, uint8_t can_dlc) {

#ifdef UDS_DEBUG
  logging.printf("UDS TX: ID=0x%03X DLC=%d data=", can_id, can_dlc);
  for (int i = 0; i < can_dlc; i++) {
    logging.printf("%02X ", can_data[i]);
  }
  logging.println();
#endif

  // This is called by isotp_poll() from transmit_uds_can(..)
  CAN_frame frame = {};
  frame.ID = uds_address;  // Ignore the can_id from the ISO-TP layer, use our own.
  frame.DLC = can_dlc;
  memcpy(frame.data.u8, can_data, can_dlc);
  transmit_can_frame(&frame);
}

void UdsCanBattery::on_isotp_rx_complete(const uint8_t* data, int len, isotp_tatype tatype) {
  // The ISO-TP layer has reassembled a complete UDS response, pass it on for processing.
  on_uds_receive(data, len);
}

// Automatic PID scanning support

static inline uint32_t parseBigEndianValue(const uint8_t* data, uint16_t length) {
  uint32_t val = 0;
  for (uint16_t i = 0; i < length && i < 4; i++) {
    val = (val << 8) | data[i];
  }
  return val;
}

bool UdsCanBattery::transmit_uds_pid_scan() {
  // Called during the transmit phase if there's nothing else to do. Will send
  // the next PID request in the scanning cycle.

  if (next_pid == 0) {
    // Reset PID cycle
    next_pid = first_pid;
  }

  if (next_pid) {
    if (++pid_age < UDS_PID_SCAN_INTERVAL) {
      //return false;
    }
    pid_age = 0;

    // Request the next PID

    pending_pid = next_pid;
    // uds_send(SID::ReadDataByIdentifier, {(uint8_t)((next_pid >> 8) & 0xFF), (uint8_t)(next_pid & 0xFF)},
    //          UDS_TIMEOUT_READ_DID);
    const uint8_t data[2] = {(uint8_t)((next_pid >> 8) & 0xFF), (uint8_t)(next_pid & 0xFF)};
    uds_send(SID::ReadDataByIdentifier, data, sizeof(data), UDS_TIMEOUT_READ_DID);
    return true;
  }
  return false;
}

bool UdsCanBattery::on_uds_pid_scan_response(uint8_t sid, const uint8_t* data, uint16_t len) {
  // Possibly a PID response - if so, handle and return true.

  if (sid == UDS_RESPONSE_SID_OF(SID::ReadDataByIdentifier)) {
    // This is a normal PID response, pass it to the handler
    uint16_t did = (data[1] << 8) | data[2];
    // Value starts at data[3]
    // Decode up to 4 bytes of value, big endian.
    uint32_t val = len > 3 ? parseBigEndianValue(&data[3], len - 3) : 0;

    if (!uds_started) {
      if (val != 0) {
        // We got a nonzero response, so UDS is working. Start the PID cycle.
        uds_started = true;
      } else {
        // Ignore responses until we get a non-zero value.
        return true;
      }
    }

    // The handler returns the next PID to query.
    next_pid = handle_pid(did, val, &data[3], len - 3, UdsStatus::OK);
    pending_pid = 0;
    pid_retries = 0;
    return true;
  } else if (sid == SID::NegativeResponse && len >= 3 && data[1] == (uint8_t)SID::ReadDataByIdentifier) {
    if (!uds_started) {
      // Ignore negative responses until we get a non-zero value.
      return true;
    }

    // This is a negative response to a PID request
    union {
      uint32_t u32;
      uint8_t u8[4];
    } val = {};
    next_pid = handle_pid(pending_pid, val.u32, val.u8, 4, UdsStatus::NEGATIVE_RESPONSE);
    pending_pid = 0;
    pid_retries = 0;
    return true;
  }

  return false;
}

void UdsCanBattery::on_uds_pid_scan_timeout() {
  // Called when a PID scan request times out.

  pid_retries++;
  if (pid_retries < UDS_PID_MAX_RETRIES) {
    // Keep retrying...
    return;
  }
  //logging.printf("UDS PID 0x%04X request timed out after %d retries\n", pending_pid, pid_retries);

  if (!uds_started) {
    // Ignore timeouts until we get a non-zero value.
    pid_retries = 0;
    return;
  }

  union {
    uint32_t u32;
    uint8_t u8[4];
  } val = {};
  next_pid = handle_pid(pending_pid, val.u32, val.u8, 4, UdsStatus::TIMEOUT);
  pending_pid = 0;
  pid_retries = 0;
}

void UdsCanBattery::on_uds_receive(const uint8_t* data, uint16_t len) {
  // We've received a complete UDS response message.

  if (pending_action == UdsAction::PAUSE) {
    // If we're pausing, ignore any responses we receive until the timeout
    // expires.
    return;
  }

  // The current transaction is now finished
  uds_transaction_timeout = 0;
  uds_current_response_address = 0;

  if (len < 1) {
    return;
  }

  const SID sid = (SID)data[0];

  if (sid != UDS_RESPONSE_SID_OF(SID::ReadDataByIdentifier)) {  //} && sid != SID::NegativeResponse) {
    logging.printf("UDS RX: SID=0x%02X data=", (uint8_t)sid);
    for (int i = 0; i < len; i++) {
      logging.printf("%02X ", data[i]);
    }
    logging.println();
  }

  if (pending_action == UdsAction::NONE) {
    // There's no pending action.

    if (on_custom_uds_response(sid, data, len)) {
      // This was a response for a custom request, and a subclass handled it.
      return;
    } else if (pending_pid != 0 && on_uds_pid_scan_response(sid, data, len)) {
      // This was a PID scan response, and we handled it.
      return;
    }

    return;
  }

  if (sid == SID::NegativeResponse && len >= 3) {
    SID origSid = (SID)data[1];
    uint8_t nrc = data[2];

    if (uds_action_attempts > uds_action_max_retries) {
      // We aren't going to retry, so give up on this action.
      //logging.printf("UDS negative response to 0x%02X: NRC=0x%02X\n", origSid, nrc);
      return;
    }

    switch (nrc) {
      case NegativeResponseCode::SecurityAccessDenied:
        // Request security key
        if (++uds_promotion_attempts < 3) {
          //logging.printf("UDS service 0x%02X denied, requesting security key\n", origSid);
          //uds_send(SID::SecurityAccess, {0x01}, UDS_TIMEOUT_SESSION_CONTROL);
          uds_send(SID::SecurityAccess, (const uint8_t*)"\x01", 1, UDS_TIMEOUT_SESSION_CONTROL);
        }
        return;

        // ?
        //logging.printf("UDS response pending for 0x%02X\n", origSid);
      //  break;
      case NegativeResponseCode::ServiceNotSupportedInActiveSession:
        if (++uds_promotion_attempts < 3) {
          // logging.printf("UDS service 0x%02X not supported in current session, trying to enter extended session\n",
          //                origSid);
          static constexpr uint8_t data[1] = {Session::ExtendedSession};
          uds_send(SID::DiagnosticSessionControl, data, sizeof(data), UDS_TIMEOUT_SESSION_CONTROL);
        }
        return;
      default:
        //logging.printf("UDS negative response to 0x%02X: NRC=0x%02X\n", origSid, nrc);
        break;
    }
  }

  if (expected_response_sid != 0 && sid != (SID)expected_response_sid) {
    // The reply wasn't in response to the original request we sent.
    if (sid == UDS_RESPONSE_SID_OF(SID::DiagnosticSessionControl)) {
      // We'll retry the original request in the next tick.
    } else if (sid == UDS_RESPONSE_SID_OF(SID::SecurityAccess) && len >= 4 && data[1] == 0x02) {
      // Got a security access seed, calculate the key and send it back.
      uint16_t seed = (data[2] << 8) | data[3];
      int32_t solution = calculate_uds_security_key(seed);
      if (solution < 0) {
        //logging.printf("Received security access seed 0x%04X, but no solution available\n", seed);
        return;
      }
      //logging.printf("Received seed 0x%04X, sending security access solution 0x%04X\n", seed, solution);
      uint8_t data[3] = {0x02, (uint8_t)((solution >> 8) & 0xFF), (uint8_t)(solution & 0xFF)};
      uds_send(SID::SecurityAccess, data, sizeof(data), UDS_TIMEOUT_SESSION_CONTROL);
    } else {
      // logging.printf("Received unexpected UDS response SID 0x%02X (expected 0x%02X), bytes: ", sid,
      //                expected_response_sid);
      // for (int i = 0; i < len; i++) {
      //   logging.printf("%02X ", data[i]);
      // }
      // logging.println();
    }
    return;
  }
  expected_response_sid = 0;

  on_uds_action_complete(sid, data, len);
}

void UdsCanBattery::on_uds_action_complete(uint8_t response_sid, const uint8_t* data, uint16_t len) {
  switch (response_sid) {
    case UDS_RESPONSE_SID_OF(SID::ReadDTCInformation):
      // memcpy(dtc_buffer, data, len);
      // dtc_len = len;
      handle_dtc_response(data, len);
      break;
    case UDS_RESPONSE_SID_OF(SID::ECUReset):
      logging.println("UDS ECUReset successful");
      // Avoid sending any UDS requests to give a chance to reset.
      uds_transaction_timeout = UDS_POST_BMS_RESET_PAUSE;
      // When we come back up we'll restart PID scanning afresh.
      next_pid = 0;
      uds_started = false;
      break;
    default:
      //logging.printf("Successful SID response 0x%02X\n", (uint8_t)response_sid);
      break;
  }

  pending_action = UdsAction::NONE;
  uds_action_attempts = 0;  //?
}

bool UdsCanBattery::perform_uds_action(UdsAction action, int16_t max_retries, uint32_t cooldown) {
  // Set the pending action and timeout. The transmit_uds_can function will then
  // trigger this action (and keep retrying until it completes or the timeout
  // expires).

  if (pending_action != UdsAction::NONE) {
    // Already an action in progress, ignore this request
    return false;
  }

  if (uds_action_cooldown > 0) {
    // Can't start a new action while we're still in cooldown from the last one
    return false;
  }

  pending_action = action;
  uds_action_max_retries = max_retries;
  uds_action_attempts = 0;
  uds_action_cooldown = cooldown;
  uds_promotion_attempts = 0;

  return true;
}

void UdsCanBattery::handle_dtc_response(const uint8_t* data, uint16_t len) {
  if (dtc == nullptr)
    return;

  if (data[1] != 0x02) {
    // Unexpected report type — treat as a failed readout.
    dtc->dtc_read_failed = true;
  } else {
    dtc->dtc_read_failed = false;
    int dtcStartIndex = 3;  // Skip 59 02 <statusAvailabilityMask>
    int availableBytes = len - dtcStartIndex;
    int maxDtcCount = availableBytes / 4;

    if (maxDtcCount > dtc->MAX_DTC_COUNT) {
      maxDtcCount = dtc->MAX_DTC_COUNT;
      logging.println("DTC count exceeds buffer, truncating");
    }
    if (maxDtcCount < 0)
      maxDtcCount = 0;

    for (int i = 0; i < maxDtcCount; i++) {
      int offset = dtcStartIndex + (i * 4);
      // Bounds check to ensure we don't read beyond the buffer
      if (offset + 3 > len)
        break;
      // Combine 3 bytes into a single uint32
      uint32_t dtcCode =
          ((uint32_t)data[offset] << 16) | ((uint32_t)data[offset + 1] << 8) | (uint32_t)data[offset + 2];
      uint8_t dtcStatus = data[offset + 3];
      dtc->dtc_codes[i] = dtcCode;
      dtc->dtc_status[i] = dtcStatus;
    }
    dtc->dtc_count = maxDtcCount;
  }
  dtc->dtc_last_read_millis = millis();
}

// Low level UDS send

//void UdsCanBattery::uds_send(SID service_id, const std::string_view data, uint32_t timeout) {
void UdsCanBattery::uds_send(SID service_id, const uint8_t* data, uint16_t length, uint32_t timeout) {
  uint8_t payload[256];
  // if (data.size() >= sizeof(payload)) {
  //   return;
  // }
  // payload[0] = static_cast<uint8_t>(service_id);
  // memcpy(&payload[1], data.data(), data.size());

  // isotp_send(payload, data.size() + 1);

  if (length >= sizeof(payload)) {
    return;
  }
  payload[0] = static_cast<uint8_t>(service_id);
  memcpy(&payload[1], data, length);
  isotp_send(payload, length + 1);

  if (service_id != SID::ReadDataByIdentifier) {
    logging.printf("UDS TX: SID=0x%02X data=", (uint8_t)service_id);
    for (int i = 0; i < length; i++) {
      logging.printf("%02X ", data[i]);
    }
    logging.println();
  }

  uds_transaction_timeout = timeout;
}

void UdsCanBattery::setup_uds(uint16_t uds_address, uint16_t uds_response_address, uint32_t first_pid) {
  this->uds_address = uds_address;
  isotp_init(uds_address);
  this->uds_response_address = uds_response_address;
  this->first_pid = first_pid;
  this->next_pid = first_pid;
  this->pending_pid = 0;
}

bool UdsCanBattery::supports_read_DTC() {
  return true;
}

bool UdsCanBattery::supports_reset_DTC() {
  return true;
}

bool UdsCanBattery::supports_reset_BMS() {
  return true;
}

void UdsCanBattery::read_DTC() {
  perform_uds_action(UdsAction::READ_DTC, 2, UDS_DEFAULT_ACTION_COOLDOWN);
}

void UdsCanBattery::reset_DTC() {
  perform_uds_action(UdsAction::CLEAR_DTC, 2, UDS_DEFAULT_ACTION_COOLDOWN);
}

void UdsCanBattery::reset_BMS() {
  // We send a long cooldown, to give the BMS time to reset.
  perform_uds_action(UdsAction::RESET_BMS, 2, 20);
}

void UdsCanBattery::enter_extended_diag() {
  // Enter extended diagnostic session
  static constexpr uint8_t data[1] = {Session::ExtendedSession};
  uds_send(SID::DiagnosticSessionControl, data, sizeof(data), UDS_TIMEOUT_SESSION_CONTROL);
}
