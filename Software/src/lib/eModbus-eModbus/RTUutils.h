// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _RTU_UTILS_H
#define _RTU_UTILS_H
#include <stdint.h>
#include <vector>
#include "Stream.h"
#include "ModbusTypeDefs.h"
#include <functional> 

typedef std::function<void(bool level)> RTScallback;

using namespace Modbus;  // NOLINT

// RTUutils is bundling the send, receive and CRC functions for Modbus RTU communications.
// RTU server and client will make use of it. 
// All functions are static!
class RTUutils {
public:
  friend class ModbusClientRTU;
  friend class ModbusServerRTU;

// calcCRC: calculate the CRC16 value for a given block of data
  static uint16_t calcCRC(const uint8_t *data, uint16_t len);

// calcCRC: calculate the CRC16 value for a given block of data
  static uint16_t calcCRC(ModbusMessage msg);

// validCRC #1: check the CRC in a block of data for validity
  static bool validCRC(const uint8_t *data, uint16_t len);

// validCRC #2: check the CRC of a block of data against a given one
  static bool validCRC(const uint8_t *data, uint16_t len, uint16_t CRC);

// validCRC #1: check the CRC in a message for validity
  static bool validCRC(ModbusMessage msg);

// validCRC #2: check the CRC of a message against a given one
  static bool validCRC(ModbusMessage msg, uint16_t CRC);

// addCRC: extend a RTUMessage by a valid CRC
  static void addCRC(ModbusMessage& raw);

// calculateInterval: determine the minimal gap time between messages
  static uint32_t calculateInterval(uint32_t baudRate);

// RTSauto: dummy callback for auto half duplex RS485 boards
  inline static void RTSauto(bool level) { return; } // NOLINT

#if HAS_FREERTOS
// Necessary preparations for a HardwareSerial
static void prepareHardwareSerial(HardwareSerial& s, uint16_t bufferSize = 260) {
  s.setRxBufferSize(bufferSize);
  s.setTxBufferSize(bufferSize);
}
#endif

protected:
// Printable characters for ASCII protocol: 012345678ABCDEF
  static const char ASCIIwrite[];
  static const char ASCIIread[];

  RTUutils() = delete;

// receive: get a Modbus message from serial, maintaining timeouts etc.
  static ModbusMessage receive(uint8_t caller, Stream& serial, uint32_t timeout, unsigned long& lastMicros, uint32_t interval, bool ASCIImode, bool skipLeadingZeroBytes = false);

// send: send a Modbus message in either format (ModbusMessage or data/len)
  static void send(Stream& serial, unsigned long& lastMicros, uint32_t interval, RTScallback r, const uint8_t *data, uint16_t len, bool ASCIImode);
  static void send(Stream& serial, unsigned long& lastMicros, uint32_t interval, RTScallback r, ModbusMessage raw, bool ASCIImode);
};

#endif
