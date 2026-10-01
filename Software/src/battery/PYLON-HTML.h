#ifndef _PYLON_HTML_H
#define _PYLON_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
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
    content += "<h4>BMS charge cutoff voltage: " + String(data->charge_cutoff_dV / 10.0, 1) + " V</h4>";
    content += "<h4>BMS discharge cutoff voltage: " + String(data->discharge_cutoff_dV / 10.0, 1) + " V</h4>";
    content += "<h4>Design voltage in use: " + String(datalayer.battery.info.min_design_voltage_dV / 10.0, 1) + " - " +
               String(datalayer.battery.info.max_design_voltage_dV / 10.0, 1) + " V</h4>";
    if (data->cell_max_number > 0 || data->cell_min_number > 0) {
      content += "<h4>Highest cell: #" + String(data->cell_max_number) + " &nbsp; Lowest cell: #" +
                 String(data->cell_min_number) + "</h4>";
    }
    if (data->temp_max_sensor > 0 || data->temp_min_sensor > 0) {
      content += "<h4>Hottest sensor: #" + String(data->temp_max_sensor) + " &nbsp; Coldest sensor: #" +
                 String(data->temp_min_sensor) + "</h4>";
    }
    return content;
  }

 private:
  PylonExtendedData* data;
};

#endif
