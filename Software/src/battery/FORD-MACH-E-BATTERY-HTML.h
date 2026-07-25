#ifndef _FORD_MACH_E_BATTERY_HTML_H
#define _FORD_MACH_E_BATTERY_HTML_H

#include <cstring>
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class FordMachEHtmlRenderer : public BatteryHtmlRenderer {
 public:
  String get_status_html() {
    String content;
    content += "<h3>Ford Mach-E Extra Information</h2>";
    //If values are not sampled yet (255), show "N/A" instead of 255

    content += "<h4>Average temperature:";
    if (datalayer_extended.fordMachE.pid_hvb_temp == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_temp) + " °C </h4>";
    }

    content += "<h4>High precision voltage:";
    if (datalayer_extended.fordMachE.pid_hvb_voltage == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_voltage / 100.0, 2) + " V </h4>";
    }

    content += "<h4>State of health:";
    if (datalayer_extended.fordMachE.pid_hvb_soh == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_soh) + " % </h4>";
    }

    content += "<h4>State of charge:";
    if (datalayer_extended.fordMachE.pid_hvb_soc == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_soc / 1000.0, 3) + " % </h4>";
    }

    content += "<h4>Contactor status:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 255) {
      content += "N/A</h4>";
    } else {
      if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0xA00A8400) {
        content += "Interlock Seated OK</h4>";
      } else if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0) {
        content += "Interlock Not evaluated yet</h4>";
      } else if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 0x00000400) {
        content += "Interlock OPEN!</h4>";
      } else {
        content += "Unknown enumeration: " + String(datalayer_extended.fordMachE.pid_hvb_contactor_status) + "</h4>";
      }
    }

    content += "<h4>Pos contactor leak voltage:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage) + " mV </h4>";
    }

    content += "<h4>Neg contactor leak voltage:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage) + " mV </h4>";
    }

    content += "<h4>Pos contactor voltage:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage) + " mV </h4>";
    }

    content += "<h4>Neg contactor voltage:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage) + " mV </h4>";
    }

    content += "<h4>Pos contactor bus leak resistance:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance == 255) {
      content += "N/A</h4>";
    } else {
      content +=
          " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance) + " kOhm </h4>";
    }

    content += "<h4>Neg contactor bus leak resistance:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance == 255) {
      content += "N/A</h4>";
    } else {
      content +=
          " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance) + " kOhm </h4>";
    }

    content += "<h4>Overall contactor leak resistance:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance) + " kOhm </h4>";
    }

    content += "<h4>Open contactor leak resistance:";
    if (datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance) + " kOhm </h4>";
    }

    content += "<h4>Capacity:";
    if (datalayer_extended.fordMachE.pid_battery_capacity_ah == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_battery_capacity_ah / 10.0, 1) + " Ah </h4>";
    }

    content += "<h4>Maintenance rebalance status:";
    if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 255) {
      content += "N/A</h4>";
    } else {
      if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x04) {
        content += " Initializing</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x01) {
        content += " In progress</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x02) {
        content += " Successfully</h4>";
      } else if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x03) {
        content += " Aborted pack fault</h4>";
      } else {
        content += " " + String(datalayer_extended.fordMachE.pid_maintenance_rebalance_status) + "</h4>";
      }
    }

    content += "<h4>Calendar age:";
    if (datalayer_extended.fordMachE.pid_hvb_calendar_age_months == 255) {
      content += "N/A</h4>";
    } else {
      content += " " + String(datalayer_extended.fordMachE.pid_hvb_calendar_age_months / 100.0, 0) + " Months </h4>";
    }
    return content;
  }
};

#endif
