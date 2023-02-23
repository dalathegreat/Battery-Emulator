// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_ERROR_H
#define _MODBUS_ERROR_H
#include "ModbusTypeDefs.h"

using namespace Modbus;  // NOLINT

class ModbusError {
public:
  // Constructor with error code
  inline explicit ModbusError(Error e) : err(e) {}
  // Empty constructor defaults to 0
  inline ModbusError() : err(SUCCESS) {}
  // Assignment operators
  inline ModbusError& operator=(const ModbusError& e) { err = e.err; return *this; }
  inline ModbusError& operator=(const Error e) { err = e; return *this; }
  // Copy constructor
  inline ModbusError(const ModbusError& m) : err(m.err) {}
  // Equality comparison
  inline bool operator==(const ModbusError& m) { return (err == m.err); }
  inline bool operator==(const Error e) { return (err == e); }
  // Inequality comparison
  inline bool operator!=(const ModbusError& m) { return (err != m.err); }
  inline bool operator!=(const Error e) { return (err != e); }
  inline explicit operator Error() { return err; }
  inline operator int() { return static_cast<int>(err); }

#ifndef MINIMAL
  inline explicit operator const char *() { return getText(err); }
#endif

private:
  Error err;          // The error code

#ifndef MINIMAL
// Return error as static text
  inline static const char *getText(Error err) {
    switch (err) {
    case SUCCESS               : // 0x00,
      return "Success";
      break;
    case ILLEGAL_FUNCTION      : // 0x01,
      return "Illegal function code";
      break;
    case ILLEGAL_DATA_ADDRESS  : // 0x02,
      return "Illegal data address";
      break;
    case ILLEGAL_DATA_VALUE    : // 0x03,
      return "Illegal data value";
      break;
    case SERVER_DEVICE_FAILURE : // 0x04,
      return "Server device failure";
      break;
    case ACKNOWLEDGE           : // 0x05,
      return "Acknowledge";
      break;
    case SERVER_DEVICE_BUSY    : // 0x06,
      return "Server device busy";
      break;
    case NEGATIVE_ACKNOWLEDGE  : // 0x07,
      return "Negative acknowledge";
      break;
    case MEMORY_PARITY_ERROR   : // 0x08,
      return "Memory parity error";
      break;
    case GATEWAY_PATH_UNAVAIL  : // 0x0A,
      return "Gateway path unavailable";
      break;
    case GATEWAY_TARGET_NO_RESP: // 0x0B,
      return "Gateway target not responding";
      break;
    case TIMEOUT               : // 0xE0,
      return "Timeout";
      break;
    case INVALID_SERVER        : // 0xE1,
      return "Invalid server";
      break;
    case CRC_ERROR             : // 0xE2, // only for Modbus-RTU
      return "CRC check error";
      break;
    case FC_MISMATCH           : // 0xE3,
      return "Function code mismatch";
      break;
    case SERVER_ID_MISMATCH    : // 0xE4,
      return "Server ID mismatch";
      break;
    case PACKET_LENGTH_ERROR   : // 0xE5,
      return "Packet length error";
      break;
    case PARAMETER_COUNT_ERROR : // 0xE6,
      return "Wrong # of parameters";
      break;
    case PARAMETER_LIMIT_ERROR : // 0xE7,
      return "Parameter out of bounds";
      break;
    case REQUEST_QUEUE_FULL    : // 0xE8,
      return "Request queue full";
      break;
    case ILLEGAL_IP_OR_PORT    : // 0xE9,
      return "Illegal IP or port";
      break;
    case IP_CONNECTION_FAILED  : // 0xEA,
      return "IP connection failed";
      break;
    case TCP_HEAD_MISMATCH     : // 0xEB,
      return "TCP header mismatch";
      break;
    case EMPTY_MESSAGE         : // 0xEC,
      return "Incomplete request";
      break;
    case ASCII_FRAME_ERR       : // 0xED,
      return "Invalid ASCII frame";
      break;
    case ASCII_CRC_ERR         : // 0xEE,
      return "Invalid ASCII CRC";
      break;
    case ASCII_INVALID_CHAR    : // 0xEF,
      return "Invalid ASCII character";
      break;
    case BROADCAST_ERROR       : // 0xF0,
      return "Broadcast data invalid";
      break;
    case UNDEFINED_ERROR       : // 0xFF  // otherwise uncovered communication error
    default:
      return "Unspecified error";
      break;
    }
    return "What?";
  }

#endif

};

#endif
