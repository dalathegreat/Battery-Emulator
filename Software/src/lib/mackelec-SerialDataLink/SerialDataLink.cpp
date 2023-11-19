// SerialDataLink.cpp

#include "SerialDataLink.h"


const uint16_t crcTable[256] = {
    0, 32773, 32783, 10, 32795, 30, 20, 32785,
    32819, 54, 60, 32825, 40, 32813, 32807, 34,
    32867, 102, 108, 32873, 120, 32893, 32887, 114,
    80, 32853, 32863, 90, 32843, 78, 68, 32833,
    32963, 198, 204, 32969, 216, 32989, 32983, 210,
    240, 33013, 33023, 250, 33003, 238, 228, 32993,
    160, 32933, 32943, 170, 32955, 190, 180, 32945,
    32915, 150, 156, 32921, 136, 32909, 32903, 130,
    33155, 390, 396, 33161, 408, 33181, 33175, 402,
    432, 33205, 33215, 442, 33195, 430, 420, 33185,
    480, 33253, 33263, 490, 33275, 510, 500, 33265,
    33235, 470, 476, 33241, 456, 33229, 33223, 450,
    320, 33093, 33103, 330, 33115, 350, 340, 33105,
    33139, 374, 380, 33145, 360, 33133, 33127, 354,
    33059, 294, 300, 33065, 312, 33085, 33079, 306,
    272, 33045, 33055, 282, 33035, 270, 260, 33025,
    33539, 774, 780, 33545, 792, 33565, 33559, 786,
    816, 33589, 33599, 826, 33579, 814, 804, 33569,
    864, 33637, 33647, 874, 33659, 894, 884, 33649,
    33619, 854, 860, 33625, 840, 33613, 33607, 834,
    960, 33733, 33743, 970, 33755, 990, 980, 33745,
    33779, 1014, 1020, 33785, 1000, 33773, 33767, 994,
    33699, 934, 940, 33705, 952, 33725, 33719, 946,
    912, 33685, 33695, 922, 33675, 910, 900, 33665,
    640, 33413, 33423, 650, 33435, 670, 660, 33425,
    33459, 694, 700, 33465, 680, 33453, 33447, 674,
    33507, 742, 748, 33513, 760, 33533, 33527, 754,
    720, 33493, 33503, 730, 33483, 718, 708, 33473,
    33347, 582, 588, 33353, 600, 33373, 33367, 594,
    624, 33397, 33407, 634, 33387, 622, 612, 33377,
    544, 33317, 33327, 554, 33339, 574, 564, 33329,
    33299, 534, 540, 33305, 520, 33293, 33287, 514
};

union Convert
{
  uint16_t u16;
  int16_t i16;
  struct
  {
    byte low;
    byte high;
  };
}convert;





// Constructor
SerialDataLink::SerialDataLink(Stream &serial, uint8_t transmitID, uint8_t receiveID, uint8_t maxIndexTX, uint8_t maxIndexRX, bool enableRetransmit)
    : serial(serial), transmitID(transmitID), receiveID(receiveID), maxIndexTX(maxIndexTX), maxIndexRX(maxIndexRX), retransmitEnabled(enableRetransmit) {
    // Initialize buffers and state variables
    txBufferIndex = 0;
    isTransmitting = false;
    isReceiving = false;
    transmissionError = false;
    readError = false;
    newData = false;

    // Initialize data arrays and update flags

    memset(dataArrayTX, 0, sizeof(dataArrayTX));
    memset(dataArrayRX, 0, sizeof(dataArrayRX));
    memset(dataUpdated, 0, sizeof(dataUpdated));
    memset(lastSent   , 0, sizeof(lastSent   ));

    // Additional initialization as required
}

void SerialDataLink::updateData(uint8_t index, int16_t value)
{
  if (index < maxIndexTX)
  {
    if (dataArrayTX[index] != value)
    {
      dataArrayTX[index] = value;
      dataUpdated[index] = true;
      lastSent[index] = millis();
    }
  }
}

int16_t SerialDataLink::getReceivedData(uint8_t index) 
{
  if (index < dataArraySizeRX) {
    return dataArrayRX[index];
  } else {
    // Handle the case where the index is out of bounds
    return -1; 
  }
}

bool SerialDataLink::checkTransmissionError(bool resetFlag) 
{
  bool currentStatus = transmissionError;
  if (resetFlag && transmissionError) {
    transmissionError = false;
  }
  return currentStatus;
}

bool SerialDataLink::checkReadError(bool reset) 
{
  bool error = readError;
  if (reset) {
    readError = false;
  }
  return error;
}


bool SerialDataLink::checkNewData(bool resetFlag) {
  bool currentStatus = newData;
  if (resetFlag && newData) {
    newData = false;
  }
  return currentStatus;
}

void SerialDataLink::run() 
{
    switch (currentState) 
    {
      case DataLinkState::Idle:
          // Decide if the device should start transmitting
          currentState = DataLinkState::Receiving;
          if (shouldTransmit())
          {
              currentState = DataLinkState::Transmitting;
          }
          break;

        case DataLinkState::Transmitting:
          if (isTransmitting) 
          {
              sendNextByte(); // Continue sending the current data
          } 
          else 
          {
              
              constructPacket(); // Construct a new packet if not currently transmitting
              
              uint8_t ack; 
              // now it is known which acknoledge need sending since last Reception
              if (needToACK)
              {
                needToACK = false;
                ack = (txBufferIndex > 5) ? ACK_RTT_CODE : ACK_CODE;
                serial.write(ack);
              }
              if (needToNACK)
              {
                needToNACK = false;
                ack = (txBufferIndex > 5) ? NACK_RTT_CODE : NACK_CODE;
                serial.write(ack);
              }
          }

          if (maxIndexTX < 1) 
          {
            currentState = DataLinkState::Receiving;
          }
          // Check if the transmission is complete
          if (transmissionComplete) 
          {
            transmissionComplete = false;
            isTransmitting = false;
            currentState = DataLinkState::WaitingForAck; // Move to WaitingForAck state
          }
          break;


        case DataLinkState::WaitingForAck:
          if (ackTimeout())
          {
            // Handle ACK timeout scenario
            transmissionError = true;
            isTransmitting = false;
            //handleAckTimeout();
            //--- if no ACK's etc received may as well move to Transmitting
            currentState = DataLinkState::Transmitting;
          }
          if (ackReceived()) 
          {
            // No data to send from the other device
            currentState = DataLinkState::Transmitting;
          }
          if (requestToSend)
          {
            // The other device has data to send (indicated by ACK+RTT)
            currentState = DataLinkState::Receiving;
          }
          break;

          
        case DataLinkState::Receiving:
          read();
          if (readComplete) 
          {
            readComplete = false;
            // transition to transmit mode
            currentState = DataLinkState::Transmitting;
          }
          break;

        default:
          currentState = DataLinkState::Idle;
    }
}

bool SerialDataLink::shouldTransmit() 
{
    // Priority condition: Device with transmitID = 1 and receiveID = 0 has the highest priority
    if (transmitID == 1 && receiveID == 0)
    {
        return true;
    }
    return false;
}

void SerialDataLink::constructPacket()
{
  if (maxIndexTX <1) return;
  if (!isTransmitting)
  {
    lastTransmissionTime = millis();
    txBufferIndex = 0; // Reset the TX buffer index

    addToTxBuffer(headerChar);
    addToTxBuffer(transmitID);
    addToTxBuffer(0); // EOT position - place holder
    unsigned long currentTime = millis();
    int count = txBufferIndex;

    for (uint8_t i = 0; i < maxIndexTX; i++)
    {
      if (dataUpdated[i] || (currentTime - lastSent[i] >= updateInterval))
      {
        addToTxBuffer(i);
        convert.i16 = dataArrayTX[i];
        addToTxBuffer(convert.high);
        addToTxBuffer(convert.low);

        dataUpdated[i] = false;
        lastSent[i] = currentTime; // Update the last sent time for this index
      }
    }

    if (count == txBufferIndex)
    {
      // No data was added to the buffer, so no need to send a packet
      return;
    }

    addToTxBuffer(eotChar);
    //-----  assign EOT position 
    txBuffer[2] = txBufferIndex - 1;
    uint16_t crc = calculateCRC16(txBuffer, txBufferIndex);
    convert.u16 = crc;
    addToTxBuffer(convert.high);
    addToTxBuffer(convert.low);
    isTransmitting = true;
  }
}


void SerialDataLink::addToTxBuffer(uint8_t byte)
{
    if (txBufferIndex < txBufferSize)
    {
        txBuffer[txBufferIndex] = byte;
        txBufferIndex++;
    }
}

bool SerialDataLink::sendNextByte()
{
  if (!isTransmitting) return false;

  if (txBufferIndex >= txBufferSize)
  {
    txBufferIndex = 0; // Reset the TX buffer index
    isTransmitting = false;
    return false; // Buffer was fully sent, end transmission
  }  
  serial.write(txBuffer[sendBufferIndex]);
  sendBufferIndex++;

  if (sendBufferIndex >= txBufferIndex)
  {
    isTransmitting = false;
    txBufferIndex = 0; // Reset the TX buffer index for the next packet
    sendBufferIndex = 0;
    transmissionComplete = true;
    return true; // Packet was fully sent
  }
  return false; // More bytes remain to be sent
}

bool SerialDataLink::ackReceived()
{
    // Check if there is data available to read
    if (serial.available() > 0) 
    {
        // Peek at the next byte without removing it from the buffer
        uint8_t nextByte = serial.peek();

        if (nextByte == headerChar) 
        {
            requestToSend = true;
            return false;
        }

        uint8_t receivedByte = serial.read();

        switch (receivedByte) 
        {
          case ACK_CODE:
              // Handle standard ACK
              return true;
      
          case ACK_RTT_CODE:
              // Handle ACK with request to transmit
              requestToSend = true;
              return true;

          case NACK_RTT_CODE:
              requestToSend = true;
          case NACK_CODE:
              transmissionError = true;
              return false;
      
          default:
              break;
      }

    }

    return false; // No ACK, NACK, or new packet received
}

bool SerialDataLink::ackTimeout() 
{
    // Check if the current time has exceeded the last transmission time by the ACK timeout period
    if (millis() - lastTransmissionTime > ACK_TIMEOUT) {
        return true;  // Timeout occurred
    }
    return false;  // No timeout
}



void SerialDataLink::read()
{
  if (maxIndexRX < 1) return;
  if (serial.available()) 
  {
    //Serial.print(".");
    if (millis() - lastHeaderTime > PACKET_TIMEOUT  && rxBufferIndex > 0) 
    {
      // Timeout occurred, reset buffer and pointer
      rxBufferIndex = 0;
      eotPosition = 0;
      readError = true;
    }
    uint8_t incomingByte = serial.read();
    switch (rxBufferIndex) {
      case 0: // Looking for the header
        if (incomingByte == headerChar) 
        {
          lastHeaderTime = millis();
          rxBuffer[rxBufferIndex] = incomingByte;
          rxBufferIndex++;
        }
        break;

      case 1: // Looking for the address
        if (incomingByte == receiveID) {
          rxBuffer[rxBufferIndex] = incomingByte;
          rxBufferIndex++;
        } else {
          // Address mismatch, reset to look for a new packet
          rxBufferIndex = 0;
        }
        break;
        
      case 2: // EOT position
        eotPosition = incomingByte;
        rxBuffer[rxBufferIndex] = incomingByte;
        rxBufferIndex++;
        break;
        
      default:
        // Normal data handling
        rxBuffer[rxBufferIndex] = incomingByte;
        rxBufferIndex++;
        
        if (isCompletePacket()) 
        {
          processPacket();
          rxBufferIndex = 0; // Reset for the next packet
          readComplete = true; // Indicate that read operation is complete
        }

        // Check for buffer overflow
        if (rxBufferIndex >= rxBufferSize) 
        {
          rxBufferIndex = 0;
        }
        break;
    }
  }
}

bool SerialDataLink::isCompletePacket() {
  if (rxBufferIndex - 3 < eotPosition) return false;
  // Ensure there are enough bytes for EOT + 2-byte CRC
  
  // Check if the third-last byte is the EOT character
  if (rxBuffer[eotPosition] == eotChar) 
  {
    return true;
  }
  return false;
}

bool SerialDataLink::checkCRC() 
{
  uint16_t receivedCrc;
  if (rxBufferIndex < 3) 
  {
    // Not enough data for CRC check
    return false;
  }

  
  convert.high = rxBuffer[rxBufferIndex - 2];
  convert.low = rxBuffer[rxBufferIndex - 1];
  receivedCrc = convert.u16;
  
  // Calculate CRC for the received data (excluding the CRC bytes themselves)
  uint16_t calculatedCrc = calculateCRC16(rxBuffer, rxBufferIndex - 2);
  return receivedCrc == calculatedCrc;
}


void SerialDataLink::processPacket() 
{

  if (!checkCRC()) {
    // CRC check failed, handle the error
    readError = true;
    return;
  }

  // Start from index 3 to skip the SOT and ADDRESS and EOT Position characters
  uint8_t i = 3; 
  while (i < eotPosition) 
  {
    uint8_t arrayID = rxBuffer[i++];
    
    // Make sure there's enough data for a complete int16 (2 bytes)
    if (i + 1 >= rxBufferIndex) {
      readError = true;
      needToNACK = true;
      return; // Incomplete packet or buffer overflow
    }

    // Combine the next two bytes into an int16 value
    int16_t value = (int16_t(rxBuffer[i]) << 8) | int16_t(rxBuffer[i + 1]);
    i += 2;

    // Handle the array ID and value here
    if (arrayID < dataArraySizeRX) {
      dataArrayRX[arrayID] = value;
    } 
    else 
    {
      // Handle invalid array ID
      readError = true;
      needToNACK = true;
      return;
    }
    newData = true;
    needToACK = true;
  }
}



void SerialDataLink::setUpdateInterval(unsigned long interval) {
    updateInterval = interval;
}

void SerialDataLink::setAckTimeout(unsigned long timeout) {
    ACK_TIMEOUT = timeout; 
}

void SerialDataLink::setPacketTimeout(unsigned long timeout) {
    PACKET_TIMEOUT = timeout; 
}

void SerialDataLink::setHeaderChar(char header)
{
  headerChar = header;
}

void SerialDataLink::setEOTChar(char eot)
{
  eotChar = eot;
}



uint16_t SerialDataLink::calculateCRC16(const uint8_t* data, size_t length)
{
  uint16_t crc = 0xFFFF; // Start value for CRC
  for (size_t i = 0; i < length; i++)
  {
    uint8_t index = (crc >> 8) ^ data[i];
    crc = (crc << 8) ^ crcTable[index];
  }
  return crc;
}
