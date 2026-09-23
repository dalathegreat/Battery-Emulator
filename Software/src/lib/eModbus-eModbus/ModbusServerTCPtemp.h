// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_SERVER_TCP_TEMP_H
#define _MODBUS_SERVER_TCP_TEMP_H

#include <Arduino.h>
#include <mutex>  // NOLINT
#include "ModbusServer.h"
#undef LOCAL_LOG_LEVEL
// #define LOCAL_LOG_LEVEL LOG_LEVEL_VERBOSE
#include "Logging.h"

extern "C" {
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
}

using std::vector;
using std::mutex;
using std::lock_guard;

template <typename ST, typename CT>
class ModbusServerTCP : public ModbusServer {
public:
  // Constructor
  ModbusServerTCP();

  // Destructor: closes the connections
  ~ModbusServerTCP();

  // activeClients: return number of clients currently employed
  uint16_t activeClients();

  // start: create task with TCP server to accept requests
  bool start(uint16_t port, uint8_t maxClients, uint32_t timeout, int coreID = -1);

  // stop: drop all connections and kill server task
  bool stop();

protected:
  // Prevent copy construction and assignment
  ModbusServerTCP(ModbusServerTCP& m) = delete;
  ModbusServerTCP& operator=(ModbusServerTCP& m) = delete;

  uint8_t numClients;
  TaskHandle_t serverTask;
  uint16_t serverPort;
  uint32_t serverTimeout;
  bool serverGoDown;
  mutex clientLock;

  struct ClientData {
    ClientData() : task(nullptr), client(0), timeout(0), parent(nullptr) {}
    ClientData(TaskHandle_t t, CT& c, uint32_t to, ModbusServerTCP<ST, CT> *p) : 
      task(t), client(c), timeout(to), parent(p) {}
    ~ClientData() {
      if (client) {
        client.stop();
      }
      if (task != nullptr) {
        vTaskDelete(task);
        LOG_D("Killed client task %d\n", (uint32_t)task);
      }
    }
    TaskHandle_t task;
    CT client;
    uint32_t timeout;
    ModbusServerTCP<ST, CT> *parent;
  };
  ClientData **clients;

  // serve: loop function for server task
  static void serve(ModbusServerTCP<ST, CT> *myself);

  // worker: loop function for client tasks
  static void worker(ClientData *myData);

  // receive: read data from TCP
  ModbusMessage receive(CT& client, uint32_t timeWait);

  // accept: start a task to receive requests and respond to a given client
  bool accept(CT& client, uint32_t timeout, int coreID = -1);

  // clientAvailable: return true,. if a client slot is currently unused
  bool clientAvailable() { return (numClients - activeClients()) > 0; }
};

// Constructor
template <typename ST, typename CT>
ModbusServerTCP<ST, CT>::ModbusServerTCP() :
  ModbusServer(),
  numClients(0),
  serverTask(nullptr),
  serverPort(502),
  serverTimeout(20000),
  serverGoDown(false) {
    clients = new ClientData*[numClients]();
   }

// Destructor: closes the connections
template <typename ST, typename CT>
ModbusServerTCP<ST, CT>::~ModbusServerTCP() {
  for (uint8_t i = 0; i < numClients; ++i) {
    if (clients[i] != nullptr) {
      delete clients[i];
    }
  }
  delete[] clients;
  serverGoDown = true;
}

// activeClients: return number of clients currently employed
template <typename ST, typename CT>
uint16_t ModbusServerTCP<ST, CT>::activeClients() {
  uint8_t cnt = 0;
  for (uint8_t i = 0; i < numClients; ++i) {
    // Current slot could have been previously used - look for cleared task handles
    if (clients[i] != nullptr) {
      // Empty task handle?
      if (clients[i]->task == nullptr) {
        // Yes. Delete entry and init client pointer
        lock_guard<mutex> cL(clientLock);
        delete clients[i];
        LOG_V("Delete client %d\n", i);
        clients[i] = nullptr;
      }
    }
    if (clients[i] != nullptr) cnt++;
  }
  return cnt;
}

  // start: create task with TCP server to accept requests
template <typename ST, typename CT>
  bool ModbusServerTCP<ST, CT>::start(uint16_t port, uint8_t maxClients, uint32_t timeout, int coreID) {
    // Task already running?
    if (serverTask != nullptr) {
      // Yes. stop it first
      stop();
    }
    // Does the required number of slots fit?
    if (numClients != maxClients) {
      // No. Drop array and allocate a new one
      delete[] clients;
      // Now allocate a new one
      numClients = maxClients;
      clients = new ClientData*[numClients]();
    }
    serverPort = port;
    serverTimeout = timeout;
    serverGoDown = false;

    // Create unique task name
    char taskName[18];
    snprintf(taskName, 18, "MBserve%04X", port);

    // Start task to handle the client
    xTaskCreatePinnedToCore((TaskFunction_t)&serve, taskName, SERVER_TASK_STACK, this, 5, &serverTask, coreID >= 0 ? coreID : tskNO_AFFINITY);
    LOG_D("Server task %s started (%d).\n", taskName, (uint32_t)serverTask);

    // Wait two seconds for it to establish
    delay(2000);

    return true;
  }

  // stop: drop all connections and kill server task
template <typename ST, typename CT>
  bool ModbusServerTCP<ST, CT>::stop() {
    // Check for clients still connected
    for (uint8_t i = 0; i < numClients; ++i) {
      // Client is alive?
      if (clients[i] != nullptr) {
        // Yes. Close the connection
        delete clients[i];
        clients[i] = nullptr;
      }
    }
    if (serverTask != nullptr) {
      // Signal server task to stop
      serverGoDown = true;
      delay(5000);
      LOG_D("Killed server task %d\n", (uint32_t)(serverTask));
      serverTask = nullptr;
      serverGoDown = false;
    }
    return true;
  }

// accept: start a task to receive requests and respond to a given client
template <typename ST, typename CT>
bool ModbusServerTCP<ST, CT>::accept(CT& client, uint32_t timeout, int coreID) {
  // Look for an empty client slot
  for (uint8_t i = 0; i < numClients; ++i) {
    // Empty slot?
    if (clients[i] == nullptr) {
      // Yes. allocate new client data in slot
      clients[i] = new ClientData(0, client, timeout, this);

      // Create unique task name
      char taskName[18];
      snprintf(taskName, 18, "MBsrv%02Xclnt", i);

      // Start task to handle the client
      xTaskCreatePinnedToCore((TaskFunction_t)&worker, taskName, SERVER_TASK_STACK, clients[i], 5, &clients[i]->task, coreID >= 0 ? coreID : tskNO_AFFINITY);
      LOG_D("Started client %d task %d\n", i, (uint32_t)(clients[i]->task));

      return true;
    }
  }
  LOG_D("No client slot available.\n");
  return false;
}

template <typename ST, typename CT>
void ModbusServerTCP<ST, CT>::serve(ModbusServerTCP<ST, CT> *myself) {
  // need a local scope here to delete the server at termination time
  if (1) {
    // Set up server with given port
    ST server(myself->serverPort);

    // Start it
    server.begin();

    // Loop until being killed
    while (!myself->serverGoDown) {
      // Do we have clients left to use?
      if (myself->clientAvailable()) {
        // Yes. accept one, when it connects
        CT ec = server.accept();
        // Did we get a connection?
        if (ec) {
          // Yes. Forward it to the Modbus server
          myself->accept(ec, myself->serverTimeout, 0);
          LOG_D("Accepted connection - %d clients running\n", myself->activeClients());
        }
      }
      // Give scheduler room to breathe
      delay(10);
    }
    LOG_E("Server going down\n");
    // We must go down
    SERVER_END;
  }
  vTaskDelete(NULL);
}

template <typename ST, typename CT>
void ModbusServerTCP<ST, CT>::worker(ClientData *myData) {
  // Get own reference data in handier form
  CT myClient = myData->client;
  uint32_t myTimeOut = myData->timeout;
  // TaskHandle_t myTask = myData->task;
  ModbusServerTCP<ST, CT> *myParent = myData->parent;
  unsigned long myLastMessage = millis();

  LOG_D("Worker started, timeout=%d\n", myTimeOut);

  // loop forever, if timeout is 0, or until timeout was hit
  while (myClient.connected() && (!myTimeOut || (millis() - myLastMessage < myTimeOut))) {
    ModbusMessage response;               // Data buffer to hold prepared response
    // Get a request
    if (myClient.available()) {
      response.clear();
      ModbusMessage m = myParent->receive(myClient, 100);

      // has it the minimal length (6 bytes TCP header plus serverID plus FC)?
      if (m.size() >= 8) {
        {
          LOCK_GUARD(cntLock, myParent->m);
          myParent->messageCount++;
        }
        // Extract request data
        ModbusMessage request;
        request.add(m.data() + 6, m.size() - 6);

        // Protocol ID shall be 0x0000 - is it?
        if (m[2] == 0 && m[3] == 0) {
          // ServerID shall be at [6], FC at [7]. Check both
          if (myParent->isServerFor(request.getServerID())) {
            // Server is correct - in principle. Do we serve the FC?
            MBSworker callBack = myParent->getWorker(request.getServerID(), request.getFunctionCode());
            if (callBack) {
              // Yes, we do.
              // Invoke the worker method to get a response
              ModbusMessage data = callBack(request);
              // Process Response
              // One of the predefined types?
              if (data[0] == 0xFF && (data[1] == 0xF0 || data[1] == 0xF1)) {
                // Yes. Check it
                switch (data[1]) {
                case 0xF0: // NIL
                  response.clear();
                  LOG_D("NIL response\n");
                  break;
                case 0xF1: // ECHO
                  response = request;
                  if (request.getFunctionCode() == WRITE_MULT_REGISTERS ||
                      request.getFunctionCode() == WRITE_MULT_COILS) {
                    response.resize(6);
                  }
                  LOG_D("ECHO response\n");
                  break;
                default:   // Will not get here!
                  break;
                }
              } else {
                // No. User provided data response
                response = data;
                LOG_D("Data response\n");
              }
            } else {
              // No, function code is not served here
              response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_FUNCTION);
            }
          } else {
            // No, serverID is not served here
            response.setError(request.getServerID(), request.getFunctionCode(), INVALID_SERVER);
          }
        } else {
          // No, protocol ID was something weird
          response.setError(request.getServerID(), request.getFunctionCode(), TCP_HEAD_MISMATCH);
        }
      }
      delay(1);
      // Do we have a response to send?
      if (response.size() >= 3) {
        // Yes. Do it now.
        // Cut off length and request data, then update TCP header
        m.resize(4);
        m.add(static_cast<uint16_t>(response.size()));
        // Append response
        m.append(response);
        myClient.write(m.data(), m.size());
        HEXDUMP_V("Response", m.data(), m.size());
        // count error responses
        if (response.getError() != SUCCESS) {
          LOCK_GUARD(cntLock, myParent->m);
          myParent->errorCount++;
        }
      }
      // We did something communicationally - rewind timeout timer
      myLastMessage = millis();
    }
    delay(1);
  }

  if (millis() - myLastMessage >= myTimeOut) {
    // Timeout!
    LOG_D("Worker stopping due to timeout.\n");
  } else {
    // Disconnected!
    LOG_D("Worker stopping due to client disconnect.\n");
  }

  // Read away all that may still hang in the buffer
  while (myClient.read() != -1) {}
  // Now stop the client
  myClient.stop();

  {
    lock_guard<mutex> cL(myParent->clientLock);
    myData->task = nullptr;
  }

  delay(50);
  vTaskDelete(NULL);
}

// receive: get request via Client connection
template <typename ST, typename CT>
ModbusMessage ModbusServerTCP<ST, CT>::receive(CT& client, uint32_t timeWait) {
  unsigned long lastMillis = millis();     // Timer to check for timeout
  ModbusMessage m;                    // to take read data
  uint16_t lengthVal = 0;
  uint16_t cnt = 0;
  const uint16_t BUFFERSIZE(300);
  uint8_t buffer[BUFFERSIZE];

  // wait for sufficient packet data or timeout
  while ((millis() - lastMillis < timeWait) && ((cnt < 6) || (cnt < lengthVal)) && (cnt < BUFFERSIZE)) 
  {
    // Is there data waiting?
    if (client.available()) {
        buffer[cnt] = client.read();
        // Are we at the TCP header length field byte #1?
        if (cnt == 4) lengthVal = buffer[cnt] << 8;
        // Are we at the TCP header length field byte #2?
        if (cnt == 5) {
          lengthVal |= buffer[cnt];
          lengthVal += 6;
        }
        cnt++;
        // Rewind EOT and timeout timers
        lastMillis = millis();
    } else {
      delay(1); // Give scheduler room to breathe
    }
  }
  // Did we receive some data?
  if (cnt) {
    // Yes. Is it too much?
    if (cnt >= BUFFERSIZE) {
      // Yes, likely a buffer overflow of some sort
      // Adjust message size in TCP header
      buffer[4] = (cnt >> 8) & 0xFF;
      buffer[5] = cnt & 0xFF;
      LOG_E("Potential buffer overrun (>%d)!\n", cnt);
    }
    // Get as much buffer as was read
    m.add(buffer, cnt);
  }
  return m;
}

#endif
