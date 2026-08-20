#include "advanced_battery_html.h"
#include <Arduino.h>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../i18n/tr.h"
#include "html_escape.h"
#include "index_html.h"

// Available generic battery commands that are taken into use based on what the selected battery supports.
std::vector<BatteryCommand> battery_commands = {
    {"clearIsolation", TrKey::UI_CLEAR_ISOLATION_FAULT, TrKey::UI_CLEAR_ANY_ACTIVE_ISOLATION_FAULT,
     [](Battery* b) { return b && b->supports_clear_isolation(); }, [](Battery* b) { b->clear_isolation(); }},
    {"calibrateSOC", TrKey::UI_CALIBRATE_SOC, TrKey::UI_CALIBRATE_SOC_NOTE_WILL_CALIBRATE_BMS_ACCORDING_SET_TARGETS,
     [](Battery* b) { return b && b->supports_calibrate_SOC(); }, [](Battery* b) { b->reset_SOC(); }},
    {"chademoRestart", TrKey::UI_RESTART, TrKey::UI_RESTART_V2X_SESSION,
     [](Battery* b) { return b && b->supports_chademo_restart(); }, [](Battery* b) { b->chademo_restart(); }},
    {"chademoStop", TrKey::UI_STOP, TrKey::UI_STOP_V2X, [](Battery* b) { return b && b->supports_chademo_restart(); },
     [](Battery* b) { b->chademo_restart(); }},
    {"resetBMS", TrKey::UI_BMS_RESET, TrKey::UI_RESET_BMS, [](Battery* b) { return b && b->supports_reset_BMS(); },
     [](Battery* b) { b->reset_BMS(); }},
    {"resetSOC", TrKey::UI_SOC_RESET, TrKey::UI_RESET_SOC, [](Battery* b) { return b && b->supports_reset_SOC(); },
     [](Battery* b) { b->reset_SOC(); }},
    {"resetCrash", TrKey::UI_UNLOCK_CRASHED_BMS,
     TrKey::UI_RESET_CRASH_DATA_NOTE_WILL_UNLOCK_YOUR_BMS_ENABLE_CONTACTOR_CLOSING_SOC_CALCULATION,
     [](Battery* b) { return b && b->supports_reset_crash(); }, [](Battery* b) { b->reset_crash(); }},
    {"resetNVROL", TrKey::UI_PERFORM_NVROL_RESET,
     TrKey::UI_TRIGGER_NVROL_RESET_BATTERY_WILL_UNAVAILABLE_30_SECONDS_WHILE_ACTIVE,
     [](Battery* b) { return b && b->supports_reset_NVROL(); }, [](Battery* b) { b->reset_NVROL(); }},
    {"resetContactor", TrKey::UI_PERFORM_CONTACTOR_RESET, TrKey::UI_RESET_CONTACTORS,
     [](Battery* b) { return b && b->supports_contactor_reset(); }, [](Battery* b) { b->reset_contactor(); }},
    {"resetDTC", TrKey::UI_ERASE_DTC, TrKey::UI_ERASE_DTCS, [](Battery* b) { return b && b->supports_reset_DTC(); },
     [](Battery* b) { b->reset_DTC(); }, true},
    {"startBalancing", TrKey::UI_BALANCING,
     TrKey::UI_BALANCING_IT_OFTEN_TAKES_MANY_HOURS_THERE_WILL_NO_PROGRESS_INDICATION,
     [](Battery* b) { return b && b->supports_balancing() && !b->is_balancing_active(); },
     [](Battery* b) { b->initiate_balancing(); }},
    {"endBalancing", TrKey::UI_STOP_BALANCING_MODE, TrKey::UI_END_OFFLINE_BALANCING,
     [](Battery* b) { return b && b->supports_balancing() && b->is_balancing_active(); },
     [](Battery* b) { b->end_balancing(); }},
    {"startBalancingRequest", TrKey::UI_START_BALANCING, TrKey::UI_REQUEST_BMS_START_CELL_BALANCING,
     [](Battery* b) { return b && b->supports_balancing_request(); }, [](Battery* b) { b->initiate_balancing(); }},
    {"stopBalancingRequest", TrKey::UI_STOP_BALANCING, TrKey::UI_REQUEST_BMS_STOP_CELL_BALANCING,
     [](Battery* b) { return b && b->supports_balancing_request(); }, [](Battery* b) { b->end_balancing(); }},
    {"isolationTest", TrKey::UI_ISOLATION_TEST, TrKey::UI_START_ISOLATION_TEST,
     [](Battery* b) { return b && b->supports_isolation_test(); }, [](Battery* b) { b->request_isolation_test(); }},
    {"readDTC", TrKey::UI_READ_DTC, TR_NONE, [](Battery* b) { return b && b->supports_read_DTC(); },
     [](Battery* b) { b->read_DTC(); }, true},
    {"resetBECM", TrKey::UI_RESTART_BECM_MODULE, TrKey::UI_RESTART_BECM,
     [](Battery* b) { return b && b->supports_reset_BECM(); }, [](Battery* b) { b->reset_BECM(); }},
    {"contactorClose", TrKey::UI_CLOSE_CONTACTORS, TrKey::UI_CONTACTOR_CLOSE_REQUEST,
     [](Battery* b) { return b && b->supports_contactor_close(); }, [](Battery* b) { b->request_close_contactors(); }},
    {"contactorOpen", TrKey::UI_OPEN_CONTACTORS, TrKey::UI_CONTACTOR_OPEN_REQUEST,
     [](Battery* b) { return b && b->supports_contactor_close(); }, [](Battery* b) { b->request_open_contactors(); }},
    {"resetSOH", TrKey::UI_RESET_DEGRADATION_DATA, TrKey::UI_RESET_DEGRADATION_DATA_PROMPT,
     [](Battery* b) { return b && b->supports_reset_SOH(); }, [](Battery* b) { b->reset_SOH(); }},
    {"setFactoryMode", TrKey::UI_SET_FACTORY_MODE, TrKey::UI_SET_FACTORY_MODE_DISABLE_ISOLATION_MEASUREMENT,
     [](Battery* b) { return b && b->supports_factory_mode_method(); }, [](Battery* b) { b->set_factory_mode(); }},
    {"toggleSOC", TrKey::UI_TOGGLE_SOC_METHOD,
     TrKey::UI_TOGGLE_SOC_METHOD_WILL_TOGGLE_BETWEEN_ESTIMATED_MEASURED_SOC_METHODS,
     [](Battery* b) { return b && b->supports_toggle_SOC_method(); }, [](Battery* b) { b->toggle_SOC_method(); }},
    {"resetEnergySavingMode", TrKey::UI_RESET_ENERGY_SAVING_MODE, TrKey::UI_RESET_ENERGY_SAVING_MODE_NORMAL,
     [](Battery* b) { return b && b->supports_energy_saving_mode_reset(); },
     [](Battery* b) { b->reset_energy_saving_mode(); }},
};

String advanced_battery_processor(const String& var) {
  // COMMON_JAVASCRIPT is part of every template this serves; resolve its
  // placeholder here or the raw token ships to the browser.
  String common = common_javascript_processor(var);
  if (common.length() > 0) {
    return common;
  }

  if (var == "X") {
    String content = "";
    //Page format
    content += "<style>";
    content += "body { background-color: black; color: white; }";
    content +=
        "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin: 5px; "
        "cursor: pointer; border-radius: 10px; }";
    content += "button:hover { background-color: #3A4A52; }";
    content += "h4 { margin: 0.6em 0; line-height: 1.2; }";
    content += "</style>";
    content += "<button onclick='goToMainPage()'>" + TR(TrKey::UI_BACK_MAIN_PAGE) + "</button>";

    // Start a new block with a specific background color
    content += "<div style='background-color: #303E47; padding: 10px; margin-bottom: 10px;border-radius: 50px'>";

    // Render buttons dynamically based on what commands the battery supports.
    auto render_command_buttons = [&content](Battery* batt, int ix) {
      for (const auto& cmd : battery_commands) {
        if (cmd.condition(batt)) {
          // Button for user action
          content +=
              "<button onclick='ask" + String(cmd.identifier) + "(" + String(ix) + ")'>" + TR(cmd.title) + "</button>";

          // Script that calls the backend to perform the command
          content += "<script>";
          content += "function ask" + String(cmd.identifier) + "(batteryNum) { ";

          if (cmd.prompt != TR_NONE) {
            // JS string literal, not markup: the prompt name is substituted
            // into the question and the whole result escaped for the literal
            content += "if (window.confirm('" + TR_JS(TrKey::UI_ARE_YOU_SURE_YOU_WANT, TR_RAW(cmd.prompt)) + "'))";
          }

          content += "{" + String(cmd.identifier) + "(batteryNum); } }";
          content += "function " + String(cmd.identifier) + "(batteryNum) {";
          content += "  var xhr = new XMLHttpRequest();";
          content += "  xhr.open('PUT', '/" + String(cmd.identifier) + "', true);";
          if (cmd.reload_after) {
            content += "  xhr.onload = function(){ setTimeout(function(){ location.reload(); }, 1500); };";
          }
          // Send index of the battery as PUT content
          content += "  xhr.send(batteryNum);";
          content += "}";
          content += "</script>";
        }
      }
    };

    // Renders battery specific status for battery 2/3. Only renderers that
    // read per-instance data (renders_own_battery_data) are used; legacy
    // renderers hardcode the main battery datalayer and would show battery 1
    // values here, so a notice is shown instead until they are reworked.
    auto render_secondary_battery_status = [&content](Battery* batt) {
      BatteryHtmlRenderer& renderer = batt->get_status_renderer();
      if (renderer.renders_own_battery_data()) {
        content += renderer.get_status_html();
      } else {
        content += "<h4 style='color: #f39c12;'>" +
                   TR(TrKey::UI_ADVANCED_DETAILED_INFO_CURRENTLY_LIMITED_MAIN_BATTERY) + "</h4>";
      }
    };

    if (battery) {
      content += battery->get_status_renderer().get_status_html();
      render_command_buttons(battery, 0);
    }

    if (battery2) {
      content += "<hr>";
      tr_h4(content, TrKey::UI_VALUES_FROM_BATTERY_2);
      render_secondary_battery_status(battery2);
      render_command_buttons(battery2, 1);
    }

    if (battery3) {
      content += "<hr>";
      tr_h4(content, TrKey::UI_VALUES_FROM_BATTERY_3);
      render_secondary_battery_status(battery3);
      render_command_buttons(battery3, 2);
    }

    content += "</div>";

    content += "<script>";
    content += "function exportLog() { window.location.href = '/export_log'; }";
    content += "function goToMainPage() { window.location.href = '/'; }";
    content += "</script>";

    return content;
  }
  return String();
}
