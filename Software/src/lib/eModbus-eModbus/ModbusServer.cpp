// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include <Arduino.h>
#include "ModbusServer.h"

#undef LOCAL_LOG_LEVEL
// #define LOCAL_LOG_LEVEL LOG_LEVEL_VERBOSE
#include "Logging.h"

// registerWorker: register a worker function for a certain serverID/FC combination
// If there is one already, it will be overwritten!
void ModbusServer::registerWorker(uint8_t serverID, uint8_t functionCode, MBSworker worker) {
  workerMap[serverID][functionCode] = worker;
  LOG_D("Registered worker for %02X/%02X\n", serverID, functionCode);
}

// getWorker: if a worker function is registered, return its address, nullptr otherwise
MBSworker ModbusServer::getWorker(uint8_t serverID, uint8_t functionCode) {
  bool serverFound = false;
  LOG_D("Need worker for %02X-%02X : ", serverID, functionCode);
  // Search the FC map associated with the serverID
  auto svmap = workerMap.find(serverID);
  // Is there one?
  if (svmap != workerMap.end()) {
    serverFound = true;
  // No explicit serverID entry found, but we may have one for ANY_SERVER
  } else {
    svmap = workerMap.find(ANY_SERVER);
    if (svmap != workerMap.end()) {
      serverFound = true;
    }
  }
  // Did we find a serverID?
  if (serverFound) {
    // Yes. Now look for the function code in the inner map
    bool functionCodeFound = false;
    auto fcmap = svmap->second.find(functionCode);;
    // Found it?
    if (fcmap != svmap->second.end()) {
      // Yes. Return the function pointer for it.
      functionCodeFound = true;
      // No, no explicit worker found, but may be there is one for ANY_FUNCTION_CODE?
    } else {
      fcmap = svmap->second.find(ANY_FUNCTION_CODE);;
      // Found it?
      if (fcmap != svmap->second.end()) {
        // Yes. Return the function pointer for it.
        functionCodeFound = true;
      }
    }
    if (functionCodeFound) {
      // Yes. Return the function pointer for it.
      LOGRAW_D("Worker found for %02X/%02X\n", serverID, functionCode);
      return fcmap->second;
    }
  }
  // No matching function pointer found
  LOGRAW_D("No matching worker found\n");
  return nullptr;
}

// unregisterWorker; remove again all or part of the registered workers for a given server ID
// Returns true if the worker was found and removed
bool ModbusServer::unregisterWorker(uint8_t serverID, uint8_t functionCode) {
  uint16_t numEntries = 0;    // Number of entries removed

  // Is there at least one entry for the serverID?
  auto svmap = workerMap.find(serverID);
  // Is there one?
  if (svmap != workerMap.end()) {
    // Yes. we may proceed with it
    // Are we to look for a single serverID/FC combination?
    if (functionCode) {
      // Yes. 
      numEntries = svmap->second.erase(functionCode);
    } else {
      // No, the serverID shall be removed with all references
      numEntries = workerMap.erase(serverID);
    }
  } 
  LOG_D("Removed %d worker entries for %d/%d\n", numEntries, serverID, functionCode);
  return (numEntries ? true : false);
}

// isServerFor: if a worker function is registered for the given serverID, return true
//              functionCode defaults to ANY_FUNCTION_CODE and will yield true for any function code,
//              including ANY_FUNCTION_CODE :D
bool ModbusServer::isServerFor(uint8_t serverID, uint8_t functionCode) {
  // Check if there is a non-nullptr function for the given combination
  if (getWorker(serverID, functionCode)) {
    return true;
  }
  return false;
}

// isServerFor: short version to look up if the server is known at all
bool ModbusServer::isServerFor(uint8_t serverID) {
  // Check if there is a non-nullptr function for the given combination
  auto svmap = workerMap.find(serverID);
  // Is there one?
  if (svmap != workerMap.end()) {
    return true;
  } else {
    svmap = workerMap.find(ANY_SERVER);
    if (svmap != workerMap.end()) {
      return true;
    }
  }
  return false;
}


// getMessageCount: read number of messages processed
uint32_t ModbusServer::getMessageCount() { 
  return messageCount;
}

// getErrorCount: read number of errors responded
uint32_t ModbusServer::getErrorCount() { 
  return errorCount;
}

// resetCounts: set both message and error counts to zero
void ModbusServer::resetCounts() {
  {
    LOCK_GUARD(cntLock, m);
    messageCount = 0;
    errorCount = 0;
  }
}

// LocalRequest: get response from locally running server.
ModbusMessage ModbusServer::localRequest(ModbusMessage msg) {
  ModbusMessage m;
  uint8_t serverID = msg.getServerID();
  uint8_t functionCode = msg.getFunctionCode();
  LOG_D("Local request for %02X/%02X\n", serverID, functionCode);
  HEXDUMP_V("Request", msg.data(), msg.size());
  messageCount++;
  // Try to get a worker for the request
  MBSworker worker = getWorker(serverID, functionCode);
  // Did we get one?
  if (worker != nullptr) {
    // Yes. call it and return the response
    LOG_D("Call worker\n");
    m = worker(msg);
    LOG_D("Worker responded\n");
    HEXDUMP_V("Worker response", m.data(), m.size());
    // Process Response. Is it one of the predefined types?
    if (m[0] == 0xFF && (m[1] == 0xF0 || m[1] == 0xF1)) {
      // Yes. Check it
      switch (m[1]) {
      case 0xF0: // NIL
        m.clear();
        break;
      case 0xF1: // ECHO
        m.clear();
        m.append(msg);
        break;
      default:   // Will not get here, but lint likes it!
        break;
      }
    }
    HEXDUMP_V("Response", m.data(), m.size());
    if (m.getError() != SUCCESS) {
      errorCount++;
    }
    return m;
  } else {
    LOG_D("No worker found. Error response.\n");
    // No. Is there at least one worker for the serverID?
    if (isServerFor(serverID)) {
      // Yes. Respond with "illegal function code"
      m.setError(serverID, functionCode, ILLEGAL_FUNCTION);
    } else {
      // No. Respond with "Invalid server ID"
      m.setError(serverID, functionCode, INVALID_SERVER);
    }
    errorCount++;
    return m;
  }
  // We should never get here...
  LOG_C("Internal problem: should not get here!\n");
  m.setError(serverID, functionCode, UNDEFINED_ERROR);
  errorCount++;
  return m;
}

// Constructor
ModbusServer::ModbusServer() :
  messageCount(0),
  errorCount(0) { }

// Destructor
ModbusServer::~ModbusServer() {
}

// listServer: Print out all mapped server/FC combinations
void ModbusServer::listServer() {
  for (auto it = workerMap.begin(); it != workerMap.end(); ++it) {
    LOG_N("Server %3d: ", it->first);
    for (auto it2 = it->second.begin(); it2 != it->second.end(); it2++) {
      LOGRAW_N(" %02X", it2->first);
    }
    LOGRAW_N("\n");
  }
}
