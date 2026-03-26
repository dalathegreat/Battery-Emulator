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

#ifndef _ECCX08_H_
#define _ECCX08_H_

#include <Arduino.h>
#include <Wire.h>

class ECCX08Class
{
public:
  ECCX08Class(TwoWire& wire, uint8_t address);
  virtual ~ECCX08Class();

  int begin();
  int begin(uint8_t i2cAddress);
  void end();

  int serialNumber(byte sn[]);
  String serialNumber();

  long random(long max);
  long random(long min, long max);
  int random(byte data[], size_t length);

  int generatePrivateKey(int slot, byte publicKey[]);
  int generatePublicKey(int slot, byte publicKey[]);

  int ecdsaVerify(const byte message[], const byte signature[], const byte pubkey[]);
  int ecSign(int slot, const byte message[], byte signature[]);

  int beginSHA256();
  int updateSHA256(const byte data[]); // 64 bytes
  int endSHA256(byte result[]);
  int endSHA256(const byte data[], int length, byte result[]);

  int readSlot(int slot, byte data[], int length);
  int writeSlot(int slot, const byte data[], int length);

  int locked();
  int writeConfiguration(const byte data[]);
  int readConfiguration(byte data[]);
  int lock();

  int beginHMAC(uint16_t keySlot);
  int updateHMAC(const byte data[], int length);
  int endHMAC(byte result[]);
  int endHMAC(const byte data[], int length, byte result[]);

  int nonce(const byte data[]);

  int incrementCounter(int counterId, long& counter);
  long incrementCounter(int counterId);
  int readCounter(int counterId, long& counter);
  long readCounter(int counterId);

private:
  int wakeup();
  int sleep();
  int idle();

  long version();
  int challenge(const byte message[]);
  int verify(const byte signature[], const byte pubkey[]);
  int sign(int slot, byte signature[]);

  int read(int zone, int address, byte buffer[], int length);
  int write(int zone, int address, const byte buffer[], int length);
  int lock(int zone);

  int addressForSlotOffset(int slot, int offset);

  int sendCommand(uint8_t opcode, uint8_t param1, uint16_t param2, const byte data[] = NULL, size_t dataLength = 0);
  int receiveResponse(void* response, size_t length);
  uint16_t crc16(const byte data[], size_t length);

private:
  TwoWire* _wire;
  uint8_t _address;

  static const uint32_t _wakeupFrequency;
  static const uint32_t _normalFrequency;
};

extern ECCX08Class ECCX08;

#endif
