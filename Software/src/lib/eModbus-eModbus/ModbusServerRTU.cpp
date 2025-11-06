// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include "ModbusServerRTU.h"

#if HAS_FREERTOS

#undef LOG_LEVEL_LOCAL
#include "Logging.h"

// Init number of created ModbusServerRTU objects
uint8_t ModbusServerRTU::instanceCounter = 0;

// Constructor with RTS pin GPIO (or -1)
ModbusServerRTU::ModbusServerRTU(uint32_t timeout, int rtsPin) :
  ModbusServer(),
  serverTask(nullptr),
  serverTimeout(timeout),
  MSRserial(nullptr),
  MSRinterval(2000),     // will be calculated in begin()!
  MSRlastMicros(0),
  MSRrtsPin(rtsPin), 
  MSRuseASCII(false),
  MSRskipLeadingZeroByte(false),
  listener(nullptr),
  sniffer(nullptr) {
  // Count instances one up
  instanceCounter++;
  // If we have a GPIO RE/DE pin, configure it.
  if (MSRrtsPin >= 0) {
    pinMode(MSRrtsPin, OUTPUT);
    MRTSrts = [this](bool level) {
      digitalWrite(MSRrtsPin, level);
    };
    MRTSrts(LOW);
  } else {
    MRTSrts = RTUutils::RTSauto;
  }
}

// Constructor with RTS callback
ModbusServerRTU::ModbusServerRTU(uint32_t timeout, RTScallback rts) :
  ModbusServer(),
  serverTask(nullptr),
  serverTimeout(timeout),
  MSRserial(nullptr),
  MSRinterval(2000),     // will be calculated in begin()!
  MSRlastMicros(0),
  MRTSrts(rts), 
  MSRuseASCII(false),
  MSRskipLeadingZeroByte(false),
  listener(nullptr),
  sniffer(nullptr) {
  // Count instances one up
  instanceCounter++;
  // Configure RTS callback
  MSRrtsPin = -1;
  MRTSrts(LOW);
}

// Destructor
ModbusServerRTU::~ModbusServerRTU() {
}

// start: create task with RTU server - general version
void ModbusServerRTU::begin(Stream& serial, uint32_t baudRate, int coreID, uint32_t userInterval) {
  MSRserial = &serial;
  doBegin(baudRate, coreID, userInterval);
}

// start: create task with RTU server - HardwareSerial versions
void ModbusServerRTU::begin(HardwareSerial& serial, int coreID, uint32_t userInterval) {
  MSRserial = &serial;
  uint32_t baudRate = serial.baudRate();
  serial.setRxFIFOFull(1);
  doBegin(baudRate, coreID, userInterval);
}

void ModbusServerRTU::doBegin(uint32_t baudRate, int coreID, uint32_t userInterval) {
  // Task already running? Stop it in case.
  end();

  // Set minimum interval time
  MSRinterval = RTUutils::calculateInterval(baudRate);

  // If user defined interval is longer, use that
  if (MSRinterval < userInterval) {
    MSRinterval = userInterval;
  }

  // Create unique task name
  char taskName[18];
  snprintf(taskName, 18, "MBsrv%02XRTU", instanceCounter);

  // Start task to handle the client
  xTaskCreatePinnedToCore((TaskFunction_t)&serve, taskName, SERVER_TASK_STACK, this, 8, &serverTask, coreID >= 0 ? coreID : tskNO_AFFINITY);

  LOG_D("Server task %d started. Interval=%d\n", (uint32_t)serverTask, MSRinterval);
}

// end: kill server task
void ModbusServerRTU::end() {
  if (serverTask != nullptr) {
    vTaskDelete(serverTask);
    LOG_D("Server task %d stopped.\n", (uint32_t)serverTask);
    serverTask = nullptr;
  }
}

// Toggle protocol to ModbusASCII
void ModbusServerRTU::useModbusASCII(unsigned long timeout) {
  MSRuseASCII = true;
  serverTimeout = timeout; // Set timeout to ASCII's value
  LOG_D("Protocol mode: ASCII\n");
}

// Toggle protocol to ModbusRTU
void ModbusServerRTU::useModbusRTU() {
  MSRuseASCII = false;
  LOG_D("Protocol mode: RTU\n");
}

// Inquire protocol mode
bool ModbusServerRTU::isModbusASCII() {
  return MSRuseASCII;
}

// set timeout
void ModbusServerRTU::setModbusTimeout(unsigned long timeout)
{
  serverTimeout = timeout;
}

// Toggle skipping of leading 0x00 byte
void ModbusServerRTU::skipLeading0x00(bool onOff) {
  MSRskipLeadingZeroByte = onOff;
  LOG_D("Skip leading 0x00 mode = %s\n", onOff ? "ON" : "OFF");
}

// Special case: worker to react on broadcast requests
void ModbusServerRTU::registerBroadcastWorker(MSRlistener worker) {
  // If there is one already, it will be overwritten!
  listener = worker;
  LOG_D("Registered worker for broadcast requests\n");
}

// Even more special: register a sniffer worker
void ModbusServerRTU::registerSniffer(MSRlistener worker) {
  // If there is one already, it will be overwritten!
  // This holds true for the broadcast worker as well, 
  // so a sniffer never will do else but to sniff on broadcast requests!
  sniffer = worker;
  LOG_D("Registered sniffer\n");
}

// serve: loop until killed and receive messages from the RTU interface
void ModbusServerRTU::serve(ModbusServerRTU *myServer) {
  ModbusMessage request;                // received request message
  ModbusMessage m;                      // Application's response data
  ModbusMessage response;               // Response proper to be sent

  // init microseconds timer
  myServer->MSRlastMicros = micros();

  while (true) {
    // Initialize all temporary vectors
    request.clear();
    response.clear();
    m.clear();

    // Wait for and read an request
    request = RTUutils::receive(
      'S',
      *(myServer->MSRserial), 
      myServer->serverTimeout, 
      myServer->MSRlastMicros, 
      myServer->MSRinterval, 
      myServer->MSRuseASCII, 
      myServer->MSRskipLeadingZeroByte);

    // Request longer than 1 byte (that will signal an error in receive())? 
    if (request.size() > 1) {
      LOG_D("Request received.\n");

      // Yes. 
      // Do we have a sniffer listening?
      if (myServer->sniffer) {
        // Yes. call it
        myServer->sniffer(request);
      }
      // Is it a broadcast?
      if (request[0] == 0) {
        LOG_D("Broadcast!\n");
        // Yes. Do we have a listener?
        if (myServer->listener) {
          // Yes. call it
          myServer->listener(request);
          LOG_D("Broadcast served.\n");
        }
        // else we simply ignore it
      } else {
        // No Broadcast. 
        // Do we have a callback function registered for it?
        MBSworker callBack = myServer->getWorker(request[0], request[1]);
        if (callBack) {
          LOG_D("Callback found.\n");
          // Yes, we do. Count the message
          {
            LOCK_GUARD(cntLock, myServer->m);
            myServer->messageCount++;
          }
          // Get the user's response
          LOG_D("Callback called.\n");
          m = callBack(request);
          HEXDUMP_V("Callback response", m.data(), m.size());

          // Process Response. Is it one of the predefined types?
          if (m[0] == 0xFF && (m[1] == 0xF0 || m[1] == 0xF1)) {
            // Yes. Check it
            switch (m[1]) {
            case 0xF0: // NIL
              response.clear();
              break;
            case 0xF1: // ECHO
              response = request;
              if (request.getFunctionCode() == WRITE_MULT_REGISTERS ||
                  request.getFunctionCode() == WRITE_MULT_COILS) {
                response.resize(6);
              }
              break;
            default:   // Will not get here, but lint likes it!
              break;
            }
          } else {
            // No predefined. User provided data in free format
            response = m;
          }
        } else {
          // No callback. Is at least the serverID valid?
          if (myServer->isServerFor(request[0])) {
            // Yes. Send back a ILLEGAL_FUNCTION error
            response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_FUNCTION);
          }
          // Else we will ignore the request, as it is not meant for us and we do not deal with broadcasts!
        }
        // Do we have gathered a valid response now?
        if (response.size() >= 3) {
          // Yes. send it back.
          RTUutils::send(*(myServer->MSRserial), myServer->MSRlastMicros, myServer->MSRinterval, myServer->MRTSrts, response, myServer->MSRuseASCII);
          LOG_D("Response sent.\n");
          // Count it, in case we had an error response
          if (response.getError() != SUCCESS) {
            LOCK_GUARD(errorCntLock, myServer->m);
            myServer->errorCount++;
          }
        }
      }
    } else {
      // No, we got a 1-byte request, meaning an error has happened in receive()
      // This is a server, so we will ignore TIMEOUT.
      if (request[0] != TIMEOUT) {
        // Any other error could be important for debugging, so print it
        ModbusError me((Error)request[0]);
        LOG_E("RTU receive: %02X - %s\n", (int)me, (const char *)me);
      }
    }
    // Give scheduler room to breathe
    delay(1);
  }
}

#endif
