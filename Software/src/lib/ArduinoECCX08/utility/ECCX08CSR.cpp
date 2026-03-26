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

#include "../ECCX08Config.h"
#include "../ArduinoECCX08.h"

#include "ASN1Utils.h"
#include "PEMUtils.h"
#include "../ECCX08.h"
#include "ECCX08CSR.h"

ECCX08CSRClass::ECCX08CSRClass()
{
}

ECCX08CSRClass::~ECCX08CSRClass()
{
}

int ECCX08CSRClass::begin(int slot, bool newPrivateKey)
{
  _slot = slot;

  if (newPrivateKey) {
    if (!ECCX08.generatePrivateKey(slot, _publicKey)) {
      return 0;
    }
  } else {
    if (!ECCX08.generatePublicKey(slot, _publicKey)) {
      return 0;
    }
  }

  return 1;
}

String ECCX08CSRClass::end()
{
  int versionLen = ASN1Utils.versionLength();
  int subjectLen = ASN1Utils.issuerOrSubjectLength(_countryName,
                                                    _stateProvinceName,
                                                    _localityName,
                                                    _organizationName,
                                                    _organizationalUnitName,
                                                    _commonName);
  int subjectHeaderLen = ASN1Utils.sequenceHeaderLength(subjectLen);
  int publicKeyLen = ASN1Utils.publicKeyLength();

  int csrInfoLen = versionLen + subjectHeaderLen + subjectLen + publicKeyLen + 2;
  int csrInfoHeaderLen = ASN1Utils.sequenceHeaderLength(csrInfoLen);

  byte csrInfo[csrInfoHeaderLen + csrInfoLen];
  byte* out = csrInfo;

  ASN1Utils.appendSequenceHeader(csrInfoLen, out);
  out += csrInfoHeaderLen;

  // version
  ASN1Utils.appendVersion(0x00, out);
  out += versionLen;

  // subject
  ASN1Utils.appendSequenceHeader(subjectLen, out);
  out += subjectHeaderLen;
  ASN1Utils.appendIssuerOrSubject(_countryName,
                                  _stateProvinceName,
                                  _localityName,
                                  _organizationName,
                                  _organizationalUnitName,
                                  _commonName, out);
  out += subjectLen;

  // public key
  ASN1Utils.appendPublicKey(_publicKey, out);
  out += publicKeyLen;

  // terminator
  *out++ = 0xa0;
  *out++ = 0x00;

  byte csrInfoSha256[64];
  byte signature[64];

  if (!ECCX08.beginSHA256()) {
    return "";
  }

  for (int i = 0; i < (csrInfoHeaderLen + csrInfoLen); i += 64) {
    int chunkLength = (csrInfoHeaderLen + csrInfoLen) - i;

    if (chunkLength > 64) {
      chunkLength = 64;
    }

    if (chunkLength == 64) {
      if (!ECCX08.updateSHA256(&csrInfo[i])) {
        return "";
      }
    } else {
      if (!ECCX08.endSHA256(&csrInfo[i], chunkLength, csrInfoSha256)) {
        return "";
      }
    }
  }

  if (!ECCX08.ecSign(_slot, csrInfoSha256, signature)) {
    return "";
  }

  int signatureLen = ASN1Utils.signatureLength(signature);
  int csrLen = csrInfoHeaderLen + csrInfoLen + signatureLen;
  int csrHeaderLen = ASN1Utils.sequenceHeaderLength(csrLen);

  byte csr[csrLen + csrHeaderLen];
  out = csr;

  ASN1Utils.appendSequenceHeader(csrLen, out);
  out += csrHeaderLen;

  // info
  memcpy(out, csrInfo, csrInfoHeaderLen + csrInfoLen);
  out += (csrInfoHeaderLen + csrInfoLen);

  // signature
  ASN1Utils.appendSignature(signature, out);
  out += signatureLen;

  return PEMUtils.base64Encode(csr, csrLen + csrHeaderLen, "-----BEGIN CERTIFICATE REQUEST-----\n", "\n-----END CERTIFICATE REQUEST-----\n");
}

void ECCX08CSRClass::setCountryName(const char *countryName)
{
  _countryName = countryName;
}

void ECCX08CSRClass::setStateProvinceName(const char* stateProvinceName)
{
  _stateProvinceName = stateProvinceName;
}

void ECCX08CSRClass::setLocalityName(const char* localityName)
{
  _localityName = localityName;
}

void ECCX08CSRClass::setOrganizationName(const char* organizationName)
{
  _organizationName = organizationName;
}

void ECCX08CSRClass::setOrganizationalUnitName(const char* organizationalUnitName)
{
  _organizationalUnitName = organizationalUnitName;
}

void ECCX08CSRClass::setCommonName(const char* commonName)
{
  _commonName = commonName;
}

#if !defined(ECCX08_DISABLE_CSR)
ECCX08CSRClass ECCX08CSR;
#endif
