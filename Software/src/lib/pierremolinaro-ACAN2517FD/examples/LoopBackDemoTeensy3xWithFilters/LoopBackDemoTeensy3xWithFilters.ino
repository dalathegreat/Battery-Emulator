//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Filter Demo in loopback mode, using hardware SPI1, with an external interrupt
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
//  ACAN2517FD Driver object
//——————————————————————————————————————————————————————————————————————————————

ACAN2517FD can (MCP2517_CS, SPI1, MCP2517_INT) ;

//——————————————————————————————————————————————————————————————————————————————
//   RECEIVE FUNCTION
//——————————————————————————————————————————————————————————————————————————————

void receiveFromFilter0 (const CANFDMessage & inMessage) {
  Serial.println ("Match filter 0") ;
}
//——————————————————————————————————————————————————————————————————————————————

void receiveFromFilter1 (const CANFDMessage & inMessage) {
  Serial.println ("Match filter 1") ;
}

//——————————————————————————————————————————————————————————————————————————————

void receiveFromFilter2 (const CANFDMessage & inMessage) {
  Serial.println ("Match filter 2") ;
}

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
//----------------------------------- Define alternate pins for SPI1 (see https://www.pjrc.com/teensy/td_libs_SPI.html)
  SPI1.setMOSI (MCP2517_SDI) ;
  SPI1.setMISO (MCP2517_SDO) ;
  SPI1.setSCK (MCP2517_SCK) ;
//----------------------------------- Begin SPI1
  SPI1.begin () ;
//----------------------------------- Configure ACAN2517FD
  Serial.println ("Configure ACAN2517FD") ;
//--- For version >= 2.1.0
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, DataBitRateFactor::x1) ;
//--- For version < 2.1.0
//  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, ACAN2517FDSettings::DATA_BITRATE_x1) ;
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack ; // Select loopback mode
//----------------------------------- Append filters
  ACAN2517FDFilters filters ;
  filters.appendFrameFilter (kStandard, 0x123, receiveFromFilter0) ; // Filter #0: receive standard frame with identifier 0x123
  filters.appendFrameFilter (kExtended, 0x12345678, receiveFromFilter1) ; // Filter #1: receive extended frame with identifier 0x12345678
  filters.appendFilter (kStandard, 0x70F, 0x304, receiveFromFilter2) ; // Filter #2: receive standard frame with identifier 0x3n4
//----------------------------------- Filters ok ?
  if (filters.filterStatus () != ACAN2517FDFilters::kFiltersOk) {
    Serial.print ("Error filter ") ;
    Serial.print (filters.filterErrorIndex ()) ;
    Serial.print (": ") ;
    Serial.println (filters.filterStatus ()) ;
  }
//----------------------------------- Enter configuration
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }, filters) ;
//----------------------------------- Config ok ?
 if (errorCode != 0) {
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//----------------------------------------------------------------------------------------------------------------------

static uint32_t gBlinkLedDate = 0 ;
static uint8_t gPhase = 0 ;

//——————————————————————————————————————————————————————————————————————————————

void loop() {
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 2000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    CANFDMessage frame ;
    if (gPhase == 0) {
      frame.id = 0x123 ; // Will match filter #0
    }else if (gPhase == 1) {
      frame.ext = true ;
      frame.id = 0x12345678 ; // Will match filter #1
    }else if (gPhase == 2) {
      frame.id = 0x334 ; // Will match filter #2
    }
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      Serial.print ("Sent for filter ") ;
      Serial.println (gPhase) ;
    }else{
      Serial.println ("Send failure") ;
    }
    gPhase = (gPhase + 1) % 3 ;
  }
//--- Dispatch received messages
  can.dispatchReceivedMessage () ;
}

//——————————————————————————————————————————————————————————————————————————————
