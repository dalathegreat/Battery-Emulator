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

//#include "../ECCX08Config.h"
#include <src/lib/ArduinoECCX08/ECCX08Config.h>
//#include "../ECCX08.h"
#include <src/lib/ArduinoECCX08/ECCX08.h>
#include "ASN1Utils.h"
#include "PEMUtils.h"
#include "ECCX08JWS.h"
#include <src/lib/ArduinoECCX08/ECCX08.h>

static String base64urlEncode(const byte in[], unsigned int length)
{
  static const char* CODES = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_=";

  int b;
  String out;

  int reserveLength = 4 * ((length + 2) / 3);
  out.reserve(reserveLength);

  for (unsigned int i = 0; i < length; i += 3) {
    b = (in[i] & 0xFC) >> 2;
    out += CODES[b];

    b = (in[i] & 0x03) << 4;
    if (i + 1 < length) {
      b |= (in[i + 1] & 0xF0) >> 4;
      out += CODES[b];
      b = (in[i + 1] & 0x0F) << 2;
      if (i + 2 < length) {
         b |= (in[i + 2] & 0xC0) >> 6;
         out += CODES[b];
         b = in[i + 2] & 0x3F;
         out += CODES[b];
      } else {
        out += CODES[b];
      }
    } else {
      out += CODES[b];
    }
  }

  while (out.lastIndexOf('=') != -1) {
    out.remove(out.length() - 1);
  }

  return out;
}

ECCX08JWSClass::ECCX08JWSClass()
{
}

ECCX08JWSClass::~ECCX08JWSClass()
{
}

String ECCX08JWSClass::publicKey(int slot, bool newPrivateKey)
{
  if (slot < 0 || slot > 8) {
    return "";
  }

  byte publicKey[64];

  if (newPrivateKey) {
    if (!ECCX08.generatePrivateKey(slot, publicKey)) {
      return "";
    }
  } else {
    if (!ECCX08.generatePublicKey(slot, publicKey)) {
      return "";
    }
  }

  int length = ASN1Utils.publicKeyLength();
  byte out[length];

  ASN1Utils.appendPublicKey(publicKey, out);

  return PEMUtils.base64Encode(out, length, "-----BEGIN PUBLIC KEY-----\n", "\n-----END PUBLIC KEY-----\n");
}

String ECCX08JWSClass::sign(int slot, const char* header, const char* payload)
{
  if (slot < 0 || slot > 8) {
    return "";
  }

  String encodedHeader = base64urlEncode((const byte*)header, strlen(header));
  String encodedPayload = base64urlEncode((const byte*)payload, strlen(payload));

  String toSign;
  toSign.reserve(encodedHeader.length() + 1 + encodedPayload.length());

  toSign += encodedHeader;
  toSign += '.';
  toSign += encodedPayload;


  byte toSignSha256[32];
  byte signature[64];

  if (!ECCX08.beginSHA256()) {
    return "";
  }

  for (unsigned int i = 0; i < toSign.length(); i += 64) {
    int chunkLength = toSign.length() - i;

    if (chunkLength > 64) {
      chunkLength = 64;
    }

    if (chunkLength == 64) {
      if (!ECCX08.updateSHA256((const byte*)toSign.c_str() + i)) {
        return "";
      }
    } else {
      if (!ECCX08.endSHA256((const byte*)toSign.c_str() + i, chunkLength, toSignSha256)) {
        return "";
      }
    }
  }

  if (!ECCX08.ecSign(slot, toSignSha256, signature)) {
    return "";
  }

  String encodedSignature = base64urlEncode(signature, sizeof(signature));

  String result;
  result.reserve(toSign.length() + 1 + encodedSignature.length());

  result += toSign;
  result += '.';
  result += encodedSignature;

  return result;
}

String ECCX08JWSClass::sign(int slot, const String& header, const String& payload)
{
  return sign(slot, header.c_str(), payload.c_str());
}

#if !defined(ECCX08_DISABLE_JWS)
ECCX08JWSClass ECCX08JWS;
#endif
