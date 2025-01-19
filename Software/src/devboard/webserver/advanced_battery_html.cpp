#include "advanced_battery_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"

String advanced_battery_processor(const String& var) {
  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content +=
        "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
        "cursor: pointer; border-radius: 10px; }";
    content += "button:hover { background-color: #3A4A52; }";
    content += "</style>";
    content += "<button onclick='goToMainPage()'>Back to main page</button>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

#ifdef BOLT_AMPERA_BATTERY
    content += "<h4>5V Reference: " + String(datalayer_extended.boltampera.battery_5V_ref) + "</h4>";
    content += "<h4>Module 1 temp: " + String(datalayer_extended.boltampera.battery_module_temp_1) + "</h4>";
    content += "<h4>Module 2 temp: " + String(datalayer_extended.boltampera.battery_module_temp_2) + "</h4>";
    content += "<h4>Module 3 temp: " + String(datalayer_extended.boltampera.battery_module_temp_3) + "</h4>";
    content += "<h4>Module 4 temp: " + String(datalayer_extended.boltampera.battery_module_temp_4) + "</h4>";
    content += "<h4>Module 5 temp: " + String(datalayer_extended.boltampera.battery_module_temp_5) + "</h4>";
    content += "<h4>Module 6 temp: " + String(datalayer_extended.boltampera.battery_module_temp_6) + "</h4>";
    content +=
        "<h4>Cell average voltage: " + String(datalayer_extended.boltampera.battery_cell_average_voltage) + "</h4>";
    content +=
        "<h4>Cell average voltage 2: " + String(datalayer_extended.boltampera.battery_cell_average_voltage_2) + "</h4>";
    content += "<h4>Terminal voltage: " + String(datalayer_extended.boltampera.battery_terminal_voltage) + "</h4>";
    content +=
        "<h4>Ignition power mode: " + String(datalayer_extended.boltampera.battery_ignition_power_mode) + "</h4>";
    content += "<h4>Battery current (7E7): " + String(datalayer_extended.boltampera.battery_current_7E7) + "</h4>";
    content += "<h4>Capacity MY17-18: " + String(datalayer_extended.boltampera.battery_capacity_my17_18) + "</h4>";
    content += "<h4>Capacity MY19+: " + String(datalayer_extended.boltampera.battery_capacity_my19plus) + "</h4>";
    content += "<h4>SOC Display: " + String(datalayer_extended.boltampera.battery_SOC_display) + "</h4>";
    content += "<h4>SOC Raw highprec: " + String(datalayer_extended.boltampera.battery_SOC_raw_highprec) + "</h4>";
    content += "<h4>Max temp: " + String(datalayer_extended.boltampera.battery_max_temperature) + "</h4>";
    content += "<h4>Min temp: " + String(datalayer_extended.boltampera.battery_min_temperature) + "</h4>";
    content += "<h4>Cell max mV: " + String(datalayer_extended.boltampera.battery_max_cell_voltage) + "</h4>";
    content += "<h4>Cell min mV: " + String(datalayer_extended.boltampera.battery_min_cell_voltage) + "</h4>";
    content += "<h4>Lowest cell: " + String(datalayer_extended.boltampera.battery_lowest_cell) + "</h4>";
    content += "<h4>Highest cell: " + String(datalayer_extended.boltampera.battery_highest_cell) + "</h4>";
    content +=
        "<h4>Internal resistance: " + String(datalayer_extended.boltampera.battery_internal_resistance) + "</h4>";
    content += "<h4>Voltage: " + String(datalayer_extended.boltampera.battery_voltage_polled) + "</h4>";
    content += "<h4>Isolation Ohm: " + String(datalayer_extended.boltampera.battery_vehicle_isolation) + "</h4>";
    content += "<h4>Isolation kOhm: " + String(datalayer_extended.boltampera.battery_isolation_kohm) + "</h4>";
    content += "<h4>HV locked: " + String(datalayer_extended.boltampera.battery_HV_locked) + "</h4>";
    content += "<h4>Crash event: " + String(datalayer_extended.boltampera.battery_crash_event) + "</h4>";
    content += "<h4>HVIL: " + String(datalayer_extended.boltampera.battery_HVIL) + "</h4>";
    content += "<h4>HVIL status: " + String(datalayer_extended.boltampera.battery_HVIL_status) + "</h4>";
    content += "<h4>Current (7E4): " + String(datalayer_extended.boltampera.battery_current_7E4) + "</h4>";
#endif  //BOLT_AMPERA_BATTERY

#ifdef BMW_IX_BATTERY
    content +=
        "<h4>Battery Voltage after Contactor: " + String(datalayer_extended.bmwix.battery_voltage_after_contactor) +
        " dV</h4>";
    content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
    content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
    content +=
        "<h4>Min Cell Voltage Data Age: " + String(datalayer_extended.bmwix.min_cell_voltage_data_age) + " ms</h4>";
    content +=
        "<h4>Max Cell Voltage Data Age: " + String(datalayer_extended.bmwix.max_cell_voltage_data_age) + " ms</h4>";
    content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
    content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
    content += "<h4>T30 Terminal Voltage: " + String(datalayer_extended.bmwix.T30_Voltage) + " mV</h4>";
    content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    static const char* balanceText[5] = {"0 No balancing mode active", "1 Voltage-Controlled Balancing Mode",
                                         "2 Time-Controlled Balancing Mode with Demand Calculation at End of Charging",
                                         "3 Time-Controlled Balancing Mode with Demand Calculation at Resting Voltage",
                                         "4 No balancing mode active, qualifier invalid"};
    content += "<h4>Balancing: " + String((balanceText[datalayer_extended.bmwix.balancing_status])) + "</h4>";
    static const char* hvilText[2] = {"Error (Loop Open)", "OK (Loop Closed)"};
    content += "<h4>HVIL Status: " + String(hvilText[datalayer_extended.bmwix.hvil_status]) + "</h4>";
    content += "<h4>BMS Uptime: " + String(datalayer_extended.bmwix.bms_uptime) + " seconds</h4>";
    content += "<h4>BMS Allowed Charge Amps: " + String(datalayer_extended.bmwix.allowable_charge_amps) + " A</h4>";
    content +=
        "<h4>BMS Allowed Disharge Amps: " + String(datalayer_extended.bmwix.allowable_discharge_amps) + " A</h4>";
    content += "<br>";
    content += "<h3>HV Isolation (2147483647kOhm = maximum/invalid)</h3>";
    content += "<h4>Isolation Positive: " + String(datalayer_extended.bmwix.iso_safety_positive) + " kOhm</h4>";
    content += "<h4>Isolation Negative: " + String(datalayer_extended.bmwix.iso_safety_negative) + " kOhm</h4>";
    content += "<h4>Isolation Parallel: " + String(datalayer_extended.bmwix.iso_safety_parallel) + " kOhm</h4>";
    static const char* pyroText[5] = {"0 Value Invalid", "1 Successfully Blown", "2 Disconnected",
                                      "3 Not Activated - Pyro Intact", "4 Unknown"};
    content += "<h4>Pyro Status PSS1: " + String((pyroText[datalayer_extended.bmwix.pyro_status_pss1])) + "</h4>";
    content += "<h4>Pyro Status PSS4: " + String((pyroText[datalayer_extended.bmwix.pyro_status_pss4])) + "</h4>";
    content += "<h4>Pyro Status PSS6: " + String((pyroText[datalayer_extended.bmwix.pyro_status_pss6])) + "</h4>";
#endif  //BMW_IX_BATTERY

#ifdef BMW_PHEV_BATTERY
    content +=
        "<h4>Battery Voltage after Contactor: " + String(datalayer_extended.bmwphev.battery_voltage_after_contactor) +
        " dV</h4>";
    content += "<h4>Allowed Discharge Power: " + String(datalayer.battery.status.max_discharge_power_W) + " W</h4>";
    content += "<h4>Allowed Charge Power: " + String(datalayer.battery.status.max_charge_power_W) + " W</h4>";
    static const char* balanceText[5] = {"0 Balancing Inactive - Balancing not needed", "1 Balancing Active",
                                         "2 Balancing Inactive - Cells not in rest break wait 10mins",
                                         "3 Balancing Inactive", "4 Unknown"};
    content += "<h4>Balancing: " + String((balanceText[datalayer_extended.bmwphev.balancing_status])) + "</h4>";
    static const char* pyroText[5] = {"0 Value Invalid", "1 Successfully Blown", "2 Disconnected",
                                      "3 Not Activated - Pyro Intact", "4 Unknown"};
    static const char* statusText[16] = {
        "Not evaluated", "OK", "Error!", "Invalid signal", "", "", "", "", "", "", "", "", "", "", "", ""};
    content += "<h4>Interlock: " + String(statusText[datalayer_extended.bmwphev.ST_interlock]) + "</h4>";
    content += "<h4>Isolation external: " + String(statusText[datalayer_extended.bmwphev.ST_iso_ext]) + "</h4>";
    content += "<h4>Isolation internal: " + String(statusText[datalayer_extended.bmwphev.ST_iso_int]) + "</h4>";
    content += "<h4>Isolation: " + String(statusText[datalayer_extended.bmwphev.ST_isolation]) + "</h4>";
    content += "<h4>Cooling valve: " + String(statusText[datalayer_extended.bmwphev.ST_valve_cooling]) + "</h4>";
    content += "<h4>Emergency: " + String(statusText[datalayer_extended.bmwphev.ST_EMG]) + "</h4>";
    static const char* prechargeText[16] = {"Not evaluated",
                                            "Not active, closing not blocked",
                                            "Error precharge blocked",
                                            "Invalid signal",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            ""};
    content += "<h4>Precharge: " + String(prechargeText[datalayer_extended.bmwphev.ST_precharge]) +
               "</h4>";  //Still unclear of enum
    static const char* DCSWText[16] = {"Contactors open",
                                       "Precharge ongoing",
                                       "Contactors engaged",
                                       "Invalid signal",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       ""};
    content += "<h4>Contactor status: " + String(DCSWText[datalayer_extended.bmwphev.ST_DCSW]) + "</h4>";
    static const char* contText[16] = {"Contactors OK",
                                       "One contactor welded!",
                                       "Two contactors welded!",
                                       "Invalid signal",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       ""};
    content += "<h4>Contactor weld: " + String(contText[datalayer_extended.bmwphev.ST_WELD]) + "</h4>";
    static const char* valveText[16] = {"OK",
                                        "Short circuit to GND",
                                        "Short circuit to 12V",
                                        "Line break",
                                        "",
                                        "",
                                        "Driver error",
                                        "",
                                        "",
                                        "",
                                        "",
                                        "",
                                        "Stuck",
                                        "Stuck",
                                        "",
                                        "Invalid Signal"};
    content +=
        "<h4>Cold shutoff valve: " + String(valveText[datalayer_extended.bmwphev.ST_cold_shutoff_valve]) + "</h4>";
    content +=
        "<h4>Min Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.min_cell_voltage_data_age) + " ms</h4>";
    content +=
        "<h4>Max Cell Voltage Data Age: " + String(datalayer_extended.bmwphev.max_cell_voltage_data_age) + " ms</h4>";
    content += "<h4>Max Design Voltage: " + String(datalayer.battery.info.max_design_voltage_dV) + " dV</h4>";
    content += "<h4>Min Design Voltage: " + String(datalayer.battery.info.min_design_voltage_dV) + " dV</h4>";
    content += "<h4>BMS Allowed Charge Amps: " + String(datalayer_extended.bmwphev.allowable_charge_amps) + " A</h4>";
    content +=
        "<h4>BMS Allowed Disharge Amps: " + String(datalayer_extended.bmwphev.allowable_discharge_amps) + " A</h4>";
    content += "<h4>Detected Cell Count: " + String(datalayer.battery.info.number_of_cells) + "</h4>";
    content += "<h4>iso_safety_int_kohm: " + String(datalayer_extended.bmwphev.iso_safety_int_kohm) + "</h4>";
    content += "<h4>iso_safety_ext_kohm: " + String(datalayer_extended.bmwphev.iso_safety_ext_kohm) + "</h4>";
    content += "<h4>iso_safety_trg_kohm: " + String(datalayer_extended.bmwphev.iso_safety_trg_kohm) + "</h4>";
    content += "<h4>iso_safety_ext_plausible: " + String(datalayer_extended.bmwphev.iso_safety_ext_plausible) + "</h4>";
    content += "<h4>iso_safety_int_plausible: " + String(datalayer_extended.bmwphev.iso_safety_int_plausible) + "</h4>";
    content += "<h4>iso_safety_trg_plausible: " + String(datalayer_extended.bmwphev.iso_safety_trg_plausible) + "</h4>";
    content += "<h4>iso_safety_kohm: " + String(datalayer_extended.bmwphev.iso_safety_kohm) + "</h4>";
    content += "<h4>iso_safety_kohm_quality: " + String(datalayer_extended.bmwphev.iso_safety_kohm_quality) + "</h4>";
    content += "<br>";
    content += "<h4>Todo";
    content += "<br>";
    content += "<h4>Max Cell Design Voltage: " + String(datalayer.battery.info.max_cell_voltage_mV) + " mV</h4>";
    content += "<h4>Min Cell Design Voltage: " + String(datalayer.battery.info.min_cell_voltage_mV) + " mV</h4>";
    content += "<h4>T30 Terminal Voltage: " + String(datalayer_extended.bmwphev.T30_Voltage) + " mV</h4>";
    content += "<br>";
#endif  //BMW_PHEV_BATTERY

#ifdef BMW_I3_BATTERY
    content += "<h4>SOC raw: " + String(datalayer_extended.bmwi3.SOC_raw) + "</h4>";
    content += "<h4>SOC dash: " + String(datalayer_extended.bmwi3.SOC_dash) + "</h4>";
    content += "<h4>SOC OBD2: " + String(datalayer_extended.bmwi3.SOC_OBD2) + "</h4>";
    static const char* statusText[16] = {
        "Not evaluated", "OK", "Error!", "Invalid signal", "", "", "", "", "", "", "", "", "", "", "", ""};
    content += "<h4>Interlock: " + String(statusText[datalayer_extended.bmwi3.ST_interlock]) + "</h4>";
    content += "<h4>Isolation external: " + String(statusText[datalayer_extended.bmwi3.ST_iso_ext]) + "</h4>";
    content += "<h4>Isolation internal: " + String(statusText[datalayer_extended.bmwi3.ST_iso_int]) + "</h4>";
    content += "<h4>Isolation: " + String(statusText[datalayer_extended.bmwi3.ST_isolation]) + "</h4>";
    content += "<h4>Cooling valve: " + String(statusText[datalayer_extended.bmwi3.ST_valve_cooling]) + "</h4>";
    content += "<h4>Emergency: " + String(statusText[datalayer_extended.bmwi3.ST_EMG]) + "</h4>";
    static const char* prechargeText[16] = {"Not evaluated",
                                            "Not active, closing not blocked",
                                            "Error precharge blocked",
                                            "Invalid signal",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            "",
                                            ""};
    content += "<h4>Precharge: " + String(prechargeText[datalayer_extended.bmwi3.ST_precharge]) +
               "</h4>";  //Still unclear of enum
    static const char* DCSWText[16] = {"Contactors open",
                                       "Precharge ongoing",
                                       "Contactors engaged",
                                       "Invalid signal",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       ""};
    content += "<h4>Contactor status: " + String(DCSWText[datalayer_extended.bmwi3.ST_DCSW]) + "</h4>";
    static const char* contText[16] = {"Contactors OK",
                                       "One contactor welded!",
                                       "Two contactors welded!",
                                       "Invalid signal",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       "",
                                       ""};
    content += "<h4>Contactor weld: " + String(contText[datalayer_extended.bmwi3.ST_WELD]) + "</h4>";
    static const char* valveText[16] = {"OK",
                                        "Short circuit to GND",
                                        "Short circuit to 12V",
                                        "Line break",
                                        "",
                                        "",
                                        "Driver error",
                                        "",
                                        "",
                                        "",
                                        "",
                                        "",
                                        "Stuck",
                                        "Stuck",
                                        "",
                                        "Invalid Signal"};
    content += "<h4>Cold shutoff valve: " + String(contText[datalayer_extended.bmwi3.ST_cold_shutoff_valve]) + "</h4>";

#endif  //BMW_I3_BATTERY

#ifdef CELLPOWER_BMS
    static const char* falseTrue[2] = {"False", "True"};
    content += "<h3>States:</h3>";
    content += "<h4>Discharge: " + String(falseTrue[datalayer_extended.cellpower.system_state_discharge]) + "</h4>";
    content += "<h4>Charge: " + String(falseTrue[datalayer_extended.cellpower.system_state_charge]) + "</h4>";
    content +=
        "<h4>Cellbalancing: " + String(falseTrue[datalayer_extended.cellpower.system_state_cellbalancing]) + "</h4>";
    content +=
        "<h4>Tricklecharging: " + String(falseTrue[datalayer_extended.cellpower.system_state_tricklecharge]) + "</h4>";
    content += "<h4>Idle: " + String(falseTrue[datalayer_extended.cellpower.system_state_idle]) + "</h4>";
    content += "<h4>Charge completed: " + String(falseTrue[datalayer_extended.cellpower.system_state_chargecompleted]) +
               "</h4>";
    content +=
        "<h4>Maintenance charge: " + String(falseTrue[datalayer_extended.cellpower.system_state_maintenancecharge]) +
        "</h4>";
    content += "<h3>IO:</h3>";
    content +=
        "<h4>Main positive relay: " + String(falseTrue[datalayer_extended.cellpower.IO_state_main_positive_relay]) +
        "</h4>";
    content +=
        "<h4>Main negative relay: " + String(falseTrue[datalayer_extended.cellpower.IO_state_main_negative_relay]) +
        "</h4>";
    content +=
        "<h4>Charge enabled: " + String(falseTrue[datalayer_extended.cellpower.IO_state_charge_enable]) + "</h4>";
    content +=
        "<h4>Precharge relay: " + String(falseTrue[datalayer_extended.cellpower.IO_state_precharge_relay]) + "</h4>";
    content +=
        "<h4>Discharge enable: " + String(falseTrue[datalayer_extended.cellpower.IO_state_discharge_enable]) + "</h4>";
    content += "<h4>IO 6: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_6]) + "</h4>";
    content += "<h4>IO 7: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_7]) + "</h4>";
    content += "<h4>IO 8: " + String(falseTrue[datalayer_extended.cellpower.IO_state_IO_8]) + "</h4>";
    content += "<h3>Errors:</h3>";
    content +=
        "<h4>Cell overvoltage: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_overvoltage]) + "</h4>";
    content +=
        "<h4>Cell undervoltage: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_undervoltage]) + "</h4>";
    content += "<h4>Cell end of life voltage: " +
               String(falseTrue[datalayer_extended.cellpower.error_Cell_end_of_life_voltage]) + "</h4>";
    content +=
        "<h4>Cell voltage misread: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_voltage_misread]) +
        "</h4>";
    content +=
        "<h4>Cell over temperature: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_over_temperature]) +
        "</h4>";
    content +=
        "<h4>Cell under temperature: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_under_temperature]) +
        "</h4>";
    content += "<h4>Cell unmanaged: " + String(falseTrue[datalayer_extended.cellpower.error_Cell_unmanaged]) + "</h4>";
    content +=
        "<h4>LMU over temperature: " + String(falseTrue[datalayer_extended.cellpower.error_LMU_over_temperature]) +
        "</h4>";
    content +=
        "<h4>LMU under temperature: " + String(falseTrue[datalayer_extended.cellpower.error_LMU_under_temperature]) +
        "</h4>";
    content += "<h4>Temp sensor open circuit: " +
               String(falseTrue[datalayer_extended.cellpower.error_Temp_sensor_open_circuit]) + "</h4>";
    content += "<h4>Temp sensor short circuit: " +
               String(falseTrue[datalayer_extended.cellpower.error_Temp_sensor_short_circuit]) + "</h4>";
    content += "<h4>SUB comm: " + String(falseTrue[datalayer_extended.cellpower.error_SUB_communication]) + "</h4>";
    content += "<h4>LMU comm: " + String(falseTrue[datalayer_extended.cellpower.error_LMU_communication]) + "</h4>";
    content +=
        "<h4>Over current In: " + String(falseTrue[datalayer_extended.cellpower.error_Over_current_IN]) + "</h4>";
    content +=
        "<h4>Over current Out: " + String(falseTrue[datalayer_extended.cellpower.error_Over_current_OUT]) + "</h4>";
    content += "<h4>Short circuit: " + String(falseTrue[datalayer_extended.cellpower.error_Short_circuit]) + "</h4>";
    content += "<h4>Leak detected: " + String(falseTrue[datalayer_extended.cellpower.error_Leak_detected]) + "</h4>";
    content +=
        "<h4>Leak detection failed: " + String(falseTrue[datalayer_extended.cellpower.error_Leak_detection_failed]) +
        "</h4>";
    content +=
        "<h4>Voltage diff: " + String(falseTrue[datalayer_extended.cellpower.error_Voltage_difference]) + "</h4>";
    content += "<h4>BMCU supply overvoltage: " +
               String(falseTrue[datalayer_extended.cellpower.error_BMCU_supply_over_voltage]) + "</h4>";
    content += "<h4>BMCU supply undervoltage: " +
               String(falseTrue[datalayer_extended.cellpower.error_BMCU_supply_under_voltage]) + "</h4>";
    content += "<h4>Main positive contactor: " +
               String(falseTrue[datalayer_extended.cellpower.error_Main_positive_contactor]) + "</h4>";
    content += "<h4>Main negative contactor: " +
               String(falseTrue[datalayer_extended.cellpower.error_Main_negative_contactor]) + "</h4>";
    content += "<h4>Precharge contactor: " + String(falseTrue[datalayer_extended.cellpower.error_Precharge_contactor]) +
               "</h4>";
    content +=
        "<h4>Midpack contactor: " + String(falseTrue[datalayer_extended.cellpower.error_Midpack_contactor]) + "</h4>";
    content +=
        "<h4>Precharge timeout: " + String(falseTrue[datalayer_extended.cellpower.error_Precharge_timeout]) + "</h4>";
    content += "<h4>EMG connector override: " +
               String(falseTrue[datalayer_extended.cellpower.error_Emergency_connector_override]) + "</h4>";
    content += "<h3>Warnings:</h3>";
    content +=
        "<h4>High cell voltage: " + String(falseTrue[datalayer_extended.cellpower.warning_High_cell_voltage]) + "</h4>";
    content +=
        "<h4>Low cell voltage: " + String(falseTrue[datalayer_extended.cellpower.warning_Low_cell_voltage]) + "</h4>";
    content +=
        "<h4>High cell temperature: " + String(falseTrue[datalayer_extended.cellpower.warning_High_cell_temperature]) +
        "</h4>";
    content +=
        "<h4>Low cell temperature: " + String(falseTrue[datalayer_extended.cellpower.warning_Low_cell_temperature]) +
        "</h4>";
    content +=
        "<h4>High LMU temperature: " + String(falseTrue[datalayer_extended.cellpower.warning_High_LMU_temperature]) +
        "</h4>";
    content +=
        "<h4>Low LMU temperature: " + String(falseTrue[datalayer_extended.cellpower.warning_Low_LMU_temperature]) +
        "</h4>";
    content +=
        "<h4>SUB comm interf: " + String(falseTrue[datalayer_extended.cellpower.warning_SUB_communication_interfered]) +
        "</h4>";
    content +=
        "<h4>LMU comm interf: " + String(falseTrue[datalayer_extended.cellpower.warning_LMU_communication_interfered]) +
        "</h4>";
    content +=
        "<h4>High current In: " + String(falseTrue[datalayer_extended.cellpower.warning_High_current_IN]) + "</h4>";
    content +=
        "<h4>High current Out: " + String(falseTrue[datalayer_extended.cellpower.warning_High_current_OUT]) + "</h4>";
    content += "<h4>Pack resistance diff: " +
               String(falseTrue[datalayer_extended.cellpower.warning_Pack_resistance_difference]) + "</h4>";
    content +=
        "<h4>High pack resistance: " + String(falseTrue[datalayer_extended.cellpower.warning_High_pack_resistance]) +
        "</h4>";
    content += "<h4>Cell resistance diff: " +
               String(falseTrue[datalayer_extended.cellpower.warning_Cell_resistance_difference]) + "</h4>";
    content +=
        "<h4>High cell resistance: " + String(falseTrue[datalayer_extended.cellpower.warning_High_cell_resistance]) +
        "</h4>";
    content += "<h4>High BMCU supply voltage: " +
               String(falseTrue[datalayer_extended.cellpower.warning_High_BMCU_supply_voltage]) + "</h4>";
    content += "<h4>Low BMCU supply voltage: " +
               String(falseTrue[datalayer_extended.cellpower.warning_Low_BMCU_supply_voltage]) + "</h4>";
    content += "<h4>Low SOC: " + String(falseTrue[datalayer_extended.cellpower.warning_Low_SOC]) + "</h4>";
    content += "<h4>Balancing required: " +
               String(falseTrue[datalayer_extended.cellpower.warning_Balancing_required_OCV_model]) + "</h4>";
    content += "<h4>Charger not responding: " +
               String(falseTrue[datalayer_extended.cellpower.warning_Charger_not_responding]) + "</h4>";
#endif  //CELLPOWER_BMS

#ifdef KIA_HYUNDAI_64_BATTERY
    content += "<h4>Cells: " + String(datalayer_extended.KiaHyundai64.total_cell_count) + "S</h4>";
    content += "<h4>12V voltage: " + String(datalayer_extended.KiaHyundai64.battery_12V / 10.0, 1) + "</h4>";
    content += "<h4>Waterleakage: " + String(datalayer_extended.KiaHyundai64.waterleakageSensor) + "</h4>";
    content +=
        "<h4>Temperature, water inlet: " + String(datalayer_extended.KiaHyundai64.temperature_water_inlet) + "</h4>";
    content +=
        "<h4>Temperature, power relay: " + String(datalayer_extended.KiaHyundai64.powerRelayTemperature) + "</h4>";
    content += "<h4>Batterymanagement mode: " + String(datalayer_extended.KiaHyundai64.batteryManagementMode) + "</h4>";
    content += "<h4>BMS ignition: " + String(datalayer_extended.KiaHyundai64.BMS_ign) + "</h4>";
    content += "<h4>Battery relay: " + String(datalayer_extended.KiaHyundai64.batteryRelay) + "</h4>";
#endif  //KIA_HYUNDAI_64_BATTERY

#ifdef BYD_ATTO_3_BATTERY
    static const char* SOCmethod[2] = {"Estimated from voltage", "Measured by BMS"};
    content += "<h4>SOC method used: " + String(SOCmethod[datalayer_extended.bydAtto3.SOC_method]) + "</h4>";
    content += "<h4>SOC estimated: " + String(datalayer_extended.bydAtto3.SOC_estimated) + "</h4>";
    content += "<h4>SOC highprec: " + String(datalayer_extended.bydAtto3.SOC_highprec) + "</h4>";
    content += "<h4>SOC OBD2: " + String(datalayer_extended.bydAtto3.SOC_polled) + "</h4>";
    content += "<h4>Voltage periodic: " + String(datalayer_extended.bydAtto3.voltage_periodic) + "</h4>";
    content += "<h4>Voltage OBD2: " + String(datalayer_extended.bydAtto3.voltage_polled) + "</h4>";
#endif  //BYD_ATTO_3_BATTERY

#ifdef TESLA_BATTERY
    float beginning_of_life = static_cast<float>(datalayer_extended.tesla.battery_beginning_of_life);
    float battTempPct = static_cast<float>(datalayer_extended.tesla.battery_battTempPct) * 0.4;
    float dcdcLvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvBusVolt) * 0.0390625;
    float dcdcHvBusVolt = static_cast<float>(datalayer_extended.tesla.battery_dcdcHvBusVolt) * 0.146484;
    float dcdcLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.battery_dcdcLvOutputCurrent) * 0.1;
    float nominal_full_pack_energy =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy) * 0.1;
    float nominal_full_pack_energy_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_full_pack_energy_m0) * 0.02;
    float nominal_energy_remaining =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining) * 0.1;
    float nominal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_nominal_energy_remaining_m0) * 0.02;
    float ideal_energy_remaining = static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining) * 0.1;
    float ideal_energy_remaining_m0 =
        static_cast<float>(datalayer_extended.tesla.battery_ideal_energy_remaining_m0) * 0.02;
    float energy_to_charge_complete =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete) * 0.1;
    float energy_to_charge_complete_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_energy_to_charge_complete_m1) * 0.02;
    float energy_buffer = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer) * 0.1;
    float energy_buffer_m1 = static_cast<float>(datalayer_extended.tesla.battery_energy_buffer_m1) * 0.01;
    float expected_energy_remaining_m1 =
        static_cast<float>(datalayer_extended.tesla.battery_expected_energy_remaining_m1) * 0.02;
    float total_discharge = static_cast<float>(datalayer_extended.tesla.battery_total_discharge);
    float total_charge = static_cast<float>(datalayer_extended.tesla.battery_total_charge);
    float packMass = static_cast<float>(datalayer_extended.tesla.battery_packMass);
    float platformMaxBusVoltage =
        static_cast<float>(datalayer_extended.tesla.battery_platformMaxBusVoltage) * 0.1 + 375;
    float bms_min_voltage = static_cast<float>(datalayer_extended.tesla.battery_bms_min_voltage) * 0.01 * 2;
    float bms_max_voltage = static_cast<float>(datalayer_extended.tesla.battery_bms_max_voltage) * 0.01 * 2;
    float max_charge_current = static_cast<float>(datalayer_extended.tesla.battery_max_charge_current);
    float max_discharge_current = static_cast<float>(datalayer_extended.tesla.battery_max_discharge_current);
    float soc_ave = static_cast<float>(datalayer_extended.tesla.battery_soc_ave) * 0.1;
    float soc_max = static_cast<float>(datalayer_extended.tesla.battery_soc_max) * 0.1;
    float soc_min = static_cast<float>(datalayer_extended.tesla.battery_soc_min) * 0.1;
    float soc_ui = static_cast<float>(datalayer_extended.tesla.battery_soc_ui) * 0.1;
    float BrickVoltageMax = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMax) * 0.002;
    float BrickVoltageMin = static_cast<float>(datalayer_extended.tesla.battery_BrickVoltageMin) * 0.002;
    float BrickModelTMax = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMax) * 0.5 - 40;
    float BrickModelTMin = static_cast<float>(datalayer_extended.tesla.battery_BrickModelTMin) * 0.5 - 40;
    float isolationResistance = static_cast<float>(datalayer_extended.tesla.battery_BMS_isolationResistance) * 10;
    float PCS_dcdcMaxOutputCurrentAllowed =
        static_cast<float>(datalayer_extended.tesla.battery_PCS_dcdcMaxOutputCurrentAllowed) * 0.1;
    float PCS_dcdcTemp = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTemp) * 0.1 + 40;
    float PCS_ambientTemp = static_cast<float>(datalayer_extended.tesla.PCS_ambientTemp) * 0.1 + 40;
    float PCS_chgPhATemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhATemp) * 0.1 + 40;
    float PCS_chgPhBTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhBTemp) * 0.1 + 40;
    float PCS_chgPhCTemp = static_cast<float>(datalayer_extended.tesla.PCS_chgPhCTemp) * 0.1 + 40;
    float BMS_maxRegenPower = static_cast<float>(datalayer_extended.tesla.BMS_maxRegenPower) * 0.01;
    float BMS_maxDischargePower = static_cast<float>(datalayer_extended.tesla.BMS_maxDischargePower) * 0.013;
    float BMS_maxStationaryHeatPower = static_cast<float>(datalayer_extended.tesla.BMS_maxStationaryHeatPower) * 0.01;
    float BMS_hvacPowerBudget = static_cast<float>(datalayer_extended.tesla.BMS_hvacPowerBudget) * 0.02;
    float BMS_powerDissipation = static_cast<float>(datalayer_extended.tesla.BMS_powerDissipation) * 0.02;
    float BMS_flowRequest = static_cast<float>(datalayer_extended.tesla.BMS_flowRequest) * 0.3;
    float BMS_inletActiveCoolTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveCoolTargetT) * 0.25 - 25;
    float BMS_inletPassiveTargetT = static_cast<float>(datalayer_extended.tesla.BMS_inletPassiveTargetT) * 0.25 - 25;
    float BMS_inletActiveHeatTargetT =
        static_cast<float>(datalayer_extended.tesla.BMS_inletActiveHeatTargetT) * 0.25 - 25;
    float BMS_packTMin = static_cast<float>(datalayer_extended.tesla.BMS_packTMin) * 0.25 - 25;
    float BMS_packTMax = static_cast<float>(datalayer_extended.tesla.BMS_packTMax) * 0.25 - 25;
    float PCS_dcdcMaxLvOutputCurrent = static_cast<float>(datalayer_extended.tesla.PCS_dcdcMaxLvOutputCurrent) * 0.1;
    float PCS_dcdcCurrentLimit = static_cast<float>(datalayer_extended.tesla.PCS_dcdcCurrentLimit) * 0.1;
    float PCS_dcdcLvOutputCurrentTempLimit =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcLvOutputCurrentTempLimit) * 0.1;
    float PCS_dcdcUnifiedCommand = static_cast<float>(datalayer_extended.tesla.PCS_dcdcUnifiedCommand) * 0.001;
    float PCS_dcdcCLAControllerOutput =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcCLAControllerOutput * 0.001);
    float PCS_dcdcTankVoltage = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltage);
    float PCS_dcdcTankVoltageTarget = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTankVoltageTarget);
    float PCS_dcdcClaCurrentFreq = static_cast<float>(datalayer_extended.tesla.PCS_dcdcClaCurrentFreq) * 0.0976563;
    float PCS_dcdcTCommMeasured = static_cast<float>(datalayer_extended.tesla.PCS_dcdcTCommMeasured) * 0.00195313;
    float PCS_dcdcShortTimeUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcShortTimeUs) * 0.000488281;
    float PCS_dcdcHalfPeriodUs = static_cast<float>(datalayer_extended.tesla.PCS_dcdcHalfPeriodUs) * 0.000488281;
    float PCS_dcdcIntervalMaxFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxFrequency);
    float PCS_dcdcIntervalMaxHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxHvBusVolt) * 0.1;
    float PCS_dcdcIntervalMaxLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvBusVolt) * 0.1;
    float PCS_dcdcIntervalMaxLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMaxLvOutputCurr);
    float PCS_dcdcIntervalMinFrequency = static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinFrequency);
    float PCS_dcdcIntervalMinHvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinHvBusVolt) * 0.1;
    float PCS_dcdcIntervalMinLvBusVolt =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvBusVolt) * 0.1;
    float PCS_dcdcIntervalMinLvOutputCurr =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdcIntervalMinLvOutputCurr);
    float PCS_dcdc12vSupportLifetimekWh =
        static_cast<float>(datalayer_extended.tesla.PCS_dcdc12vSupportLifetimekWh) * 0.01;
    float HVP_hvp1v5Ref = static_cast<float>(datalayer_extended.tesla.HVP_hvp1v5Ref) * 0.1;
    float HVP_shuntCurrentDebug = static_cast<float>(datalayer_extended.tesla.HVP_shuntCurrentDebug) * 0.1;
    float HVP_dcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkVoltage) * 0.1;
    float HVP_packVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packVoltage) * 0.1;
    float HVP_fcLinkVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkVoltage) * 0.1;
    float HVP_packContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_packContVoltage) * 0.1;
    float HVP_packNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_packNegativeV) * 0.1;
    float HVP_packPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_packPositiveV) * 0.1;
    float HVP_pyroAnalog = static_cast<float>(datalayer_extended.tesla.HVP_pyroAnalog) * 0.1;
    float HVP_dcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkNegativeV) * 0.1;
    float HVP_dcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_dcLinkPositiveV) * 0.1;
    float HVP_fcLinkNegativeV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkNegativeV) * 0.1;
    float HVP_fcContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_fcContCoilCurrent) * 0.1;
    float HVP_fcContVoltage = static_cast<float>(datalayer_extended.tesla.HVP_fcContVoltage) * 0.1;
    float HVP_hvilInVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilInVoltage) * 0.1;
    float HVP_hvilOutVoltage = static_cast<float>(datalayer_extended.tesla.HVP_hvilOutVoltage) * 0.1;
    float HVP_fcLinkPositiveV = static_cast<float>(datalayer_extended.tesla.HVP_fcLinkPositiveV) * 0.1;
    float HVP_packContCoilCurrent = static_cast<float>(datalayer_extended.tesla.HVP_packContCoilCurrent) * 0.1;
    float HVP_battery12V = static_cast<float>(datalayer_extended.tesla.HVP_battery12V) * 0.1;
    float HVP_shuntRefVoltageDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntRefVoltageDbg) * 0.001;
    float HVP_shuntAuxCurrentDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAuxCurrentDbg) * 0.1;
    float HVP_shuntBarTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntBarTempDbg) * 0.01;
    float HVP_shuntAsicTempDbg = static_cast<float>(datalayer_extended.tesla.HVP_shuntAsicTempDbg) * 0.01;

    static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                          "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                          "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
    static const char* hvilStatusState[] = {"NOT Ok",
                                            "STATUS_OK",
                                            "CURRENT_SOURCE_FAULT",
                                            "INTERNAL_OPEN_FAULT",
                                            "VEHICLE_OPEN_FAULT",
                                            "PENTHOUSE_LID_OPEN_FAULT",
                                            "UNKNOWN_LOCATION_OPEN_FAULT",
                                            "VEHICLE_NODE_FAULT",
                                            "NO_12V_SUPPLY",
                                            "VEHICLE_OR_PENTHOUSE_LID_OPENFAULT",
                                            "UNKNOWN(10)",
                                            "UNKNOWN(11)",
                                            "UNKNOWN(12)",
                                            "UNKNOWN(13)",
                                            "UNKNOWN(14)",
                                            "UNKNOWN(15)"};
    static const char* contactorState[] = {"SNA",        "OPEN",       "PRECHARGE",   "BLOCKED",
                                           "PULLED_IN",  "OPENING",    "ECONOMIZED",  "WELDED",
                                           "UNKNOWN(8)", "UNKNOWN(9)", "UNKNOWN(10)", "UNKNOWN(11)"};
    static const char* BMS_state[] = {"STANDBY",     "DRIVE", "SUPPORT", "CHARGE", "FEIM",
                                      "CLEAR_FAULT", "FAULT", "WELD",    "TEST",   "SNA"};
    static const char* BMS_contactorState[] = {"SNA", "OPEN", "OPENING", "CLOSING", "CLOSED", "WELDED", "BLOCKED"};
    static const char* BMS_hvState[] = {"DOWN",          "COMING_UP",        "GOING_DOWN", "UP_FOR_DRIVE",
                                        "UP_FOR_CHARGE", "UP_FOR_DC_CHARGE", "UP"};
    static const char* BMS_uiChargeStatus[] = {"DISCONNECTED", "NO_POWER",        "ABOUT_TO_CHARGE",
                                               "CHARGING",     "CHARGE_COMPLETE", "CHARGE_STOPPED"};
    static const char* PCS_dcdcStatus[] = {"IDLE", "ACTIVE", "FAULTED"};
    static const char* PCS_dcdcMainState[] = {"STANDBY",          "12V_SUPPORT_ACTIVE", "PRECHARGE_STARTUP",
                                              "PRECHARGE_ACTIVE", "DIS_HVBUS_ACTIVE",   "SHUTDOWN",
                                              "FAULTED"};
    static const char* PCS_dcdcSubState[] = {"PWR_UP_INIT",
                                             "STANDBY",
                                             "12V_SUPPORT_ACTIVE",
                                             "DIS_HVBUS",
                                             "PCHG_FAST_DIS_HVBUS",
                                             "PCHG_SLOW_DIS_HVBUS",
                                             "PCHG_DWELL_CHARGE",
                                             "PCHG_DWELL_WAIT",
                                             "PCHG_DI_RECOVERY_WAIT",
                                             "PCHG_ACTIVE",
                                             "PCHG_FLT_FAST_DIS_HVBUS",
                                             "SHUTDOWN",
                                             "12V_SUPPORT_FAULTED",
                                             "DIS_HVBUS_FAULTED",
                                             "PCHG_FAULTED",
                                             "CLEAR_FAULTS",
                                             "FAULTED",
                                             "NUM"};
    static const char* BMS_powerLimitState[] = {"NOT_CALCULATED_FOR_DRIVE", "CALCULATED_FOR_DRIVE"};
    static const char* HVP_status[] = {"INVALID", "NOT_AVAILABLE", "STALE", "VALID"};
    static const char* HVP_contactor[] = {"NOT_ACTIVE", "ACTIVE", "COMPLETED"};
    static const char* falseTrue[] = {"False", "True"};
    static const char* noYes[] = {"No", "Yes"};
    static const char* Fault[] = {"NOT_ACTIVE", "ACTIVE"};
    //Buttons for user action
    content += "<button onclick='askClearIsolation()'>Clear isolation fault</button>";
    //0x20A 522 HVP_contatorState
    content += "<h4>Contactor Status: " + String(contactorText[datalayer_extended.tesla.status_contactor]) + "</h4>";
    content += "<h4>HVIL: " + String(hvilStatusState[datalayer_extended.tesla.hvil_status]) + "</h4>";
    content +=
        "<h4>Negative contactor: " + String(contactorState[datalayer_extended.tesla.packContNegativeState]) + "</h4>";
    content +=
        "<h4>Positive contactor: " + String(contactorState[datalayer_extended.tesla.packContPositiveState]) + "</h4>";
    content += "<h4>Closing allowed?: " + String(noYes[datalayer_extended.tesla.packCtrsClosingAllowed]) + "</h4>";
    content += "<h4>Pyrotest in Progress: " + String(noYes[datalayer_extended.tesla.pyroTestInProgress]) + "</h4>";
    content += "<h4>Contactors Open Now Requested: " +
               String(noYes[datalayer_extended.tesla.battery_packCtrsOpenNowRequested]) + "</h4>";
    content +=
        "<h4>Contactors Open Requested: " + String(noYes[datalayer_extended.tesla.battery_packCtrsOpenRequested]) +
        "</h4>";
    content += "<h4>Contactors Request Status: " +
               String(HVP_contactor[datalayer_extended.tesla.battery_packCtrsRequestStatus]) + "</h4>";
    content += "<h4>Contactors Reset Request Required: " +
               String(noYes[datalayer_extended.tesla.battery_packCtrsResetRequestRequired]) + "</h4>";
    content +=
        "<h4>DC Link Allowed to Energize: " + String(noYes[datalayer_extended.tesla.battery_dcLinkAllowedToEnergize]) +
        "</h4>";
    char readableSerialNumber[15];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.tesla.BMS_SerialNumber,
           sizeof(datalayer_extended.tesla.BMS_SerialNumber));
    readableSerialNumber[14] = '\0';  // Null terminate the string
    content += "<h4>BMS Serial number: " + String(readableSerialNumber) + "</h4>";
    // Comment what data you would like to dislay, order can be changed.
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == false) {
      content += "<h3>BMS 0x352 w/o mux</h3>";  //if using older BMS <2021 and comment 0x352 without MUX
      content += "<h4>Calculated SOH: " + String(nominal_full_pack_energy * 100 / beginning_of_life) + "</h4>";
      content += "<h4>Nominal Full Pack Energy: " + String(nominal_full_pack_energy) + " KWh</h4>";
      content += "<h4>Nominal Energy Remaining: " + String(nominal_energy_remaining) + " KWh</h4>";
      content += "<h4>Ideal Energy Remaining: " + String(ideal_energy_remaining) + " KWh</h4>";
      content += "<h4>Energy to Charge Complete: " + String(energy_to_charge_complete) + " KWh</h4>";
      content += "<h4>Energy Buffer: " + String(energy_buffer) + " KWh</h4>";
      content += "<h4>Full Charge Complete: " + String(noYes[datalayer_extended.tesla.battery_full_charge_complete]) +
                 "</h4>";  //bool
    }
    //0x352 850 BMS_energyStatus
    if (datalayer_extended.tesla.BMS352_mux == true) {
      content += "<h3>BMS 0x352 w/ mux</h3>";  //if using newer BMS >2021 and comment 0x352 with MUX
      content += "<h4>Calculated SOH: " + String(nominal_full_pack_energy_m0 * 100 / beginning_of_life) + "</h4>";
      content += "<h4>Nominal Full Pack Energy: " + String(nominal_full_pack_energy_m0) + " KWh</h4>";
      content += "<h4>Nominal Energy Remaining: " + String(nominal_energy_remaining_m0) + " KWh</h4>";
      content += "<h4>Ideal Energy Remaining: " + String(ideal_energy_remaining_m0) + " KWh</h4>";
      content += "<h4>Energy to Charge Complete: " + String(energy_to_charge_complete_m1) + " KWh</h4>";
      content += "<h4>Energy Buffer: " + String(energy_buffer_m1) + " KWh</h4>";
      content += "<h4>Expected Energy Remaining: " + String(expected_energy_remaining_m1) + " KWh</h4>";
      content += "<h4>Fully Charged: " + String(noYes[datalayer_extended.tesla.battery_fully_charged]) + "</h4>";
    }
    //0x3D2 978 BMS_kwhCounter
    content += "<h4>Total Discharge: " + String(total_discharge) + " KWh</h4>";
    content += "<h4>Total Charge: " + String(total_charge) + " KWh</h4>";
    //0x292 658 BMS_socStates
    content += "<h4>Battery Beginning of Life: " + String(beginning_of_life) + " KWh</h4>";
    content += "<h4>Battery SOC UI: " + String(soc_ui) + " </h4>";
    content += "<h4>Battery SOC Ave: " + String(soc_ave) + " </h4>";
    content += "<h4>Battery SOC Max: " + String(soc_max) + " </h4>";
    content += "<h4>Battery SOC Min: " + String(soc_min) + " </h4>";
    content += "<h4>Battery Temp Percent: " + String(battTempPct) + " </h4>";
    //0x2B4 PCS_dcdcRailStatus
    content += "<h4>PCS Lv Output: " + String(dcdcLvOutputCurrent) + " A</h4>";
    content += "<h4>PCS Lv Bus: " + String(dcdcLvBusVolt) + " V</h4>";
    content += "<h4>PCS Hv Bus: " + String(dcdcHvBusVolt) + " V</h4>";
    //0x392 BMS_packConfig
    //content += "<h4>packConfigMultiplexer: " + String(datalayer_extended.tesla.battery_packConfigMultiplexer) + "</h4>"; // Not giving useable data
    //content += "<h4>moduleType: " + String(datalayer_extended.tesla.battery_moduleType) + "</h4>";  // Not giving useable data
    //content += "<h4>reserveConfig: " + String(datalayer_extended.tesla.battery_reservedConfig) + "</h4>";  // Not giving useable data
    content += "<h4>Battery Pack Mass: " + String(packMass) + " KG</h4>";
    content += "<h4>Platform Max Bus Voltage: " + String(platformMaxBusVoltage) + " V</h4>";
    //0x2D2 722 BMSVAlimits
    content += "<h4>BMS Min Voltage: " + String(bms_min_voltage) + " V</h4>";
    content += "<h4>BMS Max Voltage: " + String(bms_max_voltage) + " V</h4>";
    content += "<h4>Max Charge Current: " + String(max_charge_current) + " A</h4>";
    content += "<h4>Max Discharge Current: " + String(max_discharge_current) + " A</h4>";
    //0x332 818 BMS_bmbMinMax
    content += "<h4>Brick Voltage Max: " + String(BrickVoltageMax) + " V</h4>";
    content += "<h4>Brick Voltage Min: " + String(BrickVoltageMin) + " V</h4>";
    content += "<h4>Brick Temp Max Num: " + String(datalayer_extended.tesla.battery_BrickTempMaxNum) + " </h4>";
    content += "<h4>Brick Temp Min Num: " + String(datalayer_extended.tesla.battery_BrickTempMinNum) + " </h4>";
    //content += "<h4>Brick Model Temp Max: " + String(BrickModelTMax) + " C</h4>";// Not giving useable data
    //content += "<h4>Brick Model Temp Min: " + String(BrickModelTMin) + " C</h4>";// Not giving useable data
    //0x2A4 676 PCS_thermalStatus
    content += "<h4>PCS dcdc Temp: " + String(PCS_dcdcTemp, 2) + " DegC</h4>";
    content += "<h4>PCS Ambient Temp: " + String(PCS_ambientTemp, 2) + " DegC</h4>";
    content += "<h4>PCS Chg PhA Temp: " + String(PCS_chgPhATemp, 2) + " DegC</h4>";
    content += "<h4>PCS Chg PhB Temp: " + String(PCS_chgPhBTemp, 2) + " DegC</h4>";
    content += "<h4>PCS Chg PhC Temp: " + String(PCS_chgPhCTemp, 2) + " DegC</h4>";
    //0x252 594 BMS_powerAvailable
    content += "<h4>Max Regen Power: " + String(BMS_maxRegenPower) + " KW</h4>";
    content += "<h4>Max Discharge Power: " + String(BMS_maxDischargePower) + " KW</h4>";
    //content += "<h4>Max Stationary Heat Power: " + String(BMS_maxStationaryHeatPower) + " KWh</h4>"; // Not giving useable data
    //content += "<h4>HVAC Power Budget: " + String(BMS_hvacPowerBudget) + " KW</h4>"; // Not giving useable data
    //content += "<h4>Not Enough Power For Heat Pump: " + String(falseTrue[datalayer_extended.tesla.BMS_notEnoughPowerForHeatPump]) + "</h4>"; // Not giving useable data
    content +=
        "<h4>Power Limit State: " + String(BMS_powerLimitState[datalayer_extended.tesla.BMS_powerLimitState]) + "</h4>";
    //content += "<h4>Inverter TQF: " + String(datalayer_extended.tesla.BMS_inverterTQF) + "</h4>"; // Not giving useable data
    //0x212 530 BMS_status
    content += "<h4>Isolation Resistance: " + String(isolationResistance) + " kOhms</h4>";
    content +=
        "<h4>BMS Contactor State: " + String(BMS_contactorState[datalayer_extended.tesla.battery_BMS_contactorState]) +
        "</h4>";
    content += "<h4>BMS State: " + String(BMS_state[datalayer_extended.tesla.battery_BMS_state]) + "</h4>";
    content += "<h4>BMS HV State: " + String(BMS_hvState[datalayer_extended.tesla.battery_BMS_hvState]) + "</h4>";
    content += "<h4>BMS UI Charge Status: " + String(BMS_uiChargeStatus[datalayer_extended.tesla.battery_BMS_hvState]) +
               "</h4>";
    content +=
        "<h4>BMS PCS PWM Enabled: " + String(Fault[datalayer_extended.tesla.battery_BMS_pcsPwmEnabled]) + "</h4>";
    //0x312 786 BMS_thermalStatus
    content += "<h4>Power Dissipation: " + String(BMS_powerDissipation) + " kW</h4>";
    content += "<h4>Flow Request: " + String(BMS_flowRequest) + " LPM</h4>";
    content += "<h4>Inlet Active Cool Target Temp: " + String(BMS_inletActiveCoolTargetT) + " DegC</h4>";
    content += "<h4>Inlet Passive Target Temp: " + String(BMS_inletPassiveTargetT) + " DegC</h4>";
    content += "<h4>Inlet Active Heat Target Temp: " + String(BMS_inletActiveHeatTargetT) + " DegC</h4>";
    content += "<h4>Pack Temp Min: " + String(BMS_packTMin) + " DegC</h4>";
    content += "<h4>Pack Temp Max: " + String(BMS_packTMax) + " DegC</h4>";
    content += "<h4>PCS No Flow Request: " + String(Fault[datalayer_extended.tesla.BMS_pcsNoFlowRequest]) + "</h4>";
    content += "<h4>BMS No Flow Request: " + String(Fault[datalayer_extended.tesla.BMS_noFlowRequest]) + "</h4>";
    //0x224 548 PCS_dcdcStatus
    content +=
        "<h4>Precharge Status: " + String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdcPrechargeStatus]) +
        "</h4>";
    content +=
        "<h4>12V Support Status: " + String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdc12VSupportStatus]) +
        "</h4>";
    content += "<h4>HV Bus Discharge Status: " +
               String(PCS_dcdcStatus[datalayer_extended.tesla.battery_PCS_dcdcHvBusDischargeStatus]) + "</h4>";
    content +=
        "<h4>Main State: " + String(PCS_dcdcMainState[datalayer_extended.tesla.battery_PCS_dcdcMainState]) + "</h4>";
    content +=
        "<h4>Sub State: " + String(PCS_dcdcSubState[datalayer_extended.tesla.battery_PCS_dcdcSubState]) + "</h4>";
    content += "<h4>PCS Faulted: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcFaulted]) + "</h4>";
    content +=
        "<h4>Output Is Limited: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcOutputIsLimited]) + "</h4>";
    content += "<h4>Max Output Current Allowed: " + String(PCS_dcdcMaxOutputCurrentAllowed) + " A</h4>";
    content += "<h4>Precharge Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcPrechargeRtyCnt]) +
               "</h4>";
    content +=
        "<h4>12V Support Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdc12VSupportRtyCnt]) +
        "</h4>";
    content += "<h4>Discharge Rty Cnt: " + String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcDischargeRtyCnt]) +
               "</h4>";
    content +=
        "<h4>PWM Enable Line: " + String(Fault[datalayer_extended.tesla.battery_PCS_dcdcPwmEnableLine]) + "</h4>";
    content += "<h4>Supporting Fixed LV Target: " +
               String(Fault[datalayer_extended.tesla.battery_PCS_dcdcSupportingFixedLvTarget]) + "</h4>";
    content += "<h4>Precharge Restart Cnt: " +
               String(falseTrue[datalayer_extended.tesla.battery_PCS_dcdcPrechargeRestartCnt]) + "</h4>";
    content += "<h4>Initial Precharge Substate: " +
               String(PCS_dcdcSubState[datalayer_extended.tesla.battery_PCS_dcdcInitialPrechargeSubState]) + "</h4>";
    //0x2C4 708 PCS_logging
    content += "<h4>PCS_dcdcMaxLvOutputCurrent: " + String(PCS_dcdcMaxLvOutputCurrent) + " A</h4>";
    content += "<h4>PCS_dcdcCurrentLimit: " + String(PCS_dcdcCurrentLimit) + " A</h4>";
    content += "<h4>PCS_dcdcLvOutputCurrentTempLimit: " + String(PCS_dcdcLvOutputCurrentTempLimit) + " A</h4>";
    content += "<h4>PCS_dcdcUnifiedCommand: " + String(PCS_dcdcUnifiedCommand) + "</h4>";
    content += "<h4>PCS_dcdcCLAControllerOutput: " + String(PCS_dcdcCLAControllerOutput) + "</h4>";
    content += "<h4>PCS_dcdcTankVoltage: " + String(PCS_dcdcTankVoltage) + " V</h4>";
    content += "<h4>PCS_dcdcTankVoltageTarget: " + String(PCS_dcdcTankVoltageTarget) + " V</h4>";
    content += "<h4>PCS_dcdcClaCurrentFreq: " + String(PCS_dcdcClaCurrentFreq) + " kHz</h4>";
    content += "<h4>PCS_dcdcTCommMeasured: " + String(PCS_dcdcTCommMeasured) + " us</h4>";
    content += "<h4>PCS_dcdcShortTimeUs: " + String(PCS_dcdcShortTimeUs) + " us</h4>";
    content += "<h4>PCS_dcdcHalfPeriodUs: " + String(PCS_dcdcHalfPeriodUs) + " us</h4>";
    content += "<h4>PCS_dcdcIntervalMaxFrequency: " + String(PCS_dcdcIntervalMaxFrequency) + " kHz</h4>";
    content += "<h4>PCS_dcdcIntervalMaxHvBusVolt: " + String(PCS_dcdcIntervalMaxHvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMaxLvBusVolt: " + String(PCS_dcdcIntervalMaxLvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMaxLvOutputCurr: " + String(PCS_dcdcIntervalMaxLvOutputCurr) + " A</h4>";
    content += "<h4>PCS_dcdcIntervalMinFrequency: " + String(PCS_dcdcIntervalMinFrequency) + " kHz</h4>";
    content += "<h4>PCS_dcdcIntervalMinHvBusVolt: " + String(PCS_dcdcIntervalMinHvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMinLvBusVolt: " + String(PCS_dcdcIntervalMinLvBusVolt) + " V</h4>";
    content += "<h4>PCS_dcdcIntervalMinLvOutputCurr: " + String(PCS_dcdcIntervalMinLvOutputCurr) + " A</h4>";
    content += "<h4>PCS_dcdc12vSupportLifetimekWh: " + String(PCS_dcdc12vSupportLifetimekWh) + " kWh</h4>";
    //0x7AA 1962 HVP_debugMessage
    content += "<h4>HVP_battery12V: " + String(HVP_battery12V) + " V</h4>";
    content += "<h4>HVP_dcLinkVoltage: " + String(HVP_dcLinkVoltage) + " V</h4>";
    content += "<h4>HVP_packVoltage: " + String(HVP_packVoltage) + " V</h4>";
    content += "<h4>HVP_packContVoltage: " + String(HVP_packContVoltage) + " V</h4>";
    content += "<h4>HVP_packContCoilCurrent: " + String(HVP_packContCoilCurrent) + " A</h4>";
    content += "<h4>HVP_pyroAnalog: " + String(HVP_pyroAnalog) + " V</h4>";
    content += "<h4>HVP_hvp1v5Ref: " + String(HVP_hvp1v5Ref) + " V</h4>";
    content += "<h4>HVP_hvilInVoltage: " + String(HVP_hvilInVoltage) + " V</h4>";
    content += "<h4>HVP_hvilOutVoltage: " + String(HVP_hvilOutVoltage) + " V</h4>";
    content +=
        "<h4>HVP_gpioPassivePyroDepl: " + String(Fault[datalayer_extended.tesla.HVP_gpioPassivePyroDepl]) + "</h4>";
    content += "<h4>HVP_gpioPyroIsoEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioPyroIsoEn]) + "</h4>";
    content += "<h4>HVP_gpioCpFaultIn: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpFaultIn]) + "</h4>";
    content +=
        "<h4>HVP_gpioPackContPowerEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioPackContPowerEn]) + "</h4>";
    content += "<h4>HVP_gpioHvCablesOk: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvCablesOk]) + "</h4>";
    content += "<h4>HVP_gpioHvpSelfEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvpSelfEnable]) + "</h4>";
    content += "<h4>HVP_gpioLed: " + String(Fault[datalayer_extended.tesla.HVP_gpioLed]) + "</h4>";
    content += "<h4>HVP_gpioCrashSignal: " + String(Fault[datalayer_extended.tesla.HVP_gpioCrashSignal]) + "</h4>";
    content +=
        "<h4>HVP_gpioShuntDataReady: " + String(Fault[datalayer_extended.tesla.HVP_gpioShuntDataReady]) + "</h4>";
    content += "<h4>HVP_gpioFcContPosAux: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContPosAux]) + "</h4>";
    content += "<h4>HVP_gpioFcContNegAux: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContNegAux]) + "</h4>";
    content += "<h4>HVP_gpioBmsEout: " + String(Fault[datalayer_extended.tesla.HVP_gpioBmsEout]) + "</h4>";
    content += "<h4>HVP_gpioCpFaultOut: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpFaultOut]) + "</h4>";
    content += "<h4>HVP_gpioPyroPor: " + String(Fault[datalayer_extended.tesla.HVP_gpioPyroPor]) + "</h4>";
    content += "<h4>HVP_gpioShuntEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioShuntEn]) + "</h4>";
    content += "<h4>HVP_gpioHvpVerEn: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvpVerEn]) + "</h4>";
    content +=
        "<h4>HVP_gpioPackCoontPosFlywheel: " + String(Fault[datalayer_extended.tesla.HVP_gpioPackCoontPosFlywheel]) +
        "</h4>";
    content += "<h4>HVP_gpioCpLatchEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioCpLatchEnable]) + "</h4>";
    content += "<h4>HVP_gpioPcsEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsEnable]) + "</h4>";
    content +=
        "<h4>HVP_gpioPcsDcdcPwmEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsDcdcPwmEnable]) + "</h4>";
    content += "<h4>HVP_gpioPcsChargePwmEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioPcsChargePwmEnable]) +
               "</h4>";
    content +=
        "<h4>HVP_gpioFcContPowerEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioFcContPowerEnable]) + "</h4>";
    content += "<h4>HVP_gpioHvilEnable: " + String(Fault[datalayer_extended.tesla.HVP_gpioHvilEnable]) + "</h4>";
    content += "<h4>HVP_gpioSecDrdy: " + String(Fault[datalayer_extended.tesla.HVP_gpioSecDrdy]) + "</h4>";
    content += "<h4>HVP_shuntCurrentDebug: " + String(HVP_shuntCurrentDebug) + " A</h4>";
    content += "<h4>HVP_packCurrentMia: " + String(noYes[datalayer_extended.tesla.HVP_packCurrentMia]) + "</h4>";
    content += "<h4>HVP_auxCurrentMia: " + String(noYes[datalayer_extended.tesla.HVP_auxCurrentMia]) + "</h4>";
    content += "<h4>HVP_currentSenseMia: " + String(noYes[datalayer_extended.tesla.HVP_currentSenseMia]) + "</h4>";
    content +=
        "<h4>HVP_shuntRefVoltageMismatch: " + String(noYes[datalayer_extended.tesla.HVP_shuntRefVoltageMismatch]) +
        "</h4>";
    content +=
        "<h4>HVP_shuntThermistorMia: " + String(noYes[datalayer_extended.tesla.HVP_shuntThermistorMia]) + "</h4>";
    content += "<h4>HVP_shuntHwMia: " + String(noYes[datalayer_extended.tesla.HVP_shuntHwMia]) + "</h4>";
    //content += "<h4>HVP_fcLinkVoltage: " + String(HVP_fcLinkVoltage) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_packNegativeV: " + String(HVP_packNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_packPositiveV: " + String(HVP_packPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_dcLinkNegativeV: " + String(HVP_dcLinkNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_dcLinkPositiveV: " + String(HVP_dcLinkPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcLinkNegativeV: " + String(HVP_fcLinkNegativeV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcContCoilCurrent: " + String(HVP_fcContCoilCurrent) + " A</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcContVoltage: " + String(HVP_fcContVoltage) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_fcLinkPositiveV: " + String(HVP_fcLinkPositiveV) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntRefVoltageDbg: " + String(HVP_shuntRefVoltageDbg) + " V</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAuxCurrentDbg: " + String(HVP_shuntAuxCurrentDbg) + " A</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntBarTempDbg: " + String(HVP_shuntBarTempDbg) + " DegC</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAsicTempDbg: " + String(HVP_shuntAsicTempDbg) + " DegC</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAuxCurrentStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntAuxCurrentStatus]) + "</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntBarTempStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntBarTempStatus]) + "</h4>"; // Not giving useable data
    //content += "<h4>HVP_shuntAsicTempStatus: " + String(HVP_status[datalayer_extended.tesla.HVP_shuntAsicTempStatus]) + "</h4>"; // Not giving useable data
#endif

#ifdef NISSAN_LEAF_BATTERY
    static const char* LEAFgen[] = {"ZE0", "AZE0", "ZE1"};
    content += "<h4>LEAF generation: " + String(LEAFgen[datalayer_extended.nissanleaf.LEAF_gen]) + "</h4>";
    char readableSerialNumber[16];  // One extra space for null terminator
    memcpy(readableSerialNumber, datalayer_extended.nissanleaf.BatterySerialNumber,
           sizeof(datalayer_extended.nissanleaf.BatterySerialNumber));
    readableSerialNumber[15] = '\0';  // Null terminate the string
    content += "<h4>Serial number: " + String(readableSerialNumber) + "</h4>";
    char readablePartNumber[8];  // One extra space for null terminator
    memcpy(readablePartNumber, datalayer_extended.nissanleaf.BatteryPartNumber,
           sizeof(datalayer_extended.nissanleaf.BatteryPartNumber));
    readablePartNumber[7] = '\0';  // Null terminate the string
    content += "<h4>Part number: " + String(readablePartNumber) + "</h4>";
    char readableBMSID[9];  // One extra space for null terminator
    memcpy(readableBMSID, datalayer_extended.nissanleaf.BMSIDcode, sizeof(datalayer_extended.nissanleaf.BMSIDcode));
    readableBMSID[8] = '\0';  // Null terminate the string
    content += "<h4>BMS ID: " + String(readableBMSID) + "</h4>";
    content += "<h4>GIDS: " + String(datalayer_extended.nissanleaf.GIDS) + "</h4>";
    content += "<h4>Regen kW: " + String(datalayer_extended.nissanleaf.ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(datalayer_extended.nissanleaf.MaxPowerForCharger) + "</h4>";
    content += "<h4>Interlock: " + String(datalayer_extended.nissanleaf.Interlock) + "</h4>";
    content += "<h4>Insulation: " + String(datalayer_extended.nissanleaf.Insulation) + "</h4>";
    content += "<h4>Relay cut request: " + String(datalayer_extended.nissanleaf.RelayCutRequest) + "</h4>";
    content += "<h4>Failsafe status: " + String(datalayer_extended.nissanleaf.FailsafeStatus) + "</h4>";
    content += "<h4>Fully charged: " + String(datalayer_extended.nissanleaf.Full) + "</h4>";
    content += "<h4>Battery empty: " + String(datalayer_extended.nissanleaf.Empty) + "</h4>";
    content += "<h4>Main relay ON: " + String(datalayer_extended.nissanleaf.MainRelayOn) + "</h4>";
    content += "<h4>Heater present: " + String(datalayer_extended.nissanleaf.HeatExist) + "</h4>";
    content += "<h4>Heating stopped: " + String(datalayer_extended.nissanleaf.HeatingStop) + "</h4>";
    content += "<h4>Heating started: " + String(datalayer_extended.nissanleaf.HeatingStart) + "</h4>";
    content += "<h4>Heating requested: " + String(datalayer_extended.nissanleaf.HeaterSendRequest) + "</h4>";
    content += "<button onclick='askResetSOH()'>Reset degradation data</button>";
    content += "<h4>CryptoChallenge: " + String(datalayer_extended.nissanleaf.CryptoChallenge) + "</h4>";
    content += "<h4>SolvedChallenge: " + String(datalayer_extended.nissanleaf.SolvedChallengeMSB) +
               String(datalayer_extended.nissanleaf.SolvedChallengeLSB) + "</h4>";
    content += "<h4>Challenge failed: " + String(datalayer_extended.nissanleaf.challengeFailed) + "</h4>";
#endif

#ifdef MEB_BATTERY
    content += datalayer_extended.meb.SDSW ? "<h4>Service disconnect switch: Missing!</h4>"
                                           : "<h4>Service disconnect switch: OK</h4>";
    content += datalayer_extended.meb.pilotline ? "<h4>Pilotline: Open!</h4>" : "<h4>Pilotline: OK</h4>";
    content += datalayer_extended.meb.transportmode ? "<h4>Transportmode: Locked!</h4>" : "<h4>Transportmode: OK</h4>";
    content += datalayer_extended.meb.shutdown_active ? "<h4>Shutdown: Active!</h4>" : "<h4>Shutdown: No</h4>";
    content += datalayer_extended.meb.componentprotection ? "<h4>Component protection: Active!</h4>"
                                                          : "<h4>Component protection: No</h4>";
    content += "<h4>HVIL status: ";
    switch (datalayer_extended.meb.HVIL) {
      case 0:
        content += String("Init");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("Open!");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>KL30C status: ";
    switch (datalayer_extended.meb.BMS_Kl30c_Status) {
      case 0:
        content += String("Init");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("Open!");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>BMS mode: ";
    switch (datalayer_extended.meb.BMS_mode) {
      case 0:
        content += String("HV inactive");
        break;
      case 1:
        content += String("HV active");
        break;
      case 2:
        content += String("Balancing");
        break;
      case 3:
        content += String("Extern charging");
        break;
      case 4:
        content += String("AC charging");
        break;
      case 5:
        content += String("Battery error");
        break;
      case 6:
        content += String("DC charging");
        break;
      case 7:
        content += String("Init");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>Diagnostic: ";
    switch (datalayer_extended.meb.battery_diagnostic) {
      case 0:
        content += String("Init");
        break;
      case 1:
        content += String("Battery display");
        break;
      case 4:
        content += String("Battery display OK");
        break;
      case 6:
        content += String("Battery display check");
        break;
      case 7:
        content += String("Fault");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>HV line status: ";
    switch (datalayer_extended.meb.status_HV_line) {
      case 0:
        content += String("Init");
        break;
      case 1:
        content += String("No open HV line detected");
        break;
      case 2:
        content += String("Open HV line");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("? ") + String(datalayer_extended.meb.status_HV_line);
    }
    content += "</h4><h4>Warning support: ";
    switch (datalayer_extended.meb.warning_support) {
      case 0:
        content += String("OK");
        break;
      case 1:
        content += String("Not OK");
        break;
      case 6:
        content += String("Init");
        break;
      case 7:
        content += String("Fault");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>Interm. Voltage (" + String(datalayer_extended.meb.BMS_voltage_intermediate_dV / 10.0, 1) +
               "V) status: ";
    switch (datalayer_extended.meb.BMS_status_voltage_free) {
      case 0:
        content += String("Init");
        break;
      case 1:
        content += String("BMS interm circuit voltage free (U<20V)");
        break;
      case 2:
        content += String("BMS interm circuit not voltage free (U >= 25V)");
        break;
      case 3:
        content += String("Error");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>BMS error status: ";
    switch (datalayer_extended.meb.BMS_error_status) {
      case 0:
        content += String("Component IO");
        break;
      case 1:
        content += String("Iso Error 1");
        break;
      case 2:
        content += String("Iso Error 2");
        break;
      case 3:
        content += String("Interlock");
        break;
      case 4:
        content += String("SD");
        break;
      case 5:
        content += String("Performance red");
        break;
      case 6:
        content += String("No component function");
        break;
      case 7:
        content += String("Init");
        break;
      default:
        content += String("?");
    }
    content += "</h4><h4>BMS voltage: " + String(datalayer_extended.meb.BMS_voltage_dV / 10.0, 1) + "</h4>";
    content += datalayer_extended.meb.BMS_OBD_MIL ? "<h4>OBD MIL: ON!</h4>" : "<h4>OBD MIL: Off</h4>";
    content +=
        datalayer_extended.meb.BMS_error_lamp_req ? "<h4>Red error lamp: ON!</h4>" : "<h4>Red error lamp: Off</h4>";
    content += datalayer_extended.meb.BMS_warning_lamp_req ? "<h4>Yellow warning lamp: ON!</h4>"
                                                           : "<h4>Yellow warning lamp: Off</h4>";
    content += "<h4>Isolation resistance: " + String(datalayer_extended.meb.isolation_resistance) + " kOhm</h4>";
    content +=
        datalayer_extended.meb.battery_heating ? "<h4>Battery heating: Active!</h4>" : "<h4>Battery heating: Off</h4>";
    const char* rt_enum[] = {"No", "Error level 1", "Error level 2", "Error level 3"};
    content += "<h4>Overcurrent: " + String(rt_enum[datalayer_extended.meb.rt_overcurrent & 0x03]) + "</h4>";
    content += "<h4>CAN fault: " + String(rt_enum[datalayer_extended.meb.rt_CAN_fault & 0x03]) + "</h4>";
    content += "<h4>Overcharged: " + String(rt_enum[datalayer_extended.meb.rt_overcharge & 0x03]) + "</h4>";
    content += "<h4>SOC too high: " + String(rt_enum[datalayer_extended.meb.rt_SOC_high & 0x03]) + "</h4>";
    content += "<h4>SOC too low: " + String(rt_enum[datalayer_extended.meb.rt_SOC_low & 0x03]) + "</h4>";
    content += "<h4>SOC jumping: " + String(rt_enum[datalayer_extended.meb.rt_SOC_jumping & 0x03]) + "</h4>";
    content += "<h4>Temp difference: " + String(rt_enum[datalayer_extended.meb.rt_temp_difference & 0x03]) + "</h4>";
    content += "<h4>Cell overtemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_overtemp & 0x03]) + "</h4>";
    content += "<h4>Cell undertemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_undertemp & 0x03]) + "</h4>";
    content +=
        "<h4>Battery overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_overvolt & 0x03]) + "</h4>";
    content +=
        "<h4>Battery undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_undervol & 0x03]) + "</h4>";
    content += "<h4>Cell overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_overvolt & 0x03]) + "</h4>";
    content += "<h4>Cell undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_undervol & 0x03]) + "</h4>";
    content += "<h4>Cell imbalance: " + String(rt_enum[datalayer_extended.meb.rt_cell_imbalance & 0x03]) + "</h4>";
    content +=
        "<h4>Battery unathorized: " + String(rt_enum[datalayer_extended.meb.rt_battery_unathorized & 0x03]) + "</h4>";
#endif  //MEB_BATTERY

#ifdef RENAULT_ZOE_GEN2_BATTERY
    content += "<h4>soc: " + String(datalayer_extended.zoePH2.battery_soc) + "</h4>";
    content += "<h4>usable soc: " + String(datalayer_extended.zoePH2.battery_usable_soc) + "</h4>";
    content += "<h4>soh: " + String(datalayer_extended.zoePH2.battery_soh) + "</h4>";
    content += "<h4>pack voltage: " + String(datalayer_extended.zoePH2.battery_pack_voltage) + "</h4>";
    content += "<h4>max cell voltage: " + String(datalayer_extended.zoePH2.battery_max_cell_voltage) + "</h4>";
    content += "<h4>min cell voltage: " + String(datalayer_extended.zoePH2.battery_min_cell_voltage) + "</h4>";
    content += "<h4>12v: " + String(datalayer_extended.zoePH2.battery_12v) + "</h4>";
    content += "<h4>avg temp: " + String(datalayer_extended.zoePH2.battery_avg_temp) + "</h4>";
    content += "<h4>min temp: " + String(datalayer_extended.zoePH2.battery_min_temp) + "</h4>";
    content += "<h4>max temp: " + String(datalayer_extended.zoePH2.battery_max_temp) + "</h4>";
    content += "<h4>max power: " + String(datalayer_extended.zoePH2.battery_max_power) + "</h4>";
    content += "<h4>interlock: " + String(datalayer_extended.zoePH2.battery_interlock) + "</h4>";
    content += "<h4>kwh: " + String(datalayer_extended.zoePH2.battery_kwh) + "</h4>";
    content += "<h4>current: " + String(datalayer_extended.zoePH2.battery_current) + "</h4>";
    content += "<h4>current offset: " + String(datalayer_extended.zoePH2.battery_current_offset) + "</h4>";
    content += "<h4>max generated: " + String(datalayer_extended.zoePH2.battery_max_generated) + "</h4>";
    content += "<h4>max available: " + String(datalayer_extended.zoePH2.battery_max_available) + "</h4>";
    content += "<h4>current voltage: " + String(datalayer_extended.zoePH2.battery_current_voltage) + "</h4>";
    content += "<h4>charging status: " + String(datalayer_extended.zoePH2.battery_charging_status) + "</h4>";
    content += "<h4>remaining charge: " + String(datalayer_extended.zoePH2.battery_remaining_charge) + "</h4>";
    content +=
        "<h4>balance capacity total: " + String(datalayer_extended.zoePH2.battery_balance_capacity_total) + "</h4>";
    content += "<h4>balance time total: " + String(datalayer_extended.zoePH2.battery_balance_time_total) + "</h4>";
    content +=
        "<h4>balance capacity sleep: " + String(datalayer_extended.zoePH2.battery_balance_capacity_sleep) + "</h4>";
    content += "<h4>balance time sleep: " + String(datalayer_extended.zoePH2.battery_balance_time_sleep) + "</h4>";
    content +=
        "<h4>balance capacity wake: " + String(datalayer_extended.zoePH2.battery_balance_capacity_wake) + "</h4>";
    content += "<h4>balance time wake: " + String(datalayer_extended.zoePH2.battery_balance_time_wake) + "</h4>";
    content += "<h4>bms state: " + String(datalayer_extended.zoePH2.battery_bms_state) + "</h4>";
    content += "<h4>balance switches: " + String(datalayer_extended.zoePH2.battery_balance_switches) + "</h4>";
    content += "<h4>energy complete: " + String(datalayer_extended.zoePH2.battery_energy_complete) + "</h4>";
    content += "<h4>energy partial: " + String(datalayer_extended.zoePH2.battery_energy_partial) + "</h4>";
    content += "<h4>slave failures: " + String(datalayer_extended.zoePH2.battery_slave_failures) + "</h4>";
    content += "<h4>mileage: " + String(datalayer_extended.zoePH2.battery_mileage) + "</h4>";
    content += "<h4>fan speed: " + String(datalayer_extended.zoePH2.battery_fan_speed) + "</h4>";
    content += "<h4>fan period: " + String(datalayer_extended.zoePH2.battery_fan_period) + "</h4>";
    content += "<h4>fan control: " + String(datalayer_extended.zoePH2.battery_fan_control) + "</h4>";
    content += "<h4>fan duty: " + String(datalayer_extended.zoePH2.battery_fan_duty) + "</h4>";
    content += "<h4>temporisation: " + String(datalayer_extended.zoePH2.battery_temporisation) + "</h4>";
    content += "<h4>time: " + String(datalayer_extended.zoePH2.battery_time) + "</h4>";
    content += "<h4>pack time: " + String(datalayer_extended.zoePH2.battery_pack_time) + "</h4>";
    content += "<h4>soc min: " + String(datalayer_extended.zoePH2.battery_soc_min) + "</h4>";
    content += "<h4>soc max: " + String(datalayer_extended.zoePH2.battery_soc_max) + "</h4>";
#endif  //RENAULT_ZOE_GEN2_BATTERY

#ifdef VOLVO_SPA_BATTERY
    content += "<h4>BECM reported SOC: " + String(datalayer_extended.VolvoPolestar.soc_bms) + "</h4>";
    content += "<h4>Calculated SOC: " + String(datalayer_extended.VolvoPolestar.soc_calc) + "</h4>";
    content += "<h4>Rescaled SOC: " + String(datalayer_extended.VolvoPolestar.soc_rescaled / 10) + "</h4>";
    content += "<h4>BECM reported SOH: " + String(datalayer_extended.VolvoPolestar.soh_bms) + "</h4>";
    content += "<h4>BECM supply voltage: " + String(datalayer_extended.VolvoPolestar.BECMsupplyVoltage) + " mV</h4>";

    content += "<h4>HV voltage: " + String(datalayer_extended.VolvoPolestar.BECMBatteryVoltage) + " V</h4>";
    content += "<h4>HV current: " + String(datalayer_extended.VolvoPolestar.BECMBatteryCurrent) + " A</h4>";
    content += "<h4>Dynamic max voltage: " + String(datalayer_extended.VolvoPolestar.BECMUDynMaxLim) + " V</h4>";
    content += "<h4>Dynamic min voltage: " + String(datalayer_extended.VolvoPolestar.BECMUDynMinLim) + " V</h4>";

    content +=
        "<h4>Discharge power limit 1: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDcha1) + " kW</h4>";
    content +=
        "<h4>Discharge soft power limit: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSoft) + " kW</h4>";
    content +=
        "<h4>Discharge power limit slow aging: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimDchaSlowAgi) +
        " kW</h4>";
    content +=
        "<h4>Charge power limit slow aging: " + String(datalayer_extended.VolvoPolestar.HvBattPwrLimChrgSlowAgi) +
        " kW</h4>";

    content += "<h4>HV system relay status: ";
    switch (datalayer_extended.VolvoPolestar.HVSysRlySts) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("OpenAndRequestActiveDischarge");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 1: ";
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts1) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system relay status 2: ";
    switch (datalayer_extended.VolvoPolestar.HVSysDCRlySts2) {
      case 0:
        content += String("Open");
        break;
      case 1:
        content += String("Closed");
        break;
      case 2:
        content += String("KeepStatus");
        break;
      case 3:
        content += String("Fault");
        break;
      default:
        content += String("Not valid");
    }
    content += "</h4><h4>HV system isolation resistance monitoring status: ";
    switch (datalayer_extended.VolvoPolestar.HVSysIsoRMonrSts) {
      case 0:
        content += String("Not valid 1");
        break;
      case 1:
        content += String("False");
        break;
      case 2:
        content += String("True");
        break;
      case 3:
        content += String("Not valid 2");
        break;
      default:
        content += String("Not valid");
    }

    content += "<br><br><button onclick='Volvo_askEraseDTC()'>Erase DTC</button><br>";
    content += "<button onclick='Volvo_askReadDTC()'>Read DTC (result must be checked in CANlog)</button><br>";
    content += "<button onclick='Volvo_BECMecuReset()'>Restart BECM module</button>";
#endif  // VOLVO_SPA_BATTERY

#if !defined(BMW_PHEV_BATTERY) && !defined(BMW_IX_BATTERY) && !defined(BOLT_AMPERA_BATTERY) &&       \
    !defined(TESLA_BATTERY) && !defined(NISSAN_LEAF_BATTERY) && !defined(BMW_I3_BATTERY) &&          \
    !defined(BYD_ATTO_3_BATTERY) && !defined(RENAULT_ZOE_GEN2_BATTERY) && !defined(CELLPOWER_BMS) && \
    !defined(MEB_BATTERY) && !defined(VOLVO_SPA_BATTERY) &&                                          \
    !defined(KIA_HYUNDAI_64_BATTERY)  //Only the listed types have extra info
    content += "No extra information available for this battery type";
#endif

    content += "</div>";
    content += "<script>";
    content +=
        "function askClearIsolation() { if (window.confirm('Are you sure you want to clear any active isolation "
        "fault?')) { "
        "clearIsolation(); } }";
    content += "function clearIsolation() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/clearIsolation', true);";
    content += "  xhr.send();";
    content += "}";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    content += "<script>";
    content +=
        "function askResetSOH() { if (window.confirm('Are you sure you want to reset degradation data? "
        "Note this should only be used on 2011-2017 24/30kWh batteries!')) { "
        "resetSOH(); } }";
    content += "function resetSOH() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/resetSOH', true);";
    content += "  xhr.send();";
    content += "}";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    content += "<script>";
    content +=
        "function Volvo_askEraseDTC() { if (window.confirm('Are you sure you want to erase DTCs?')) { "
        "volvoEraseDTC(); } }";
    content += "function volvoEraseDTC() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/volvoEraseDTC', true);";
    content += "  xhr.send();";
    content += "}";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";

    content += "<script>";
    content += "function Volvo_askReadDTC() { volvoReadDTC(); } ";
    content += "function volvoReadDTC() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/volvoReadDTC', true);";
    content += "  xhr.send();";
    content += "}";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";

    content += "<script>";
    content +=
        "function Volvo_BECMecuReset() { if (window.confirm('Are you sure you want to restart BECM?')) { "
        "volvoBECMecuReset(); } }";
    content += "function volvoBECMecuReset() {";
    content += "  var xhr = new XMLHttpRequest();";
    content += "  xhr.open('GET', '/volvoBECMecuReset', true);";
    content += "  xhr.send();";
    content += "}";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    // Additial functions added
    content += "<script>";
    content += "function exportLog() { window.location.href = '/export_log'; }";
    content += "</script>";

    return content;
  }
  return String();
}
