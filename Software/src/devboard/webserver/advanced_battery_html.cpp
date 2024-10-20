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
    content += "</style>";

    content += "<button onclick='goToMainPage()'>Back to main page</button>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

#ifdef TESLA_BATTERY
    content += "<h4>Contactor Status: " + String(datalayer_extended.tesla.status_contactor) + "</h4>";
    content += "<h4>HVIL: " + String(datalayer_extended.tesla.hvil_status) + "</h4>";
    content += "<h4>Negative contactor: " + String(datalayer_extended.tesla.packContNegativeState) + "</h4>";
    content += "<h4>Positive contactor: " + String(datalayer_extended.tesla.packContPositiveState) + "</h4>";
    content += "<h4>Contactor set state: " + String(datalayer_extended.tesla.packContactorSetState) + "</h4>";
    content += "<h4>Closing allowed?: " + String(datalayer_extended.tesla.packCtrsClosingAllowed) + "</h4>";
    content += "<h4>Pyrotest: " + String(datalayer_extended.tesla.pyroTestInProgress) + "</h4>";
#endif

#ifdef NISSAN_LEAF_BATTERY
    content += "<h4>LEAF generation: " + String(datalayer_extended.nissanleaf.LEAF_gen) + "</h4>";
    content += "<h4>GIDS: " + String(datalayer_extended.nissanleaf.GIDS) + "</h4>";
    content += "<h4>Regen kW: " + String(datalayer_extended.nissanleaf.ChargePowerLimit) + "</h4>";
    content += "<h4>Charge kW: " + String(datalayer_extended.nissanleaf.MaxPowerForCharger) + "</h4>";
    content += "<h4>Interlock: " + String(datalayer_extended.nissanleaf.Interlock) + "</h4>";
    content += "<h4>Relay cut request: " + String(datalayer_extended.nissanleaf.RelayCutRequest) + "</h4>";
    content += "<h4>Failsafe status: " + String(datalayer_extended.nissanleaf.FailsafeStatus) + "</h4>";
    content += "<h4>Fully charged: " + String(datalayer_extended.nissanleaf.Full) + "</h4>";
    content += "<h4>Battery empty: " + String(datalayer_extended.nissanleaf.Empty) + "</h4>";
    content += "<h4>Main relay ON: " + String(datalayer_extended.nissanleaf.MainRelayOn) + "</h4>";
    content += "<h4>Heater present: " + String(datalayer_extended.nissanleaf.HeatExist) + "</h4>";
    content += "<h4>Heating stopped: " + String(datalayer_extended.nissanleaf.HeatingStop) + "</h4>";
    content += "<h4>Heating started: " + String(datalayer_extended.nissanleaf.HeatingStart) + "</h4>";
    content += "<h4>Heating requested: " + String(datalayer_extended.nissanleaf.HeaterSendRequest) + "</h4>";
#endif

#ifdef MEB_BATTERY
    content += datalayer_extended.meb.SDSW ? "<h4>Service disconnect switch: Missing!</h4>"
                                           : "<h4>Service disconnect switch: OK</h4>";
    content += datalayer_extended.meb.pilotline ? "<h4>Pilotline: Open!</h4>" : "<h4>Pilotline: OK</h4>";
    content += datalayer_extended.meb.transportmode ? "<h4>Transportmode: Locked!</h4>" : "<h4>Transportmode: OK</h4>";
    content += datalayer_extended.meb.shutdown_active ? "<h4>Shutdown: Active!</h4>" : "<h4>Shutdown: No</h4>";
    content += datalayer_extended.meb.componentprotection ? "<h4>Component protection: Active!</h4>"
                                                          : "<h4>Component protection: No</h4>";
    const char* HVIL_status[] = {"Init", "Closed", "Open!", "Fault"};
    content += "<h4>HVIL status: " + String(HVIL_status[datalayer_extended.meb.HVIL]) + "</h4>";
    const char* BMS_modes[] = {"HV inactive", "HV active",     "Balancing",   "Extern charging",
                               "AC charging", "Battery error", "DC charging", "Init"};
    content += "<h4>BMS mode: " + String(BMS_modes[datalayer_extended.meb.BMS_mode]) + "</h4>";
    const char* diagnostic_modes[] = {"Init", "Battery display",       "",     "", "Battery display OK",
                                      "",     "Display battery check", "Fault"};
    content += "<h4>Diagnostic: " + String(diagnostic_modes[datalayer_extended.meb.battery_diagnostic]) + "</h4>";
    const char* HV_line_status[] = {"Init", "No open HV line detected", "Open HV line", "Fault"};
    content += "<h4>HV line status: " + String(HV_line_status[datalayer_extended.meb.status_HV_line]) + "</h4>";
    const char* warning_support_status[] = {"OK", "Not OK", "", "", "", "", "Init", "Fault"};
    content +=
        "<h4>Warning support: " + String(warning_support_status[datalayer_extended.meb.warning_support]) + "</h4>";
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
#endif

#if !defined(TESLA_BATTERY) && !defined(NISSAN_LEAF_BATTERY) && \
    !defined(MEB_BATTERY)  //Only the listed types have extra info
    content += "No extra information available for this battery type";
#endif

    content += "</div>";

    content += "<script>";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";
    return content;
  }
  return String();
}
