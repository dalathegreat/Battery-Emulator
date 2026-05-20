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

    auto uds_response = battery.get_uds_response();
    ret += "<div></div><script>(()=>{var uds = [";
    for (int i = 0; i < uds_response.first; i++) {
      snprintf(buf, sizeof(buf), "%u,", uds_response.second[i]);
      ret += buf;
    }
    ret +=
        "];\n"
        "var h='<table>';\n"
        "if(uds[0]==2){\n"
        "for(let i=2;i<uds.length;i+=4){\n"
        "  let a=uds[i],b=uds[i+1],s=uds[i+3],f=[];\n"
        "  [[8,'<b>CONFIRMED</b>'],[4,'Pending'],[32,'FailSinceClear'],[1,'<b>FAIL</b>'],   "
        "[16,'NotCompSinceClear'],[64,'NotCompThisCycle'],[128,'MIL'],[2,'FailThisCycle']]  "
        ".map(x=>{if(s&x[0])f.push(x[1])});\n"
        "  let d='PCBU'[a>>6]+((a>>4)&3).toString(16)+(a&15).toString(16)+(b>>4).toString(16)+(b&15).toString(16);\n"
        "  h+=`<tr><td><b>${d.toUpperCase()}</b></td><td>${f.join(', ')||'NoFlags'}</td></tr>`;\n"
        "}}\n"
        "document.currentScript.previousElementSibling.innerHTML = h+'</table>';\n"
        "})();</script>\n";

    return ret;
  }

 private:
  MgHsPHEVBattery& battery;
};
