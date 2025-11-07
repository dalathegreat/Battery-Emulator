#pragma once
//
//    FILE: MCP23017.h
//  AUTHOR: Rob Tillaart
// VERSION: 0.9.1
// PURPOSE: Arduino library for I2C MCP23017 16 channel port expander
//    DATE: 2019-10-12
//     URL: https://github.com/RobTillaart/MCP23017_RT
//
// WARNING: please read REV D note in readme.md
//          about pin 7 and 15 and INPUT mode.

#include "Arduino.h"
#include "Wire.h"
#include "MCP23x17_registers.h"

#define MCP23017_OK                       0x00
#define MCP23017_PIN_ERROR                0x81
#define MCP23017_I2C_ERROR                0x82
#define MCP23017_VALUE_ERROR              0x83
#define MCP23017_PORT_ERROR               0x84
#define MCP23017_REGISTER_ERROR           0xFF
#define MCP23017_INVALID_READ             0xFF


class MCP23017
{
public:
  MCP23017(uint8_t address, TwoWire *wire = &Wire);

  bool     begin(bool pullup = true);
  bool     isConnected();
  uint8_t  getAddress();

  //       Patch #46
  void     setAddress(uint8_t address) { _address = address; };
  void     setWire(TwoWire *wire) {_wire = wire; };


  //       Fix #44, reverse the byte order of the 16 bit API.
  //       reverse == false ==> backwards compatible (default)
  //       reverse == true ==> swaps the A and B byte to be more intuitive.
  void     reverse16ByteOrder(bool reverse = false);


  //       single pin interface
  //       mode = INPUT, OUTPUT, INPUT_PULLUP (= same as INPUT)
  //             do not use 0, 1 for mode.
  //       See readme.md section REV D
  bool     pinMode1(uint8_t pin, uint8_t mode);
  bool     write1(uint8_t pin, uint8_t value);
  uint8_t  read1(uint8_t pin);

  bool     setPolarity(uint8_t pin, bool reversed);
  bool     getPolarity(uint8_t pin, bool &reversed);
  bool     setPullup(uint8_t pin, bool pullup);
  bool     getPullup(uint8_t pin, bool &pullup);


  //       8 pins interface
  //       port  = 0..1
  //       mask  = 0x00..0xFF  bit pattern,
  //               bit 0 = output mode, bit 1 = input mode
  //       value = bit pattern.
  bool     pinMode8(uint8_t port, uint8_t mask);
  bool     write8(uint8_t port, uint8_t value);
  int      read8(uint8_t port);

  bool     setPolarity8(uint8_t port, uint8_t mask);
  bool     getPolarity8(uint8_t port, uint8_t &mask);
  bool     setPullup8(uint8_t port, uint8_t mask);
  bool     getPullup8(uint8_t port, uint8_t &mask);


  //       16 pins interface
  //       mask = 0x0000..0xFFFF bit pattern
  //              bit 0 = output mode, bit 1 = input mode
  //       value = bit pattern.
  bool     pinMode16(uint16_t mask);
  bool     write16(uint16_t value);
  uint16_t read16();

  bool     setPolarity16(uint16_t mask);
  bool     getPolarity16(uint16_t &mask);
  bool     setPullup16(uint16_t mask);
  bool     getPullup16(uint16_t &mask);


  //       INTERRUPTS (experimental)
  //       pin = 0..15, mode = { RISING, FALLING, CHANGE }
  bool     enableInterrupt(uint8_t pin, uint8_t mode);
  bool     disableInterrupt(uint8_t pin);

  //       mask = 0x00..0xFF  (overrides all earlier settings.
  bool     enableInterrupt8(uint8_t port, uint8_t mask, uint8_t mode);
  bool     disableInterrupt8(uint8_t port, uint8_t mask);

  //       mask = 0x0000..0xFFFF  (overrides all earlier settings.
  bool     enableInterrupt16(uint16_t mask, uint8_t mode);
  bool     disableInterrupt16(uint16_t mask);

  //       which pins caused the INT?
  uint16_t getInterruptFlagRegister();
  uint16_t getInterruptCaptureRegister();
  uint8_t  getInterruptCaptureRegister8(uint8_t port);

  //       polarity: 0 = LOW, 1 = HIGH, 2 = NONE/ODR
  bool     setInterruptPolarity(uint8_t polarity);
  uint8_t  getInterruptPolarity();

  //       merge INTA and INTB
  bool     mirrorInterrupts(bool on);
  bool     isMirroredInterrupts();

  //       debugging
  uint8_t  lastError();

  //       set/clear IOCR bit fields
  bool     enableControlRegister(uint8_t mask);
  bool     disableControlRegister(uint8_t mask);


protected:
  //       access to low level registers (just make these functions public).
  //       USE WITH CARE !!!
  bool     writeReg(uint8_t reg, uint8_t value);
  uint8_t  readReg(uint8_t reg);
  bool     writeReg16(uint8_t reg, uint16_t value);
  uint16_t readReg16(uint8_t reg);

  bool     _reverse16ByteOrder = false;

  uint8_t   _address;
  TwoWire*  _wire;
  uint8_t   _error;
};
