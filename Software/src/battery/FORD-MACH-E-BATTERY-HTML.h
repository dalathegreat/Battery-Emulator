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
    if (datalayer_extended.fordMachE.pid_hvb_temp == 255) {
      content += "<h4>Average temperature: N/A </h4>";
    } else {
      content += "<h4>Average temperature: " + String(datalayer_extended.fordMachE.pid_hvb_temp) + " °C </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_voltage == 255) {
      content += "<h4>High precision voltage: N/A </h4>";
    } else {
      content += "<h4>High precision voltage: " + String(datalayer_extended.fordMachE.pid_hvb_voltage) + " mV </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_soh == 255) {
      content += "<h4>State of health: N/A </h4>";
    } else {
      content += "<h4>State of health: " + String(datalayer_extended.fordMachE.pid_hvb_soh) + " % </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_soc == 255) {
      content += "<h4>State of charge: N/A </h4>";
    } else {
      content += "<h4>State of charge: " + String(datalayer_extended.fordMachE.pid_hvb_soc) + " % </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_status == 255) {
      content += "<h4>Contactor status: N/A </h4>";
    } else {
      content += "<h4>Contactor status: " + String(datalayer_extended.fordMachE.pid_hvb_contactor_status) + "</h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage == 255) {
      content += "<h4>Pos contactor leak voltage: N/A </h4>";
    } else {
      content += "<h4>Pos contactor leak voltage: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_leak_voltage) + " mV </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage == 255) {
      content += "<h4>Neg contactor leak voltage: N/A </h4>";
    } else {
      content += "<h4>Neg contactor leak voltage: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_leak_voltage) + " mV </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage == 255) {
      content += "<h4>Pos contactor voltage: N/A </h4>";
    } else {
      content +=
          "<h4>Pos contactor voltage: " + String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_voltage) +
          " mV </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage == 255) {
      content += "<h4>Neg contactor voltage: N/A </h4>";
    } else {
      content +=
          "<h4>Neg contactor voltage: " + String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_voltage) +
          " mV </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance == 255) {
      content += "<h4>Pos contactor bus leak resistance: N/A </h4>";
    } else {
      content += "<h4>Pos contactor bus leak resistance: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_positive_bus_leak_resistance) + " kOhm </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance == 255) {
      content += "<h4>Neg contactor bus leak resistance: N/A </h4>";
    } else {
      content += "<h4>Neg contactor bus leak resistance: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_negative_bus_leak_resistance) + " kOhm </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance == 255) {
      content += "<h4>Overall contactor leak resistance: N/A </h4>";
    } else {
      content += "<h4>Overall contactor leak resistance: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_overall_leak_resistance) + " kOhm </h4>";
    }
    if (datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance == 255) {
      content += "<h4>Open contactor leak resistance: N/A </h4>";
    } else {
      content += "<h4>Open contactor leak resistance: " +
                 String(datalayer_extended.fordMachE.pid_hvb_contactor_open_leak_resistance) + " kOhm </h4>";
    }
    if (datalayer_extended.fordMachE.pid_battery_capacity_ah == 255) {
      content += "<h4>Capacity: N/A </h4>";
    } else {
      content += "<h4>Capacity: " + String(datalayer_extended.fordMachE.pid_battery_capacity_ah) + " AH+1 </h4>";
    }
    if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 255) {
      content += "<h4>Maintenance rebalance status: N/A </h4>";
    } else {
      if (datalayer_extended.fordMachE.pid_maintenance_rebalance_status == 0x04) {
        content += "<h4>Maintenance rebalance status: Initializing</h4>";
      } else {
        content += "<h4>Maintenance rebalance status: " +
                   String(datalayer_extended.fordMachE.pid_maintenance_rebalance_status) + "</h4>";
      }
    }
    if (datalayer_extended.fordMachE.pid_hvb_calendar_age_months == 255) {
      content += "<h4>Unknown 1: N/A </h4>";
    } else {
      content += "<h4>Unknown 1: " + String(datalayer_extended.fordMachE.pid_hvb_calendar_age_months) + " </h4>";
    }
    return content;
  }
};

#endif
