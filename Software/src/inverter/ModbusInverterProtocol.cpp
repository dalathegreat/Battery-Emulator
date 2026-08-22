#include "ModbusInverterProtocol.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "../lib/eModbus-eModbus/ModbusServerRTU.h"

// Fill up the RS485 DE pin with -1 if not available
// The library ignores it if its defibe to -1
#ifndef RS485_DE_PIN
#define RS485_DE_PIN -1
#endif
// Creates a ModbusRTU server instance with 2000ms timeout
ModbusInverterProtocol::ModbusInverterProtocol(int serverId) : MBserver(2000, RS485_DE_PIN) {
  _serverId = serverId;

  MBserver.registerWorker(_serverId, READ_HOLD_REGISTER,
                          [this](ModbusMessage request) -> ModbusMessage { return FC03(request); });
  MBserver.registerWorker(_serverId, WRITE_HOLD_REGISTER,
                          [this](ModbusMessage request) -> ModbusMessage { return FC06(request); });
  MBserver.registerWorker(_serverId, WRITE_MULT_REGISTERS,
                          [this](ModbusMessage request) -> ModbusMessage { return FC16(request); });
  MBserver.registerWorker(_serverId, R_W_MULT_REGISTERS,
                          [this](ModbusMessage request) -> ModbusMessage { return FC23(request); });
}

ModbusInverterProtocol::~ModbusInverterProtocol() {
  MBserver.unregisterWorker(_serverId, READ_HOLD_REGISTER);
  MBserver.unregisterWorker(_serverId, WRITE_HOLD_REGISTER);
  MBserver.unregisterWorker(_serverId, WRITE_MULT_REGISTERS);
  MBserver.unregisterWorker(_serverId, R_W_MULT_REGISTERS);
}

#ifdef MODBUS_LOG_INVERTER_WRITES
/* Register 401 is the watchdog toggle the Fronius flips between 0x00FF and 0xFF00, already consumed
   by BydModbusInverter::verify_inverter_modbus(). Logging it would only produce a line a minute. */
static const uint16_t MODBUS_HANDLED_WRITE_ADDR = 401;
/* ControlData.UTC, the inverter's clock. All four words carry a new value on essentially every
   write, so the backoff never settles into silence, and nothing here reacts to them anyway -
   handle_inverter_control_data() keeps the value for display only. Not worth a log line. */
static const uint16_t MODBUS_UTC_FIRST_ADDR = 403;
static const uint16_t MODBUS_UTC_LAST_ADDR = 406;

/* Names from the Fronius GenericStorage register map. Only ControlData (400-408) and the droop law
   block (500-506) are declared master-writable there, so a write anywhere else means the inverter
   uses a register we believe to be read-only - worth seeing in the log verbatim. */
static const char* inverter_write_target(uint16_t addr) {
  switch (addr) {
    case 400:
      return "ControlData.opMode";
    case 402:
      return "ControlData.WatchDogTimeout";
    case 403:
    case 404:
    case 405:
    case 406:
      return "ControlData.UTC";
    case 407:
      return "ControlData.RebootCommand";
    case 408:
      return "ControlData.DarkstartEnable";
    case 500:
      return "Drooplaw.DeadbandLowVoltage";
    case 501:
      return "Drooplaw.DeadbandHighVoltage";
    case 502:
      return "Drooplaw.PowerLimitCharge";
    case 503:
      return "Drooplaw.PowerLimitDischarge";
    case 504:
      return "Drooplaw.SlopeCharge";
    case 505:
      return "Drooplaw.SlopeDischarge";
    case 506:
      return "Drooplaw.SetDCDCCharacteristic";
    default:
      break;
  }
  if (addr >= 100 && addr <= 167) {
    return "DeviceIdentificationData, expected read-only";
  }
  if (addr >= 200 && addr <= 212) {
    return "NameplateData, expected read-only";
  }
  if (addr >= 300 && addr <= 323) {
    return "MonitoringData, expected read-only";
  }
  if (addr >= 1000 && addr <= 1099) {
    return "FaultBuffers, expected read-only";
  }
  return "not in the known register map";
}

void ModbusInverterProtocol::log_inverter_write(uint16_t addr, uint16_t value) {
  if (addr == MODBUS_HANDLED_WRITE_ADDR) {
    return;
  }
  if (addr >= MODBUS_UTC_FIRST_ADDR && addr <= MODBUS_UTC_LAST_ADDR) {
    return;
  }

  const uint32_t now = millis();
  InverterWriteLogSlot* slot = nullptr;

  for (uint8_t i = 0; i < WRITE_LOG_SLOTS; i++) {  // Already tracking this address?
    if (write_log[i].used && write_log[i].addr == addr) {
      slot = &write_log[i];
      break;
    }
  }
  if (slot == nullptr) {  // No, take a free slot
    for (uint8_t i = 0; i < WRITE_LOG_SLOTS; i++) {
      if (!write_log[i].used) {
        slot = &write_log[i];
        break;
      }
    }
  }
  if (slot == nullptr) {  // Table full, evict the least recently logged address
    slot = &write_log[0];
    for (uint8_t i = 1; i < WRITE_LOG_SLOTS; i++) {
      if ((int32_t)(write_log[i].last_log_ms - slot->last_log_ms) < 0) {
        slot = &write_log[i];
      }
    }
    slot->used = false;
  }

  const bool first_seen = !slot->used;
  if (!first_seen) {
    // Same value again carries no new information, and a fresh value still has to wait out the
    // backoff. Either way remember what we saw, so the next line printed shows the current value.
    if (slot->value == value || (uint32_t)(now - slot->last_log_ms) < (uint32_t)slot->backoff_s * 1000u) {
      slot->value = value;
      if (slot->suppressed < UINT16_MAX) {
        slot->suppressed++;
      }
      return;
    }
  }

  if (first_seen) {
    LOG_SET_NEXT_SEVERITY(5);  // notice
    DEBUG_PRINTF("Modbus inverter write, first time seen: reg %u (%s) = %u (0x%04X)\n", addr,
                 inverter_write_target(addr), value, value);
    slot->backoff_s = WRITE_LOG_BACKOFF_MIN_S;
  } else {
    DEBUG_PRINTF("Modbus inverter write: reg %u (%s) = %u (0x%04X), %u write(s) not logged since the last line\n", addr,
                 inverter_write_target(addr), value, value, slot->suppressed);
    const uint32_t next_backoff_s = (uint32_t)slot->backoff_s * 2u;
    slot->backoff_s = (next_backoff_s > WRITE_LOG_BACKOFF_MAX_S) ? WRITE_LOG_BACKOFF_MAX_S : (uint16_t)next_backoff_s;
  }

  slot->used = true;
  slot->addr = addr;
  slot->value = value;
  slot->last_log_ms = now;
  slot->suppressed = 0;
}
#endif  // MODBUS_LOG_INVERTER_WRITES

void ModbusInverterProtocol::notify_inverter_communication() {
  if (!inverter_detected) {
    inverter_detected = true;
    set_event(EVENT_MODBUS_INVERTER_DETECTED, 1);
  }
}

// Server function to handle FC 0x03
ModbusMessage ModbusInverterProtocol::FC03(ModbusMessage request) {
  notify_inverter_communication();
  ModbusMessage response;  // The Modbus message we are going to give back
  uint16_t addr = 0;       // Start address
  uint16_t words = 0;      // # of words requested
  request.get(2, addr);    // read address from request
  request.get(4, words);   // read # of words from request

  // Address overflow?
  if ((addr + words) > MBPV_MAX) {
    // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    logging.printf("Modbus FC03 error: illegal request addr=%d words=%d\n", addr, words);
    return response;
  }

  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
  for (uint8_t i = 0; i < words; ++i) {
    // send increasing data values
    response.add((uint16_t)(mbPV[addr + i]));
  }

  return response;
}

// Server function to handle FC 0x06
ModbusMessage ModbusInverterProtocol::FC06(ModbusMessage request) {
  notify_inverter_communication();
  ModbusMessage response;  // The Modbus message we are going to give back
  uint16_t addr = 0;       // Start address
  uint16_t val = 0;        // value to write
  request.get(2, addr);    // read address from request
  request.get(4, val);     // read # of words from request

  // Address overflow?
  if ((addr) > MBPV_MAX) {
    // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    logging.printf("Modbus FC06 error: illegal request addr=%d val=%d\n", addr, val);
    return response;
  }

  // Do the write
  log_inverter_write(addr, val);
  mbPV[addr] = val;

  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), mbPV[addr]);
  return response;
}

// Server function to handle FC 0x10 (FC16)
ModbusMessage ModbusInverterProtocol::FC16(ModbusMessage request) {
  notify_inverter_communication();
  ModbusMessage response;  // The Modbus message we are going to give back
  uint16_t addr = 0;       // Start address
  uint16_t words = 0;      // total words to write
  uint8_t bytes = 0;       // # of data bytes in request
  uint16_t val = 0;        // value to be written
  request.get(2, addr);    // read address from request
  request.get(4, words);   // read # of words from request
  request.get(6, bytes);   // read # of data bytes from request (seems redundant with # of words)

  // # of registers proper?
  if ((bytes != (words * 2))  // byte count in request must match # of words in request
      || (words > 123))       // can't support more than this in request packet
  {                           // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
    logging.printf("Modbus FC16 error: bad registers addr=%d words=%d bytes=%d\n", addr, words, bytes);
    return response;
  }
  // Address overflow?
  if ((addr + words) > MBPV_MAX) {
    // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    logging.printf("Modbus FC16 error: overflow addr=%d words=%d\n", addr, words);
    return response;
  }

  // Do the writes
  for (uint8_t i = 0; i < words; ++i) {
    request.get(7 + (i * 2), val);  //data starts at byte 6 in request packet
    log_inverter_write((uint16_t)(addr + i), val);
    mbPV[addr + i] = val;
  }

  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), addr, words);
  return response;
}

// Server function to handle FC 0x17 (FC23)
ModbusMessage ModbusInverterProtocol::FC23(ModbusMessage request) {
  notify_inverter_communication();
  ModbusMessage response;        // The Modbus message we are going to give back
  uint16_t read_addr = 0;        // Start address for read
  uint16_t read_words = 0;       // # of words requested for read
  uint16_t write_addr = 0;       // Start address for write
  uint16_t write_words = 0;      // total words to write
  uint8_t write_bytes = 0;       // # of data bytes in write request
  uint16_t write_val = 0;        // value to be written
  request.get(2, read_addr);     // read address from request
  request.get(4, read_words);    // read # of words from request
  request.get(6, write_addr);    // read address from request
  request.get(8, write_words);   // read # of words from request
  request.get(10, write_bytes);  // read # of data bytes from request (seems redundant with # of words)

  // ERROR CHECKS
  // # of registers proper?
  if ((write_bytes != (write_words * 2))  // byte count in request must match # of words in request
      || (write_words > 121)              // can't fit more than this in the packet for FC23
      || (read_words > 125))              // can't fit more than this in the response packet
  {                                       // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
    logging.printf("Modbus FC23 error: bad registers write_addr=%d write_words=%d write_bytes=%d read_words=%d\n",
                   write_addr, write_words, write_bytes, read_words);
    return response;
  }
  // Address overflow?
  if (((write_addr + write_words) > MBPV_MAX) ||
      ((read_addr + read_words) > MBPV_MAX)) {  // Yes - send respective error response
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    logging.printf("Modbus FC23 error: overflow write_addr=%d write_words=%d read_addr=%d read_words=%d\n", write_addr,
                   write_words, read_addr, read_words);
    return response;
  }

  //WRITE SECTION  - write is done before read for FC23
  // Do the writes
  for (uint8_t i = 0; i < write_words; ++i) {
    request.get(11 + (i * 2), write_val);  //data starts at byte 6 in request packet
    log_inverter_write((uint16_t)(write_addr + i), write_val);
    mbPV[write_addr + i] = write_val;
  }

  // READ SECTION
  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(read_words * 2));
  for (uint8_t i = 0; i < read_words; ++i) {
    // send increasing data values
    response.add((uint16_t)(mbPV[read_addr + i]));
  }

  return response;
}
