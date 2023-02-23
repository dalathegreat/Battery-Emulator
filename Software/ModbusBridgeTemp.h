// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#ifndef _MODBUS_BRIDGE_TEMP_H
#define _MODBUS_BRIDGE_TEMP_H

#include <map>
#include <functional>
#include "ModbusClient.h"
#include "ModbusClientTCP.h"  // Needed for client.setTarget()
#include "RTUutils.h"  // Needed for RTScallback

#undef LOCAL_LOG_LEVEL
#define LOCAL_LOG_LEVEL LOG_LEVEL_VERBOSE
#include "Logging.h"

using std::bind;
using std::placeholders::_1;

// Known server types: TCP (client, host/port) and RTU (client)
enum ServerType : uint8_t { TCP_SERVER, RTU_SERVER };

// Bridge class template, takes one of ModbusServerRTU, ModbusServerWiFi, ModbusServerEthernet or ModbusServerTCPasync as parameter
template<typename SERVERCLASS>
class ModbusBridge : public SERVERCLASS {
public:
  // Constructor for TCP server variants.
  ModbusBridge();

  // Constructors for the RTU variant. Parameters as are for ModbusServerRTU
  ModbusBridge(HardwareSerial& serial, uint32_t timeout, int rtsPin = -1);
  ModbusBridge(HardwareSerial& serial, uint32_t timeout, RTScallback rts);

  // Destructor
  ~ModbusBridge();

  // Method to link external servers to the bridge
  bool attachServer(uint8_t aliasID, uint8_t serverID, uint8_t functionCode, ModbusClient *client, IPAddress host = IPAddress(0, 0, 0, 0), uint16_t port = 0);

  // Link another function code to the server
  bool addFunctionCode(uint8_t aliasID, uint8_t functionCode);

  // Block a function code (respond with ILLEGAL_FUNCTION error)
  bool denyFunctionCode(uint8_t aliasID, uint8_t functionCode);

protected:
  // ServerData holds all data necessary to address a single server
  struct ServerData {
    uint8_t serverID;             // External server id
    ModbusClient *client;         // client to be used to request the server
    ServerType serverType;        // TCP_SERVER or RTU_SERVER
    IPAddress host;               // TCP: host IP address, else 0.0.0.0
    uint16_t port;                // TCP: host port number, else 0

    // RTU constructor
    ServerData(uint8_t sid, ModbusClient *c) :
      serverID(sid),
      client(c),
      serverType(RTU_SERVER),
      host(IPAddress(0, 0, 0, 0)),
      port(0) {}
    
    // TCP constructor
    ServerData(uint8_t sid, ModbusClient *c, IPAddress h, uint16_t p) :
      serverID(sid),
      client(c),
      serverType(TCP_SERVER),
      host(h),
      port(p) {}
  };

  // Default worker functions
  ModbusMessage bridgeWorker(ModbusMessage msg);
  ModbusMessage bridgeDenyWorker(ModbusMessage msg);

  // Map of servers attached
  std::map<uint8_t, ServerData *> servers;
};

// Constructor for TCP variants
template<typename SERVERCLASS>
ModbusBridge<SERVERCLASS>::ModbusBridge() :
  SERVERCLASS() { } 

// Constructors for RTU variant
template<typename SERVERCLASS>
ModbusBridge<SERVERCLASS>::ModbusBridge(HardwareSerial& serial, uint32_t timeout, int rtsPin) :
  SERVERCLASS(serial, timeout, rtsPin) { }

// Alternate constructors for RTU variant
template<typename SERVERCLASS>
ModbusBridge<SERVERCLASS>::ModbusBridge(HardwareSerial& serial, uint32_t timeout, RTScallback rts) :
  SERVERCLASS(serial, timeout, rts) { }

// Destructor
template<typename SERVERCLASS>
ModbusBridge<SERVERCLASS>::~ModbusBridge() { 
  // Release ServerData storage in servers array
  for (auto itr = servers.begin(); itr != servers.end(); itr++) {
    delete (itr->second);
  }
  servers.clear();
}

// attachServer: memorize the access data for an external server with ID serverID under bridge ID aliasID
template<typename SERVERCLASS>
bool ModbusBridge<SERVERCLASS>::attachServer(uint8_t aliasID, uint8_t serverID, uint8_t functionCode, ModbusClient *client, IPAddress host, uint16_t port) {

  // Is there already an entry for the aliasID?
  if (servers.find(aliasID) == servers.end()) {
    // No. Store server data in map.

    // Do we have a port number?
    if (port != 0) {
      // Yes. Must be a TCP client
      servers[aliasID] = new ServerData(serverID, static_cast<ModbusClient *>(client), host, port);
      LOG_D("(TCP): %02X->%02X %d.%d.%d.%d:%d\n", aliasID, serverID, host[0], host[1], host[2], host[3], port);
    } else {
      // No - RTU client required
      servers[aliasID] = new ServerData(serverID, static_cast<ModbusClient *>(client));
      LOG_D("(RTU): %02X->%02X\n", aliasID, serverID);
    }
  }

  // Register the server/FC combination for the bridgeWorker
  addFunctionCode(aliasID, functionCode);
  return true;
}

template<typename SERVERCLASS>
bool ModbusBridge<SERVERCLASS>::addFunctionCode(uint8_t aliasID, uint8_t functionCode) {
  // Is there already an entry for the aliasID?
  if (servers.find(aliasID) != servers.end()) {
    // Yes. Link server to own worker function
    this->registerWorker(aliasID, functionCode, std::bind(&ModbusBridge<SERVERCLASS>::bridgeWorker, this, std::placeholders::_1));
    LOG_D("FC %02X added for server %02X\n", functionCode, aliasID);
  } else {
    LOG_E("Server %d not attached to bridge!\n", aliasID);
    return false;
  }
  return true;
}

template<typename SERVERCLASS>
bool ModbusBridge<SERVERCLASS>::denyFunctionCode(uint8_t aliasID, uint8_t functionCode) {
  // Is there already an entry for the aliasID?
  if (servers.find(aliasID) != servers.end()) {
    // Yes. Link server to own worker function
    this->registerWorker(aliasID, functionCode, std::bind(&ModbusBridge<SERVERCLASS>::bridgeDenyWorker, this, std::placeholders::_1));
    LOG_D("FC %02X blocked for server %02X\n", functionCode, aliasID);
  } else {
    LOG_E("Server %d not attached to bridge!\n", aliasID);
    return false;
  }
  return true;
}

// bridgeWorker: default worker function to process bridge requests
template<typename SERVERCLASS>
ModbusMessage ModbusBridge<SERVERCLASS>::bridgeWorker(ModbusMessage msg) {
  uint8_t aliasID = msg.getServerID();
  uint8_t functionCode = msg.getFunctionCode();
  ModbusMessage response;

  // Find the (alias) serverID
  if (servers.find(aliasID) != servers.end()) {
    // Found it. We may use servers[aliasID] now without allocating a new map slot

    // Set real target server ID
    msg.setServerID(servers[aliasID]->serverID);

    // Issue the request
    LOG_D("Request (%02X/%02X) sent\n", servers[aliasID]->serverID, functionCode);
    // TCP servers have a target host/port that needs to be set in the client
    if (servers[aliasID]->serverType == TCP_SERVER) {
      response = reinterpret_cast<ModbusClientTCP *>(servers[aliasID]->client)->syncRequestMT(msg, (uint32_t)millis(), servers[aliasID]->host, servers[aliasID]->port);
    } else {
      response = servers[aliasID]->client->syncRequestM(msg, (uint32_t)millis());
    }

    // Re-set the requested server ID
    response.setServerID(aliasID);
  } else {
    // If we get here, something has gone wrong internally. We send back an error response anyway.
    response.setError(aliasID, functionCode, INVALID_SERVER);
  }
  return response;
}

// bridgeDenyWorker: worker function to block function codes
template<typename SERVERCLASS>
ModbusMessage ModbusBridge<SERVERCLASS>::bridgeDenyWorker(ModbusMessage msg) {
  ModbusMessage response;
  response.setError(msg.getServerID(), msg.getFunctionCode(), ILLEGAL_FUNCTION);
  return response;
}

#endif
