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

    // Comment what data you would like to dislay, order can be changed.
    content += "<h4>Battery Beginning of Life: " + String(beginning_of_life) + " kWh</h4>";
    content += "<h4>BattTempPct: " + String(battTempPct) + " </h4>";
    content += "<h4>PCS Lv Bus: " + String(dcdcLvBusVolt) + " V</h4>";
    content += "<h4>PCS Hv Bus: " + String(dcdcHvBusVolt) + " V</h4>";
    content += "<h4>PCS Lv Output: " + String(dcdcLvOutputCurrent) + " A</h4>";

    //if using older BMS <2021 and comment 0x352 without MUX
    //content += "<h4>Nominal Full Pack Energy: " + String(nominal_full_pack_energy) + " kWh</h4>";
    //content += "<h4>Nominal Energy Remaining: " + String(nominal_energy_remaining) + " kWh</h4>";
    //content += "<h4>Ideal Energy Remaining: " + String(ideal_energy_remaining) + " kWh</h4>";
    //content += "<h4>Energy to Charge Complete: " + String(energy_to_charge_complete) + " kWh</h4>";
    //content += "<h4>Energy Buffer: " + String(energy_buffer) + " kWh</h4>";

    //if using newer BMS >2021 and comment 0x352 with MUX
    content += "<h4>Nominal Full Pack Energy m0: " + String(nominal_full_pack_energy_m0) + " kWh</h4>";
    content += "<h4>Nominal Energy Remaining m0: " + String(nominal_energy_remaining_m0) + " kWh</h4>";
    content += "<h4>Ideal Energy Remaining m0: " + String(ideal_energy_remaining_m0) + " kWh</h4>";
    content += "<h4>Energy to Charge Complete m1: " + String(energy_to_charge_complete_m1) + " kWh</h4>";
    content += "<h4>Energy Buffer m1: " + String(energy_buffer_m1) + " kWh</h4>";

    content += "<h4>packConfigMultiplexer: " + String(datalayer_extended.tesla.battery_packConfigMultiplexer) + "</h4>";
    content += "<h4>moduleType: " + String(datalayer_extended.tesla.battery_moduleType) + "</h4>";
    content += "<h4>reserveConfig: " + String(datalayer_extended.tesla.battery_reservedConfig) + "</h4>";
    content += "<h4>Full Charge Complete: " + String(datalayer_extended.tesla.battery_full_charge_complete) + "</h4>";
    content += "<h4>Total Discharge: " + String(total_discharge) + " kWh</h4>";
    content += "<h4>Total Charge: " + String(total_charge) + " kWh</h4>";
    content += "<h4>Battery Pack Mass: " + String(packMass) + " KG</h4>";
    content += "<h4>Platform Max Bus Voltage: " + String(platformMaxBusVoltage) + " V</h4>";
    content += "<h4>BMS Min Voltage: " + String(bms_min_voltage) + " V</h4>";
    content += "<h4>BMS Max Voltage: " + String(bms_max_voltage) + " V</h4>";
    content += "<h4>Max Charge Current: " + String(max_charge_current) + " A</h4>";
    content += "<h4>Max Discharge Current: " + String(max_discharge_current) + " A</h4>";
    content += "<h4>Battery SOC Ave: " + String(soc_ave) + " </h4>";
    content += "<h4>Battery SOC Max: " + String(soc_max) + " </h4>";
    content += "<h4>Battery SOC Min: " + String(soc_min) + " </h4>";
    content += "<h4>Battery SOC UI: " + String(soc_ui) + " </h4>";

    static const char* contactorText[] = {"UNKNOWN(0)",  "OPEN",        "CLOSING",    "BLOCKED", "OPENING",
                                          "CLOSED",      "UNKNOWN(6)",  "WELDED",     "POS_CL",  "NEG_CL",
                                          "UNKNOWN(10)", "UNKNOWN(11)", "UNKNOWN(12)"};
    content += "<h4>Contactor Status: " + String(contactorText[datalayer_extended.tesla.status_contactor]) + "</h4>";
    static const char* hvilStatusState[] = {"NOT OK",
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
    content += "<h4>HVIL: " + String(hvilStatusState[datalayer_extended.tesla.hvil_status]) + "</h4>";
    static const char* contactorState[] = {"SNA",        "OPEN",       "PRECHARGE",   "BLOCKED",
                                           "PULLED_IN",  "OPENING",    "ECONOMIZED",  "WELDED",
                                           "UNKNOWN(8)", "UNKNOWN(9)", "UNKNOWN(10)", "UNKNOWN(11)"};
    content +=
        "<h4>Negative contactor: " + String(contactorState[datalayer_extended.tesla.packContNegativeState]) + "</h4>";
    content +=
        "<h4>Positive contactor: " + String(contactorState[datalayer_extended.tesla.packContPositiveState]) + "</h4>";
    static const char* falseTrue[] = {"False", "True"};
    content += "<h4>Closing allowed?: " + String(falseTrue[datalayer_extended.tesla.packCtrsClosingAllowed]) + "</h4>";
    content += "<h4>Pyrotest: " + String(falseTrue[datalayer_extended.tesla.pyroTestInProgress]) + "</h4>";
#endif

#ifdef NISSAN_LEAF_BATTERY
    static const char* LEAFgen[] = {"ZE0", "AZE0", "ZE1"};
    content += "<h4>LEAF generation: " + String(LEAFgen[datalayer_extended.nissanleaf.LEAF_gen]) + "</h4>";
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
    content += "<h4>Overcurrent: " + String(rt_enum[datalayer_extended.meb.rt_overcurrent]) + "</h4>";
    content += "<h4>CAN fault: " + String(rt_enum[datalayer_extended.meb.rt_CAN_fault]) + "</h4>";
    content += "<h4>Overcharged: " + String(rt_enum[datalayer_extended.meb.rt_overcharge]) + "</h4>";
    content += "<h4>SOC too high: " + String(rt_enum[datalayer_extended.meb.rt_SOC_high]) + "</h4>";
    content += "<h4>SOC too low: " + String(rt_enum[datalayer_extended.meb.rt_SOC_low]) + "</h4>";
    content += "<h4>SOC jumping: " + String(rt_enum[datalayer_extended.meb.rt_SOC_jumping]) + "</h4>";
    content += "<h4>Temp difference: " + String(rt_enum[datalayer_extended.meb.rt_temp_difference]) + "</h4>";
    content += "<h4>Cell overtemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_overtemp]) + "</h4>";
    content += "<h4>Cell undertemp: " + String(rt_enum[datalayer_extended.meb.rt_cell_undertemp]) + "</h4>";
    content += "<h4>Battery overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_overvolt]) + "</h4>";
    content += "<h4>Battery undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_battery_undervol]) + "</h4>";
    content += "<h4>Cell overvoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_overvolt]) + "</h4>";
    content += "<h4>Cell undervoltage: " + String(rt_enum[datalayer_extended.meb.rt_cell_undervol]) + "</h4>";
    content += "<h4>Cell imbalance: " + String(rt_enum[datalayer_extended.meb.rt_cell_imbalance]) + "</h4>";
    content += "<h4>Battery unathorized: " + String(rt_enum[datalayer_extended.meb.rt_battery_unathorized]) + "</h4>";
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

#if !defined(TESLA_BATTERY) && !defined(NISSAN_LEAF_BATTERY) && !defined(BMW_I3_BATTERY) &&          \
    !defined(BYD_ATTO_3_BATTERY) && !defined(RENAULT_ZOE_GEN2_BATTERY) && !defined(CELLPOWER_BMS) && \
    !defined(MEB_BATTERY)  //Only the listed types have extra info
    content += "No extra information available for this battery type";
#endif

    content += "</div>";

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
    return content;
  }
  return String();
}
