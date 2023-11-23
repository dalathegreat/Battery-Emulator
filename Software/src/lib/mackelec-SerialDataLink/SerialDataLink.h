/**
 * @file SerialDataLink.h
 * @brief Half-Duplex Serial Data Link for Arduino
 *
 * This file contains the definition of the SerialDataLink class, designed to facilitate 
 * half-duplex communication between Arduino controllers. The class employs a non-blocking, 
 * poll-based approach to transmit and receive data, making it suitable for applications 
 * where continuous monitoring and variable transfer between controllers are required.
 *
 * The half-duplex nature of this implementation allows for data transfer in both directions, 
 * but not simultaneously, ensuring a controlled communication flow and reducing the likelihood 
 * of data collision.
 *
 *
 * @author MackElec
 * @web https://github.com/mackelec/SerialDataLink
 * @license MIT
 */

// ... Class definition ...

/**
 * @class SerialDataLink
 * @brief Class for managing half-duplex serial communication.
 *
 * Provides functions to send and receive data in a half-duplex manner over a serial link.
 * It supports non-blocking operation with a polling approach to check for new data and 
 * transmission errors.
 *
 * Public Methods:
 * - SerialDataLink(): Constructor to initialize the communication parameters.
 * - run(): Main method to be called frequently to handle data transmission and reception.
 * - updateData(): Method to update data to be transmitted.
 * - getReceivedData(): Retrieves data received from the serial link.
 * - checkNewData(): Checks if new data has been received.
 * - checkTransmissionError(): Checks for transmission errors.
 * - checkReadError(): Checks for read errors.
 * - setUpdateInterval(): Sets the interval for data updates.
 * - setAckTimeout(): Sets the timeout for acknowledgments.
 * - setPacketTimeout(): Sets the timeout for packet reception.
 * - setHeaderChar(): Sets the character used to denote the start of a packet.
 * - setEOTChar(): Sets the character used to denote the end of a packet.
 */




#ifndef SERIALDATALINK_H
#define SERIALDATALINK_H

#include <Arduino.h>



class SerialDataLink {
public:
    // Constructor
    SerialDataLink(Stream &serial, uint8_t transmitID, uint8_t receiveID, uint8_t maxIndexTX, uint8_t maxIndexRX, bool enableRetransmit = false);

    // Method to handle data transmission and reception
    void run();
    
    void updateData(uint8_t index, int16_t value);

     // Check if new data has been received
    bool checkNewData(bool resetFlag);
    int16_t getReceivedData(uint8_t index);

    // Check for  errors
    bool checkTransmissionError(bool resetFlag);
    bool checkReadError(bool resetFlag);

    // Setter methods for various parameters and special characters
    
    void setUpdateInterval(unsigned long interval);
    void setAckTimeout(unsigned long timeout);
    void setPacketTimeout(unsigned long timeout);

    void setHeaderChar(char header);
    void setEOTChar(char eot);
    void muteACK(bool mute);

private:
  enum class DataLinkState 
  {
    Idle,
    Transmitting,
    WaitingForAck,
    Receiving,
    Error
  };

    DataLinkState currentState;
    Stream &serial;
    uint8_t transmitID;
    uint8_t receiveID;

    // Separate max indices for TX and RX
    const uint8_t maxIndexTX;
    const uint8_t maxIndexRX;


    // Buffer and state management
    static const uint8_t txBufferSize = 128; // Adjust size as needed
    static const uint8_t rxBufferSize = 128; // Adjust size as needed

    uint8_t txBuffer[txBufferSize];
    uint8_t rxBuffer[rxBufferSize];

    uint8_t txBufferIndex;
    uint8_t rxBufferIndex;
    uint8_t sendBufferIndex = 0;
    
    bool isTransmitting;
    bool transmissionComplete = false;
    bool isReceiving;
    bool readComplete = false;
    bool retransmitEnabled;
    bool transmissionError = false;
    bool readError = false;
    bool muteAcknowledgement  = false;

    // Data arrays and update management

    static const uint8_t dataArraySizeTX = 20;  // Adjust size as needed for TX
    static const uint8_t dataArraySizeRX = 20;  // Adjust size as needed for RX

    int16_t dataArrayTX[dataArraySizeTX];
    int16_t dataArrayRX[dataArraySizeRX];
    bool    dataUpdated[dataArraySizeTX];
    unsigned long lastSent[dataArraySizeTX];
    
    unsigned long updateInterval = 1000;
    unsigned long ACK_TIMEOUT = 200;
    unsigned long PACKET_TIMEOUT = 200; // Timeout in milliseconds

    unsigned long lastStateChangeTime = 0;
    unsigned long stateChangeTimeout = 300;

    // Special characters for packet framing
    char headerChar = '<';
    char eotChar = '>';
    
    static const uint8_t ACK_CODE = 0x06;        // Standard acknowledgment
    static const uint8_t ACK_RTT_CODE = 0x07;    // Acknowledgment with request to transmit
    static const uint8_t NACK_CODE = 0x08;       // Negative acknowledgment
    static const uint8_t NACK_RTT_CODE = 0x09;   // Negative acknowledgment with request to transmit

    

    // Internal methods for packet construction, transmission, and reception
    bool shouldTransmit();
    void constructPacket();
    void addToTxBuffer(uint8_t byte);
    bool sendNextByte();
    bool ackReceived();
    bool ackTimeout();
    void updateState(DataLinkState newState);  
    
      // Internal methods for reception
    void read();
    void handleResendRequest();
    bool isCompletePacket();
    void processPacket();
    void sendACK();
    bool checkCRC();
    uint16_t calculateCRC16(const uint8_t* data, size_t length);

    unsigned long lastTransmissionTime;
    bool  requestToSend = false;
    unsigned long lastHeaderTime = 0;
    bool newData = false;
    bool needToACK = false;
    bool needToNACK = false;
    uint8_t eotPosition = 0;
};

#endif // SERIALDATALINK_H
