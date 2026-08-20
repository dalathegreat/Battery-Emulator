#ifndef _GEELY_GEOMETRY_C_HTML_H
#define _GEELY_GEOMETRY_C_HTML_H

#include <cstring>  //For unit test
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

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
    tr_h4(content, TrKey::DRV_SERIAL_NUMBER, String(readableSoftwareVersion));
    tr_h4(content, TrKey::DRV_SOFTWARE_VERSION, String(readableSerialNumber));
    tr_h4(content, TrKey::DRV_HARDWARE_VERSION, String(readableHardwareVersion));
    tr_h4(content, TrKey::DRV_SOC_DISPLAY, String(datalayer_extended.geometryC.soc), "ppt");
    tr_h4(content, TrKey::DRV_CC2_VOLTAGE, String(datalayer_extended.geometryC.CC2voltage), "mV");
    tr_h4(content, TrKey::DRV_CELL_MAX_VOLTAGE_NUMBER, String(datalayer_extended.geometryC.cellMaxVoltageNumber));
    tr_h4(content, TrKey::DRV_CELL_MIN_VOLTAGE_NUMBER, String(datalayer_extended.geometryC.cellMinVoltageNumber));
    tr_h4(content, TrKey::DRV_CELL_TOTAL_AMOUNT, String(datalayer_extended.geometryC.cellTotalAmount), "S");
    tr_h4(content, TrKey::DRV_SPECIFICIAL_VOLTAGE, String(datalayer_extended.geometryC.specificialVoltage), "dV");
    tr_h4(content, TrKey::DRV_UNKNOWN1, String(datalayer_extended.geometryC.unknown1));
    tr_h4(content, TrKey::DRV_RAW_SOC_MAX, String(datalayer_extended.geometryC.rawSOCmax));
    tr_h4(content, TrKey::DRV_RAW_SOC_MIN, String(datalayer_extended.geometryC.rawSOCmin));
    tr_h4(content, TrKey::DRV_UNKNOWN4, String(datalayer_extended.geometryC.unknown4));
    tr_h4(content, TrKey::DRV_CAPACITY_MODULE_MAX, String((datalayer_extended.geometryC.capModMax / 10)), "Ah");
    tr_h4(content, TrKey::DRV_CAPACITY_MODULE_MIN, String((datalayer_extended.geometryC.capModMin / 10)), "Ah");
    tr_h4(content, TrKey::DRV_UNKNOWN7, String(datalayer_extended.geometryC.unknown7));
    tr_h4(content, TrKey::DRV_UNKNOWN8, String(datalayer_extended.geometryC.unknown8));
    tr_h4(content, TrKey::DRV_MODULE_1_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[0]),
          " &deg;C");
    tr_h4(content, TrKey::DRV_MODULE_2_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[1]),
          " &deg;C");
    tr_h4(content, TrKey::DRV_MODULE_3_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[2]),
          " &deg;C");
    tr_h4(content, TrKey::DRV_MODULE_4_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[3]),
          " &deg;C");
    tr_h4(content, TrKey::DRV_MODULE_5_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[4]),
          " &deg;C");
    tr_h4(content, TrKey::DRV_MODULE_6_TEMPERATURE, String(datalayer_extended.geometryC.ModuleTemperatures[5]),
          " &deg;C");
    return content;
  }
};

#endif
