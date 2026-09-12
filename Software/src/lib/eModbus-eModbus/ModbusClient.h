// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_CLIENT_H
#define _MODBUS_CLIENT_H

#include <functional> 
#include <map>
#include "options.h"
#include "ModbusMessage.h"

#if HAS_FREERTOS
extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
}
#elif IS_LINUX
#include <pthread.h>
#endif

#if USE_MUTEX
#include <mutex>                    // NOLINT
using std::mutex;
using std::lock_guard;
#endif

typedef std::function<void(ModbusMessage msg, uint32_t token)> MBOnData;
typedef std::function<void(Modbus::Error errorCode, uint32_t token)> MBOnError;
typedef std::function<void(ModbusMessage msg, uint32_t token)> MBOnResponse;

class ModbusClient {
public:
  bool onDataHandler(MBOnData handler);   // Accept onData handler 
  bool onErrorHandler(MBOnError handler); // Accept onError handler 
  bool onResponseHandler(MBOnResponse handler); // Accept onResponse handler 
  uint32_t getMessageCount();             // Informative: return number of messages created
  uint32_t getErrorCount();              // Informative: return number of errors received
  void resetCounts();                    // Set both message and error counts to zero
  inline Error addRequest(const ModbusMessage& m, uint32_t token) { return addRequestM(m, token); }
  inline ModbusMessage syncRequest(const ModbusMessage& m, uint32_t token) { return syncRequestM(m, token); }

  // Template function to generate syncRequest functions as long as there is a 
  // matching ModbusMessage::setMessage() call
  template <typename... Args>
  ModbusMessage syncRequest(uint32_t token, Args&&... args) {
    Error rc = SUCCESS;
    // Create request, if valid
    ModbusMessage m;
    rc = m.setMessage(std::forward<Args>(args) ...);

    // Add it to the queue and wait for a response, if valid
    if (rc == SUCCESS) {
      return syncRequestM(m, token);
    } 
    // Else return the error as a message
    return buildErrorMsg(rc, std::forward<Args>(args) ...);
  }

  // Template function to create an error response message from a variadic pattern
  template <typename... Args>
  ModbusMessage buildErrorMsg(Error e, uint8_t serverID, uint8_t functionCode, Args&&... args) {
    ModbusMessage m;
    m.setError(serverID, functionCode, e);
    return m;
  }

  // Template function to generate addRequest functions as long as there is a 
  // matching ModbusMessage::setMessage() call
  template <typename... Args>
  Error addRequest(uint32_t token, Args&&... args) {
    Error rc = SUCCESS;        // Return value

    // Create request, if valid
    ModbusMessage m;
    rc = m.setMessage(std::forward<Args>(args) ...);

    // Add it to the queue, if valid
    if (rc == SUCCESS) {
      return addRequestM(m, token);
    }
    // Else return the error
    return rc;
  }

protected:
  ModbusClient();             // Default constructor
  virtual ~ModbusClient();            // Destructor
  ModbusMessage waitSync(uint8_t serverID, uint8_t functionCode, uint32_t token); // wait for syncRequest response to arrive
  // Virtual addRequest variant needed internally. All others done by template!
  virtual Error addRequestM(ModbusMessage msg, uint32_t token) = 0;
  // Virtual syncRequest variant following the same pattern
  virtual ModbusMessage syncRequestM(ModbusMessage msg, uint32_t token) = 0;
  // Prevent copy construction or assignment
  ModbusClient(ModbusClient& other) = delete;
  ModbusClient& operator=(ModbusClient& other) = delete;

  uint32_t messageCount;           // Number of requests generated. Used for transactionID in TCPhead
  uint32_t errorCount;             // Number of errors received
#if HAS_FREERTOS
  TaskHandle_t worker;             // Interface instance worker task
#elif IS_LINUX
  pthread_t worker;
#endif
  MBOnData onData;                 // Data response handler
  MBOnError onError;               // Error response handler
  MBOnResponse onResponse;         // Uniform response handler
  static uint16_t instanceCounter; // Number of ModbusClients created
  std::map<uint32_t, ModbusMessage> syncResponse; // Map to hold response messages on synchronous requests
#if USE_MUTEX
  std::mutex syncRespM;            // Mutex protecting syncResponse map against race conditions
  std::mutex countAccessM;         // Mutex protecting access to the message and error counts
#endif

  // Let any ModbusBridge class use protected members
  template<typename SERVERCLASS> friend class ModbusBridge;
};

#endif
