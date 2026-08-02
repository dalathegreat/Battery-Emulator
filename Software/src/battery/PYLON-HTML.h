#ifndef _PYLON_HTML_H
#define _PYLON_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

/** Extra data parsed from the Pylon HV protocol that has no datalayer home.
 *  Filled by PylonBattery, displayed by PylonHtmlRenderer. */
struct PylonExtendedData {
  uint16_t charge_cutoff_dV = 0;
  uint16_t discharge_cutoff_dV = 0;
  uint16_t cell_max_number = 0;
  uint16_t cell_min_number = 0;
  uint16_t temp_max_sensor = 0;
  uint16_t temp_min_sensor = 0;
};

class PylonHtmlRenderer : public BatteryHtmlRenderer {
 public:
  PylonHtmlRenderer(PylonExtendedData* d) : data(d) {}

  String get_status_html() {
    String content;
    tr_h4(content, TrKey::DRV_BMS_CHARGE_CUTOFF_VOLTAGE, String(data->charge_cutoff_dV / 10.0, 1), " V");
    tr_h4(content, TrKey::DRV_BMS_DISCHARGE_CUTOFF_VOLTAGE, String(data->discharge_cutoff_dV / 10.0, 1), " V");
    tr_h4(content, TrKey::DRV_DESIGN_VOLTAGE_USE,
          String(datalayer.battery.info.min_design_voltage_dV / 10.0, 1) + " - " +
              String(datalayer.battery.info.max_design_voltage_dV / 10.0, 1),
          " V");
    if (data->cell_max_number > 0 || data->cell_min_number > 0) {
      tr_h4_pair(content, TrKey::DRV_HIGHEST_CELL, ": #", String(data->cell_max_number), "", TrKey::DRV_LOWEST_CELL,
                 ": #", String(data->cell_min_number), "");
    }
    if (data->temp_max_sensor > 0 || data->temp_min_sensor > 0) {
      content += "<h4>" + TR(TrKey::DRV_HOTTEST_SENSOR) + String(data->temp_max_sensor) + " &nbsp; " +
                 TR(TrKey::DRV_COLDEST_SENSOR) + String(data->temp_min_sensor) + "</h4>";
    }
    return content;
  }

 private:
  PylonExtendedData* data;
};

#endif
