// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include "ModbusMessage.h"
#undef LOCAL_LOG_LEVEL
// #define LOCAL_LOG_LEVEL LOG_LEVEL_ERROR
#include "Logging.h"
#include <algorithm>

// Default Constructor - takes optional size of MM_data to allocate memory
ModbusMessage::ModbusMessage(uint16_t dataLen) {
  if (dataLen) MM_data.reserve(dataLen);
}

// Special message Constructor - takes a std::vector<uint8_t>
ModbusMessage::ModbusMessage(std::vector<uint8_t> s) :
MM_data(s) { }

// Destructor
ModbusMessage::~ModbusMessage() { 
  // If paranoid, one can use the below :D
  // std::vector<uint8_t>().swap(MM_data);
}

// Assignment operator
ModbusMessage& ModbusMessage::operator=(const ModbusMessage& m) {
  // Do anything only if not self-assigning
  if (this != &m) {
    // Copy data from source to target
    MM_data = m.MM_data;
  }
  return *this;
}

#ifndef NO_MOVE
  // Move constructor
ModbusMessage::ModbusMessage(ModbusMessage&& m) {
  MM_data = std::move(m.MM_data);
}
  
	// Move assignment
ModbusMessage& ModbusMessage::operator=(ModbusMessage&& m) {
  MM_data = std::move(m.MM_data);
  return *this;
}
#endif

// Copy constructor
ModbusMessage::ModbusMessage(const ModbusMessage& m) :
  MM_data(m.MM_data) { }

// Equality comparison
bool ModbusMessage::operator==(const ModbusMessage& m) {
  // Prevent self-compare
  if (this == &m) return true;
  // If size is different, we assume inequality
  if (MM_data.size() != m.MM_data.size()) return false;
  // We will compare bytes manually - for uint8_t it should work out-of-the-box,
  // but the data type might be changed later.
  // If we find a difference byte, we found inequality
  for (uint16_t i = 0; i < MM_data.size(); ++i) {
    if (MM_data[i] != m.MM_data[i]) return false;
  }
  // Both tests passed ==> equality
  return true;
}

// Inequality comparison
bool ModbusMessage::operator!=(const ModbusMessage& m) {
  return (!(*this == m));
}

// Conversion to bool
ModbusMessage::operator bool() {
  if (MM_data.size() >= 2) return true;
  return false;
}

// Exposed methods of std::vector
const uint8_t *ModbusMessage::data() { return MM_data.data(); }
uint16_t       ModbusMessage::size() { return MM_data.size(); }
void           ModbusMessage::push_back(const uint8_t& val) { MM_data.push_back(val); }
void           ModbusMessage::clear() { MM_data.clear(); }
// provide restricted operator[] interface
uint8_t  ModbusMessage::operator[](uint16_t index) const {
  if (index < MM_data.size()) {
    return MM_data[index];
  }
  LOG_W("Index %d out of bounds (>=%d).\n", index, MM_data.size());
  return 0;
}
// Resize internal MM_data
uint16_t ModbusMessage::resize(uint16_t newSize) { 
  MM_data.resize(newSize); 
  return MM_data.size(); 
}

// Add append() for two ModbusMessages or a std::vector<uint8_t> to be appended
void ModbusMessage::append(ModbusMessage& m) { 
  MM_data.reserve(size() + m.size()); 
  MM_data.insert(MM_data.end(), m.begin(), m.end()); 
}

void ModbusMessage::append(std::vector<uint8_t>& m) { 
  MM_data.reserve(size() + m.size()); 
  MM_data.insert(MM_data.end(), m.begin(), m.end()); 
}

uint8_t ModbusMessage::getServerID() const {
  // Only if we have data and it is at least as long to fit serverID and function code, return serverID
  if (MM_data.size() >= 2) { return MM_data[0]; }
  // Else return 0 - normally the Broadcast serverID, but we will not support that. Full stop. :-D
  return 0;
}

// Get MM_data[0] (server ID) and MM_data[1] (function code)
uint8_t ModbusMessage::getFunctionCode() const {
  // Only if we have data and it is at least as long to fit serverID and function code, return FC
  if (MM_data.size() >= 2) { return MM_data[1]; }
  // Else return 0 - which is no valid Modbus FC.
  return 0;
}

// getError() - returns error code
Error ModbusMessage::getError() const {
  // Do we have data long enough?
  if (MM_data.size() > 2) {
    // Yes. Does it indicate an error?
    if (MM_data[1] & 0x80)
    {
      // Yes. Get it.
      return static_cast<Modbus::Error>(MM_data[2]);
    }
  }
  // Default: everything OK - SUCCESS
  return SUCCESS;
}

// Modbus data manipulation
void    ModbusMessage::setServerID(uint8_t serverID) {
  // We accept here that [0] may allocate a byte!
  if (MM_data.empty()) {
    MM_data.reserve(3);  // At least an error message should fit
  }
  MM_data[0] = serverID;
}

void    ModbusMessage::setFunctionCode(uint8_t FC) {
   if (MM_data.size() < 2) {
    MM_data.resize(2);    // Resize to at least 2 to ensure indices 0 and 1 are valid
    MM_data[0] = 0;       // Optional:  Invalid server ID as a placeholder
  }
  MM_data[1] = FC;        // Safely set the function code
}

// add() variant to copy a buffer into MM_data. Returns updated size
uint16_t ModbusMessage::add(const uint8_t *arrayOfBytes, uint16_t count) {
  uint16_t originalSize = MM_data.size();
  MM_data.resize(originalSize + count);
  // Copy it
  std::copy(arrayOfBytes, arrayOfBytes + count, MM_data.begin() + originalSize);
  // Return updated size (logical length of message so far)
  return MM_data.size();
}

// determineFloatOrder: calculate the sequence of bytes in a float value
uint8_t ModbusMessage::determineFloatOrder() {
  constexpr uint8_t floatSize = sizeof(float);
  // Only do it if not done yet
  if (floatOrder[0] == 0xFF) {
    // We need to calculate it.
    // This will only work for 32bit floats, so check that
    if (floatSize != 4) {
      // OOPS! we cannot proceed.
      LOG_E("Oops. float seems to be %d bytes wide instead of 4.\n", floatSize);
      return 0;
    }

    uint32_t i = 77230;                             // int value to go into a float without rounding error
    float f = i;                                    // assign it
    uint8_t *b = (uint8_t *)&f;                     // Pointer to bytes of f
    const uint8_t expect[floatSize] = { 0x47, 0x96, 0xd7, 0x00 }; // IEEE754 representation 
    uint8_t matches = 0;                            // number of bytes successfully matched
     
    // Loop over the bytes of the expected sequence
    for (uint8_t inx = 0; inx < floatSize; ++inx) {
      // Loop over the real bytes of f
      for (uint8_t trg = 0; trg < floatSize; ++trg) {
        if (expect[inx] == b[trg]) {
          floatOrder[inx] = trg;
          matches++;
          break;
        }
      }
    }

    // All bytes found?
    if (matches != floatSize) {
      // No! There is something fishy...
      LOG_E("Unable to determine float byte order (matched=%d of %d)\n", matches, floatSize);
      floatOrder[0] = 0xFF;
      return 0;
    } else {
      HEXDUMP_V("floatOrder", floatOrder, floatSize);
    }
  }
  return floatSize;
}

// determineDoubleOrder: calculate the sequence of bytes in a double value
uint8_t ModbusMessage::determineDoubleOrder() {
  constexpr uint8_t doubleSize = sizeof(double);
  // Only do it if not done yet
  if (doubleOrder[0] == 0xFF) {
    // We need to calculate it.
    // This will only work for 64bit doubles, so check that
    if (doubleSize != 8) {
      // OOPS! we cannot proceed.
      LOG_E("Oops. double seems to be %d bytes wide instead of 8.\n", doubleSize);
      return 0;
    }

    uint64_t i = 5791007487489389;                  // int64 value to go into a double without rounding error
    double f = i;                                   // assign it
    uint8_t *b = (uint8_t *)&f;                     // Pointer to bytes of f
    const uint8_t expect[doubleSize] = { 0x43, 0x34, 0x92, 0xE4, 0x00, 0x2E, 0xF5, 0x6D }; // IEEE754 representation 
    uint8_t matches = 0;                            // number of bytes successfully matched
     
    // Loop over the bytes of the expected sequence
    for (uint8_t inx = 0; inx < doubleSize; ++inx) {
      // Loop over the real bytes of f
      for (uint8_t trg = 0; trg < doubleSize; ++trg) {
        if (expect[inx] == b[trg]) {
          doubleOrder[inx] = trg;
          matches++;
          break;
        }
      }
    }

    // All bytes found?
    if (matches != doubleSize) {
      // No! There is something fishy...
      LOG_E("Unable to determine double byte order (matched=%d of %d)\n", matches, doubleSize);
      doubleOrder[0] = 0xFF;
      return 0;
    } else {
      HEXDUMP_V("doubleOrder", doubleOrder, doubleSize);
    }
  }
  return doubleSize;
}

// swapFloat() and swapDouble() will re-order the bytes of a float or double value
// according a user-given pattern
float ModbusMessage::swapFloat(float& f, int swapRule) {
  LOG_V("swap float, swapRule=%02X\n", swapRule);
  // Make a byte pointer to the given float
  uint8_t *src = (uint8_t *)&f;
  // Define a "work bench" float and byte pointer to it
  float interim;
  uint8_t *dst = (uint8_t *)&interim;
  // Loop over all bytes of a float
  for (uint8_t i = 0; i < sizeof(float); ++i) {
    // Get i-th byte from the spot the swap table tells
    // (only the first 4 tables are valid for floats)
    LOG_V("dst[%d] = src[%d]\n", i, swapTables[swapRule & 0x03][i]);
    dst[i] = src[swapTables[swapRule & 0x03][i]];
    // Does the swar rule require nibble swaps?
    if (swapRule & 0x08) {
      // Yes, it does. 
      uint8_t nib = ((dst[i] & 0x0f) << 4) | ((dst[i] >> 4) & 0x0F);
      dst[i] = nib;
    }
  }
  // Save and return result
  f = interim;
  return interim;
}

double ModbusMessage::swapDouble(double& f, int swapRule) {
  LOG_V("swap double, swapRule=%02X\n", swapRule);
  // Make a byte pointer to the given double
  uint8_t *src = (uint8_t *)&f;
  // Define a "work bench" double and byte pointer to it
  double interim;
  uint8_t *dst = (uint8_t *)&interim;
  // Loop over all bytes of a double
  for (uint8_t i = 0; i < sizeof(double); ++i) {
    // Get i-th byte from the spot the swap table tells
    LOG_V("dst[%d] = src[%d]\n", i, swapTables[swapRule & 0x07][i]);
    dst[i] = src[swapTables[swapRule & 0x07][i]];
    // Does the swar rule require nibble swaps?
    if (swapRule & 0x08) {
      // Yes, it does. 
      uint8_t nib = ((dst[i] & 0x0f) << 4) | ((dst[i] >> 4) & 0x0F);
      dst[i] = nib;
    }
  }
  // Save and return result
  f = interim;
  return interim;
}

// add() variant for a vector of uint8_t
uint16_t ModbusMessage::add(vector<uint8_t> v) {
  return add(v.data(), v.size());
}

// add() variants for float and double values
// values will be added in IEEE754 byte sequence (MSB first)
uint16_t ModbusMessage::add(float v, int swapRule) {
  // First check if we need to determine byte order
  LOG_V("add float, swapRule=%02X\n", swapRule);
  HEXDUMP_V("float", (uint8_t *)&v, sizeof(float));
  if (determineFloatOrder()) {
    // If we get here, the floatOrder is known
    float interim = 0;
    uint8_t *dst = (uint8_t *)&interim;
    uint8_t *src = (uint8_t *)&v;
    // Put out the bytes of v in normalized sequence
    for (uint8_t i = 0; i < sizeof(float); ++i) {
      dst[i] = src[floatOrder[i]];
    }
    HEXDUMP_V("normalized float", (uint8_t *)&interim, sizeof(float));
    // Do we need to apply a swap rule?
    if (swapRule & 0x0B) {
      // Yes, so do it.
      swapFloat(interim, swapRule & 0x0B);
    }
    HEXDUMP_V("swapped float", (uint8_t *)&interim, sizeof(float));
    // Put out the bytes of v in normalized (and swapped) sequence
    for (uint8_t i = 0; i < sizeof(float); ++i) {
      MM_data.push_back(dst[i]);
    }
  }

  return MM_data.size();
}

uint16_t ModbusMessage::add(double v, int swapRule) {
  // First check if we need to determine byte order
  LOG_V("add double, swapRule=%02X\n", swapRule);
  HEXDUMP_V("double", (uint8_t *)&v, sizeof(double));
  if (determineDoubleOrder()) {
    // If we get here, the doubleOrder is known
    double interim = 0;
    uint8_t *dst = (uint8_t *)&interim;
    uint8_t *src = (uint8_t *)&v;
    // Put out the bytes of v in normalized sequence
    for (uint8_t i = 0; i < sizeof(double); ++i) {
      dst[i] = src[doubleOrder[i]];
    }
    HEXDUMP_V("normalized double", (uint8_t *)&interim, sizeof(double));
    // Do we need to apply a swap rule?
    if (swapRule & 0x0F) {
      // Yes, so do it.
      swapDouble(interim, swapRule & 0x0F);
    }
    HEXDUMP_V("swapped double", (uint8_t *)&interim, sizeof(double));
    // Put out the bytes of v in normalized (and swapped) sequence
    for (uint8_t i = 0; i < sizeof(double); ++i) {
      MM_data.push_back(dst[i]);
    }
  }

  return MM_data.size();
}

// get() variants for float and double values
// values will be read in IEEE754 byte sequence (MSB first)
uint16_t ModbusMessage::get(uint16_t index, float& v, int swapRule) const {
  // First check if we need to determine byte order
  if (determineFloatOrder()) {
    // If we get here, the floatOrder is known
    // Will it fit?
    if (index <= MM_data.size() - sizeof(float)) {
      // Yes. Get the bytes of v in normalized sequence
      uint8_t *bytes = (uint8_t *)&v;
      for (uint8_t i = 0; i < sizeof(float); ++i) {
        bytes[i] = MM_data[index + floatOrder[i]];
      }
      HEXDUMP_V("got float", (uint8_t *)&v, sizeof(float));
      // Do we need to apply a swap rule?
      if (swapRule & 0x0B) {
        // Yes, so do it.
        swapFloat(v, swapRule & 0x0B);
      }
      HEXDUMP_V("got float swapped", (uint8_t *)&v, sizeof(float));
      index += sizeof(float);
    }
  }

  return index;
}

uint16_t ModbusMessage::get(uint16_t index, double& v, int swapRule) const {
  // First check if we need to determine byte order
  if (determineDoubleOrder()) {
    // If we get here, the doubleOrder is known
    // Will it fit?
    if (index <= MM_data.size() - sizeof(double)) {
      // Yes. Get the bytes of v in normalized sequence
      uint8_t *bytes = (uint8_t *)&v;
      for (uint8_t i = 0; i < sizeof(double); ++i) {
        bytes[i] = MM_data[index + doubleOrder[i]];
      }
      HEXDUMP_V("got double", (uint8_t *)&v, sizeof(double));
      // Do we need to apply a swap rule?
      if (swapRule & 0x0F) {
        // Yes, so do it.
        swapDouble(v, swapRule & 0x0F);
      }
      HEXDUMP_V("got double swapped", (uint8_t *)&v, sizeof(double));
      index += sizeof(double);
    }
  }

  return index;
}

// get() - read a byte array of a given size into a vector<uint8_t>. Returns updated index
uint16_t ModbusMessage::get(uint16_t index, vector<uint8_t>& v, uint8_t count) const {
  // Clean target vector
  v.clear();
  // Loop until required count is complete or the source is exhausted
  while (index < MM_data.size() && count--) {
    v.push_back(MM_data[index++]);
  }
  return index;
}

// Data validation methods for the different factory calls
// 0. serverID and function code - used by all of the below
Error ModbusMessage::checkServerFC(uint8_t serverID, uint8_t functionCode) {
  if (serverID == 0)      return INVALID_SERVER;   // Broadcast - not supported here
  if (serverID > 247)     return INVALID_SERVER;   // Reserved server addresses
  if (FCT::getType(functionCode) == FCILLEGAL)  return ILLEGAL_FUNCTION; // FC 0 does not exist
  return SUCCESS;
}

// 1. no additional parameter (FCs 0x07, 0x0b, 0x0c, 0x11)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode) {
  LOG_V("Check data #1\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC07_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    }
  }
  return returnCode;
}

// 2. one uint16_t parameter (FC 0x18)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t p1) {
  LOG_V("Check data #2\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC18_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    }
  }
  return returnCode;
}

// 3. two uint16_t parameters (FC 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2) {
  LOG_V("Check data #3\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC01_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    } else {
      switch (functionCode) {
      case 0x01:
      case 0x02:
        if ((p2 > 0x7d0) || (p2 == 0)) returnCode = PARAMETER_LIMIT_ERROR;
        break;
      case 0x03:
      case 0x04:
        if ((p2 > 0x7d) || (p2 == 0)) returnCode = PARAMETER_LIMIT_ERROR;
        break;
      case 0x05:
        if ((p2 != 0) && (p2 != 0xff00)) returnCode = PARAMETER_LIMIT_ERROR;
        break;
      }
    }
  }
  return returnCode;
}

// 4. three uint16_t parameters (FC 0x16)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint16_t p3) {
  LOG_V("Check data #4\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC16_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    } 
  }
  return returnCode;
}

// 5. two uint16_t parameters, a uint8_t length byte and a uint16_t* pointer to array of words (FC 0x10)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint16_t *arrayOfWords) {
  LOG_V("Check data #5\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC10_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    } else {
      if ((p2 == 0) || (p2 > 0x7b)) returnCode = PARAMETER_LIMIT_ERROR;
      else if (count != (p2 * 2)) returnCode = ILLEGAL_DATA_VALUE;
    }
  }
  return returnCode;
}

// 6. two uint16_t parameters, a uint8_t length byte and a uint16_t* pointer to array of bytes (FC 0x0f)
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint8_t *arrayOfBytes) {
  LOG_V("Check data #6\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FC0F_TYPE && ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    } else {
      if ((p2 == 0) || (p2 > 0x7b0)) returnCode = PARAMETER_LIMIT_ERROR;
      else if (count != ((p2 / 8 + (p2 % 8 ? 1 : 0)))) returnCode = ILLEGAL_DATA_VALUE;
    }
  }
  return returnCode;
}

// 7. generic constructor for preformatted data ==> count is counting bytes!
Error ModbusMessage::checkData(uint8_t serverID, uint8_t functionCode, uint16_t count, uint8_t *arrayOfBytes) {
  LOG_V("Check data #7\n");
  Error returnCode = checkServerFC(serverID, functionCode);
  if (returnCode == SUCCESS)
  {
    FCType ft = FCT::getType(functionCode);
    if (ft != FCUSER && ft != FCGENERIC) {
      returnCode = PARAMETER_COUNT_ERROR;
    } 
  }
  return returnCode;
}

// Factory methods to create valid Modbus messages from the parameters
// 1. no additional parameter (FCs 0x07, 0x0b, 0x0c, 0x11)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(2);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode);
  }
  return returnCode;
}

// 2. one uint16_t parameter (FC 0x18)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t p1) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, p1);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(4);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode, p1);
  }
  return returnCode;
}

// 3. two uint16_t parameters (FC 0x01, 0x02, 0x03, 0x04, 0x05, 0x06)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, p1, p2);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(6);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode, p1, p2);
  }
  return returnCode;
}

// 4. three uint16_t parameters (FC 0x16)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint16_t p3) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, p1, p2, p3);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(8);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode, p1, p2, p3);
  }
  return returnCode;
}

// 5. two uint16_t parameters, a uint8_t length byte and a uint16_t* pointer to array of words (FC 0x10)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint16_t *arrayOfWords) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, p1, p2, count, arrayOfWords);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(7 + count * 2);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode, p1, p2);
    add(count);
    for (uint8_t i = 0; i < (count >> 1); ++i) {
      add(arrayOfWords[i]);
    }
  }
  return returnCode;
}

// 6. two uint16_t parameters, a uint8_t length byte and a uint8_t* pointer to array of bytes (FC 0x0f)
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t p1, uint16_t p2, uint8_t count, uint8_t *arrayOfBytes) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, p1, p2, count, arrayOfBytes);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(7 + count);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode, p1, p2);
    add(count);
    for (uint8_t i = 0; i < count; ++i) {
      add(arrayOfBytes[i]);
    }
  }
  return returnCode;
}

// 7. generic constructor for preformatted data ==> count is counting bytes!
Error ModbusMessage::setMessage(uint8_t serverID, uint8_t functionCode, uint16_t count, uint8_t *arrayOfBytes) {
  // Check parameter for validity
  Error returnCode = checkData(serverID, functionCode, count, arrayOfBytes);
  // No error? 
  if (returnCode == SUCCESS)
  {
    // Yes, all fine. Create new ModbusMessage
    MM_data.reserve(2 + count);
    MM_data.shrink_to_fit();
    MM_data.clear();
    add(serverID, functionCode);
    for (uint8_t i = 0; i < count; ++i) {
      add(arrayOfBytes[i]);
    }
  }
  return returnCode;
}

// 8. Error response generator
Error ModbusMessage::setError(uint8_t serverID, uint8_t functionCode, Error errorCode) {
  // No error checking for server ID or function code here, as both may be the cause for the message!? 
  MM_data.reserve(3);
  MM_data.shrink_to_fit();
  MM_data.clear();
  add(serverID, static_cast<uint8_t>((functionCode | 0x80) & 0xFF), static_cast<uint8_t>(errorCode));
  return SUCCESS;
}

// Error output in case a message constructor will fail
void ModbusMessage::printError(const char *file, int lineNo, Error e, uint8_t serverID, uint8_t functionCode) {
  LOG_E("(%s, line %d) Error in constructor: %02X - %s (%02X/%02X)\n", file_name(file), lineNo, e, (const char *)(ModbusError(e)), serverID, functionCode);
}

uint8_t ModbusMessage::floatOrder[] = { 0xFF };
uint8_t ModbusMessage::doubleOrder[] = { 0xFF };
