#include "advanced_battery_html.h"
#include <Arduino.h>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/safety/safety.h"

// This variable stores the list of all commands (which I accidentally deleted last time; this block is also needed)
std::vector<BatteryCommand> battery_commands = {
    {"clearIsolation", "Clear isolation fault", "clear any active isolation fault?",
     [](Battery* b) { return b && b->supports_clear_isolation(); }, [](Battery* b) { b->clear_isolation(); }},
    {"calibrateSOC", "Calibrate SOC", "calibrate SOC? Note this will calibrate BMS according to set targets",
     [](Battery* b) { return b && b->supports_calibrate_SOC(); }, [](Battery* b) { b->reset_SOC(); }},
    {"chademoRestart", "Restart", "restart the V2X session?",
     [](Battery* b) { return b && b->supports_chademo_restart(); }, [](Battery* b) { b->chademo_restart(); }},
    {"chademoStop", "Stop", "stop V2X?", [](Battery* b) { return b && b->supports_chademo_restart(); },
     [](Battery* b) { b->chademo_restart(); }},
    {"resetBMS", "BMS Reset", "Reset the BMS?", [](Battery* b) { return b && b->supports_reset_BMS(); },
     [](Battery* b) { b->reset_BMS(); }},
    {"resetSOC", "SOC Reset", "Reset SOC?", [](Battery* b) { return b && b->supports_reset_SOC(); },
     [](Battery* b) { b->reset_SOC(); }},
    {"resetCrash", "Unlock crashed BMS",
     "reset crash data? Note this will unlock your BMS and enable contactor closing and SOC calculation.",
     [](Battery* b) { return b && b->supports_reset_crash(); }, [](Battery* b) { b->reset_crash(); }},
    {"resetNVROL", "Perform NVROL reset",
     "trigger an NVROL reset? Battery will be unavailable for 30 seconds while this is active!",
     [](Battery* b) { return b && b->supports_reset_NVROL(); }, [](Battery* b) { b->reset_NVROL(); }},
    {"resetContactor", "Perform contactor reset", "reset contactors?",
     [](Battery* b) { return b && b->supports_contactor_reset(); }, [](Battery* b) { b->reset_contactor(); }},
    {"resetDTC", "Erase DTC", "erase DTCs?", [](Battery* b) { return b && b->supports_reset_DTC(); },
     [](Battery* b) { b->reset_DTC(); }},
    {"startBalancing", "Balancing",
     "continue? Please charge battery fully for this to work. After a couple of minutes, battery will sleep and do "
     "balancing. It often takes many hours. There will be no progress indication.",
     [](Battery* b) { return b && b->supports_balancing() && !b->is_balancing_active(); },
     [](Battery* b) { b->initiate_balancing(); }},
    {"endBalancing", "Stop Balancing Mode", "end offline balancing?",
     [](Battery* b) { return b && b->supports_balancing() && b->is_balancing_active(); },
     [](Battery* b) { b->end_balancing(); }},
    {"readDTC", "Read DTC", nullptr, [](Battery* b) { return b && b->supports_read_DTC(); },
     [](Battery* b) { b->read_DTC(); }},
    {"resetBECM", "Restart BECM module", "restart BECM??", [](Battery* b) { return b && b->supports_reset_BECM(); },
     [](Battery* b) { b->reset_BECM(); }},
    {"contactorClose", "Close Contactors", "a contactor close request?",
     [](Battery* b) { return b && b->supports_contactor_close(); }, [](Battery* b) { b->request_close_contactors(); }},
    {"contactorOpen", "Open Contactors", "a contactor open request?",
     [](Battery* b) { return b && b->supports_contactor_close(); }, [](Battery* b) { b->request_open_contactors(); }},
    {"resetSOH", "Reset degradation data", "reset degradation data?",
     [](Battery* b) { return b && b->supports_reset_SOH(); }, [](Battery* b) { b->reset_SOH(); }},
    {"setFactoryMode", "Set Factory Mode", "set factory mode and disable isolation measurement?",
     [](Battery* b) { return b && b->supports_factory_mode_method(); }, [](Battery* b) { b->set_factory_mode(); }},
    {"toggleSOC", "Toggle SOC method",
     "toggle SOC method? This will toggle between ESTIMATED and MEASURED SOC methods.",
     [](Battery* b) { return b && b->supports_toggle_SOC_method(); }, [](Battery* b) { b->toggle_SOC_method(); }},
    {"resetEnergySavingMode", "Reset Energy Saving Mode", "reset energy saving mode to normal?",
     [](Battery* b) { return b && b->supports_energy_saving_mode_reset(); },
     [](Battery* b) { b->reset_energy_saving_mode(); }},
};

// =========================================================================
// 🚀 Advanced Battery Info Processor (Stream Mode)
// =========================================================================
void print_advanced_battery_html(AsyncResponseStream* response) {
  // 🌟 1. Check status and prepare data for button
  String status = get_emulator_pause_status().c_str();
  String pauseBtnText = (status == "PAUSED" || status == "PAUSING") ? "▶️ START Batteries" : "⏸ PAUSE Batteries";
  String pauseBtnPrompt = (status == "PAUSED" || status == "PAUSING") ? "start the system?" : "pause the system?";

  response->print(R"rawliteral(
    <style>
      .adv-wrap { display: flex; flex-direction: column; gap: 20px; }
      .control-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #e74c3c; padding: 20px; }
      .action-bar { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 15px; }
      .btn-cmd { background: #3498db; color: white; border: none; padding: 12px 20px; border-radius: 6px; font-weight: bold; cursor: pointer; transition: 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.1); font-size: 0.95rem; }
      .btn-cmd:hover { background: #2980b9; transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }
      .btn-pause { background: #f39c12; }
      .btn-pause:hover { background: #d68910; }
      .bat-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #3498db; padding: 20px; overflow-x: auto; }
      .bat-title { margin: 0 0 15px 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; border-bottom: 1px solid #f0f0f0; padding-bottom: 10px; }
      .bat-card table { border-collapse: collapse; font-size: 0.95rem; min-width: 300px; width: 85vw; max-width: 1000px; }
      .bat-card th, .bat-card td { padding: 12px 15px; border-bottom: 1px solid #eee; text-align: left; color: #444; }
      .bat-card tr:nth-child(even) { background-color: #fbfcfc; }
      .bat-card tr:hover { background-color: #f1f4f6; }
      .bat-card td:first-child { font-weight: 600; color: #34495e; min-width: 150px; max-width: 250px; }
      @media (max-width: 768px) {
        .action-bar { flex-direction: column; align-items: stretch; }
        .btn-cmd { align-self: stretch; }
        .bat-card th, .bat-card td { padding: 10px 8px; }
      }
    </style>
    <script>
      function sendCmd(url, method, promptMsg) {
        if (promptMsg && promptMsg !== "null") {
          if (!window.confirm("Are you sure you want to " + promptMsg)) return;
        }
        var xhr = new XMLHttpRequest();
        xhr.open(method, url, true);
        xhr.onreadystatechange = function() {
          if (xhr.readyState == 4) {
            if (xhr.status == 200) { setTimeout(function() { window.location.reload(); }, 500); }
            else { alert('❌ Failed to execute command!'); }
          }
        };
        xhr.send("0");
      }
    </script>
    <div class="adv-wrap">
    <div class="control-card">
      <h3 style="margin:0; color:#333; font-size:1.25rem;">⚙️ Advanced Controls</h3>
      <div class="action-bar">
  )rawliteral");

  response->print("<button class=\"btn-cmd btn-pause\" onclick=\"sendCmd('/pause?value=toggle', 'GET', '" + pauseBtnPrompt + "')\">" + pauseBtnText + "</button>");

  if (battery) {
    for (const auto& cmd : battery_commands) {
      if (cmd.condition(battery)) {
        String promptStr = cmd.prompt ? String("'") + cmd.prompt + "'" : "null";
        response->print("<button class=\"btn-cmd\" onclick=\"sendCmd('/" + String(cmd.identifier) + "', 'PUT', " + promptStr + ")\">⚡ " + String(cmd.title) + "</button>");
      }
    }
  }

  response->print(R"rawliteral(
      </div>
    </div>
  )rawliteral");

  // Try to manage Multi-batt
  auto addBatteryInfo = [&](Battery* bat, String title, String color, bool isMain) {
    if (bat) {
      response->print("<div class='bat-card' style='border-top-color: " + color + ";'>");
      response->print("<h3 class='bat-title'>🔋 " + title + "</h3>");

      if (isMain) {
        // Main batt let show deep info as normal
        String batHtml = bat->get_status_renderer().get_status_html();
        if (batHtml.length() > 0) {
          batHtml.replace("color: white;", ""); batHtml.replace("color:white;", "");
          batHtml.replace("color: white", ""); batHtml.replace("color:#fff", "");
          response->print(batHtml);
        } else {
          response->print("<h4>No advanced info available.</h4>");
        }
      } else {
        // If it's a secondary block (block 2 or 3), skip fetching to prevent a Null Pointer Crash!
        response->print("<h4 style='color: #7f8c8d;'>⚠️ Advanced detailed info is currently limited to the Main Battery.</h4>");
      }

      response->print("</div>");
    }
  };

  addBatteryInfo(battery, "Main Battery Pack Info", "#3498db", true);
  addBatteryInfo(battery2, "Battery Pack #2 Info", "#9b59b6", false);
  addBatteryInfo(battery3, "Battery Pack #3 Info", "#1abc9c", false);

  response->print("</div>");
}