//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Demo for "ThreeAttempts" and "Disabled" setting,
//  using hardware SPI1, with an external interrupt
//  For this sketch, the MCP2517FD should be alone on the CAN bus.
//
//  A CAN message should be acknowledged by a receiver.
//  By default, no receiver -> no acknnowledge, the message is sent indefinitely
//  The settings.mControllerTransmitFIFORetransmissionAttempts can be used for
//  specifying:
//    - only one attempt (ACAN2517Settings::Disabled);
//    - three attempts (ACAN2517Settings::ThreeAttempts);
//    - unlimited number of attempts (ACAN2517Settings::Unlimited, default setting).
//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2517FD.h>

//——————————————————————————————————————————————————————————————————————————————
//  MCP2517 connections: adapt theses settings to your design
//  As hardware SPI is used, you should select pins that support SPI functions.
//  This sketch is designed for a Teensy 3.5, using SPI1
//  But standard Teensy 3.5 SPI1 pins are not used
//    SCK input of MCP2517 is connected to pin #32
//    SDI input of MCP2517 is connected to pin #0
//    SDO output of MCP2517 is connected to pin #1
//  CS input of MCP2517 should be connected to a digital output port
//  INT output of MCP2517 should be connected to a digital input port, with interrupt capability
//——————————————————————————————————————————————————————————————————————————————

static const byte MCP2517_SCK = 32 ; // SCK input of MCP2517
static const byte MCP2517_SDI =  0 ; // SDI input of MCP2517
static const byte MCP2517_SDO =  1 ; // SDO output of MCP2517

static const byte MCP2517_CS  = 31 ; // CS input of MCP2517
static const byte MCP2517_INT = 38 ; // INT output of MCP2517

//——————————————————————————————————————————————————————————————————————————————
//  MCP2517 Driver object
//——————————————————————————————————————————————————————————————————————————————

ACAN2517FD can (MCP2517_CS, SPI1, MCP2517_INT) ;

//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

void setup () {
//--- Switch on builtin led
  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
//--- Start serial
  Serial.begin (38400) ;
//--- Wait for serial (blink led at 10 Hz during waiting)
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
//--- Define alternate pins for SPI1 (see https://www.pjrc.com/teensy/td_libs_SPI.html)
  Serial.print ("Using pin #") ;
  Serial.print (MCP2517_SDI) ;
  Serial.print (" for MOSI: ") ;
  Serial.println (SPI1.pinIsMOSI (MCP2517_SDI) ? "yes" : "NO!!!") ;
  Serial.print ("Using pin #") ;
  Serial.print (MCP2517_SDO) ;
  Serial.print (" for MISO: ") ;
  Serial.println (SPI1.pinIsMISO (MCP2517_SDO) ? "yes" : "NO!!!") ;
  Serial.print ("Using pin #") ;
  Serial.print (MCP2517_SCK) ;
  Serial.print (" for SCK: ") ;
  Serial.println (SPI1.pinIsSCK (MCP2517_SCK) ? "yes" : "NO!!!") ;
  SPI1.setMOSI (MCP2517_SDI) ;
  SPI1.setMISO (MCP2517_SDO) ;
  SPI1.setSCK (MCP2517_SCK) ;
//----------------------------------- Begin SPI1
  SPI1.begin () ;
//--- Configure ACAN2517
  Serial.print ("sizeof (ACAN2517Settings): ") ;
  Serial.print (sizeof (ACAN2517FDSettings)) ;
  Serial.println (" bytes") ;
  Serial.println ("Configure ACAN2517") ;
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, DataBitRateFactor::x4) ;
//--- The MCP2517FD internal RAM is limited, the size of the receive buffer is reduced
  settings.mControllerReceiveFIFOSize = 2 ;
//--- Configure regular transmit chain (used when message.idx == 0, default)
  settings.mDriverTransmitFIFOSize = 0 ;
  settings.mControllerTransmitFIFOSize = 1 ;
  settings.mControllerTransmitFIFORetransmissionAttempts = ACAN2517FDSettings::Disabled ;
//--- Configure TXQ transmit chain (used when message.idx == 255)
  settings.mControllerTXQSize = 1 ;
  settings.mControllerTXQBufferRetransmissionAttempts = ACAN2517FDSettings::Disabled ;
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
  if (errorCode == 0) {
    Serial.print ("Bit Rate prescaler: ") ;
    Serial.println (settings.mBitRatePrescaler) ;
    Serial.print ("Arbitration Phase segment 1: ") ;
    Serial.println (settings.mArbitrationPhaseSegment1) ;
    Serial.print ("Arbitration Phase segment 2: ") ;
    Serial.println (settings.mArbitrationPhaseSegment2) ;
    Serial.print ("Arbitration SJW:") ;
    Serial.println (settings.mArbitrationSJW) ;
    Serial.print ("Actual Arbitration Bit Rate: ") ;
    Serial.print (settings.actualArbitrationBitRate ()) ;
    Serial.println (" bit/s") ;
    Serial.print ("Exact Arbitration Bit Rate ? ") ;
    Serial.println (settings.exactArbitrationBitRate () ? "yes" : "no") ;
    Serial.print ("Arbitration Sample point: ") ;
    Serial.print (settings.arbitrationSamplePointFromBitStart ()) ;
    Serial.println ("%") ;
    Serial.print ("Data Phase segment 1: ") ;
    Serial.println (settings.mDataPhaseSegment1) ;
    Serial.print ("Data Phase segment 2: ") ;
    Serial.println (settings.mDataPhaseSegment2) ;
    Serial.print ("Data SJW:") ;
    Serial.println (settings.mDataSJW) ;
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 1000 ;
static uint32_t gReceivedFrameCount = 0 ;
static uint32_t gSentFrameCount = 0 ;

//——————————————————————————————————————————————————————————————————————————————

void loop() {
  CANFDMessage frame ;
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 500 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    frame.len = 64 ;
    for (uint8_t i=0 ; i<frame.len ; i++) {
      frame.data [i] = i ;
    }
    // frame.idx = 255 ;  // Uncomment this for sending throught TXQ
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      gSentFrameCount += 1 ;
      Serial.print ("Sent: ") ;
      Serial.println (gSentFrameCount) ;
    }else{
      Serial.println ("Send failure") ;
    }
  }
  if (can.available ()) {
    can.receive (frame) ;
    gReceivedFrameCount ++ ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedFrameCount) ;
  }
}

//——————————————————————————————————————————————————————————————————————————————
