// =================================================================================================
// eModbus: Copyright 2020, 2021 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================

#ifndef _COILDATA_H
#define _COILDATA_H

#include <vector>
#include <cstdint>
#include "options.h"

using std::vector;

// CoilData: representing Modbus coil (=bit) values
class CoilData {
public:
  // Constructor: optional size in bits, optional initial value for all bits
  // Maximum size is 2000 coils (=250 bytes)
  explicit CoilData(uint16_t size = 0, bool initValue = false);

  // Alternate constructor, taking a "1101..." bit image char array to init
  explicit CoilData(const char *initVector);

  // Destructor: take care of cleaning up
  ~CoilData();

  // Assignment operator
  CoilData& operator=(const CoilData& m);
  
  // Copy constructor
  CoilData(const CoilData& m);

#ifndef NO_MOVE
  // Move constructor
	CoilData(CoilData&& m);
  
	// Move assignment
	CoilData& operator=(CoilData&& m);
#endif

  // Comparison operators
  bool operator==(const CoilData& m);
  bool operator!=(const CoilData& m);
  bool operator==(const char *initVector);
  bool operator!=(const char *initVector);

  // Assignment of a bit image char array to re-init
  CoilData& operator=(const char *initVector);

  // If used as vector<uint8_t>, return the complete set
  operator vector<uint8_t> const ();

  // slice: return a new CoilData object with coils shifted leftmost
  // will return empty set if illegal parameters are detected
  // Default start is first coil, default length all to the end
  CoilData slice(uint16_t start = 0, uint16_t length = 0);

  // operator[]: return value of a single coil
  bool operator[](uint16_t index) const;

  // Set functions to change coil value(s)
  // Will return true if done, false if impossible (wrong address or data)

  // set #1: alter one single coil
  bool set(uint16_t index, bool value);

  // set #2: alter a group of coils, overwriting it by the bits from newValue
  bool set(uint16_t index, uint16_t length, vector<uint8_t> newValue);

  // set #3: alter a group of coils, overwriting it by the bits from unit8_t buffer newValue
  bool set(uint16_t index, uint16_t length, uint8_t *newValue);

  // set #4: alter a group of coils, overwriting it by the coils in another CoilData object
  // Setting stops when either target storage or source coils are exhausted
  bool set(uint16_t index, const CoilData& c);

  // set #5: alter a group of coils, overwriting it by a bit image array
  // Setting stops when either target storage or source bits are exhausted
  bool set(uint16_t index, const char *initVector);

  // (Re-)init complete coil set to 1 or 0
  void init(bool value = false);

  // get size in coils
  inline uint16_t coils() const { return CDsize; }

  // Raw access to coil data buffer
  inline uint8_t *data() const { return CDbuffer; };
  inline uint8_t size() const { return CDbyteSize; };

  // Test if there are any coils in object
  inline bool empty() const { return (CDsize >0) ? true : false; }
  inline operator bool () const { return empty(); }

  // Return number of coils set to 1 (or ON)
  uint16_t coilsSetON() const;
  // Return number of coils set to 0 (or OFF)
  uint16_t coilsSetOFF() const;

#if !LINUX
  // Helper function to dump out coils in logical order
  void print(const char *label, Print& s);
#endif

protected:
  // bit masks for bits left of a bit index in a byte
  const uint8_t CDfilter[8] = { 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF };
  // Calculate byte index and bit index within that byte
  inline uint8_t byteIndex(uint16_t index) const { return index >> 3; }
  inline uint8_t bitIndex(uint16_t index) const { return index & 0x07; }
  // Calculate reversed bit sequence for a byte (taken from http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith32Bits)
  inline uint8_t reverseBits(uint8_t b) { return ((b * 0x0802LU & 0x22110LU) | (b * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16; }
  // (Re-)init with bit image vector
  bool setVector(const char *initVector);

  uint16_t CDsize;         // Size of the CoilData store in bits
  uint8_t  CDbyteSize;     // Size in bytes
  uint8_t *CDbuffer;       // Pointer to bit storage
};

#endif
