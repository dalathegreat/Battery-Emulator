#ifndef _GEELY_GEOMETRY_C_HTML_H
#define _GEELY_GEOMETRY_C_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "src/devboard/webserver/BatteryHtmlRenderer.h"

class GeelyGeometryCHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    char readableSerialNumber[29];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.geometryC.BatterySerialNumber,
           sizeof(datalayer_extended.geometryC.BatterySerialNumber));
    readableSerialNumber[28] = '\0';   // Null terminate the string
    char readableSoftwareVersion[17];  // One extra space for null terminator
    memcpy(readableSoftwareVersion, datalayer_extended.geometryC.BatterySoftwareVersion,
           sizeof(datalayer_extended.geometryC.BatterySoftwareVersion));
    readableSoftwareVersion[16] = '\0';  // Null terminate the string
    char readableHardwareVersion[17];    // One extra space for null terminator
    memcpy(readableHardwareVersion, datalayer_extended.geometryC.BatteryHardwareVersion,
           sizeof(datalayer_extended.geometryC.BatteryHardwareVersion));
    readableHardwareVersion[16] = '\0';  // Null terminate the string
    content += "<h4>Serial number: " + String(readableSoftwareVersion) + "</h4>";
    content += "<h4>Software version: " + String(readableSerialNumber) + "</h4>";
    content += "<h4>Hardware version: " + String(readableHardwareVersion) + "</h4>";
    content += "<h4>SOC display: " + String(datalayer_extended.geometryC.soc) + "ppt</h4>";
    content += "<h4>CC2 voltage: " + String(datalayer_extended.geometryC.CC2voltage) + "mV</h4>";
    content += "<h4>Cell max voltage number: " + String(datalayer_extended.geometryC.cellMaxVoltageNumber) + "</h4>";
    content += "<h4>Cell min voltage number: " + String(datalayer_extended.geometryC.cellMinVoltageNumber) + "</h4>";
    content += "<h4>Cell total amount: " + String(datalayer_extended.geometryC.cellTotalAmount) + "S</h4>";
    content += "<h4>Specificial Voltage: " + String(datalayer_extended.geometryC.specificialVoltage) + "dV</h4>";
    content += "<h4>Unknown1: " + String(datalayer_extended.geometryC.unknown1) + "</h4>";
    content += "<h4>Raw SOC max: " + String(datalayer_extended.geometryC.rawSOCmax) + "</h4>";
    content += "<h4>Raw SOC min: " + String(datalayer_extended.geometryC.rawSOCmin) + "</h4>";
    content += "<h4>Unknown4: " + String(datalayer_extended.geometryC.unknown4) + "</h4>";
    content += "<h4>Capacity module max: " + String((datalayer_extended.geometryC.capModMax / 10)) + "Ah</h4>";
    content += "<h4>Capacity module min: " + String((datalayer_extended.geometryC.capModMin / 10)) + "Ah</h4>";
    content += "<h4>Unknown7: " + String(datalayer_extended.geometryC.unknown7) + "</h4>";
    content += "<h4>Unknown8: " + String(datalayer_extended.geometryC.unknown8) + "</h4>";
    content +=
        "<h4>Module 1 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[0]) + " &deg;C</h4>";
    content +=
        "<h4>Module 2 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[1]) + " &deg;C</h4>";
    content +=
        "<h4>Module 3 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[2]) + " &deg;C</h4>";
    content +=
        "<h4>Module 4 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[3]) + " &deg;C</h4>";
    content +=
        "<h4>Module 5 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[4]) + " &deg;C</h4>";
    content +=
        "<h4>Module 6 temperature: " + String(datalayer_extended.geometryC.ModuleTemperatures[5]) + " &deg;C</h4>";

    return content;
  }
};

#endif
