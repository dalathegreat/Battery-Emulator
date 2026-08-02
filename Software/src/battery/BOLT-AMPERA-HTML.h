#ifndef _BOLT_AMPERA_HTML_H
#define _BOLT_AMPERA_HTML_H

#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

class BoltAmperaHtmlRenderer : public BatteryHtmlRenderer {
 public:
  BoltAmperaHtmlRenderer(DATALAYER_INFO_BOLTAMPERA* dl) : boltampera_dl(dl) {}

  bool renders_own_battery_data() { return true; }

  String get_status_html() {
    String content;

    if (!boltampera_dl) {
      return content;
    }

    tr_h4(content, TrKey::DRV_5V_REFERENCE, String(boltampera_dl->battery_5V_ref));
    tr_h4(content, TrKey::DRV_MODULE_1_TEMP, String(boltampera_dl->battery_module_temp_1));
    tr_h4(content, TrKey::DRV_MODULE_2_TEMP, String(boltampera_dl->battery_module_temp_2));
    tr_h4(content, TrKey::DRV_MODULE_3_TEMP, String(boltampera_dl->battery_module_temp_3));
    tr_h4(content, TrKey::DRV_MODULE_4_TEMP, String(boltampera_dl->battery_module_temp_4));
    tr_h4(content, TrKey::DRV_MODULE_5_TEMP, String(boltampera_dl->battery_module_temp_5));
    tr_h4(content, TrKey::DRV_MODULE_6_TEMP, String(boltampera_dl->battery_module_temp_6));
    tr_h4(content, TrKey::DRV_CELL_AVERAGE_VOLTAGE, String(boltampera_dl->battery_cell_average_voltage));
    tr_h4(content, TrKey::DRV_CELL_AVERAGE_VOLTAGE_2, String(boltampera_dl->battery_cell_average_voltage_2));
    tr_h4(content, TrKey::DRV_TERMINAL_VOLTAGE, String(boltampera_dl->battery_terminal_voltage));
    tr_h4(content, TrKey::DRV_IGNITION_POWER_MODE, String(boltampera_dl->battery_ignition_power_mode));
    tr_h4(content, TrKey::DRV_BATTERY_CURRENT_7E7, String(boltampera_dl->battery_current_7E7));
    tr_h4(content, TrKey::DRV_CAPACITY_MY17_18, String(boltampera_dl->battery_capacity_my17_18));
    tr_h4(content, TrKey::DRV_CAPACITY_MY19, String(boltampera_dl->battery_capacity_my19plus));
    tr_h4(content, TrKey::DRV_SOC_DISPLAY, String(boltampera_dl->battery_SOC_display));
    tr_h4(content, TrKey::DRV_SOC_RAW_HIGHPREC, String(boltampera_dl->battery_SOC_raw_highprec));
    tr_h4(content, TrKey::DRV_MAX_TEMP, String(boltampera_dl->battery_max_temperature));
    tr_h4(content, TrKey::DRV_MIN_TEMP, String(boltampera_dl->battery_min_temperature));
    tr_h4(content, TrKey::DRV_CELL_MAX_MV, String(boltampera_dl->battery_max_cell_voltage));
    tr_h4(content, TrKey::DRV_CELL_MIN_MV, String(boltampera_dl->battery_min_cell_voltage));
    tr_h4(content, TrKey::DRV_LOWEST_CELL, String(boltampera_dl->battery_lowest_cell));
    tr_h4(content, TrKey::DRV_HIGHEST_CELL, String(boltampera_dl->battery_highest_cell));
    tr_h4(content, TrKey::DRV_INTERNAL_RESISTANCE, String(boltampera_dl->battery_internal_resistance));
    tr_h4_colon(content, TrKey::UI_VOLTAGE, String(boltampera_dl->battery_voltage_polled));
    tr_h4(content, TrKey::DRV_ISOLATION_OHM, String(boltampera_dl->battery_vehicle_isolation));
    tr_h4(content, TrKey::DRV_ISOLATION_KOHM, String(boltampera_dl->battery_isolation_kohm));
    tr_h4(content, TrKey::DRV_HV_LOCKED, String(boltampera_dl->battery_HV_locked));
    tr_h4(content, TrKey::DRV_CRASH_EVENT, String(boltampera_dl->battery_crash_event));
    content += "<h4>HVIL: " + String(boltampera_dl->battery_HVIL) + "</h4>";
    tr_h4(content, TrKey::DRV_HVIL_STATUS, String(boltampera_dl->battery_HVIL_status));
    tr_h4(content, TrKey::DRV_CURRENT_7E4, String(boltampera_dl->battery_current_7E4));

    return content;
  }

 private:
  DATALAYER_INFO_BOLTAMPERA* boltampera_dl;
};

#endif
