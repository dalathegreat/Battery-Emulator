// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright 2016-2025 Hristo Gochkov, Mathieu Carbou, Emil Muratov

#include <ESPAsyncWebServer.h>

AsyncWebHeader::AsyncWebHeader(const String &data) {
  if (!data) {
    return;
  }
  int index = data.indexOf(':');
  if (index < 0) {
    return;
  }
  _name = data.substring(0, index);
  _value = data.substring(index + 2);
}

String AsyncWebHeader::toString() const {
  String str;
  if (str.reserve(_name.length() + _value.length() + 2)) {
    str.concat(_name);
    str.concat((char)0x3a);
    str.concat((char)0x20);
    str.concat(_value);
    str.concat(asyncsrv::T_rn);
  } else {
#ifdef ESP32
    log_e("Failed to allocate");
#endif
  }
  return str;
}
