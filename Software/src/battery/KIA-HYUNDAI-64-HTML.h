#ifndef _KIA_HYUNDAI_64_HTML_H
#define _KIA_HYUNDAI_64_HTML_H

#include <cstring>  //For unit test
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class KiaHyundai64HtmlRenderer : public BatteryHtmlRenderer {
 public:
  KiaHyundai64HtmlRenderer(DATALAYER_INFO_KIAHYUNDAI64* dl) : kia_datalayer(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;
    auto print_hyundai = [&content](DATALAYER_INFO_KIAHYUNDAI64& data) {
      char readableSerialNumber[17];  // One extra space for null terminator
      memcpy(readableSerialNumber, data.ecu_serial_number, sizeof(data.ecu_serial_number));
      readableSerialNumber[16] = '\0';  // Null terminate the string
      char readableVersionNumber[17];   // One extra space for null terminator
      memcpy(readableVersionNumber, data.ecu_version_number, sizeof(data.ecu_version_number));
      readableVersionNumber[16] = '\0';  // Null terminate the string

      tr_h4(content, TrKey::DRV_BMS_SERIAL_NUMBER, String(readableSerialNumber));
      tr_h4(content, TrKey::DRV_BMS_SOFTWARE_VERSION, String(readableVersionNumber));
      tr_h4(content, TrKey::DRV_CELLS, String(data.total_cell_count), " S");
      tr_h4(content, TrKey::DRV_12V_VOLTAGE, String(data.battery_12V / 10.0f, 1), " V");
      tr_h4_open(content, TrKey::DRV_WATERLEAKAGE);
      if (data.waterleakageSensor == 0) {
        content += " " + TR(TrKey::DRV_LEAK_DETECTED) + "</h4>";
      } else if (data.waterleakageSensor == 164) {
        content += " " + TR(TrKey::DRV_NO_LEAKAGE) + "</h4>";
      } else {
        content += String(data.waterleakageSensor) + "</h4>";
      }
      tr_h4(content, TrKey::DRV_TEMPERATURE_WATER_INLET, String(data.temperature_water_inlet), " &deg;C");
      tr_h4(content, TrKey::DRV_TEMPERATURE_POWER_RELAY, String(data.powerRelayTemperature), " &deg;C");
      tr_h4(content, TrKey::DRV_BATTERYMANAGEMENT_MODE, String(data.batteryManagementMode));
      tr_h4(content, TrKey::DRV_BMS_IGNITION, String(data.BMS_ign));
      tr_h4(content, TrKey::DRV_BATTERY_RELAY, String(data.batteryRelay));
      tr_h4(content, TrKey::DRV_INVERTER_VOLTAGE, String(data.inverterVoltage), " V");
      tr_h4(content, TrKey::DRV_ISOLATION_RESISTANCE, String(data.isolation_resistance_kOhm), " kOhm");
      tr_h4(content, TrKey::DRV_POWER_TOTAL_TIME, String(data.powered_on_total_time), " s");
      tr_h4(content, TrKey::DRV_FASTCHARGING_SESSIONS, String(data.number_of_fastcharging_sessions), " x");
      tr_h4(content, TrKey::DRV_SLOWCHARGING_SESSIONS, String(data.number_of_standard_charging_sessions), " x");
      tr_h4(content, TrKey::DRV_NORMAL_CHARGED_ENERGY_AMOUNT, String(data.accumulated_normal_charging_energy_kWh),
            " kWh");
      tr_h4(content, TrKey::DRV_FASTCHARGED_ENERGY_AMOUNT, String(data.accumulated_fastcharging_energy_kWh), " kWh");
      tr_h4(content, TrKey::DRV_TOTAL_AMOUNT_CHARGED_ENERGY, String(data.cumulative_energy_charged_kWh / 10.0), " kWh");
      tr_h4(content, TrKey::DRV_TOTAL_AMOUNT_DISCHARGED_ENERGY, String(data.cumulative_energy_discharged_kWh / 10.0),
            " kWh");
      tr_h4(content, TrKey::DRV_CUMULATIVE_CHARGE_CURRENT, String(data.cumulative_charge_current_ah / 10.0), " Ah");
      tr_h4(content, TrKey::DRV_CUMULATIVE_DISCHARGE_CURRENT, String(data.cumulative_discharge_current_ah / 10.0),
            " Ah");
    };

    print_hyundai(*kia_datalayer);

    return content;
  }

 private:
  DATALAYER_INFO_KIAHYUNDAI64* kia_datalayer;
};

#endif
