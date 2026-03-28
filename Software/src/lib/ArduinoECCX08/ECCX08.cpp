/*
  This file is part of the ArduinoECCX08 library.
  Copyright (c) 2018 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>

#include "ECCX08.h"

const uint32_t ECCX08Class::_wakeupFrequency = 100000u;  // 100 kHz
#ifdef __AVR__
const uint32_t ECCX08Class::_normalFrequency = 400000u;  // 400 kHz
#elif defined(ARDUINO_ARCH_ZEPHYR) && defined(ARDUINO_PORTENTA_H7_M7)
// FIXME speed above 400kHz require manual configuration in stm32 running on zephyr
const uint32_t ECCX08Class::_normalFrequency = 400000u;
#else
const uint32_t ECCX08Class::_normalFrequency = 1000000u; // 1 MHz
#endif

ECCX08Class::ECCX08Class(TwoWire& wire, uint8_t address) :
  _wire(&wire),
  _address(address)
{
}

ECCX08Class::~ECCX08Class()
{
}

int ECCX08Class::begin(uint8_t i2cAddress)
{
  _address = i2cAddress;
  return begin();
}

int ECCX08Class::begin()
{
  _wire->begin();

  wakeup();
  idle();
  
  long ver = version() & 0x0F00000;

  if (ver != 0x0500000 && ver != 0x0600000) {
    return 0;
  }

  return 1;
}

void ECCX08Class::end()
{
  // First wake up the device otherwise the chip didn't react to a sleep command
  wakeup();
  sleep();
#ifdef WIRE_HAS_END
  _wire->end();
#endif
}

int ECCX08Class::serialNumber(byte sn[])
{
  if (!read(0, 0, &sn[0], 4)) {
    return 0;
  }

  if (!read(0, 2, &sn[4], 4)) {
    return 0;
  }

  if (!read(0, 3, &sn[8], 4)) {
    return 0;
  }

  return 1;
}

String ECCX08Class::serialNumber()
{
  String result = (char*)NULL;
  byte sn[12];

  if (!serialNumber(sn)) {
    return result;
  }

  result.reserve(18);

  for (int i = 0; i < 9; i++) {
    byte b = sn[i];

    if (b < 16) {
      result += "0";
    }
    result += String(b, HEX);
  }

  result.toUpperCase();

  return result;
}

long ECCX08Class::random(long max)
{
  return random(0, max);
}

long ECCX08Class::random(long min, long max)
{
  if (min >= max)
  {
    return min;
  }

  long diff = max - min;

  long r;
  random((byte*)&r, sizeof(r));

  if (r < 0) {
    r = -r;
  }

  r = (r % diff);

  return (r + min);
}

int ECCX08Class::random(byte data[], size_t length)
{
  if (!wakeup()) {
    return 0;
  }

  while (length) {
    if (!sendCommand(0x1b, 0x00, 0x0000)) {
      return 0;
    }

    delay(23);

    byte response[32];

    if (!receiveResponse(response, sizeof(response))) {
      return 0;
    }

    int copyLength = min(32, (int)length);
    memcpy(data, response, copyLength);

    length -= copyLength;
    data += copyLength;
  }

  delay(1);

  idle();

  return 1;
}

int ECCX08Class::generatePrivateKey(int slot, byte publicKey[])
{
  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x40, 0x04, slot)) {
    return 0;
  }

  delay(115);

  if (!receiveResponse(publicKey, 64)) {
    return 0;
  }

  delay(1);

  idle();

  return 1;
}

int ECCX08Class::generatePublicKey(int slot, byte publicKey[])
{
  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x40, 0x00, slot)) {
    return 0;
  }

  delay(115);

  if (!receiveResponse(publicKey, 64)) {
    return 0;
  }

  delay(1);

  idle();

  return 1;
}

int ECCX08Class::ecdsaVerify(const byte message[], const byte signature[], const byte pubkey[])
{
  if (!challenge(message)) {
    return 0;
  }

  if (!verify(signature, pubkey)) {
    return 0;
  }

  return 1;
}

int ECCX08Class::ecSign(int slot, const byte message[], byte signature[])
{
  byte rand[32];

  if (!random(rand, sizeof(rand))) {
    return 0;
  }

  if (!challenge(message)) {
    return 0;
  }

  if (!sign(slot, signature)) {
    return 0;
  }

  return 1;
}

int ECCX08Class::beginSHA256()
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x47, 0x00, 0x0000)) {
    return 0;
  }

  delay(9);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::updateSHA256(const byte data[])
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x47, 0x01, 64, data, 64)) {
    return 0;
  }

  delay(9);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::endSHA256(byte result[])
{
  return endSHA256(NULL, 0, result);
}

int ECCX08Class::endSHA256(const byte data[], int length, byte result[])
{
  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x47, 0x02, length, data, length)) {
    return 0;
  }

  delay(9);

  if (!receiveResponse(result, 32)) {
    return 0;
  }

  delay(1);
  idle();

  return 1;
}

int ECCX08Class::readSlot(int slot, byte data[], int length)
{
  if (slot < 0 || slot > 15) {
    return -1;
  }

  if (length % 4 != 0) {
    return 0;
  }

  int chunkSize = 32;

  for (int i = 0; i < length; i += chunkSize) {
    if ((length - i) < 32) {
      chunkSize = 4;
    }

    if (!read(2, addressForSlotOffset(slot, i), &data[i], chunkSize)) {
      return 0;
    }
  }

  return 1;
}

int ECCX08Class::writeSlot(int slot, const byte data[], int length)
{
  if (slot < 0 || slot > 15) {
    return -1;
  }

  if (length % 4 != 0) {
    return 0;
  }

  int chunkSize = 32;

  for (int i = 0; i < length; i += chunkSize) {
    if ((length - i) < 32) {
      chunkSize = 4;
    }

    if (!write(2, addressForSlotOffset(slot, i), &data[i], chunkSize)) {
      return 0;
    }
  }

  return 1;
}

int ECCX08Class::locked()
{
  byte config[4];

  if (!read(0, 0x15, config, sizeof(config))) {
    return 0;
  }

  if (config[2] == 0x00 && config[3] == 0x00) {
    return 1; // locked
  }

  return 0;
}

int ECCX08Class::writeConfiguration(const byte data[])
{
  // skip first 16 bytes, they are not writable
  for (int i = 16; i < 128; i += 4) {
    if (i == 84) {
      // not writable
      continue;
    }

    if (!write(0, i / 4, &data[i], 4)) {
      return 0;
    }
  }

  return 1;
}

int ECCX08Class::readConfiguration(byte data[])
{
  for (int i = 0; i < 128; i += 32) {
    if (!read(0, i / 4, &data[i], 32)) {
      return 0;
    }
  }

  return 1;
}

int ECCX08Class::lock()
{
  // lock config
  if (!lock(0)) {
    return 0;
  }

  // lock data and OTP
  if (!lock(1)) {
    return 0;
  }

  return 1;
}


int ECCX08Class::beginHMAC(uint16_t keySlot)
{
  // HMAC implementation is only for ATECC608
  uint8_t status;
  long ecc608ver = 0x0600000;
  long eccCurrVer = version() & 0x0F00000;
  
  if (eccCurrVer != ecc608ver) {
    return 0;
  }

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x47, 0x04, keySlot)) {
    return 0;
  }

  delay(9);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::updateHMAC(const byte data[], int length) {
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  // Processing message
  int currLength = 0;
  while (length) {
    data += currLength;

    if (length > 64) {
      currLength = 64;
    } else {
      currLength = length;
    }
    length -= currLength;
    
    if (!sendCommand(0x47, 0x01, currLength, data, currLength)) {
      return 0;
    }

    delay(9);

    if (!receiveResponse(&status, sizeof(status))) {
      return 0;
    }

    delay(1);
  }
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::endHMAC(byte result[])
{
  return endHMAC(NULL, 0, result);
}

int ECCX08Class::endHMAC(const byte data[], int length, byte result[])
{
  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x47, 0x02, length, data, length)) {
    return 0;
  }

  delay(9);

  if (!receiveResponse(result, 32)) {
    return 0;
  }

  delay(1);
  idle();

  return 1;
}

int ECCX08Class::nonce(const byte data[])
{
  return challenge(data);
}

int ECCX08Class::incrementCounter(int counterId, long& counter)
{
  if (counterId < 0 || counterId > 1) {
    return 0;
  }

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x24, 1, counterId)) {
    return 0;
  }

  delay(20);

  if (!receiveResponse(&counter, sizeof(counter))) {
    return 0;
  }

  delay(1);
  idle();

  return 1;
}

long ECCX08Class::incrementCounter(int counterId)
{
  long counter;  // the counter can go up to 2,097,151

  if(!incrementCounter(counterId, counter)) {
    return -1;
  }

  return counter;
}

int ECCX08Class::readCounter(int counterId, long& counter)
{
  if (counterId < 0 || counterId > 1) {
    return 0;
  }

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x24, 0, counterId)) {
    return 0;
  }

  delay(20);

  if (!receiveResponse(&counter, sizeof(counter))) {
    return 0;
  }

  delay(1);
  idle();

  return 1;
}

long ECCX08Class::readCounter(int counterId)
{
  long counter;  // the counter can go up to 2,097,151

  if(!readCounter(counterId, counter)) {
    return -1;
  }

  return counter;
}

int ECCX08Class::wakeup()
{
  _wire->setClock(_wakeupFrequency);
  _wire->beginTransmission(0x00);
  _wire->endTransmission();

  delayMicroseconds(1500);

  byte response;

  if (!receiveResponse(&response, sizeof(response)) || response != 0x11) {
    return 0;
  }

  _wire->setClock(_normalFrequency);

  return 1;
}

int ECCX08Class::sleep()
{
  _wire->beginTransmission(_address);
  _wire->write(0x01);

  if (_wire->endTransmission() != 0) {
    return 0;
  }

  delay(1);

  return 1;
}

int ECCX08Class::idle()
{
  _wire->beginTransmission(_address);
  _wire->write(0x02);

  if (_wire->endTransmission() != 0) {
    return 0;
  }

  delay(1);

  return 1;
}

long ECCX08Class::version()
{
  uint32_t version = 0;

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x30, 0x00, 0x0000)) {
    return 0;
  }

  delay(2);

  if (!receiveResponse(&version, sizeof(version))) {
    return 0;
  }

  delay(1);
  idle();

  return version;
}

int ECCX08Class::challenge(const byte message[])
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  // Nonce, pass through
  if (!sendCommand(0x16, 0x03, 0x0000, message, 32)) {
    return 0;
  }

  delay(29);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::verify(const byte signature[], const byte pubkey[])
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  byte data[128];
  memcpy(&data[0], signature, 64);
  memcpy(&data[64], pubkey, 64);

  // Verify, external, P256
  if (!sendCommand(0x45, 0x02, 0x0004, data, sizeof(data))) {
    return 0;
  }

  delay(72);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::sign(int slot, byte signature[])
{
  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x41, 0x80, slot)) {
    return 0;
  }

  delay(70);

  if (!receiveResponse(signature, 64)) {
    return 0;
  }

  delay(1);
  idle();

  return 1;
}

int ECCX08Class::read(int zone, int address, byte buffer[], int length)
{
  if (!wakeup()) {
    return 0;
  }

  if (length != 4 && length != 32) {
    return 0;
  }

  if (length == 32) {
    zone |= 0x80;
  }

  if (!sendCommand(0x02, zone, address)) {
    return 0;
  }

  delay(5);

  if (!receiveResponse(buffer, length)) {
    return 0;
  }

  delay(1);
  idle();

  return length;
}

int ECCX08Class::write(int zone, int address, const byte buffer[], int length)
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  if (length != 4 && length != 32) {
    return 0;
  }

  if (length == 32) {
    zone |= 0x80;
  }

  if (!sendCommand(0x12, zone, address, buffer, length)) {
    return 0;
  }

  delay(26);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::lock(int zone)
{
  uint8_t status;

  if (!wakeup()) {
    return 0;
  }

  if (!sendCommand(0x17, 0x80 | zone, 0x0000)) {
    return 0;
  }

  delay(32);

  if (!receiveResponse(&status, sizeof(status))) {
    return 0;
  }

  delay(1);
  idle();

  if (status != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::addressForSlotOffset(int slot, int offset)
{
  int block = offset / 32;
  offset = (offset % 32) / 4;  

  return (slot << 3) | (block << 8) | (offset);
}

int ECCX08Class::sendCommand(uint8_t opcode, uint8_t param1, uint16_t param2, const byte data[], size_t dataLength)
{
  int commandLength = 8 + dataLength; // 1 for type, 1 for length, 1 for opcode, 1 for param1, 2 for param2, 2 for CRC
  byte command[commandLength]; 
  
  command[0] = 0x03;
  command[1] = sizeof(command) - 1;
  command[2] = opcode;
  command[3] = param1;
  memcpy(&command[4], &param2, sizeof(param2));
  memcpy(&command[6], data, dataLength);

  uint16_t crc = crc16(&command[1], 8 - 3 + dataLength);
  memcpy(&command[6 + dataLength], &crc, sizeof(crc));

  _wire->beginTransmission(_address);
  _wire->write(command, commandLength);
  if (_wire->endTransmission() != 0) {
    return 0;
  }

  return 1;
}

int ECCX08Class::receiveResponse(void* response, size_t length)
{
  int retries = 20;
  size_t responseSize = length + 3; // 1 for length header, 2 for CRC
  byte responseBuffer[responseSize];

  while (_wire->requestFrom((uint8_t)_address, (size_t)responseSize, (bool)true) != responseSize && retries--);

  responseBuffer[0] = _wire->read();

  // make sure length matches
  if (responseBuffer[0] != responseSize) {
    // Clear the buffer
    for (size_t i = 1; _wire->available(); i++) {
      (void) _wire->read();
    }
    delay(1);
    idle();
    return 0;
  }

  for (size_t i = 1; _wire->available(); i++) {
    responseBuffer[i] = _wire->read();
  }

  // verify CRC
  uint16_t responseCrc = responseBuffer[length + 1] | (responseBuffer[length + 2] << 8);
  if (responseCrc != crc16(responseBuffer, responseSize - 2)) {
    delay(1);
    idle();
    return 0;
  }

  memcpy(response, &responseBuffer[1], length);

  return 1;
}

uint16_t ECCX08Class::crc16(const byte data[], size_t length)
{
  if (data == NULL || length == 0) {
    return 0;
  }

  uint16_t crc = 0;

  while (length) {
    byte b = *data;

    for (uint8_t shift = 0x01; shift > 0x00; shift <<= 1) {
      uint8_t dataBit = (b & shift) ? 1 : 0;
      uint8_t crcBit = crc >> 15;

      crc <<= 1;
      
      if (dataBit != crcBit) {
        crc ^= 0x8005;
      }
    }

    length--;
    data++;
  }

  return crc;
}

#if __ZEPHYR__
  #ifndef CRYPTO_WIRE
    #define CRYPTO_WIRE Wire1
  #endif
#endif

#ifdef CRYPTO_WIRE
ECCX08Class ECCX08(CRYPTO_WIRE, 0x60);
#else
ECCX08Class ECCX08(Wire, 0x60);
#endif
