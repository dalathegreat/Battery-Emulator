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

private:
  Error err;          // The error code
};

#endif
