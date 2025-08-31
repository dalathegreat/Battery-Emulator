#include "ModbusInverterProtocol.h"
#include "../devboard/utils/logging.h"
#include "../lib/eModbus-eModbus/ModbusServerRTU.h"

// Creates a ModbusRTU server instance with 2000ms timeout
ModbusInverterProtocol::ModbusInverterProtocol(int serverId) : MBserver(2000) {
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

// Server function to handle FC 0x03
ModbusMessage ModbusInverterProtocol::FC03(ModbusMessage request) {
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
  mbPV[addr] = val;

  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), mbPV[addr]);
  return response;
}

// Server function to handle FC 0x10 (FC16)
ModbusMessage ModbusInverterProtocol::FC16(ModbusMessage request) {
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
    mbPV[addr + i] = val;
  }

  // Set up response
  response.add(request.getServerID(), request.getFunctionCode(), addr, words);
  return response;
}

// Server function to handle FC 0x17 (FC23)
ModbusMessage ModbusInverterProtocol::FC23(ModbusMessage request) {
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
