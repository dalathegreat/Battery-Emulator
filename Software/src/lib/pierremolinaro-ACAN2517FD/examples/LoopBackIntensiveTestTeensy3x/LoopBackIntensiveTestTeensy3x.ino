//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Demo in loopback mode, using hardware SPI1, with an external interrupt
//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2517FD.h>

//——————————————————————————————————————————————————————————————————————————————
//  MCP2517 connections: adapt theses settings to your design
//  As hardware SPI is used, you should select pins that support SPI functions.
//  This sketch is designed for a Teensy 3.5, using SPI1
//  But standard Teensy 3.5 SPI1 pins are not used
//    SCK input of MCP2517FD is connected to pin #32
//    SDI input of MCP2517FD is connected to pin #0
//    SDO output of MCP2517FD is connected to pin #1
//  CS input of MCP2517FD should be connected to a digital output port
//  INT output of MCP2517FD should be connected to a digital input port, with interrupt capability
//——————————————————————————————————————————————————————————————————————————————

static const byte MCP2517_SCK = 32 ; // SCK input of MCP2517
static const byte MCP2517_SDI =  0 ; // SDI input of MCP2517
static const byte MCP2517_SDO =  1 ; // SDO output of MCP2517

static const byte MCP2517_CS  = 31 ; // CS input of MCP2517
static const byte MCP2517_INT = 38 ; // INT output of MCP2517

//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Driver object
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
//--- Configure ACAN2517FD
  Serial.println ("Configure ACAN2517FD") ;
//--- Settings
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, DataBitRateFactor::x1) ;
//--- Select loopback mode
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack ;
//--- RAM Usage
  Serial.print ("MCP2517FD RAM Usage: ") ;
  Serial.print (settings.ramUsage ()) ;
  Serial.println (" bytes") ;
//--- Begin
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
    Serial.print ("Actual Data Bit Rate: ") ;
    Serial.print (settings.actualDataBitRate ()) ;
    Serial.println (" bit/s") ;
 }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 2000 ;
static uint32_t gFrameCount = 0 ;
static uint32_t gDebit = 0 ;
static uint32_t gDebitMax = 0 ;
static CANFDMessage gCurrentFrame ;
static bool gWaitingForReception = false ;

//——————————————————————————————————————————————————————————————————————————————

void loop () {
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 2000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    if (gDebitMax < gDebit) {
      gDebitMax = gDebit ;
    }
    Serial.print ("Sent: ") ;
    Serial.print (gFrameCount) ;
    Serial.print (", ") ;
    Serial.print (gDebit) ;
    Serial.print (" bytes/s, max ") ;
    Serial.print (gDebitMax) ;
    Serial.println (" bytes/s") ;
    gDebit = 0 ;
  }
//--- Send frame ?
  if (!gWaitingForReception) {
    gCurrentFrame.ext = (random () & 1) == 0 ;
    gCurrentFrame.id = (uint32_t) random () ;
    if (gCurrentFrame.ext) {
      gCurrentFrame.id &= 0x1FFFFFFF ;
    }else{
      gCurrentFrame.id &= 0x7FF ;
    }
    gCurrentFrame.len = (uint8_t) (((uint32_t) random ()) % 65) ;
    for (uint8_t i=0 ; i<gCurrentFrame.len ; i++) {
      gCurrentFrame.data [i] = (uint8_t) random () ;
    }
    gCurrentFrame.pad () ;
    gDebit += gCurrentFrame.len ;
    const bool ok = can.tryToSend (gCurrentFrame) ;
    if (ok) {
      gFrameCount += 1 ;
      gWaitingForReception = true ;
    }else{
      Serial.println ("Send failure") ;
    }
  }
//--- Receive message ?
  if (gWaitingForReception && can.available ()) {
    gWaitingForReception = false ;
    CANFDMessage frame ;
    can.receive (frame) ;
  //--- Check receive frame is identical to sent frame
    if (frame.ext != gCurrentFrame.ext) {
      Serial.println ("ext error") ;
    }
    if (frame.id != gCurrentFrame.id) {
      Serial.println ("id error") ;
    }
    if (frame.len != gCurrentFrame.len) {
      Serial.println ("length error") ;
    }else{
      bool ok = true ;
      for (uint8_t i=0 ; (i<frame.len) && ok ; i++) {
        ok = frame.data [i] == gCurrentFrame.data [i] ;
      }
      if (!ok) {
        Serial.println ("data error") ;
      }
    }
  }
}

//——————————————————————————————————————————————————————————————————————————————
