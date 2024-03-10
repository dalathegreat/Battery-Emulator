//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD or ACAN2518FD send-even, for Teensy 3.5 using SPI1
//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2517FD.h>

//——————————————————————————————————————————————————————————————————————————————
//  MCP2517FD connections: adapt theses settings to your design
//  As hardware SPI is used, you should select pins that support SPI functions.
//  This sketch is designed for a Teensy 3.5, using SPI1
//  But standard Teensy 3.5 SPI1 pins are not used
//    SCK input of MCP2517 is connected to pin #32
//    SDI input of MCP2517 is connected to pin #0
//    SDO output of MCP2517 is connected to pin #1
//  CS input of MCP2517 should be connected to a digital output port
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
//--- Wait for Serial monitor (blink at 5 Hz)
  Serial.begin (38400) ;
  while (!Serial) { digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ; }
//----------------------------------- Begin SPI1
  SPI1.begin () ;
//----------------------------------- Configure ACAN2517FD
//--- For version >= 2.1.0
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 1000 * 1000, DataBitRateFactor::x8) ;
//--- For version < 2.1.0
//  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 1000 * 1000, ACAN2517FDSettings::DATA_BITRATE_x8) ;
  settings.mDriverReceiveFIFOSize = 200 ;
//--- Begin
  const uint32_t errorCode = can.begin (settings, [] { can.isr () ; }) ;
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
  Serial.print ("TDCO:") ;
  Serial.println (settings.mTDCO) ;
  Serial.print ("Even, error code 0x") ;
  Serial.println (errorCode, HEX) ;
 //--- Endless  loop on error
  while (errorCode != 0) { }
}

//——————————————————————————————————————————————————————————————————————————————

static uint32_t gBlinkLedDate = 0 ;
static uint32_t gSentFrameCount = 0 ;
static uint32_t gReceivedFrameCount = 0 ;
static bool gCompleted = false ;
static const uint32_t SEND_COUNT = 50 * 1000 ;

//——————————————————————————————————————————————————————————————————————————————

static void handleReceivedMessages (void) {
  CANFDMessage frame ;
  while (can.receive (frame)) {
    gReceivedFrameCount ++ ;
  }
}

//——————————————————————————————————————————————————————————————————————————————

void loop () {
  handleReceivedMessages () ;
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 1000 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    if (!gCompleted) {
      Serial.print ("Sent: ") ;
      Serial.print (gSentFrameCount) ;
      Serial.print (", received: ") ;
      Serial.print (gReceivedFrameCount) ;
      Serial.print (", errors ") ;
      Serial.print (can.errorCounters (), HEX) ;
      Serial.print (", op mode ") ;
      Serial.print (can.currentOperationMode ()) ;
      Serial.print (", rcved buffer ") ;
      Serial.print (can.driverReceiveBufferPeakCount ()) ;
      Serial.print (", overflows ") ;
      Serial.println (can.hardwareReceiveBufferOverflowCount ()) ;
      gCompleted = (gReceivedFrameCount >= SEND_COUNT) && (gSentFrameCount >= SEND_COUNT) ;
    }
  }
  if (gSentFrameCount < SEND_COUNT) {
    CANFDMessage frame ;
    frame.id = micros () & 0x7FE ;
    frame.len = 64 ;
    for (uint8_t i=0 ; i<frame.len ; i++) {
      frame.data [i] = i ;
    }
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      gSentFrameCount += 1 ;
    }
  }
}

//——————————————————————————————————————————————————————————————————————————————
