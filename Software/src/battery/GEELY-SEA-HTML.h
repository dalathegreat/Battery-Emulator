#ifndef _GEELY_SEA_HTML_H
#define _GEELY_SEA_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class GeelySeaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    tr_h4(content, TrKey::DRV_BECM_REPORTED_NUMBER_DTCS, String(datalayer_extended.GeelySEA.DTCcount));
    tr_h4(content, TrKey::DRV_INHIBITION_STATUS_CRASH, String(datalayer_extended.GeelySEA.CrashStatus));
    tr_h4(content, TrKey::DRV_BECM_REPORTED_SOC, String(datalayer_extended.GeelySEA.soc_bms / 100.0), " %");
    tr_h4(content, TrKey::DRV_BECM_REPORTED_SOH, String(datalayer_extended.GeelySEA.soh_bms / 100.0), " %");
    tr_h4(content, TrKey::DRV_HV_VOLTAGE, String(datalayer_extended.GeelySEA.BECMBatteryVoltage / 100.0), " V");
    //tr_h4(content, TrKey::DRV_BATTERY_CURRENT, String((datalayer_extended.GeelySEA.BatteryCurrent / 10.0) - 1638), " A");
    tr_h4(content, TrKey::DRV_HIGHEST_CELL_VOLTAGE, String(datalayer_extended.GeelySEA.CellVoltHighest / 1000.00),
          " V");
    tr_h4(content, TrKey::DRV_LOWEST_CELL_VOLTAGE, String(datalayer_extended.GeelySEA.CellVoltLowest / 1000.00), " V");
    tr_h4(content, TrKey::DRV_BECM_SUPPLY_VOLTAGE, String(datalayer_extended.GeelySEA.BECMsupplyVoltage / 1000.0),
          " V");
    tr_h4(content, TrKey::DRV_CELL_COUNT, String(datalayer.battery.info.number_of_cells));
    tr_h4(content, TrKey::DRV_HIGHEST_CELL_TEMP, String((datalayer_extended.GeelySEA.CellTempHighest / 100.0) - 50.0),
          " ºC");
    tr_h4(content, TrKey::DRV_AVERAGE_CELL_TEMP, String((datalayer_extended.GeelySEA.CellTempAverage / 100.0) - 50.0),
          " ºC");
    tr_h4(content, TrKey::DRV_LOWEST_CELL_TEMP, String((datalayer_extended.GeelySEA.CellTempLowest / 100.0) - 50.0),
          " ºC");
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_1_M1_M2_FC_CONNECTORS_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x80) {
      case 0x80:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_2_LV_CONNECTOR_PIN_9_10_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x40) {
      case 0x40:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_HVIL_CIRCUIT_3_LV_CONNECTOR_PIN_8_12_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x04) {
      case 0x04:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_UNKNOW_CONTACTOR_STATUS_1_NEGATIVE_FC);
    switch (datalayer_extended.GeelySEA.Interlock & 0x01) {
      case 0x01:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_UNKNOWN_CONTACTOR_STATUS_2_POSITIVE_FC);
    switch (datalayer_extended.GeelySEA.Interlock & 0x02) {
      case 0x02:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_NEGATIVE_CONTACTOR_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x08) {
      case 0x08:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_PRECHARGE_CONTACTOR_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x10) {
      case 0x10:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    tr_h4_open(content, TrKey::DRV_POSITIVE_CONTACTOR_STATUS);
    switch (datalayer_extended.GeelySEA.Interlock & 0x20) {
      case 0x20:
        content += String(TR(TrKey::DRV_OPEN));
        break;
      default:
        content += String(TR(TrKey::DRV_CLOSED));
    }
    content += "<h4>";
    return content;
  }
};

#endif
