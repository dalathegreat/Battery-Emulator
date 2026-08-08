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

  String get_status_html();

 private:
  PylonExtendedData* data;
};

#endif
