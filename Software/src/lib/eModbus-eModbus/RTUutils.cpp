// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include "options.h"
#if HAS_FREERTOS
#include "ModbusMessage.h"
#include "RTUutils.h"
#undef LOCAL_LOG_LEVEL
// #define LOCAL_LOG_LEVEL LOG_LEVEL_VERBOSE
#include "Logging.h"

// calcCRC: calculate Modbus CRC16 on a given array of bytes
uint16_t RTUutils::calcCRC(const uint8_t *data, uint16_t len) {
  // CRC16 pre-calculated tables
  const uint8_t crcHiTable[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
    0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
    0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
    0x40
  };

  const uint8_t crcLoTable[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
    0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
    0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
    0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
    0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
    0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
    0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
    0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
    0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
    0x40
  };

  uint8_t crcHi = 0xFF;
  uint8_t crcLo = 0xFF;

  while (len--) {
    uint8_t index = crcLo ^ *data++;
    crcLo = crcHi ^ crcHiTable[index];
    crcHi = crcLoTable[index];
  }
  return (crcHi << 8 | crcLo);
}

// calcCRC: calculate Modbus CRC16 on a given message
uint16_t RTUutils::calcCRC(ModbusMessage msg) {
  return calcCRC(msg.data(), msg.size());
}


// validCRC #1: check the given CRC in a block of data for correctness
bool RTUutils::validCRC(const uint8_t *data, uint16_t len) {
  return validCRC(data, len - 2, data[len - 2] | (data[len - 1] << 8));
}

// validCRC #2: check the CRC of a block of data against a given one for equality
bool RTUutils::validCRC(const uint8_t *data, uint16_t len, uint16_t CRC) {
  uint16_t crc16 = calcCRC(data, len);
  if (CRC == crc16) return true;
  return false;
}

// validCRC #3: check the given CRC in a message for correctness
bool RTUutils::validCRC(ModbusMessage msg) {
  return validCRC(msg.data(), msg.size() - 2, msg[msg.size() - 2] | (msg[msg.size() - 1] << 8));
}

// validCRC #4: check the CRC of a message against a given one for equality
bool RTUutils::validCRC(ModbusMessage msg, uint16_t CRC) {
  return validCRC(msg.data(), msg.size(), CRC);
}

// addCRC: calculate the CRC for a given RTUMessage and add it to the end
void RTUutils::addCRC(ModbusMessage& raw) {
  uint16_t crc16 = calcCRC(raw.data(), raw.size());
  raw.push_back(crc16 & 0xff);
  raw.push_back((crc16 >> 8) & 0xFF);
}

// calculateInterval: determine the minimal gap time between messages
uint32_t RTUutils::calculateInterval(uint32_t baudRate) {
  uint32_t interval = 0;

  // silent interval is at least 3.5x character time
  interval = 35000000UL / baudRate;  // 3.5 * 10 bits * 1000 µs * 1000 ms / baud
  if (interval < 1750) interval = 1750;       // lower limit according to Modbus RTU standard
  LOG_V("Calc interval(%u)=%u\n", baudRate, interval);
  return interval;
}

// send: send a message via Serial, watching interval times - including CRC!
void RTUutils::send(Stream& serial, unsigned long& lastMicros, uint32_t interval, RTScallback rts, const uint8_t *data, uint16_t len, bool ASCIImode) {
  // Clear serial buffers
  while (serial.available()) serial.read();
  
  // Treat ASCII differently
  if (ASCIImode) {
    // Toggle rtsPin, if necessary
    rts(HIGH);
    // Yes, ASCII mode. Send lead-in
    serial.write(':');

    uint16_t cnt = len;
    uint8_t crc = 0;
    uint8_t *cp = (uint8_t *)data;

    // Loop over all bytes of the message
    while (cnt--) {
      // Write two nibbles as ASCII characters
      serial.write(ASCIIwrite[(*cp >> 4) & 0x0F]);
      serial.write(ASCIIwrite[*cp & 0x0F]);
      // Advance CRC
      crc += *cp;
      // Next byte
      cp++;
    }
    // Finalize CRC (2's complement)
    crc = ~crc;
    crc++;
    // Write ist - two nibbles as ASCII characters
    serial.write(ASCIIwrite[(crc >> 4) & 0x0F]);
    serial.write(ASCIIwrite[crc & 0x0F]);
    
    // Send lead-out
    serial.write("\r\n");
    serial.flush();
    // Toggle rtsPin, if necessary
    rts(LOW);
  } else {
    // RTU mode
    uint16_t crc16 = calcCRC(data, len);

    // Respect interval - we must not toggle rtsPin before
    if (micros() - lastMicros < interval) delayMicroseconds(interval - (micros() - lastMicros));

    // Toggle rtsPin, if necessary
    rts(HIGH);
    // Write message
    serial.write(data, len);
    // Write CRC in LSB order
    serial.write(crc16 & 0xff);
    serial.write((crc16 >> 8) & 0xFF);
    serial.flush();
    // Toggle rtsPin, if necessary
    rts(LOW);
  }

  HEXDUMP_D("Sent packet", data, len);

  // Mark end-of-message time for next interval
  lastMicros = micros();
}

// send: send a message via Serial, watching interval times - including CRC!
void RTUutils::send(Stream& serial, unsigned long& lastMicros, uint32_t interval, RTScallback rts, ModbusMessage raw, bool ASCIImode) {
  send(serial, lastMicros, interval, rts, raw.data(), raw.size(), ASCIImode);
}

// receive: get (any) message from Serial, taking care of timeout and interval
ModbusMessage RTUutils::receive(uint8_t caller, Stream& serial, uint32_t timeout, unsigned long& lastMicros, uint32_t interval, bool ASCIImode, bool skipLeadingZeroBytes) {
  // Allocate initial receive buffer size: 1 block of BUFBLOCKSIZE bytes
  const uint16_t BUFBLOCKSIZE(512);
  uint8_t *buffer = new uint8_t[BUFBLOCKSIZE];
  ModbusMessage rv;

  // Index into buffer
  uint16_t bufferPtr = 0;
  // Byte read
  int b = 0; 

  // State machine states, RTU mode
  enum STATES : uint8_t { WAIT_DATA = 0, IN_PACKET, DATA_READ, FINISHED };

  // State machine states, ASCII mode
  enum ASTATES : uint8_t { A_WAIT_DATA = 0, A_DATA, A_WAIT_LEAD_OUT, A_FINISHED };

  uint8_t state;

  // Timeout tracker
  unsigned long TimeOut = millis();

  // RTU mode?
  if (!ASCIImode) {
    // Yes.
    state = WAIT_DATA;
    // interval tracker 
    lastMicros = micros();
  
    while (state != FINISHED) {
      switch (state) {
      // WAIT_DATA: await first data byte, but watch timeout
      case WAIT_DATA:
        // Blindly try to read a byte
        b = serial.read();
        // Did we get one?
        if (b >= 0) {
          // Yes. Note the time.
          lastMicros = micros();
          // Do we need to skip it, if it is zero?
          if (b > 0 || !skipLeadingZeroBytes) {
            // No, we can go process it regularly
            buffer[bufferPtr++] = b;
            state = IN_PACKET;
          } 
        } else {
          // No, we had no byte. Just check the timeout period
          if (millis() - TimeOut >= timeout) {
            rv.push_back(TIMEOUT);
            state = FINISHED;
          }
          delay(1);
        }
        break;
      // IN_PACKET: read data until a gap of at least _interval time passed without another byte arriving
      case IN_PACKET:
        // tight loop until finished reading or error
        while (state == IN_PACKET) {
          // Is there a byte?
          while (serial.available()) {
            // Yes, collect it
            buffer[bufferPtr++] = serial.read();
            // Mark time of last byte
            lastMicros = micros();
            // Buffer full?
            if (bufferPtr >= BUFBLOCKSIZE) {
              // Yes. Something fishy here - bail out!
              rv.push_back(PACKET_LENGTH_ERROR);
              state = FINISHED;
              break;
            }
          } 
          // No more byte read
          if (state == IN_PACKET) {
            // Are we past the interval gap?
            if (micros() - lastMicros >= interval) {
              // Yes, terminate reading
              LOG_V("%c/%ldus without data after %u\n", (const char)caller, micros() - lastMicros, bufferPtr);
              state = DATA_READ;
              break;
            }
          }
        }
        break;
      // DATA_READ: successfully gathered some data. Prepare return object.
      case DATA_READ:
        // Did we get a sensible buffer length?
        LOG_V("%c/", (const char)caller);
        HEXDUMP_V("Raw buffer received", buffer, bufferPtr);
        if (bufferPtr >= 4)
        {
          // Yes. Check CRC
          if (!validCRC(buffer, bufferPtr)) {
            // Ooops. CRC is wrong.
            rv.push_back(CRC_ERROR);
          } else {
            // CRC was fine, Now allocate response object without the CRC
            for (uint16_t i = 0; i < bufferPtr - 2; ++i) {
              rv.push_back(buffer[i]);
            }
          }
        } else {
          // No, packet was too short for anything usable. Return error
          rv.push_back(PACKET_LENGTH_ERROR);
        }
        state = FINISHED;
        break;
      // FINISHED: we are done, clean up.
      case FINISHED:
        // CLear serial buffer in case something is left trailing
        // May happen with servers too slow!
        while (serial.available()) serial.read();
        break;
      }
    }
  } else {
    // We are in ASCII mode.
    state = A_WAIT_DATA;

    // Track nibbles in a byte
    bool byteComplete = true; 

    // Track bytes read
    bool hadBytes = false;

    // ASCII crc byte
    uint8_t crc = 0;

    while (state != A_FINISHED) {
      // Always watch timeout - 1s
      if (millis() - TimeOut >= timeout) {
        // Timeout! Bail out with error
        rv.push_back(TIMEOUT);
        state = A_FINISHED;
      } else {
        // Still in time. Check for another byte on serial
        if (!hadBytes && serial.available()) {
          b = serial.read();
          if (b >= 0) {
            hadBytes = true;
          }
        }
        // Only use state machine with new data arrived
        if (hadBytes) {
          // First reset timeout
          TimeOut = millis();
          // Is it a valid character?
          if ((b & 0x80) || ASCIIread[b] == 0xFF) {
            // No. Report error and leave.
            rv.clear();
            rv.push_back(ASCII_INVALID_CHAR);
            hadBytes = false;
            state = A_FINISHED;
          } else {
            // Yes, is valid. Furtheron use interpreted byte
            b = ASCIIread[b];
            switch (state) {
            // A_WAIT_DATA: await lead-in byte ':'
            case A_WAIT_DATA:
              // Is it the lead-in?
              if (b == 0xF0) {
                // Yes, proceed to data read state
                state = A_DATA;
              }
              // byte was consumed in any case
              hadBytes = false;
              break;
            // A_DATA: read data as it comes
            case A_DATA:
              // Lead-out byte 1 received?
              if (b == 0xF1) {
                // Yes. Was last buffer byte completed?
                if (byteComplete) {
                  // Yes. Move to final state
                  state = A_WAIT_LEAD_OUT;
                } else {
                  // No, signal with error
                  rv.push_back(PACKET_LENGTH_ERROR);
                  state = A_FINISHED;
                }
              } else {
                // No lead-out, must be data byte.
                // Is it valid?
                if (b < 0xF0) {
                  // Yes. Add it into current buffer byte
                  buffer[bufferPtr] <<= 4;
                  buffer[bufferPtr] += (b & 0x0F);
                  // Advance nibble
                  byteComplete = !byteComplete;
                  // Was it the second of the byte?
                  if (byteComplete) {
                    // Yes. Advance CRC and move buffer pointer by one
                    crc += buffer[bufferPtr];
                    bufferPtr++;
                    buffer[bufferPtr] = 0;
                  }
                } else {
                  // No, garbage. report error
                  rv.push_back(ASCII_INVALID_CHAR);
                  state = A_FINISHED;
                }
              }
              hadBytes = false;
              break;
            // A_WAIT_LEAD_OUT: await \n
            case A_WAIT_LEAD_OUT:
              if (b == 0xF2) {
                // Lead-out byte 2 received. Transfer buffer to returned message
                LOG_V("%c/", (const char)caller);
                HEXDUMP_V("Raw buffer received", buffer, bufferPtr);
                // Did we get a sensible buffer length?
                if (bufferPtr >= 3)
                {
                  // Yes. Was the CRC calculated correctly?
                  if (crc == 0) {
                    // Yes, reduce buffer by 1 to get rid of CRC byte...
                    bufferPtr--;
                    // Move data into returned message
                    for (uint16_t i = 0; i < bufferPtr; ++i) {
                      rv.push_back(buffer[i]);
                    }
                  } else {
                    // No, CRC calculation seems to have failed
                    rv.push_back(ASCII_CRC_ERR);
                  }
                } else {
                  // No, packet was too short for anything usable. Return error
                  rv.push_back(PACKET_LENGTH_ERROR);
                }
              } else {
                // No lead out byte 2, but something else - report error.
                rv.push_back(ASCII_FRAME_ERR);
              }
              state = A_FINISHED;
              break;
            // A_FINISHED: Message completed
            case A_FINISHED:
              // Clean up serial buffer
              while (serial.available()) serial.read();
              break;
            }
          }
        } else {
          // No data received, so give the task scheduler room to breathe
          delay(1);
        }
      }
    }
  }
  // Deallocate buffer
  delete[] buffer;

  LOG_D("%c/", (const char)caller);
  HEXDUMP_D("Received packet", rv.data(), rv.size());

  return rv;
}

// Lower 7 bit ASCII characters - all invalid are set to 0xFF
const char RTUutils::ASCIIread[] = { 
  /* 00-07 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 08-0F */ 0xFF, 0xFF, 0xF2, 0xFF, 0xFF, 0xF1, 0xFF, 0xFF,  // LF + CR
  /* 10-17 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 18-1F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 20-27 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 28-2F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 30-37 */    0,    1,    2,    3,    4,    5,    6,    7,  // digits 0-7
  /* 38-3F */    8,    9, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // digits 8 + 9, :
  /* 40-47 */ 0xFF,   10,   11,   12,   13,   14,   15, 0xFF,  // digits A-F
  /* 48-4F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 50-57 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 58-5F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 60-67 */ 0xFF,   10,   11,   12,   13,   14,   15, 0xFF,  // digits a-f
  /* 68-6F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 70-77 */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
  /* 78-7F */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF 
};

// Writable ASCII chars for hex digits
const char RTUutils::ASCIIwrite[] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
                                      0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 
};

#endif
