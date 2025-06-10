#ifndef _NISSAN_LEAF_HTML_H
#define _NISSAN_LEAF_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class NissanLeafHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>LEAF generation: ";
    switch (datalayer_extended.nissanleaf.LEAF_gen) {
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
    memcpy(readableSerialNumber, datalayer_extended.nissanleaf.BatterySerialNumber,
           sizeof(datalayer_extended.nissanleaf.BatterySerialNumber));
    readableSerialNumber[15] = '\0';  // Null terminate the string
    content += "<h4>Serial number: " + String(readableSerialNumber) + "</h4>";
    char readablePartNumber[8];  // One extra space for null terminator
    memcpy(readablePartNumber, datalayer_extended.nissanleaf.BatteryPartNumber,
           sizeof(datalayer_extended.nissanleaf.BatteryPartNumber));
    readablePartNumber[7] = '\0';  // Null terminate the string
    content += "<h4>Part number: " + String(readablePartNumber) + "</h4>";
    char readableBMSID[9];  // One extra space for null terminator
    memcpy(readableBMSID, datalayer_extended.nissanleaf.BMSIDcode, sizeof(datalayer_extended.nissanleaf.BMSIDcode));
    readableBMSID[8] = '\0';  // Null terminate the string
    content += "<h4>BMS ID: " + String(readableBMSID) + "</h4>";
    content += "<h4>GIDS: " + String(datalayer_extended.nissanleaf.GIDS) + "</h4>";
    content += "<h4>Regen kW: " + String(datalayer_extended.nissanleaf.ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(datalayer_extended.nissanleaf.MaxPowerForCharger) + "</h4>";
    content += "<h4>Interlock: " + String(datalayer_extended.nissanleaf.Interlock) + "</h4>";
    content += "<h4>Insulation: " + String(datalayer_extended.nissanleaf.Insulation) + "</h4>";
    content += "<h4>Relay cut request: " + String(datalayer_extended.nissanleaf.RelayCutRequest) + "</h4>";
    content += "<h4>Failsafe status: " + String(datalayer_extended.nissanleaf.FailsafeStatus) + "</h4>";
    content += "<h4>Fully charged: " + String(datalayer_extended.nissanleaf.Full) + "</h4>";
    content += "<h4>Battery empty: " + String(datalayer_extended.nissanleaf.Empty) + "</h4>";
    content += "<h4>Main relay ON: " + String(datalayer_extended.nissanleaf.MainRelayOn) + "</h4>";
    content += "<h4>Heater present: " + String(datalayer_extended.nissanleaf.HeatExist) + "</h4>";
    content += "<h4>Heating stopped: " + String(datalayer_extended.nissanleaf.HeatingStop) + "</h4>";
    content += "<h4>Heating started: " + String(datalayer_extended.nissanleaf.HeatingStart) + "</h4>";
    content += "<h4>Heating requested: " + String(datalayer_extended.nissanleaf.HeaterSendRequest) + "</h4>";
    content += "<h4>CryptoChallenge: " + String(datalayer_extended.nissanleaf.CryptoChallenge) + "</h4>";
    content += "<h4>SolvedChallenge: " + String(datalayer_extended.nissanleaf.SolvedChallengeMSB) +
               String(datalayer_extended.nissanleaf.SolvedChallengeLSB) + "</h4>";
    content += "<h4>Challenge failed: " + String(datalayer_extended.nissanleaf.challengeFailed) + "</h4>";

    return content;
  }
};

#endif
