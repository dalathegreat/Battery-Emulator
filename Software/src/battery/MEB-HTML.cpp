#include "MEB-HTML.h"
#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"

String MebHtmlRenderer::get_status_html() {
  String content;
  content.reserve(4096);

  static const char* const kl30c_enum[] = {"Init", "Closed", "Open!", "Fault"};  // also used for HVIL
  static const char* const bms_mode_enum[] = {"HV inactive", "HV active",     "Balancing",   "Extern charging",
                                              "AC charging", "Battery error", "DC charging", "Init"};
  static const char* const balancing_enum[] = {"Init", "Active", "Inactive"};
  static const char* const diagnostic_enum[] = {"Init", "Battery display",       "?",    "?", "Battery display OK",
                                                "?",    "Battery display check", "Fault"};
  static const char* const ptc_line_enum[] = {"Init", "Heater no open HV line", "Heater open HV line", "Fault"};
  static const char* const welded_enum[] = {"Init", "No contactor welded", "At least 1 contactor welded",
                                            "Protection status detection error"};
  static const char* const warning_support_enum[] = {"OK", "Not OK", "?", "?", "?", "?", "Init", "Fault"};
  static const char* const voltage_free_enum[] = {"Init", "BMS interm circuit voltage free (U<20V)",
                                                  "BMS interm circuit not voltage free (U >= 25V)", "Error"};
  static const char* const bms_error_enum[] = {
      "Component IO", "Iso Error 1",     "Iso Error 2",           "Interlock",
      "SD",           "Performance red", "No component function", "Init"};

  add_h4(content, "Service disconnect switch", datalayer_extended.meb.SDSW ? "Missing!" : "OK");
  add_h4(content, "Pilotline", datalayer_extended.meb.pilotline ? "Open!" : "OK");
  add_h4(content, "Transportmode", datalayer_extended.meb.transportmode ? "Locked!" : "OK");
  add_h4(content, "Shutdown", datalayer_extended.meb.shutdown_active ? "Active!" : "No");
  add_h4(content, "Component protection", datalayer_extended.meb.componentprotection ? "Active!" : "No");
  add_h4(content, "HVIL status", enum_str(kl30c_enum, datalayer_extended.meb.HVIL));
  add_h4(content, "KL30C status", enum_str(kl30c_enum, datalayer_extended.meb.BMS_Kl30c_Status));
  add_h4(content, "BMS mode", enum_str(bms_mode_enum, datalayer_extended.meb.BMS_mode));
  add_h4(content, "Charging", datalayer_extended.meb.charging_active ? "Active" : "Not active");
  add_h4(content, "Balancing", enum_str(balancing_enum, datalayer_extended.meb.balancing_active));
  add_h4(content, "Slow charging", datalayer_extended.meb.balancing_request ? "Requested" : "Not requested");
  add_h4(content, "Diagnostic", enum_str(diagnostic_enum, datalayer_extended.meb.battery_diagnostic));

  content += "<h4>Heater HV line status: ";
  if (datalayer_extended.meb.status_HV_PTC_line < 4) {
    content += ptc_line_enum[datalayer_extended.meb.status_HV_PTC_line];
  } else {
    content += "? ";
    content += String(datalayer_extended.meb.status_HV_PTC_line);
  }
  content += "</h4>";

  add_h4(content, "BMS fault performance", datalayer_extended.meb.BMS_fault_performance ? "Active!" : "Off");
  add_h4(content, "BMS fault emergency shutdown crash",
         datalayer_extended.meb.BMS_fault_emergency_shutdown_crash ? "Active!" : "Off");
  add_h4(content, "BMS error shutdown request",
         datalayer_extended.meb.BMS_error_shutdown_request ? "Active!" : "Inactive");
  add_h4(content, "BMS error shutdown", datalayer_extended.meb.BMS_error_shutdown ? "Active!" : "Off");
  add_h4(content, "Welded contactors", enum_str(welded_enum, datalayer_extended.meb.BMS_welded_contactors_status));
  add_h4(content, "Warning support", enum_str(warning_support_enum, datalayer_extended.meb.warning_support));

  // Raw 0xFFD-0xFFF are status codes; showing them as ~1047V would look like a real reading.
  constexpr int32_t not_meas_code_dV = meb_interm_raw_to_dV(0xFFD);
  constexpr int32_t init_code_dV = meb_interm_raw_to_dV(0xFFE);
  constexpr int32_t fault_code_dV = meb_interm_raw_to_dV(0xFFF);
  const int32_t interm_dV = datalayer_extended.meb.BMS_voltage_intermediate_dV;
  content += "<h4>Interm. Voltage (";
  if (interm_dV == not_meas_code_dV) {
    content += "NOT measurable";
  } else if (interm_dV == init_code_dV) {
    content += "Init";
  } else if (interm_dV == fault_code_dV) {
    content += "Fault";
  } else {
    content += String(interm_dV / 10.0f, 1);
    content += "V";
  }
  content += ") status: ";
  content += enum_str(voltage_free_enum, datalayer_extended.meb.BMS_status_voltage_free);
  content += "</h4>";

  add_h4(content, "BMS error status", enum_str(bms_error_enum, datalayer_extended.meb.BMS_error_status));
  add_h4(content, "BMS voltage", String(datalayer_extended.meb.BMS_voltage_dV / 10.0f, 1));
  add_h4(content, "OBD MIL", datalayer_extended.meb.BMS_OBD_MIL ? "ON!" : "Off");
  add_h4(content, "Red error lamp", datalayer_extended.meb.BMS_error_lamp_req ? "ON!" : "Off");
  add_h4(content, "Yellow warning lamp", datalayer_extended.meb.BMS_warning_lamp_req ? "ON!" : "Off");
  add_h4(content, "Isolation resistance", String(datalayer_extended.meb.isolation_resistance) + " kOhm");
  add_h4(content, "Battery heating", datalayer_extended.meb.battery_heating ? "Active!" : "Off");

  // The values are copied out by name on purpose: indexing straight off &rt_overcurrent works
  // today but would silently mislabel everything if the struct is ever reordered.
  static const char* const rt_enum[] = {"No", "Error level 1", "Error level 2", "Error level 3"};
  static const char* const rt_labels[] = {
      "Overcurrent",          "CAN fault",        "Overcharged",       "SOC too high",   "SOC too low",
      "SOC jumping",          "Temp difference",  "Cell overtemp",     "Cell undertemp", "Battery overvoltage",
      "Battery undervoltage", "Cell overvoltage", "Cell undervoltage", "Cell imbalance", "Battery unathorized"};
  const uint8_t rt_values[] = {datalayer_extended.meb.rt_overcurrent,
                               datalayer_extended.meb.rt_CAN_fault,
                               datalayer_extended.meb.rt_overcharge,
                               datalayer_extended.meb.rt_SOC_high,
                               datalayer_extended.meb.rt_SOC_low,
                               datalayer_extended.meb.rt_SOC_jumping,
                               datalayer_extended.meb.rt_temp_difference,
                               datalayer_extended.meb.rt_cell_overtemp,
                               datalayer_extended.meb.rt_cell_undertemp,
                               datalayer_extended.meb.rt_battery_overvolt,
                               datalayer_extended.meb.rt_battery_undervol,
                               datalayer_extended.meb.rt_cell_overvolt,
                               datalayer_extended.meb.rt_cell_undervol,
                               datalayer_extended.meb.rt_cell_imbalance,
                               datalayer_extended.meb.rt_battery_unathorized};
  static_assert(sizeof(rt_values) == sizeof(rt_labels) / sizeof(rt_labels[0]), "rt label/value table mismatch");
  for (size_t i = 0; i < sizeof(rt_values); i++) {
    add_h4(content, rt_labels[i], rt_enum[rt_values[i] & 0x03]);
  }

  content += "<h4>Battery temperature: ";
  if (datalayer_extended.meb.battery_temperature_dC == 875) {  //Raw value 255
    content += "Error</h4>";
  } else if (datalayer_extended.meb.battery_temperature_dC == 870) {  //Raw value 254
    content += "Init</h4>";
  } else {
    content += String(datalayer_extended.meb.battery_temperature_dC / 10.f, 1) + " &deg;C</h4>";
  }

  for (int i = 0; i < 3; i++) {
    content += "<h4>Temperature points " + String(i * 6 + 1) + "-" + String(i * 6 + 6) + " :";
    for (int j = 0; j < 6; j++)
      content += " &nbsp;" + String(datalayer_extended.meb.temp_points[i * 6 + j], 1);
    content += " &deg;C</h4>";
  }
  bool temps_done = false;
  for (int i = 0; i < 7 && !temps_done; i++) {
    content += "<h4>Cell temperatures " + String(i * 8 + 1) + "-" + String(i * 8 + 8) + " :";
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
  add_h4(content, "Total charged", String(datalayer.battery.status.total_charged_battery_Wh / 1000.0, 1) + " kWh");
  add_h4(content, "Total discharged",
         String(datalayer.battery.status.total_discharged_battery_Wh / 1000.0, 1) + " kWh");

  content += BatteryHtmlRenderer::render_dtc_section_html(datalayer.battery.dtc, dtc_json_filename, false);

  return content;
}
