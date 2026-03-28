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

#ifndef _ECCX08_CSR_H_
#define _ECCX08_CSR_H_

#include <Arduino.h>

class ECCX08CSRClass {
public:
  ECCX08CSRClass();
  virtual ~ECCX08CSRClass();

  int begin(int slot, bool newPrivateKey = true);
  String end();

  void setCountryName(const char *countryName);
  void setCountryName(const String& countryName) { setCountryName(countryName.c_str()); }

  void setStateProvinceName(const char* stateProvinceName);
  void setStateProvinceName(const String& stateProvinceName) { setStateProvinceName(stateProvinceName.c_str()); }

  void setLocalityName(const char* localityName);
  void setLocalityName(const String& localityName) { setLocalityName(localityName.c_str()); }

  void setOrganizationName(const char* organizationName);
  void setOrganizationName(const String& organizationName) { setOrganizationName(organizationName.c_str()); }

  void setOrganizationalUnitName(const char* organizationalUnitName);
  void setOrganizationalUnitName(const String& organizationalUnitName) { setOrganizationalUnitName(organizationalUnitName.c_str()); }

  void setCommonName(const char* commonName);
  void setCommonName(const String& commonName) { setCommonName(commonName.c_str()); }

private:
  int _slot;

  String _countryName;
  String _stateProvinceName;
  String _localityName;
  String _organizationName;
  String _organizationalUnitName;
  String _commonName;

  byte _publicKey[64];
};

extern ECCX08CSRClass ECCX08CSR;

#endif
