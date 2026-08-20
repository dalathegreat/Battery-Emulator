#ifndef _MEB_HTML_H
#define _MEB_HTML_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../devboard/i18n/tr.h"
#include "../devboard/webserver/BatteryHtmlRenderer.h"

// Mirrors the scaling applied in MEB-BATTERY.cpp when BMS_voltage_intermediate is decoded
static constexpr int32_t meb_interm_raw_to_dV(int32_t raw) {
  return (raw - 2000) * 10 / 2;
}

class MebHtmlRenderer : public BatteryHtmlRenderer {
 public:
  // platform's battery setup() overrides
  const char* dtc_json_filename = "vag_meb_dtc.json";

  String get_status_html() {
    String content;
    static constexpr TrKey hvil_status_values[] = {TrKey::DRV_INIT, TrKey::DRV_CLOSED, TrKey::DRV_OPEN_ALARM,
                                                   TrKey::DRV_FAULT};
    static constexpr TrKey bms_mode_values[] = {
        TrKey::DRV_HV_INACTIVE, TrKey::DRV_HV_ACTIVE,     TrKey::UI_BALANCING,    TrKey::DRV_EXTERN_CHARGING,
        TrKey::DRV_AC_CHARGING, TrKey::DRV_BATTERY_ERROR, TrKey::DRV_DC_CHARGING, TrKey::DRV_INIT};
    static constexpr TrKey balancing_values[] = {TrKey::DRV_INIT, TrKey::DRV_ACTIVE_STATUS, TrKey::DRV_INACTIVE};
    static constexpr TrKey heater_hv_line_status_values[] = {TrKey::DRV_INIT, TrKey::DRV_HEATER_NO_OPEN_HV_LINE,
                                                             TrKey::DRV_HEATER_OPEN_HV_LINE, TrKey::DRV_FAULT};
    static constexpr TrKey welded_contactors_values[] = {TrKey::DRV_INIT, TrKey::DRV_NO_CONTACTOR_WELDED,
                                                         TrKey::DRV_AT_LEAST_1_CONTACTOR_WELDED,
                                                         TrKey::DRV_PROTECTION_STATUS_DETECTION_ERROR};
    static constexpr TrKey bms_error_status_values[] = {TrKey::DRV_COMPONENT_IO,          TrKey::DRV_ISO_ERROR_1,
                                                        TrKey::DRV_ISO_ERROR_2,           TrKey::DRV_INTERLOCK,
                                                        TrKey::DRV_SERVICE_DISCONNECT,    TrKey::DRV_PERFORMANCE_RED,
                                                        TrKey::DRV_NO_COMPONENT_FUNCTION, TrKey::DRV_INIT};

    tr_h4(content, datalayer_extended.meb.SDSW ? TrKey::DRV_SERVICE_DISCONNECT_SWITCH_MISSING
                                               : TrKey::DRV_SERVICE_DISCONNECT_SWITCH_OK);
    tr_h4(content, datalayer_extended.meb.pilotline ? TrKey::DRV_PILOTLINE_OPEN : TrKey::DRV_PILOTLINE_OK);
    tr_h4(content,
          datalayer_extended.meb.transportmode ? TrKey::DRV_TRANSPORTMODE_LOCKED : TrKey::DRV_TRANSPORTMODE_OK);
    tr_h4(content, datalayer_extended.meb.shutdown_active ? TrKey::DRV_SHUTDOWN_ACTIVE : TrKey::DRV_SHUTDOWN_NO);
    tr_h4(content, datalayer_extended.meb.componentprotection ? TrKey::DRV_COMPONENT_PROTECTION_ACTIVE
                                                              : TrKey::DRV_COMPONENT_PROTECTION_NO);
    tr_h4_open(content, TrKey::DRV_HVIL_STATUS);
    content += tr_enum(hvil_status_values, datalayer_extended.meb.HVIL);
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_KL30C_STATUS);
    content += tr_enum(hvil_status_values, datalayer_extended.meb.BMS_Kl30c_Status);
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_BMS_MODE);
    content += tr_enum(bms_mode_values, datalayer_extended.meb.BMS_mode);
    content += "</h4>";
    tr_h4_start(content, TrKey::DRV_CHARGING);
    content +=
        ": " + (datalayer_extended.meb.charging_active ? TR(TrKey::DRV_ACTIVE_STATUS) : TR(TrKey::DRV_NOT_ACTIVE));
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_BALANCING);
    content += tr_enum(balancing_values, datalayer_extended.meb.balancing_active);
    content += "</h4>";
    tr_h4(content, TrKey::DRV_SLOW_CHARGING,
          datalayer_extended.meb.balancing_request ? TR(TrKey::DRV_REQUESTED) : TR(TrKey::DRV_NOT_REQUESTED));
    tr_h4_open(content, TrKey::DRV_DIAGNOSTIC);
    switch (datalayer_extended.meb.battery_diagnostic) {
      case 0:
        content += TR(TrKey::DRV_INIT);
        break;
      case 1:
        content += TR(TrKey::DRV_BATTERY_DISPLAY);
        break;
      case 4:
        content += TR(TrKey::DRV_BATTERY_DISPLAY_OK);
        break;
      case 6:
        content += TR(TrKey::DRV_BATTERY_DISPLAY_CHECK);
        break;
      case 7:
        content += TR(TrKey::DRV_FAULT);
        break;
      default:
        content += "?";
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_HEATER_HV_LINE_STATUS);
    content += tr_enum(heater_hv_line_status_values, datalayer_extended.meb.status_HV_PTC_line);
    content += "</h4>";
    tr_h4(content, datalayer_extended.meb.BMS_fault_performance ? TrKey::DRV_BMS_FAULT_PERFORMANCE_ACTIVE
                                                                : TrKey::DRV_BMS_FAULT_PERFORMANCE_OFF);
    tr_h4(content, datalayer_extended.meb.BMS_fault_emergency_shutdown_crash
                       ? TrKey::DRV_BMS_FAULT_EMERGENCY_SHUTDOWN_CRASH_ACTIVE
                       : TrKey::DRV_BMS_FAULT_EMERGENCY_SHUTDOWN_CRASH_OFF);
    tr_h4(content, datalayer_extended.meb.BMS_error_shutdown_request ? TrKey::DRV_BMS_ERROR_SHUTDOWN_REQUEST_ACTIVE
                                                                     : TrKey::DRV_BMS_ERROR_SHUTDOWN_REQUEST_INACTIVE);
    tr_h4(content, datalayer_extended.meb.BMS_error_shutdown ? TrKey::DRV_BMS_ERROR_SHUTDOWN_ACTIVE
                                                             : TrKey::DRV_BMS_ERROR_SHUTDOWN_OFF);
    tr_h4_open(content, TrKey::DRV_WELDED_CONTACTORS);
    content += tr_enum(welded_contactors_values, datalayer_extended.meb.BMS_welded_contactors_status);
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_WARNING_SUPPORT);
    switch (datalayer_extended.meb.warning_support) {
      case 0:
        content += TR(TrKey::DRV_OK);
        break;
      case 1:
        content += TR(TrKey::DRV_NOT_OK);
        break;
      case 6:
        content += TR(TrKey::DRV_INIT);
        break;
      case 7:
        content += TR(TrKey::DRV_FAULT);
        break;
      default:
        content += "?";
    }
    content += "</h4><h4>Interm. Voltage (" + String(datalayer_extended.meb.BMS_voltage_intermediate_dV / 10.0f, 1) +
               "V) status: ";
    switch (datalayer_extended.meb.BMS_status_voltage_free) {
      case 0:
        content += TR(TrKey::DRV_INIT);
        break;
      case 1:
        content += TR(TrKey::DRV_BMS_INTERM_CIRCUIT_VOLTAGE_FREE_U_20V);
        break;
      case 2:
        content += TR(TrKey::DRV_BMS_INTERM_CIRCUIT_NOT_VOLTAGE_FREE_U_25V);
        break;
      case 3:
        content += TR(TrKey::DRV_ERROR);
        break;
      default:
        content += "?";
    }
    content += "</h4>";
    tr_h4_open(content, TrKey::DRV_BMS_ERROR_STATUS);
    content += tr_enum(bms_error_status_values, datalayer_extended.meb.BMS_error_status);
    content += "</h4>";
    tr_h4(content, TrKey::DRV_BMS_VOLTAGE, String(datalayer_extended.meb.BMS_voltage_dV / 10.0f, 1));
    tr_h4(content, datalayer_extended.meb.BMS_OBD_MIL ? TrKey::DRV_OBD_MIL : TrKey::DRV_OBD_MIL_OFF);
    tr_h4(content,
          datalayer_extended.meb.BMS_error_lamp_req ? TrKey::DRV_RED_ERROR_LAMP : TrKey::DRV_RED_ERROR_LAMP_OFF);
    tr_h4(content, datalayer_extended.meb.BMS_warning_lamp_req ? TrKey::DRV_YELLOW_WARNING_LAMP
                                                               : TrKey::DRV_YELLOW_WARNING_LAMP_OFF);
    tr_h4(content, TrKey::DRV_ISOLATION_RESISTANCE, String(datalayer_extended.meb.isolation_resistance), " kOhm");
    tr_h4(content,
          datalayer_extended.meb.battery_heating ? TrKey::DRV_BATTERY_HEATING_ACTIVE : TrKey::DRV_BATTERY_HEATING_OFF);
    const String rt_enum[] = {"No", TR(TrKey::DRV_ERROR_LEVEL_1), TR(TrKey::DRV_ERROR_LEVEL_2),
                              TR(TrKey::DRV_ERROR_LEVEL_3)};
    tr_h4(content, TrKey::DRV_OVERCURRENT, String(rt_enum[datalayer_extended.meb.rt_overcurrent & 0x03]));
    tr_h4(content, TrKey::DRV_CAN_FAULT, String(rt_enum[datalayer_extended.meb.rt_CAN_fault & 0x03]));
    tr_h4(content, TrKey::DRV_OVERCHARGED, String(rt_enum[datalayer_extended.meb.rt_overcharge & 0x03]));
    tr_h4(content, TrKey::DRV_SOC_TOO_HIGH, String(rt_enum[datalayer_extended.meb.rt_SOC_high & 0x03]));
    tr_h4(content, TrKey::DRV_SOC_TOO_LOW, String(rt_enum[datalayer_extended.meb.rt_SOC_low & 0x03]));
    tr_h4(content, TrKey::DRV_SOC_JUMPING, String(rt_enum[datalayer_extended.meb.rt_SOC_jumping & 0x03]));
    tr_h4(content, TrKey::DRV_TEMP_DIFFERENCE, String(rt_enum[datalayer_extended.meb.rt_temp_difference & 0x03]));
    tr_h4(content, TrKey::DRV_CELL_OVERTEMP, String(rt_enum[datalayer_extended.meb.rt_cell_overtemp & 0x03]));
    tr_h4(content, TrKey::DRV_CELL_UNDERTEMP, String(rt_enum[datalayer_extended.meb.rt_cell_undertemp & 0x03]));
    tr_h4(content, TrKey::DRV_BATTERY_OVERVOLTAGE, String(rt_enum[datalayer_extended.meb.rt_battery_overvolt & 0x03]));
    tr_h4(content, TrKey::DRV_BATTERY_UNDERVOLTAGE, String(rt_enum[datalayer_extended.meb.rt_battery_undervol & 0x03]));
    tr_h4_colon(content, TrKey::DRV_CELL_OVERVOLTAGE, String(rt_enum[datalayer_extended.meb.rt_cell_overvolt & 0x03]));
    tr_h4_colon(content, TrKey::DRV_CELL_UNDERVOLTAGE, String(rt_enum[datalayer_extended.meb.rt_cell_undervol & 0x03]));
    tr_h4(content, TrKey::DRV_CELL_IMBALANCE, String(rt_enum[datalayer_extended.meb.rt_cell_imbalance & 0x03]));
    tr_h4(content, TrKey::DRV_BATTERY_UNATHORIZED,
          String(rt_enum[datalayer_extended.meb.rt_battery_unathorized & 0x03]));
    tr_h4_start(content, TrKey::DRV_BATTERY_TEMPERATURE);
    content += ": ";
    if (datalayer_extended.meb.battery_temperature_dC == 875) {  //Raw value 255
      tr_h4_end(content, TrKey::DRV_ERROR);
    } else if (datalayer_extended.meb.battery_temperature_dC == 870) {  //Raw value 254
      tr_h4_end(content, TrKey::DRV_INIT);
    } else {
      content += String(datalayer_extended.meb.battery_temperature_dC / 10.f, 1) + " &deg;C</h4>";
    }

    for (int i = 0; i < 3; i++) {
      content += "<h4>" + TR(TrKey::DRV_TEMPERATURE_POINTS) + " " + String(i * 6 + 1) + "-" + String(i * 6 + 6) + " :";
      for (int j = 0; j < 6; j++)
        content += " &nbsp;" + String(datalayer_extended.meb.temp_points[i * 6 + j], 1);
      content += " &deg;C</h4>";
    }
    bool temps_done = false;
    for (int i = 0; i < 7 && !temps_done; i++) {
      content += "<h4>" + TR(TrKey::DRV_CELL_TEMPERATURES) + " " + String(i * 8 + 1) + "-" + String(i * 8 + 8) + " :";
      for (int j = 0; j < 8; j++) {
        if (datalayer_extended.meb.celltemperature_dC[i * 8 + j] == 865) {
          temps_done = true;
          break;
        } else {
          content += " &nbsp;" + String(datalayer_extended.meb.celltemperature_dC[i * 8 + j] / 10.f, 1);
        }
      }
      content += " &deg;C</h4>";
    }
    tr_h4(content, TrKey::DRV_TOTAL_CHARGED, String(datalayer.battery.status.total_charged_battery_Wh / 1000.0, 1),
          " kWh");
    tr_h4(content, TrKey::DRV_TOTAL_DISCHARGED,
          String(datalayer.battery.status.total_discharged_battery_Wh / 1000.0, 1), " kWh");

    content += BatteryHtmlRenderer::render_dtc_section_html(datalayer.battery.dtc, "vag_meb_dtc.json", false);

    return content;
  }
};

#endif
