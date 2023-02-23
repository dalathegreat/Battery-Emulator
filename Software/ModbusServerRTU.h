// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_SERVER_RTU_H
#define _MODBUS_SERVER_RTU_H

#include "options.h"

#if HAS_FREERTOS

#include <Arduino.h>
#include "HardwareSerial.h"
#include "ModbusServer.h"
#include "RTUutils.h"

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
}

// Specal function signature for broadcast or sniffer listeners
using MSRlistener = std::function<void(ModbusMessage msg)>;

class ModbusServerRTU : public ModbusServer {
public:
  // Constructors
  explicit ModbusServerRTU(HardwareSerial& serial, uint32_t timeout=20000, int rtsPin = -1);
  ModbusServerRTU(HardwareSerial& serial, uint32_t timeout, RTScallback rts);

  // Destructor
  ~ModbusServerRTU();

  // start: create task with RTU server to accept requests
  bool start(int coreID = -1, uint32_t interval = 0);

  // stop: kill server task
  bool stop();

  // Toggle protocol to ModbusASCII
  void useModbusASCII(unsigned long timeout = 1000);

  // Toggle protocol to ModbusRTU
  void useModbusRTU();

  // Inquire protocol mode
  bool isModbusASCII();

  // Toggle skipping of leading 0x00 byte
  void skipLeading0x00(bool onOff = true);

  // Special case: worker to react on broadcast requests
  void registerBroadcastWorker(MSRlistener worker);

  // Even more special: register a sniffer worker
  void registerSniffer(MSRlistener worker);

protected:
  // Prevent copy construction and assignment
  ModbusServerRTU(ModbusServerRTU& m) = delete;
  ModbusServerRTU& operator=(ModbusServerRTU& m) = delete;

  inline void isInstance() { }           // Make class instantiable

  static uint8_t instanceCounter;        // Number of RTU servers created (for task names)
  TaskHandle_t serverTask;               // task of the started server
  uint32_t serverTimeout;                // given timeout for receive. Does not really
                                         // matter for a server, but is needed in 
                                         // RTUutils. After timeout without any message
                                         // the server will pause ~1ms and start 
                                         // receive again.
  HardwareSerial& MSRserial;             // The serial interface to use
  uint32_t MSRinterval;                  // Bus quiet time between messages
  unsigned long MSRlastMicros;                // microsecond time stamp of last bus activity
  int8_t MSRrtsPin;                      // GPIO number of the RS485 module's RE/DE line
  RTScallback MRTSrts;                   // Callback to set the RTS line to HIGH/LOW
  bool MSRuseASCII;                      // true=ModbusASCII, false=ModbusRTU
  bool MSRskipLeadingZeroByte;           // true=first byte ignored if 0x00, false=all bytes accepted
  MSRlistener listener;                  // Broadcast listener 
  MSRlistener sniffer;                   // Sniffer listener 

  // serve: loop function for server task
  static void serve(ModbusServerRTU *myself);
};

#endif  // HAS_FREERTOS

#endif // INCLUDE GUARD
