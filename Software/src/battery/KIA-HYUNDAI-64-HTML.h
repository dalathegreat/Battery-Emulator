#ifndef _KIA_HYUNDAI_64_HTML_H
#define _KIA_HYUNDAI_64_HTML_H

#include <cstring>  //For unit test
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class KiaHyundai64HtmlRenderer : public BatteryHtmlRenderer {
 public:
  KiaHyundai64HtmlRenderer(DATALAYER_INFO_KIAHYUNDAI64* dl) : kia_datalayer(dl) {}

  String get_status_html() {
    String content;
    auto print_hyundai = [&content](DATALAYER_INFO_KIAHYUNDAI64& data) {
      char readableSerialNumber[17];  // One extra space for null terminator
      memcpy(readableSerialNumber, data.ecu_serial_number, sizeof(data.ecu_serial_number));
      readableSerialNumber[16] = '\0';  // Null terminate the string
      char readableVersionNumber[17];   // One extra space for null terminator
      memcpy(readableVersionNumber, data.ecu_version_number, sizeof(data.ecu_version_number));
      readableVersionNumber[16] = '\0';  // Null terminate the string

      content += "<h4>BMS serial number: " + String(readableSerialNumber) + "</h4>";
      content += "<h4>BMS software version: " + String(readableVersionNumber) + "</h4>";
      content += "<h4>Cells: " + String(data.total_cell_count) + " S</h4>";
      content += "<h4>12V voltage: " + String(data.battery_12V / 10.0, 1) + " V</h4>";
      content += "<h4>Waterleakage: ";
      if (data.waterleakageSensor == 0) {
        content += " LEAK DETECTED</h4>";
      } else if (data.waterleakageSensor == 164) {
        content += " No leakage</h4>";
      } else {
        content += String(data.waterleakageSensor) + "</h4>";
      }
      content += "<h4>Temperature, water inlet: " + String(data.temperature_water_inlet) + " &deg;C</h4>";
      content += "<h4>Temperature, power relay: " + String(data.powerRelayTemperature) + " &deg;C</h4>";
      content += "<h4>Batterymanagement mode: " + String(data.batteryManagementMode) + "</h4>";
      content += "<h4>BMS ignition: " + String(data.BMS_ign) + "</h4>";
      content += "<h4>Battery relay: " + String(data.batteryRelay) + "</h4>";
      content += "<h4>Inverter voltage: " + String(data.inverterVoltage) + " V</h4>";
      content += "<h4>Isolation resistance: " + String(data.isolation_resistance_kOhm) + " kOhm</h4>";
      content += "<h4>Power on total time: " + String(data.powered_on_total_time) + " s</h4>";
      content += "<h4>Fastcharging sessions: " + String(data.number_of_fastcharging_sessions) + " x</h4>";
      content += "<h4>Slowcharging sessions: " + String(data.number_of_standard_charging_sessions) + " x</h4>";
      content +=
          "<h4>Normal charged energy amount: " + String(data.accumulated_normal_charging_energy_kWh) + " kWh</h4>";
      content += "<h4>Fastcharged energy amount: " + String(data.accumulated_fastcharging_energy_kWh) + " kWh</h4>";
      content += "<h4>Total amount charged energy: " + String(data.cumulative_energy_charged_kWh / 10.0) + " kWh</h4>";
      content +=
          "<h4>Total amount discharged energy: " + String(data.cumulative_energy_discharged_kWh / 10.0) + " kWh</h4>";
      content += "<h4>Cumulative charge current: " + String(data.cumulative_charge_current_ah / 10.0) + " Ah</h4>";
      content +=
          "<h4>Cumulative discharge current: " + String(data.cumulative_discharge_current_ah / 10.0) + " Ah</h4>";
    };

    print_hyundai(*kia_datalayer);

    return content;
  }

 private:
  DATALAYER_INFO_KIAHYUNDAI64* kia_datalayer;
};

#endif
