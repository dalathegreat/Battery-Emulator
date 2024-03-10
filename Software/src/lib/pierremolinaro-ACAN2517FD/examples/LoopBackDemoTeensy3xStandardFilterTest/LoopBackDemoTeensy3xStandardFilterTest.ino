//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Filter Demo in loopback mode, using hardware SPI1
//  This sketch tests standard receive filters
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
//----------------------------------- Define alternate pins for SPI1 (see https://www.pjrc.com/teensy/td_libs_SPI.html)
  SPI1.setMOSI (MCP2517_SDI) ;
  SPI1.setMISO (MCP2517_SDO) ;
  SPI1.setSCK (MCP2517_SCK) ;
//----------------------------------- Begin SPI1
  SPI1.begin () ;
//----------------------------------- Configure ACAN2517FD
  Serial.println ("Configure ACAN2517FD") ;
//--- For version >= 2.1.0
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, DataBitRateFactor::x4) ;
//--- For version < 2.1.0
//  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 125 * 1000, ACAN2517FDSettings::DATA_BITRATE_x4) ;
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack; // Select loopback mode
//----------------------------------- Append filters
  ACAN2517FDFilters filters ;
  for (uint32_t i=0 ; i<11 ; i++) {
    filters.appendFrameFilter (kStandard, 1 << i, NULL) ; // Filter #i: receive standard frame with identifier 1 << i
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
static uint32_t gPhase = 0 ;
static uint32_t gReceiveFlags = (1 << 11) - 1 ;

//——————————————————————————————————————————————————————————————————————————————

void loop () {
  CANFDMessage frame ;
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 100 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    if (gPhase < 11) {
      frame.ext = false ;
      frame.id = 1 << gPhase ;
      frame.data32 [0] = 1 << gPhase ;
      frame.len = 4 ;
      const bool ok = can.tryToSend (frame) ;
      if (ok) {
        gPhase +=1 ;
      }
    }
  }
//--- Dispatch received messages
  if (can.receive (frame)) {
    bool ok = (frame.len == 4) && !frame.ext ;
    if (ok) {
      ok = frame.data32 [0] == (1U << frame.idx) ;
    }
    if (!ok) {
      Serial.print ("Receive error: idx ") ;
      Serial.print (frame.idx) ;
      Serial.print (", data ") ;
      Serial.print (frame.data32 [0], HEX) ;
      Serial.print (", length ") ;
      Serial.print (frame.len) ;
      Serial.print (", ext ") ;
      Serial.println (frame.ext) ;
    }else if ((gReceiveFlags & (1 << frame.idx)) == 0) {
      Serial.print ("Multiple reception ") ;
      Serial.println (frame.idx) ;
    }else{
      gReceiveFlags &= ~ (1 << frame.idx) ;
      if (gReceiveFlags == 0) {
        Serial.println ("Done, all standard frames received") ;
      }
    }
  }
}

//——————————————————————————————————————————————————————————————————————————————
