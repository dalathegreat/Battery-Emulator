// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_SERVER_H
#define _MODBUS_SERVER_H

#include "options.h"

#include <map>
#include <vector>
#include <functional>
#if USE_MUTEX
#include <mutex>      // NOLINT
#endif
#include "ModbusTypeDefs.h"
#include "ModbusError.h"
#include "ModbusMessage.h"

#if USE_MUTEX
using std::mutex;
using std::lock_guard;
#endif

// Standard response variants for "no response" and "echo the request"
const ModbusMessage NIL_RESPONSE (std::vector<uint8_t>{0xFF, 0xF0});
const ModbusMessage ECHO_RESPONSE(std::vector<uint8_t>{0xFF, 0xF1});

// MBSworker: function signature for worker functions to handle single serverID/functionCode combinations
using MBSworker = std::function<ModbusMessage(ModbusMessage msg)>;

class ModbusServer {
public:
  // registerWorker: register a worker function for a certain serverID/FC combination
  // If there is one already, it will be overwritten!
  void registerWorker(uint8_t serverID, uint8_t functionCode, MBSworker worker);
  
  // getWorker: if a worker function is registered, return its address, nullptr otherwise
  MBSworker getWorker(uint8_t serverID, uint8_t functionCode);

  // unregisterWorker; remove again all or part of the registered workers for a given server ID
  // Returns true if the worker was found and removed
  bool unregisterWorker(uint8_t serverID, uint8_t functionCode = 0);

  // isServerFor: if a worker function is registered for the given serverID, return true
  bool isServerFor(uint8_t serverID, uint8_t functionCode);

  // isServerFor: short version to look up if the server is known at all
  bool isServerFor(uint8_t serverID);

  // getMessageCount: read number of messages processed
  uint32_t getMessageCount();

  // getErrorCount: read number of errors responded
  uint32_t getErrorCount();

  // resetCounts: set both message and error counts to zero
  void resetCounts();

  // Local request to the server
  ModbusMessage localRequest(ModbusMessage msg);

  // listServer: print out all server/FC combinations served
  void listServer();

protected:
  // Constructor
  ModbusServer();

  // Destructor
  virtual ~ModbusServer();

  // Prevent copy construction or assignment
  ModbusServer(ModbusServer& other) = delete;
  ModbusServer& operator=(ModbusServer& other) = delete;

  std::map<uint8_t, std::map<uint8_t, MBSworker>> workerMap;      // map on serverID->functionCode->worker function
  uint32_t messageCount;         // Number of Requests processed
  uint32_t errorCount;           // Number of errors responded
  #if USE_MUTEX
  mutex m;                       // mutex to cover changes to messageCount and errorCount
  #endif
};


#endif
