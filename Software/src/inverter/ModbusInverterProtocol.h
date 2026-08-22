#ifndef MODBUS_INVERTER_PROTOCOL_H
#define MODBUS_INVERTER_PROTOCOL_H

#include "../lib/eModbus-eModbus/ModbusMessage.h"
#include "../lib/eModbus-eModbus/ModbusServerRTU.h"
#include "InverterProtocol.h"

#include <HardwareSerial.h>

#include <stdint.h>
#include <map>

/* Diagnostic logging of register writes the inverter sends us but the emulator does not act on.
   The Fronius GenericStorage map lets the master write ControlData (400-408) and the droop law
   block (500-506); of these only the watchdog toggle at 401 is currently consumed, so anything
   else the inverter writes lands in mbPV unnoticed. Costs roughly 1kB of flash and 240 bytes of
   RAM, so it is compiled out of the small-flash builds. It is also left out of the host test
   build, where DEBUG_PRINTF is a no-op and the whole thing would be dead code. */
#if !defined(MODBUS_LOG_INVERTER_WRITES) && !defined(SMALL_FLASH_DEVICE) && !defined(UNIT_TEST)
#define MODBUS_LOG_INVERTER_WRITES
#endif

// The abstract base class for all Modbus inverter protocols
class ModbusInverterProtocol : public InverterProtocol {
  const char* interface_name() { return "RS485 / Modbus"; }
  InverterInterfaceType interface_type() { return InverterInterfaceType::Modbus; }

 protected:
  ModbusInverterProtocol(int serverId);
  ~ModbusInverterProtocol();

  // Sets the one-shot "inverter detected" event on the first valid request from the inverter
  void notify_inverter_communication();
  bool inverter_detected = false;

  ModbusMessage FC03(ModbusMessage request);
  ModbusMessage FC06(ModbusMessage request);
  ModbusMessage FC16(ModbusMessage request);
  ModbusMessage FC23(ModbusMessage request);

#ifdef MODBUS_LOG_INVERTER_WRITES
  /* Logs one register write received from the inverter. The first write to an address is always
     logged (at syslog NOTICE, since it means the inverter uses a register we have never seen it
     touch); after that only value changes are logged, with a per-address exponential backoff so a
     register that changes on every poll cannot flood the log. The UTC block is skipped outright,
     since a clock changes every time and none of it is acted on. */
  void log_inverter_write(uint16_t addr, uint16_t value);

  // Addresses tracked at once. The inverter only writes a handful, so this never fills in practice;
  // if it does, the least recently logged entry is evicted.
  static const uint8_t WRITE_LOG_SLOTS = 24;
  // Backoff bounds, in seconds, for repeatedly changing values
  static const uint16_t WRITE_LOG_BACKOFF_MIN_S = 5;
  static const uint16_t WRITE_LOG_BACKOFF_MAX_S = 900;

  struct InverterWriteLogSlot {
    uint16_t addr;
    uint16_t value;        // Last value seen, logged or not
    uint32_t last_log_ms;  // millis() of the last line we actually emitted for this address
    uint16_t suppressed;   // Writes swallowed since that line
    uint16_t backoff_s;
    bool used;
  };
  InverterWriteLogSlot write_log[WRITE_LOG_SLOTS] = {};
#else
  void log_inverter_write(uint16_t, uint16_t) {}
#endif

  // The highest Modbus register we allow reads/writes from
  static const int MBPV_MAX = 30000;
  // The Modbus server ID we respond to
  int _serverId;
  // The Modbus registers themselves
  std::map<uint16_t, uint16_t> mbPV;

  ModbusServerRTU MBserver;
};

#endif
