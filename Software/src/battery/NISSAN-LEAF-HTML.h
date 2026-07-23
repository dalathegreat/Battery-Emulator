#ifndef _NISSAN_LEAF_HTML_H
#define _NISSAN_LEAF_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class NissanLeafHtmlRenderer : public BatteryHtmlRenderer {
 public:
  NissanLeafHtmlRenderer(DATALAYER_INFO_NISSAN_LEAF* dl) : nissan_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    if (!nissan_dl) {
      return content;
    }

    content += "<h4>LEAF generation: ";
    switch (nissan_dl->LEAF_gen) {
      case 0:
        content += String("ZE0</h4>");
        break;
      case 1:
        content += String("AZE0</h4>");
        break;
      case 2:
        content += String("ZE1</h4>");
        break;
      default:
        content += String("Unknown</h4>");
    }
    char readableSerialNumber[16];  // One extra space for null terminator
    memcpy(readableSerialNumber, nissan_dl->BatterySerialNumber,
           sizeof(nissan_dl->BatterySerialNumber));
    readableSerialNumber[15] = '\0';  // Null terminate the string
    content += "<h4>Serial number: " + String(readableSerialNumber) + "</h4>";
    char readablePartNumber[8];  // One extra space for null terminator
    memcpy(readablePartNumber, nissan_dl->BatteryPartNumber,
           sizeof(nissan_dl->BatteryPartNumber));
    readablePartNumber[7] = '\0';  // Null terminate the string
    content += "<h4>Part number: " + String(readablePartNumber) + "</h4>";
    char readableBMSID[9];  // One extra space for null terminator
    memcpy(readableBMSID, nissan_dl->BMSIDcode, sizeof(nissan_dl->BMSIDcode));
    readableBMSID[8] = '\0';  // Null terminate the string
    content += "<h4>BMS ID: " + String(readableBMSID) + "</h4>";
    content += "<h4>GIDS: " + String(nissan_dl->GIDS) + "</h4>";
    content += "<h4>HX: " + String(nissan_dl->battery_HX) + "</h4>";
    content += "<h4>Regen kW: " + String(nissan_dl->ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(nissan_dl->MaxPowerForCharger) + "</h4>";
    content += "<h4>Interlock: " + String(nissan_dl->Interlock) + "</h4>";
    content += "<h4>Insulation: " + String(nissan_dl->Insulation) + "</h4>";
    content += "<h4>Relay cut request: " + String(nissan_dl->RelayCutRequest) + "</h4>";
    content += "<h4>Failsafe status: " + String(nissan_dl->FailsafeStatus) + "</h4>";
    content += "<h4>Fully charged: " + String(nissan_dl->Full) + "</h4>";
    content += "<h4>Battery empty: " + String(nissan_dl->Empty) + "</h4>";
    content += "<h4>Main relay ON: " + String(nissan_dl->MainRelayOn) + "</h4>";
    content += "<h4>Heater present: " + String(nissan_dl->HeatExist) + "</h4>";
    content += "<h4>Heating stopped: " + String(nissan_dl->HeatingStop) + "</h4>";
    content += "<h4>Heating started: " + String(nissan_dl->HeatingStart) + "</h4>";
    content += "<h4>Heating requested: " + String(nissan_dl->HeaterSendRequest) + "</h4>";
    content += "<h4>Temperature 1: " + String(nissan_dl->temperature1 / 10.0) + " &deg;C</h4>";
    content += "<h4>Temperature 2: " + String(nissan_dl->temperature2 / 10.0) + " &deg;C</h4>";
    if (nissan_dl->LEAF_gen == 0) {
      content += "<h4>Temperature 3: " + String(nissan_dl->temperature3 / 10.0) + " &deg;C</h4>";
    }
    content += "<h4>Temperature 4: " + String(nissan_dl->temperature4 / 10.0) + " &deg;C</h4>";
    content += "<h4>CryptoChallenge: " + String(nissan_dl->CryptoChallenge) + "</h4>";
    content += "<h4>SolvedChallenge: " + String(nissan_dl->SolvedChallengeMSB) +
               String(nissan_dl->SolvedChallengeLSB) + "</h4>";
    content += "<h4>Challenge failed: " + String(nissan_dl->challengeFailed) + "</h4>";

    return content;
  }

 private:
  DATALAYER_INFO_NISSAN_LEAF* nissan_dl;
};

#endif
