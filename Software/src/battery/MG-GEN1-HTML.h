#pragma once

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

static void print_chars_or_hex(char* buf, const uint8_t* data, uint16_t length) {
  int ptr = 0;
  for (int i = 0; i < length && ptr < 62; i++) {
    if (data[i] >= 32 && data[i] <= 126) {
      buf[ptr++] = (char)data[i];
    } else {
      int written = sprintf(buf + ptr, "[%02x]", data[i]);
      ptr += written;
    }
  }
  buf[ptr] = '\0';
}

class MgGen1HtmlRenderer : public BatteryHtmlRenderer {
 public:
  MgGen1HtmlRenderer(MgHsPHEVBattery& battery) : battery(battery) {}
  String get_status_html() {
    String ret = String();
    ret.reserve(512);  //Pre-allocate some memory to avoid fragmentation

    char buf[128];

    ret += "UDS address: ";
    ret += String(battery.get_uds_address(), 16);
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f190(), 17);
    ret += "VIN: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f18a(), 8);
    ret += "F18A: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f120(), 16);
    ret += "F120: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_b18c(), 24);
    ret += "B18C: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f183(), 10);
    ret += "F183: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f18b(), 3);
    ret += "F18B: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f191(), 5);
    ret += "F191: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f192(), 10);
    ret += "F192: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f194(), 10);
    ret += "F194: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f1a2(), 8);
    ret += "F1A2: ";
    ret += buf;
    ret += "<br>";
    print_chars_or_hex(buf, battery.get_pid_f1aa(), 5);
    ret += "F1AA: ";
    ret += buf;
    ret += "<br>";

    if (battery.dtc != nullptr)
      ret += BatteryHtmlRenderer::render_dtc_section_html(*battery.dtc, "mg_dtc.json", true);

    return ret;
  }

 private:
  MgHsPHEVBattery& battery;
};
