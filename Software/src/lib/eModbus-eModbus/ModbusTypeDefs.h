// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_TYPEDEFS_H
#define _MODBUS_TYPEDEFS_H
#include <stdint.h>
#include <stddef.h>
#include <cstdint>

namespace Modbus {

enum FunctionCode : uint8_t {
  ANY_FUNCTION_CODE       = 0x00, // Only valid for server to register function codes
  READ_COIL               = 0x01,
  READ_DISCR_INPUT        = 0x02,
  READ_HOLD_REGISTER      = 0x03,
  READ_INPUT_REGISTER     = 0x04,
  WRITE_COIL              = 0x05,
  WRITE_HOLD_REGISTER     = 0x06,
  READ_EXCEPTION_SERIAL   = 0x07,
  DIAGNOSTICS_SERIAL      = 0x08,
  READ_COMM_CNT_SERIAL    = 0x0B,
  READ_COMM_LOG_SERIAL    = 0x0C,
  WRITE_MULT_COILS        = 0x0F,
  WRITE_MULT_REGISTERS    = 0x10,
  REPORT_SERVER_ID_SERIAL = 0x11,
  READ_FILE_RECORD        = 0x14,
  WRITE_FILE_RECORD       = 0x15,
  MASK_WRITE_REGISTER     = 0x16,
  R_W_MULT_REGISTERS      = 0x17,
  READ_FIFO_QUEUE         = 0x18,
  ENCAPSULATED_INTERFACE  = 0x2B,
  USER_DEFINED_41         = 0x41,
  USER_DEFINED_42         = 0x42,
  USER_DEFINED_43         = 0x43,
  USER_DEFINED_44         = 0x44,
  USER_DEFINED_45         = 0x45,
  USER_DEFINED_46         = 0x46,
  USER_DEFINED_47         = 0x47,
  USER_DEFINED_48         = 0x48,
  USER_DEFINED_64         = 0x64,
  USER_DEFINED_65         = 0x65,
  USER_DEFINED_66         = 0x66,
  USER_DEFINED_67         = 0x67,
  USER_DEFINED_68         = 0x68,
  USER_DEFINED_69         = 0x69,
  USER_DEFINED_6A         = 0x6A,
  USER_DEFINED_6B         = 0x6B,
  USER_DEFINED_6C         = 0x6C,
  USER_DEFINED_6D         = 0x6D,
  USER_DEFINED_6E         = 0x6E,
};

enum Error : uint8_t {
  SUCCESS                = 0x00,
  ILLEGAL_FUNCTION       = 0x01,
  ILLEGAL_DATA_ADDRESS   = 0x02,
  ILLEGAL_DATA_VALUE     = 0x03,
  SERVER_DEVICE_FAILURE  = 0x04,
  ACKNOWLEDGE            = 0x05,
  SERVER_DEVICE_BUSY     = 0x06,
  NEGATIVE_ACKNOWLEDGE   = 0x07,
  MEMORY_PARITY_ERROR    = 0x08,
  GATEWAY_PATH_UNAVAIL   = 0x0A,
  GATEWAY_TARGET_NO_RESP = 0x0B,
  TIMEOUT                = 0xE0,
  INVALID_SERVER         = 0xE1,
  CRC_ERROR              = 0xE2, // only for Modbus-RTU
  FC_MISMATCH            = 0xE3,
  SERVER_ID_MISMATCH     = 0xE4,
  PACKET_LENGTH_ERROR    = 0xE5,
  PARAMETER_COUNT_ERROR  = 0xE6,
  PARAMETER_LIMIT_ERROR  = 0xE7,
  REQUEST_QUEUE_FULL     = 0xE8,
  ILLEGAL_IP_OR_PORT     = 0xE9,
  IP_CONNECTION_FAILED   = 0xEA,
  TCP_HEAD_MISMATCH      = 0xEB,
  EMPTY_MESSAGE          = 0xEC,
  ASCII_FRAME_ERR        = 0xED,
  ASCII_CRC_ERR          = 0xEE,
  ASCII_INVALID_CHAR     = 0xEF,
  BROADCAST_ERROR        = 0xF0,
  UNDEFINED_ERROR        = 0xFF  // otherwise uncovered communication error
};

// Readable expression for the "illegal" server ID of 0
#define ANY_SERVER 0x00

#ifndef MINIMAL

// Constants for float and double re-ordering
#define SWAP_BYTES     0x01
#define SWAP_REGISTERS 0x02
#define SWAP_WORDS     0x04
#define SWAP_NIBBLES   0x08

const uint8_t swapTables[8][8] = {
  { 0, 1, 2, 3, 4, 5, 6, 7 },  // no swap
  { 1, 0, 3, 2, 5, 4, 7, 6 },  // bytes only
  { 2, 3, 0, 1, 6, 7, 4, 5 },  // registers only
  { 3, 2, 1, 0, 7, 6, 5, 4 },  // registers and bytes
  { 4, 5, 6, 7, 0, 1, 2, 3 },  // words only (double)
  { 5, 4, 7, 6, 1, 0, 3, 2 },  // words and bytes (double)
  { 6, 7, 4, 5, 2, 3, 0, 1 },  // words and registers (double)
  { 7, 6, 5, 4, 3, 2, 1, 0 }   // Words, registers and bytes (double)
};

enum FCType : uint8_t {
  FC01_TYPE,         // Two uint16_t parameters (FC 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
  FC07_TYPE,         // no additional parameter (FCs 0x07, 0x0b, 0x0c, 0x11)
  FC0F_TYPE,         // two uint16_t parameters, a uint8_t length byte and a uint8_t* pointer to array of bytes (FC 0x0f)
  FC10_TYPE,         // two uint16_t parameters, a uint8_t length byte and a uint16_t* pointer to array of words (FC 0x10)
  FC16_TYPE,         // three uint16_t parameters (FC 0x16)
  FC18_TYPE,         // one uint16_t parameter (FC 0x18)
  FCGENERIC,         // for FCs not yet explicitly coded (or too complex)
  FCUSER,            // No checks except the server ID
  FCILLEGAL,         // not allowed function codes
};

// FCT: static class to hold the types of function codes
class FCT {
protected:
  static FCType table[128];     // data table
  FCT() = delete;                // No instances allowed
  FCT(const FCT&) = delete;      // No copy constructor
  FCT& operator=(const FCT& other) = delete; // No assignment either
public:
  // getType: get the function code type for a given function code
  static FCType getType(uint8_t functionCode);

  // setType: change the type of a function code.
  // This is possible only for the codes undefined yet and will return
  // the effective type
  static FCType redefineType(uint8_t functionCode, const FCType type = FCUSER);
};

#endif

}  // namespace Modbus

#endif
