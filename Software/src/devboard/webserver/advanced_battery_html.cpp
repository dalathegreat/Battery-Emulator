#include "advanced_battery_html.h"
#include <Arduino.h>
#include <algorithm>
#include <atomic>
#include <new>
#include <vector>
#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/ESPAsyncWebServer.h"
#include "../../lib/ESP32Async-ESPAsyncWebServer/src/WebResponseImpl.h"
#include "checked_html.h"
#include "index_html.h"

// Available generic battery commands that are taken into use based on what the selected battery supports.
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
     [](Battery* b) { b->reset_DTC(); }, true},
    {"startBalancing", "Balancing",
     "continue? Please charge battery fully for this to work. After a couple of minutes, battery will sleep and do "
     "balancing. It often takes many hours. There will be no progress indication.",
     [](Battery* b) { return b && b->supports_balancing() && !b->is_balancing_active(); },
     [](Battery* b) { b->initiate_balancing(); }},
    {"endBalancing", "Stop Balancing Mode", "end offline balancing?",
     [](Battery* b) { return b && b->supports_balancing() && b->is_balancing_active(); },
     [](Battery* b) { b->end_balancing(); }},
    {"startBalancingRequest", "Start Balancing", "request the BMS to start cell balancing?",
     [](Battery* b) { return b && b->supports_balancing_request(); }, [](Battery* b) { b->initiate_balancing(); }},
    {"stopBalancingRequest", "Stop Balancing", "request the BMS to stop cell balancing?",
     [](Battery* b) { return b && b->supports_balancing_request(); }, [](Battery* b) { b->end_balancing(); }},
    {"isolationTest", "Isolation Test", "start an isolation test?",
     [](Battery* b) { return b && b->supports_isolation_test(); }, [](Battery* b) { b->request_isolation_test(); }},
    {"readDTC", "Read DTC", nullptr, [](Battery* b) { return b && b->supports_read_DTC(); },
     [](Battery* b) { b->read_DTC(); }, true},
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

namespace {
std::atomic<bool> advanced_page_busy{false};

Battery* battery_at(unsigned index) {
  switch (index) {
    case 0:
      return battery;
    case 1:
      return battery2;
    case 2:
      return battery3;
    default:
      return nullptr;
  }
}

const char page_start[] = INDEX_HTML_HEADER R"html(
<style>
body{background:black;color:white}h4{margin:.6em 0;line-height:1.2}
button,.battery-tab{background:#505E67;color:white;border:0;padding:10px 20px;margin:5px;
cursor:pointer;border-radius:10px;display:inline-block;text-decoration:none;font:inherit}
button:hover,.battery-tab:hover{background:#3A4A52}
.battery-tab[aria-current=page]{background:#287c58;outline:2px solid #69c999}
.battery-panel{background:#303E47;padding:10px;margin-bottom:10px;border-radius:24px}
</style>
<script>
function goToMainPage(){window.location.href='/';}
function exportLog(){window.location.href='/export_log';}
</script>
<button onclick='goToMainPage()'>Back to main page</button>
<nav aria-label='Battery selection'>
)html";
const char page_end[] = "</div>" INDEX_HTML_FOOTER;
const char render_error[] = "<p role='alert'>Battery information could not be loaded. Please reload this page.</p>";

// No template processor: transmit each fragment directly from flash or the owned section buffer.
class AdvancedBatteryResponse : public AsyncAbstractResponse {
 public:
  AdvancedBatteryResponse(unsigned index, uint8_t http_version) : AsyncAbstractResponse(nullptr), selected(index) {
    _code = 200;
    _contentType = "text/html";
    _sendContentLength = false;
    _chunked = http_version != 0;
  }

  ~AdvancedBatteryResponse() override {
    section = static_cast<const char*>(nullptr);
    release();
  }
  bool _sourceValid() const override { return true; }

  size_t _fillBuffer(uint8_t* data, size_t len) override {
    if (!len) {
      return RESPONSE_TRY_AGAIN;
    }
    size_t written = 0;
    while (written < len) {
      if (offset == fragment_length) {
        // Drop the previous allocation before rendering the next fragment.
        // Assigning an empty String retains its capacity on ESP32; null releases it.
        section = static_cast<const char*>(nullptr);
        if (!next_fragment()) {
          release();
          break;
        }
        offset = 0;
        fragment_length = strlen(fragment);
        if (!fragment_length) {
          continue;
        }
      }
      const size_t count = std::min(len - written, fragment_length - offset);
      memcpy(data + written, fragment + offset, count);
      offset += count;
      written += count;
    }
    return written;
  }

 private:
  enum class Stage { Start, Tabs, Heading, Status, Dtc, Commands, End, Done };
  Stage stage = Stage::Start;
  unsigned selected;
  unsigned tab = 0;
  size_t command = 0;
  bool failed = false;
  bool owns_slot = true;
  String section;
  char small[192];
  const char* fragment = "";
  size_t offset = 0;
  size_t fragment_length = 0;

  void release() {
    if (owns_slot) {
      owns_slot = false;
      advanced_page_busy.store(false);
    }
  }

  bool next_fragment() {
    Battery* batt = battery_at(selected);
    while (true) {
      switch (stage) {
        case Stage::Start:
          fragment = page_start;
          stage = Stage::Tabs;
          return true;
        case Stage::Tabs:
          while (tab < 3) {
            const unsigned current = tab++;
            if (battery_at(current)) {
              snprintf(small, sizeof(small), "<a class='battery-tab' href='/advanced?battery=%u'%s>Battery %u</a>",
                       current + 1, current == selected ? " aria-current='page'" : "", current + 1);
              fragment = small;
              return true;
            }
          }
          stage = Stage::Heading;
          break;
        case Stage::Heading:
          snprintf(small, sizeof(small), "</nav><div class='battery-panel'><h3>Battery %u</h3>", selected + 1);
          fragment = small;
          stage = Stage::Status;
          return true;
        case Stage::Status: {
          stage = Stage::Dtc;
          BatteryHtmlRenderer& renderer = batt->get_status_renderer();
          if (selected && !renderer.renders_own_battery_data()) {
            fragment = "<p>Advanced detailed information is currently available only for Battery 1.</p>";
          } else {
            section = renderer.get_status_html();
            failed = renderer.html_render_failed() || !section;
            fragment = failed ? render_error : section.c_str();
          }
          return true;
        }
        case Stage::Dtc: {
          stage = Stage::Commands;
          // Diagnostics are built only after the status buffer has been freed.
          BatteryHtmlRenderer& renderer = batt->get_status_renderer();
          if (!failed && (!selected || renderer.renders_own_battery_data())) {
            section = renderer.get_dtc_html();
            failed = renderer.html_render_failed() || !section;
            fragment = failed ? render_error : section.c_str();
            return true;
          }
          break;
        }
        case Stage::Commands:
          while (!failed && command < battery_commands.size()) {
            const auto& cmd = battery_commands[command++];
            if (cmd.condition(batt)) {
              section = command_html(cmd);
              if (section.isEmpty()) {
                failed = true;
                fragment = render_error;
              } else {
                fragment = section.c_str();
              }
              return true;
            }
          }
          stage = Stage::End;
          break;
        case Stage::End:
          fragment = page_end;
          stage = Stage::Done;
          return true;
        case Stage::Done:
          return false;
      }
    }
  }

  String command_html(const BatteryCommand& cmd) const {
    CheckedHtml html;
    html.reserve(1024);
    html += "<button onclick='ask" + String(cmd.identifier) + "(" + String(selected) + ")'>" + cmd.title +
            "</button><script>function ask" + cmd.identifier + "(batteryNum){";
    if (cmd.prompt) {
      html += "if(window.confirm('Are you sure you want to " + String(cmd.prompt) + "'))";
    }
    html += "{" + String(cmd.identifier) + "(batteryNum);}}function " + cmd.identifier +
            "(batteryNum){var xhr=new XMLHttpRequest();xhr.open('PUT','/" + cmd.identifier + "',true);";
    if (cmd.reload_after) {
      html += "xhr.onload=function(){setTimeout(function(){location.reload();},1500);};";
    }
    html += "xhr.send(batteryNum);}</script>";
    return html.take();
  }
};
}  // namespace

void send_advanced_battery_page(AsyncWebServerRequest* request) {
  unsigned selected = 0;
  if (request->hasParam("battery")) {
    const String value = request->getParam("battery")->value();
    if (value.length() != 1 || value[0] < '1' || value[0] > '3') {
      request->send(400, "text/plain", "Invalid battery selection.");
      return;
    }
    selected = value[0] - '1';
  }
  if (!battery_at(selected)) {
    request->send(404, "text/plain", "This battery is not configured.");
    return;
  }
  if (advanced_page_busy.exchange(true)) {
    request->send(503, "text/plain", "Battery information is busy. Please try again.");
    return;
  }
  auto* response = new (std::nothrow) AdvancedBatteryResponse(selected, request->version());
  if (!response) {
    advanced_page_busy.store(false);
    request->send(503, "text/plain", "Battery information could not be loaded. Please try again.");
    return;
  }
  request->send(response);
}
