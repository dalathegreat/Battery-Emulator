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

#ifndef _ECCX08_DEFAULT_TLS_CONFIG_H_
#define _ECCX08_DEFAULT_TLS_CONFIG_H_

const byte ECCX08_DEFAULT_TLS_CONFIG[128] = {
// Read only - start
  // SN[0:3]
  0x01, 0x23, 0x00, 0x00,
  // RevNum
  0x00, 0x00, 0x50, 0x00,
  // SN[4:8]
  0x00, 0x00, 0x00, 0x00, 0x00,
  // Reserved
  0xC0,
  // I2C_Enable
  0x71,
  // Reserved                  
  0x00,
// Read only - end
  // I2C_Address
  0xC0,
  // Reserved
  0x00,
  // OTPmode
  0x55,
  // ChipMode
  0x00,
  // SlotConfig
  0x83, 0x20, // External Signatures | Internal Signatures | IsSecret | Write Configure Never, Default: 0x83, 0x20, 
  0x87, 0x20, // External Signatures | Internal Signatures | ECDH | IsSecret | Write Configure Never, Default: 0x87, 0x20,
  0x87, 0x20, // External Signatures | Internal Signatures | ECDH | IsSecret | Write Configure Never, Default: 0x8F, 0x20,
  0x87, 0x2F, // External Signatures | Internal Signatures | ECDH | IsSecret | WriteKey all slots | Write Configure Never, Default: 0xC4, 0x8F,
  0x87, 0x2F, // External Signatures | Internal Signatures | ECDH | IsSecret | WriteKey all slots | Write Configure Never, Default: 0x8F, 0x8F, 
  0x8F, 0x8F,
  0x9F, 0x8F, 
  0xAF, 0x8F,
  0x00, 0x00, 
  0x00, 0x00,
  0x00, 0x00, 
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00,
  0x00, 0x00, 
  0xAF, 0x8F,
  // Counter[0]
  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
  // Counter[1]
  0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
  // LastKeyUse
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF,
// Write via commands only - start
  // UserExtra
  0x00, 
  // Selector
  0x00,
  // LockValue
  0x55,
  // LockConfig
  0x55,
  // SlotLocked
  0xFF, 0xFF,
// Write via commands only - end
  // RFU
  0x00, 0x00,
  // X509format
  0x00, 0x00, 0x00, 0x00,
  // KeyConfig
  0x33, 0x00, // Private | Public | P256 NIST ECC key, Default: 0x33, 0x00,
  0x33, 0x00, // Private | Public | P256 NIST ECC key, Default: 0x33, 0x00,
  0x33, 0x00, // Private | Public | P256 NIST ECC key, Default: 0x33, 0x00,
  0x33, 0x00, // Private | Public | P256 NIST ECC key, Default: 0x1C, 0x00,
  0x33, 0x00, // Private | Public | P256 NIST ECC key, Default: 0x1C, 0x00,
  0x1C, 0x00,
  0x1C, 0x00,
  0x1C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x3C, 0x00,
  0x1C, 0x00
};

#endif
