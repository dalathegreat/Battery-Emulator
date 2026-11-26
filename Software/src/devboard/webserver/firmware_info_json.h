#pragma once

#include <stddef.h>

// {"hardware":"...","firmware":"..."} for /GetFirmwareInfo.
//
// One snprintf of the two raw strings is not enough: a quote, backslash or the usual control
// characters has to be escaped the way ArduinoJson's serializeJson does, or the bytes change.
// The result is written into the caller buffer. The return value is the length without the
// trailing NUL, and it is >= buflen when the buffer was too small (snprintf's rule).
inline int format_firmware_info_json(char* buf, size_t buflen, const char* hardware, const char* firmware) {
  size_t used = 0;

  auto put = [&](char c) {
    if (buf != nullptr && used + 1 < buflen) {
      buf[used] = c;
    }
    ++used;
  };
  auto put_raw = [&](const char* text) {
    while (*text != '\0') {
      put(*text++);
    }
  };
  auto put_string = [&](const char* value) {
    put('"');
    if (value != nullptr) {
      for (const auto* cursor = reinterpret_cast<const unsigned char*>(value); *cursor != 0; ++cursor) {
        char escaped = 0;
        switch (*cursor) {
          case '"':
            escaped = '"';
            break;
          case '\\':
            escaped = '\\';
            break;
          case '\b':
            escaped = 'b';
            break;
          case '\f':
            escaped = 'f';
            break;
          case '\n':
            escaped = 'n';
            break;
          case '\r':
            escaped = 'r';
            break;
          case '\t':
            escaped = 't';
            break;
          default:
            break;
        }
        if (escaped != 0) {
          put('\\');
          put(escaped);
        } else {
          put(static_cast<char>(*cursor));
        }
      }
    }
    put('"');
  };

  put_raw("{\"hardware\":");
  put_string(hardware);
  put_raw(",\"firmware\":");
  put_string(firmware);
  put('}');
  if (buf != nullptr && buflen > 0) {
    buf[used < buflen ? used : buflen - 1] = '\0';
  }
  return static_cast<int>(used);
}
