// =================================================================================================
// eModbus: Copyright 2020, 2021 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================

#include "CoilData.h"
#undef LOCAL_LOG_LEVEL
#include "Logging.h"

// Constructor: optional size in bits, optional initial value for all bits
// Maximum size is 2000 coils (=250 bytes)
CoilData::CoilData(uint16_t size, bool initValue) :
  CDsize(0),
  CDbyteSize(0), 
  CDbuffer(nullptr) {
  // Limit the size to 2000 (Modbus rules)
  if (size > 2000) size = 2000;
  // Do we have a size?
  if (size) {
    // Calculate number of bytes needed
    CDbyteSize = byteIndex(size - 1) + 1;
    // Allocate and init buffer
    CDbuffer = new uint8_t[CDbyteSize];
    memset(CDbuffer, initValue ? 0xFF : 0, CDbyteSize);
    if (initValue) {
      CDbuffer[CDbyteSize - 1] &= CDfilter[bitIndex(size - 1)];
    }
    CDsize = size;
  }
}

// Alternate constructor, taking a "1101..." bit image char array to init
CoilData::CoilData(const char *initVector) :
  CDsize(0),
  CDbyteSize(0), 
  CDbuffer(nullptr) {
  // Init with bit image array. 
  setVector(initVector);
}

// Destructor: take care of cleaning up
CoilData::~CoilData() {
  if (CDbuffer) {
    delete CDbuffer;
  }
}

// Assignment operator
CoilData& CoilData::operator=(const CoilData& m) {
  // Avoid self-assignment
  if (this == &m) return *this;
  // Remove old data
  if (CDbuffer) {
    delete CDbuffer;
  }
  // Are coils in source?
  if (m.CDsize > 0) {
    // Yes. Allocate new buffer and copy data
    CDbuffer = new uint8_t[m.CDbyteSize];
    memcpy(CDbuffer, m.CDbuffer, m.CDbyteSize);
    CDsize = m.CDsize;
    CDbyteSize = m.CDbyteSize;
  } else {
    // No, leave buffer empty
    CDsize = 0;
    CDbyteSize = 0;
    CDbuffer = nullptr;
  }
  return *this;
}

// Copy constructor
CoilData::CoilData(const CoilData& m) :
  CDsize(0),
  CDbyteSize(0), 
  CDbuffer(nullptr) {
  // Has the source coils at all?
  if (m.CDsize > 0) {
    // Yes. Allocate new buffer and copy data
    CDbuffer = new uint8_t[m.CDbyteSize];
    memcpy(CDbuffer, m.CDbuffer, m.CDbyteSize);
    CDsize = m.CDsize;
    CDbyteSize = m.CDbyteSize;
  }
}

#ifndef NO_MOVE
// Move constructor
CoilData::CoilData(CoilData&& m) {
  // Copy all data
  CDbuffer = m.CDbuffer;
  CDsize = m.CDsize;
  CDbyteSize = m.CDbyteSize;
  // Then clear source
  m.CDbuffer = nullptr;
  m.CDsize = 0;
  m.CDbyteSize = 0;
}

// Move assignment
CoilData& CoilData::operator=(CoilData&& m) {
  // Remove buffer, if already allocated
  if (CDbuffer) {
    delete CDbuffer;
  }
  // Are there coils in the source at all?
  if (m.CDsize > 0) {
    // Yes. Copy over all data
    CDbuffer = m.CDbuffer;
    CDsize = m.CDsize;
    CDbyteSize = m.CDbyteSize;
    // Then clear source
    m.CDbuffer = nullptr;
    m.CDsize = 0;
    m.CDbyteSize = 0;
  } else {
    // No, leave object empty.
    CDbuffer = nullptr;
    CDsize = 0;
    CDbyteSize = 0;
  }
  return *this;
}
#endif

// Comparison operators
bool CoilData::operator==(const CoilData& m) {
  // Self-compare is always true
  if (this == &m) return true;
  // Different sizes are never equal
  if (CDsize != m.CDsize) return false;
  // Compare the data
  if (CDsize > 0 && memcmp(CDbuffer, m.CDbuffer, CDbyteSize)) return false;
  return true;
}

// Inequality: invert the result of the equality comparison
bool CoilData::operator!=(const CoilData& m) {
  return !(*this == m);
}

// Assignment of a bit image char array to re-init
CoilData& CoilData::operator=(const char *initVector) {
  // setVector() may be unsuccessful - then data is deleted!
  setVector(initVector);
  return *this;
}

// If used as vector<uint8_t>, return a complete slice
CoilData::operator vector<uint8_t> const () {
  // Create new vector to return
  vector<uint8_t> retval;
  if (CDsize > 0) {
    // Copy over all buffer content
      retval.assign(CDbuffer, CDbuffer + CDbyteSize);
  }
  // return the copy (or an empty vector)
  return retval;
}

// slice: return a CoilData object with coils shifted leftmost
// will return empty object if illegal parameters are detected
CoilData CoilData::slice(uint16_t start, uint16_t length) {
  CoilData retval;

  // Any slice of an empty coilset is an empty coilset ;)
  if (CDsize == 0) return retval;

  // If start is beyond the available coils, return empty slice
  if (start > CDsize) return retval;

  // length default is all up to the end
  if (length == 0) length = CDsize - start;

  // Does the requested slice fit in the buffer?
  if ((start + length) <= CDsize) {
    // Yes, it does. Extend return object
    retval = CoilData(length);

  // Loop over all requested bits
    for (uint16_t i = start; i < start + length; ++i) {
      if (CDbuffer[byteIndex(i)] & (1 << bitIndex(i))) {
        retval.set(i - start, true);
      }
    }
  }
  return retval;
}

// operator[]: return value of a single coil
bool CoilData::operator[](uint16_t index) const {
  if (index < CDsize) {
    return (CDbuffer[byteIndex(index)] & (1 << bitIndex(index))) ? true : false;
  }
  // Wrong parameter -> always return false
  return false;
}

// set functions to change coil value(s)
// Will return true if done, false if impossible (wrong address or data)

// set #1: alter one single coil
bool CoilData::set(uint16_t index, bool value) {
  // Within coils?
  if (index < CDsize) {
    // Yes. Determine affected byte and bit therein
    uint16_t by = byteIndex(index);
    uint8_t mask = 1 << bitIndex(index);

    // Stamp out bit
    CDbuffer[by] &= ~mask;
    // If required, set it to 1 now
    if (value) {
      CDbuffer[by] |= mask;
    }
    return true;
  }
  // Wrong parameter -> always return false
  return false;
}

// set #2: alter a group of coils, overwriting it by the bits from vector newValue
bool CoilData::set(uint16_t start, uint16_t length, vector<uint8_t> newValue) {
  // Does the vector contain enough data for the specified size?
  if (newValue.size() >= (size_t)(byteIndex(length - 1) + 1)) {
    // Yes, we safely may call set #3 with it
    return set(start, length, newValue.data());
  }
  return false;
}

// set #3: alter a group of coils, overwriting it by the bits from uint8_t buffer newValue
// **** Watch out! ****
// This may be a potential risk if newValue is pointing to an array shorter than required. 
// Then heap data behind the array may be used to set coils!
bool CoilData::set(uint16_t start, uint16_t length, uint8_t *newValue) {
  // Does the requested slice fit in the buffer?
  if (length && (start + length) <= CDsize) {
    // Yes, it does. 
    // Prepare pointers to the source byte and the bit within
    uint8_t *cp = newValue;
    uint8_t bitPtr = 0;

    // Loop over all bits to be set
    for (uint16_t i = start; i < start + length; i++) {
      // Get affected byte
      uint8_t by = byteIndex(i);
      // Calculate single-bit mask in target byte
      uint8_t mask = 1 << bitIndex(i);
      // Stamp out bit
      CDbuffer[by] &= ~mask;
      // is source bit set?
      if (*cp & (1 << bitPtr)) {
        // Yes. Set it in target as well
        CDbuffer[by] |= mask;
      }
      // Advance source bit ptr
      bitPtr++;
      // Overflow?
      if (bitPtr >= 8) {
        // Yes. move pointers to first bit in next source byte
        bitPtr = 0;
        cp++;
      }
    }
    return true;
  }
  return false;
}

// set #4: alter a group of coils, overwriting it by the coils in another CoilData object
// Setting stops when either target storage or source coils are exhausted
bool CoilData::set(uint16_t index, const CoilData& c) {
  // if source object is empty, return false
  if (c.empty()) return false;

  // If target is empty, or index is beyond coils, return false
  if (CDsize == 0 || index >= CDsize) return false;

  // Take the minimum of remaining coils after index and the length of c
  uint16_t length = CDsize - index;
  if (c.coils() < length) length = c.coils();

  // Loop over all coils to be copied
  for (uint16_t i = index; i < index + length; ++i) {
    set(i, c[i - index]);
  }
  return true;
}

// set #5: alter a group of coils, overwriting it by a bit image array
// Setting stops when either target storage or source bits are exhausted
bool CoilData::set(uint16_t index, const char *initVector) {
  // if target is empty or index is beyond coils, return false
  if (CDsize == 0 || index >= CDsize) return false;

  // We do a single pass on the bit image array, until it ends or the target is exhausted
  const char *cp = initVector;   // pointer to source array
  bool skipFlag = false;         // Signal next character irrelevant

  while (*cp && index < CDsize) {
    switch (*cp) {
    case '1':  // A valid 1 bit
    case '0':  // A valid 0 bit
      // Shall we ignore it?
      if (skipFlag) {
        // Yes. just reset the ignore flag
        skipFlag = false;
      } else {
        // No, we can set it. First stamp out the existing bit
        CDbuffer[byteIndex(index)] &= ~(1 << bitIndex(index));
        // Do we have a 1 bit here?
        if (*cp == '1') {
          // Yes. set it in coil storage
          CDbuffer[byteIndex(index)] |= (1 << bitIndex(index));
        }
        index++;
      }
      break;
    case '_':  // Skip next
      skipFlag = true;
      break;
    default:  // anything else
      skipFlag = false;
      break;
    }
    cp++;
  }
  return true;
}

// Comparison against bit image array
bool CoilData::operator==(const char *initVector) {
  const char *cp = initVector;   // pointer to source array
  bool skipFlag = false;         // Signal next character irrelevant
  uint16_t index = 0;

  // We do a single pass on the bit image array, until it ends or the target is exhausted
  while (*cp && index < CDsize) {
    switch (*cp) {
    case '1':  // A valid 1 bit
    case '0':  // A valid 0 bit
      // Shall we ignore it?
      if (skipFlag) {
        // Yes. just reset the ignore flag
        skipFlag = false;
      } else {
        // No, we can compare it
        uint8_t value = CDbuffer[byteIndex(index)] & (1 << bitIndex(index));
        // Do we have a 1 bit here?
        if (*cp == '1') {
          // Yes. Is the source different? Then we can stop
          if (value == 0) return false;
        } else {
          // No, it is a 0. Different?
          if (value) return false;
        }
        index++;
      }
      break;
    case '_':  // Skip next
      skipFlag = true;
      break;
    default:  // anything else
      skipFlag = false;
      break;
    }
    cp++;
  }
  // So far everything was equal, but we may have more bits in the image array!
  if (*cp) {
    // There is more. Check for more valid bits
    while (*cp) {
      switch (*cp) {
      case '1':  // A valid 1 bit
      case '0':  // A valid 0 bit
        // Shall we ignore it?
        if (skipFlag) {
          // Yes. just reset the ignore flag
          skipFlag = false;
        } else {
          // No, a valid bit that exceeds the target coils count
          return false;
        }
        break;
      case '_':  // Skip next
        skipFlag = true;
        break;
      default:  // anything else
        skipFlag = false;
        break;
      }
      cp++;
    }
  }
  return true;
}

bool CoilData::operator!=(const char *initVector) {
  return !(*this == initVector);
}

// Init all coils by a readable bit image array
bool CoilData::setVector(const char *initVector) {
  uint16_t length = 0;           // resulting bit pattern length
  const char *cp = initVector;   // pointer to source array
  bool skipFlag = false;         // Signal next character irrelevant

  // Do a first pass to count all valid bits in array
  while (*cp) {
    switch (*cp) {
    case '1':  // A valid 1 bit
    case '0':  // A valid 0 bit
      // Shall we ignore it?
      if (skipFlag) {
        // Yes. just reset the ignore flag
        skipFlag = false;
      } else {
        // No, we can count it
        length++;
      }
      break;
    case '_':  // Skip next
      skipFlag = true;
      break;
    default:  // anything else
      skipFlag = false;
      break;
    }
    cp++;
  }

  // If there are coils already, trash them.
  if (CDbuffer) {
    delete CDbuffer;
  }
  CDsize = 0;
  CDbyteSize = 0;

  // Did we count a manageable number?
  if (length && length <= 2000) {
    // Yes. Init the coils
    CDsize = length;
    CDbyteSize = byteIndex(length - 1) + 1;
    // Allocate new coil storage
    CDbuffer = new uint8_t[CDbyteSize];
    memset(CDbuffer, 0, CDbyteSize);

    // Prepare second loop
    uint16_t ptr = 0;    // bit pointer in coil storage
    skipFlag = false;
    cp = initVector;

    // Do a second pass, converting 1 and 0 into coils (bits)
    // loop as above, only difference is setting the bits
    while (*cp) {
      switch (*cp) {
      case '1':
      case '0':
        if (skipFlag) {
          skipFlag = false;
        } else {
          // Do we have a 1 bit here?
          if (*cp == '1') {
            // Yes. set it in coil storage
            CDbuffer[byteIndex(ptr)] |= (1 << bitIndex(ptr));
          }
          // advance bit pointer in any case 0 or 1
          ptr++;
        }
        break;
      case '_':
        skipFlag = true;
        break;
      default:
        skipFlag = false;
        break;
      }
      cp++;
    }
    // We had content, so return true
    return true;
  }
  // No valid bits found, return false 
  return false;
}

// init: set all coils to 1 or 0 (default)
void CoilData::init(bool value) {
  if (CDsize > 0) {
    memset(CDbuffer, value ? 0xFF : 0, CDbyteSize);
    // Stamp out overhang bits
    CDbuffer[CDbyteSize - 1] &= CDfilter[bitIndex(CDsize - 1)];
  }
}

// Return number of coils set to 1 (or not)
// Uses Brian Kernighan's algorithm!
uint16_t CoilData::coilsSetON() const {
  uint16_t count = 0;

  // Do we have coils at all?
  if (CDbyteSize) {
    // Yes. Loop over all bytes summing up the '1' bits
    for (uint8_t i = 0; i < CDbyteSize; ++i) {
      uint8_t by = CDbuffer[i];
      while (by) {
        by &= by - 1; // this clears the LSB-most set bit
        count++;
      }
    }
  }
  return count;
}

uint16_t CoilData::coilsSetOFF() const {
  return CDsize - coilsSetON();
}

#if !IS_LINUX
// Not for Linux for the Print reference!

// Print out a coil storage in readable form to ease debugging
void CoilData::print(const char *label, Print& s) {
  uint8_t bitptr = 0;
  uint8_t labellen = strlen(label);
  uint8_t pos = labellen;

  // Put out the label
  s.print(label);

  // Print out all coils as "1" or "0"
  for (uint16_t i = 0; i < CDsize; ++i) {
    s.print((CDbuffer[byteIndex(i)] & (1 << bitptr)) ? "1" : "0");
    pos++;
    // Have a blank after every group of 4
    if (i % 4 == 3) {
      // Have a line break if > 80 characters, including the last group of 4
      if (pos >= 80) {
        s.println("");
        pos = 0;
        // Leave a nice empty space below the label
        while (pos++ < labellen) {
          s.print(" ");
        }
      } else {
        s.print(" ");
        pos++;
      }
    }
    bitptr++;
    bitptr &= 0x07;
  }
  s.println("");
}
#endif
