/*
  This file is part of the ArduinoECCX08 library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

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

#ifndef _ASN1_UTILS_H_
#define _ASN1_UTILS_H_

#include <Arduino.h>

#define ASN1_INTEGER           0x02
#define ASN1_BIT_STRING        0x03
#define ASN1_NULL              0x05
#define ASN1_OBJECT_IDENTIFIER 0x06
#define ASN1_PRINTABLE_STRING  0x13
#define ASN1_SEQUENCE          0x30
#define ASN1_SET               0x31

class ASN1UtilsClass {
public:
  int versionLength();

  int issuerOrSubjectLength(const String& countryName,
                    const String& stateProvinceName,
                    const String& localityName,
                    const String& organizationName,
                    const String& organizationalUnitName,
                    const String& commonName);

  int publicKeyLength();

  int signatureLength(const byte signature[]);

  int serialNumberLength(const byte serialNumber[], int length);

  int sequenceHeaderLength(int length);

  void appendVersion(int version, byte out[]);

  void appendIssuerOrSubject(const String& countryName,
                             const String& stateProvinceName,
                             const String& localityName,
                             const String& organizationName,
                             const String& organizationalUnitName,
                             const String& commonName,
                             byte out[]);

   int appendPublicKey(const byte publicKey[], byte out[]);

   int appendSignature(const byte signature[], byte out[]);

   int appendSerialNumber(const byte serialNumber[], int length, byte out[]);

   int appendName(const String& name, int type, byte out[]);

   int appendSequenceHeader(int length, byte out[]);

   int appendDate(int year, int month, int day, int hour, int minute, int second, byte out[]);

   int appendEcdsaWithSHA256(byte out[]);
};

extern ASN1UtilsClass ASN1Utils;

#endif
