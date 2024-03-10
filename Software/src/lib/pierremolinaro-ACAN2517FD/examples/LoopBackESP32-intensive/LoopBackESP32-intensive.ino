//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Demo in loopback mode, intensive test, for ESP32
//——————————————————————————————————————————————————————————————————————————————

#ifndef ARDUINO_ARCH_ESP32
  #error "Select an ESP32 board"
#endif

//——————————————————————————————————————————————————————————————————————————————

#include <ACAN2517FD.h>
#include <SPI.h>

//——————————————————————————————————————————————————————————————————————————————
//  For using SPI on ESP32, see demo sketch SPI_Multiple_Buses
//  Two SPI busses are available in Arduino, HSPI and VSPI.
//  By default, Arduino SPI uses VSPI, leaving HSPI unused.
//  Default VSPI pins are: SCK=18, MISO=19, MOSI=23.
//  You can change the default pin with additional begin arguments
//    SPI.begin (MCP2517_SCK, MCP2517_MISO, MCP2517_MOSI)
//  CS input of MCP2517 should be connected to a digital output port
//  INT output of MCP2517 should be connected to a digital input port, with interrupt capability
//  Notes:
//    - GPIOs 34 to 39 are GPIs – input only pins. These pins don’t have internal pull-ups or
//      pull-down resistors. They can’t be used as outputs.
//    - some pins do not support INPUT_PULLUP (see https://www.esp32.com/viewtopic.php?t=439)
//    - All GPIOs can be configured as interrupts
// See https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//——————————————————————————————————————————————————————————————————————————————

static const byte MCP2517_SCK  = 26 ; // SCK input of MCP2517FD
static const byte MCP2517_MOSI = 19 ; // SDI input of MCP2517FD
static const byte MCP2517_MISO = 18 ; // SDO output of MCP2517FD

static const byte MCP2517_CS  = 16 ; // CS input of MCP2517FD
static const byte MCP2517_INT = 32 ; // INT output of MCP2517FD

//——————————————————————————————————————————————————————————————————————————————
//  ACAN2517FD Driver object
//——————————————————————————————————————————————————————————————————————————————

SPIClass vspi (VSPI) ; // See ESP32 demo sketch: SPI_Multiple_Busses
ACAN2517FD can (MCP2517_CS, vspi, MCP2517_INT) ;

//——————————————————————————————————————————————————————————————————————————————
//   SETUP
//——————————————————————————————————————————————————————————————————————————————

void setup () {
//--- Switch on builtin led
  pinMode (LED_BUILTIN, OUTPUT) ;
  digitalWrite (LED_BUILTIN, HIGH) ;
//--- Start serial
  Serial.begin (115200) ;
//--- Wait for serial (blink led at 10 Hz during waiting)
  while (!Serial) {
    delay (50) ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
  }
//----------------------------------- Begin SPI
  vspi.begin (MCP2517_SCK, MCP2517_MISO, MCP2517_MOSI) ;
//--- Configure ACAN2517FD
  Serial.print ("sizeof (ACAN2517FDSettings): ") ;
  Serial.print (sizeof (ACAN2517FDSettings)) ;
  Serial.println (" bytes") ;
  Serial.println ("Configure ACAN2517FD") ;
//--- For version >= 2.1.0
  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 1000 * 1000, DataBitRateFactor::x8) ;
//--- For version < 2.1.0
//  ACAN2517FDSettings settings (ACAN2517FDSettings::OSC_4MHz10xPLL, 1000 * 1000, ACAN2517FDSettings::DATA_BITRATE_x8) ;
  settings.mRequestedMode = ACAN2517FDSettings::InternalLoopBack ; // Select loopback mode
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
  }else{
    Serial.print ("Configuration error 0x") ;
    Serial.println (errorCode, HEX) ;
  }
}

//——————————————————————————————————————————————————————————————————————————————
//   LOOP
//——————————————————————————————————————————————————————————————————————————————

static uint32_t gBlinkLedDate = 0 ;
static uint32_t gReceivedFrameCount = 0 ;
static uint32_t gSentFrameCount = 0 ;

static const uint32_t MESSAGE_COUNT = 100UL * 1000 ;

//——————————————————————————————————————————————————————————————————————————————

void loop () {
  if (gBlinkLedDate < millis ()) {
    gBlinkLedDate += 500 ;
    digitalWrite (LED_BUILTIN, !digitalRead (LED_BUILTIN)) ;
    Serial.print ("Sent: ") ;
    Serial.println (gSentFrameCount) ;
    Serial.print ("Received: ") ;
    Serial.println (gReceivedFrameCount) ;
  }
  if (gSentFrameCount < MESSAGE_COUNT) {
    CANFDMessage frame ;
    const bool ok = can.tryToSend (frame) ;
    if (ok) {
      gSentFrameCount += 1 ;
    }
  }
  if (can.available ()) {
    CANFDMessage frame ;
    can.receive (frame) ;
    gReceivedFrameCount ++ ;
  }
}

//——————————————————————————————————————————————————————————————————————————————
