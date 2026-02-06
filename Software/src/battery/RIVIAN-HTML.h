#ifndef _RIVIAN_HTML_H
#define _RIVIAN_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class RivianHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;

    content += "<h4>Isolation measurement ongoing: ";
    if (datalayer_extended.rivian.IsolationMeasurementOngoing) {
      content += "Yes</h4>";
    } else {
      content += "No</h4>";
    }

    content += "<h4>Contactor State: ";
    if (datalayer_extended.rivian.contactor_state == 0) {
      content += "Open";
    } else if (datalayer_extended.rivian.contactor_state == 1) {
      content += "Closed";
    } else if (datalayer_extended.rivian.contactor_state == 2) {
      content += "Precharge";
    } else if (datalayer_extended.rivian.contactor_state == 3) {
      content += "Turning off";
    } else if (datalayer_extended.rivian.contactor_state == 4) {
      content += "Initialization";
    } else if (datalayer_extended.rivian.contactor_state == 5) {
      content += "FAILURE";
    }
    content += "</h4>";

    content += "<h4>Active Errors:</h4>";

    if (datalayer_extended.rivian.error_relay_open) {
      content += "<h4>Error Relay Open</h4>";
    }
    if (datalayer_extended.rivian.error_flags_from_BMS & 0x01) {
      content += "<h4>Error Isolation Single</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x02) >> 1) {
      content += "<h4>Error Isolation Double</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x04) >> 2) {
      content += "<h4>Error Emergency Off CRASH</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x08) >> 3) {
      content += "<h4>Error Emergency Off Pilot</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x10) >> 4) {
      content += "<h4>Error Emergency Off Request</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x20) >> 5) {
      content += "<h4>Error Emergency Off</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x40) >> 6) {
      content += "<h4>Error Contactors Welded</h4>";
    }
    if ((datalayer_extended.rivian.error_flags_from_BMS & 0x80) >> 7) {
      content += "<h4>Error Limited Power</h4>";
    }

    return content;
  }
};

#endif
