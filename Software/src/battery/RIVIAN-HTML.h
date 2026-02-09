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

    content += "<h4>Isolation Status: ";
    if (datalayer_extended.rivian.isolation_fault_status == 0) {
      content += "Undefined";
    } else if (datalayer_extended.rivian.isolation_fault_status == 1) {
      content += "Stable";
    } else if (datalayer_extended.rivian.isolation_fault_status == 2) {
      content += "No Fault";
    } else if (datalayer_extended.rivian.isolation_fault_status == 3) {
      content += "High Side Fault";
    } else if (datalayer_extended.rivian.isolation_fault_status == 4) {
      content += "Low Side Fault";
    } else if (datalayer_extended.rivian.isolation_fault_status == 5) {
      content += "Dual Side Fault";
    } else if (datalayer_extended.rivian.isolation_fault_status == 6) {
      content += "Iso Circuit Failure";
    } else if (datalayer_extended.rivian.isolation_fault_status == 7) {
      content += "Iso Circuit Check timeout";
    }
    content += "</h4>";

    content += "<h4>Interlock status: ";
    if (datalayer_extended.rivian.HVIL == 0) {
      content += "NOT OK";
    } else if (datalayer_extended.rivian.HVIL == 1) {
      content += "NOT OK";
    } else if (datalayer_extended.rivian.HVIL == 2) {
      content += "NOT OK";
    } else if (datalayer_extended.rivian.HVIL == 3) {
      content += "OK";
    }
    content += "</h4>";

    content += "<h4>BMS State: ";
    if (datalayer_extended.rivian.BMS_state == 0) {
      content += "Sleep";
    } else if (datalayer_extended.rivian.BMS_state == 1) {
      content += "Standby";
    } else if (datalayer_extended.rivian.BMS_state == 2) {
      content += "Ready";
    } else if (datalayer_extended.rivian.BMS_state == 3) {
      content += "Go";
    }
    content += "</h4>";

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

    content += "<h4>Active errors and faults:</h4>";

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

    //HMI errors / status codes, bundle them also under errors

    if ((datalayer_extended.rivian.HMI_part1 & 0x10) >> 4) {
      content += "<h4>Vehicle Battery Issue</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x20) >> 5) {
      content += "<h4>Critical Battery Issue</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x40) >> 6) {
      content += "<h4>AC performance limited</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part1 & 0x80) >> 7) {
      content += "<h4>DC performance limited</h4>";
    }
    if (datalayer_extended.rivian.HMI_part2 & 0x01) {
      content += "<h4>DC charging disabled</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x02) >> 1) {
      content += "<h4>Electric hazard</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x04) >> 2) {
      content += "<h4>Fire risk</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x08) >> 3) {
      content += "<h4>Vehicle system fault</h4>";
    }
    if ((datalayer_extended.rivian.HMI_part2 & 0x10) >> 4) {
      content += "<h4>Battery electric malfunction</h4>";
    }

    //Misc
    if (datalayer_extended.rivian.puncture_fault) {
      content += "<h4>Puncture fault detected</h4>";
    }
    if (datalayer_extended.rivian.liquid_fault) {
      content += "<h4>Liquid fault detected</h4>";
    }

    return content;
  }
};

#endif
