#include "webserver.h"
#include <Preferences.h>
#include <base64.h>
#include <ctime>
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "../../battery/BATTERIES.h"
#include "../../battery/Battery.h"
#include "../../battery/Shunt.h"
#include "../../charger/CHARGERS.h"
#include "../../communication/can/comm_can.h"
#include "../../communication/contactorcontrol/comm_contactorcontrol.h"
#include "../../communication/equipmentstopbutton/comm_equipmentstopbutton.h"
#include "../../communication/nvm/comm_nvm.h"
#include "../../datalayer/datalayer.h"
#include "../../datalayer/datalayer_extended.h"
#include "../../devboard/safety/safety.h"
#include "../../inverter/INVERTERS.h"
#include "../../lib/bblanchon-ArduinoJson/ArduinoJson.h"
#include "../sdcard/sdcard.h"
#include "../utils/events.h"
#include "../utils/led_handler.h"
#include "../utils/timer.h"
#include "esp_task_wdt.h"
#include "html_escape.h"
#include <WiFi.h>
#include <string>

std::string http_username;
std::string http_password;

extern const char* version_number;

AsyncWebServer server(80);

// =========================================================================
// 🚀 Real-time CAN Stream Backend (ZERO Memory Fragmentation Mode)
// =========================================================================
SemaphoreHandle_t can_stream_mutex = NULL;

// (Raw Array) 4000 chars 100%!
#define CAN_BATCH_SIZE 4000
char can_batch_buffer[CAN_BATCH_SIZE];
size_t can_batch_len = 0;

void append_can_stream(const char* line) {
    if (!datalayer.system.info.can_logging_active) return; 

    if (can_stream_mutex == NULL) {
        can_stream_mutex = xSemaphoreCreateMutex();
    }

    // Lock Task 
    if (xSemaphoreTake(can_stream_mutex, (TickType_t)10) == pdTRUE) {
        size_t line_len = strlen(line);
        
        if (can_batch_len + line_len + 2 < CAN_BATCH_SIZE) {
            strcpy(can_batch_buffer + can_batch_len, line);
            can_batch_len += line_len;
            can_batch_buffer[can_batch_len++] = '\n';
            can_batch_buffer[can_batch_len] = '\0';
        }
        
        xSemaphoreGive(can_stream_mutex); 
    }
}
// =========================================================================

unsigned long ota_progress_millis = 0;

#include "../../system_settings.h"
#include "advanced_battery_html.h"
#include "can_logging_html.h"
#include "can_replay_html.h"
#include "cellmonitor_html.h"
#include "debug_logging_html.h"
#include "events_html.h"
#include "index_html.h"
#include "settings_html.h"

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

const char get_firmware_info_html[] = R"rawliteral(%X%)rawliteral";

String importedLogs = "";      
bool isReplayRunning = false;  
bool settingsUpdated = false;

CAN_frame currentFrame = {.FD = true, .ext_ID = false, .DLC = 64, .ID = 0x12F, .data = {0}};

String get_uptime();
String get_firmware_info_processor(const String& var);

const char dashboard_html[] PROGMEM = R"rawliteral(
<style> 
  .battery-container { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; margin-bottom: 20px; }
  .bat-card { flex: 1; min-width: 280px; max-width: 400px; transition: 0.3s; margin-bottom: 0; }
  .bat-card.fault { border-top-color: #e74c3c; background-color: #fdf2f2; }
  .bat-card.hidden { display: none; }
  
  .details-box { margin-top: 15px; padding-top: 15px; border-top: 1px dashed #ccc; display: none; text-align: left; background: #f9f9f9; border-radius: 6px; padding: 15px; color: #333; }
  .details-box.show { display: block; animation: fadeIn 0.3s; }
  
  .btn-toggle { background-color: transparent; border: 1px solid #3498db; color: #3498db; width: 100%; margin-top: 10px; padding: 8px; border-radius: 4px; cursor: pointer; transition: 0.2s; font-weight: bold; }
  .btn-toggle:hover { background-color: #3498db; color: #fff; }

  .status-text { font-size: 0.85rem; color: #777; margin-top: 15px; text-align: center; }

  /* ⚡ Energy Flow Animation ⚡ */
  .flow-wrapper { display: flex; align-items: center; justify-content: space-between; background: #fff; padding: 15px 20px; border-radius: 6px; margin: 15px 0; box-shadow: 0 2px 5px rgba(0,0,0,0.05); border-top: 4px solid #9b59b6; }
  .flow-box { background: #f4f4f4; padding: 10px; border-radius: 4px; font-weight: bold; text-align: center; width: 80px; border: 1px solid #ddd; color: #333; }
  .flow-wire { flex-grow: 1; height: 10px; background: #eee; margin: 0 15px; position: relative; overflow: hidden; border-radius: 5px; box-shadow: inset 0 1px 3px rgba(0,0,0,0.1); }
  .flow-particles { width: 200%; height: 100%; position: absolute; left: 0; background-size: 40px 100%; }
  
  @keyframes flow-charge { 0% { transform: translateX(-50%); } 100% { transform: translateX(0%); } }
  @keyframes flow-discharge { 0% { transform: translateX(0%); } 100% { transform: translateX(-50%); } }
  
  .status-charging { animation: flow-charge 1s linear infinite; background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #2ecc71 50%, #81C784 100%); }
  .status-discharging { animation: flow-discharge 1s linear infinite; background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #e74c3c 50%, #f1948a 100%); }
  .status-idle { opacity: 0.2; background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #ccc 50%, #ddd 100%); }
</style>

<div class="card card-warning" style="padding: 15px;">
  <h2 style="margin-top:0; color:#333;">⚡ System Overview <span id="sys_version" style="font-size: 0.6em; color: #888;"></span></h2>
  <h4 style="color: #555; margin: 5px 0;">Uptime: <span id="sys_uptime" style="font-weight: normal;">--</span> <br>RAM: <span id="sys_ram" style="font-weight: normal;">--</span><br><span style="color: #9b59b6;">PSRAM: <span id="sys_psram" style="font-weight: normal;">--</span></span></h4>
  <h4 style="color: #555; margin: 5px 0;">Power Status: <strong id="sys_status" style="color: #f39c12;">--</strong></h4>
  <div style="margin-top: 15px; padding: 10px; background: #f8f9fa; border: 1px solid #eee; border-radius: 4px; font-size: 0.95em; color: #555;">
    <b>Inverter:</b> <strong id="sys_inv" style="color: #2ecc71;">--</strong> &nbsp; 🔗 &nbsp; 
    <b>Battery:</b> <strong id="sys_bat" style="color: #3498db;">--</strong>
  </div>
</div>

<div class="flow-wrapper">
    <div class="flow-box">INVERTER</div>
    <div class="flow-wire"><div id="energy-flow" class="flow-particles status-idle"></div></div>
    <div class="flow-box">BATTERY</div>
</div>

<div class="battery-container">
  <div id="card_b1" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333;">🔋 Main Battery</h3>
    <h4 style="color: #555;">SOC: <strong id="b1_soc" style="color:#000;">--</strong>% | SOH: <strong id="b1_soh" style="color:#000;">--</strong>%</h4>
    <h4 style="margin: 10px 0;"><span id="b1_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b1_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b1_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b1_cmin" style="color:#000;">--</span> / <span id="b1_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b1_tmin" style="color:#000;">--</span> / <span id="b1_tmax" style="color:#000;">--</span> &deg;C</h4>
    
    <button class="btn-toggle" onclick="toggleDetails('b1_more')">🔽 Show / Hide Details</button>
    <div id="b1_more" class="details-box">
      <h4>📡 Status: <span id="b1_stat" style="color:#f39c12;">--</span></h4>
      <h4>⚡ Activity: <span id="b1_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Total Capacity: <span id="b1_tot" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Remaining: <span id="b1_rem" style="font-weight:normal;">--</span> Wh</h4>
      <h4>Max Charge: <span id="b1_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b1_md" style="font-weight:normal;">--</span> W</h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>🔌 Contactor Allowed:</h4>
      <h4 style="margin-left: 20px;">Battery: <span id="b1_cont_b" style="font-weight:normal;">--</span> | Inverter: <span id="sys_cont_i" style="font-weight:normal;">--</span></h4>
    </div>
  </div>

  <div id="card_b2" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333;">🔋 Battery 2</h3>
    <h4 style="color: #555;">SOC: <strong id="b2_soc" style="color:#000;">--</strong>% | SOH: <strong id="b2_soh" style="color:#000;">--</strong>%</h4>
    <h4 style="margin: 10px 0;"><span id="b2_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b2_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b2_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b2_cmin" style="color:#000;">--</span> / <span id="b2_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b2_tmin" style="color:#000;">--</span> / <span id="b2_tmax" style="color:#000;">--</span> &deg;C</h4>
    
    <button class="btn-toggle" onclick="toggleDetails('b2_more')">🔽 Show / Hide Details</button>
    <div id="b2_more" class="details-box">
      <h4>📡 Status: <span id="b2_stat" style="color:#f39c12;">--</span></h4>
      <h4>⚡ Activity: <span id="b2_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Max Charge: <span id="b2_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b2_md" style="font-weight:normal;">--</span> W</h4>
    </div>
  </div>

  <div id="card_b3" class="card bat-card hidden">
    <h3 style="margin-top: 0; color: #333;">🔋 Battery 3</h3>
    <h4 style="color: #555;">SOC: <strong id="b3_soc" style="color:#000;">--</strong>% | SOH: <strong id="b3_soh" style="color:#000;">--</strong>%</h4>
    <h4 style="margin: 10px 0;"><span id="b3_v" style="color: #2ecc71; font-size: 1.4em;">--</span> V &nbsp;&nbsp; <span id="b3_a" style="color: #3498db; font-size: 1.4em;">--</span> A</h4>
    <h4 style="color: #555;">Power: <span id="b3_p" style="color:#000;">--</span> W</h4>
    <h4 style="color: #555;">Cell (Min/Max): <span id="b3_cmin" style="color:#000;">--</span> / <span id="b3_cmax" style="color:#000;">--</span> mV</h4>
    <h4 style="color: #555;">Temp: <span id="b3_tmin" style="color:#000;">--</span> / <span id="b3_tmax" style="color:#000;">--</span> &deg;C</h4>
    
    <button class="btn-toggle" onclick="toggleDetails('b3_more')">🔽 Show / Hide Details</button>
    <div id="b3_more" class="details-box">
      <h4>📡 Status: <span id="b3_stat" style="color:#f39c12;">--</span></h4>
      <h4>⚡ Activity: <span id="b3_act" style="color:#3498db;">--</span></h4>
      <hr style="border: 0; border-top: 1px solid #ddd; margin: 10px 0;">
      <h4>Max Charge: <span id="b3_mc" style="font-weight:normal;">--</span> W</h4>
      <h4>Max Discharge: <span id="b3_md" style="font-weight:normal;">--</span> W</h4>
    </div>
  </div>
</div>

<div id="card_chg" class="card card-warning hidden" style="border-top-color: #e67e22;">
  <h3 style="margin-top:0; color:#333;">🔌 Charger Output</h3>
  <h4><span id="chg_v" style="font-size: 1.4em; color: #2ecc71;">--</span> V &nbsp;&nbsp; <span id="chg_a" style="font-size: 1.4em; color: #3498db;">--</span> A</h4>
</div>

<div class="status-text">Last Updated: <span id="last_update">Waiting for data...</span></div>

<script>
  function toggleDetails(id) { document.getElementById(id).classList.toggle('show'); }
  function fetchBatteryData() {
    fetch('/api/data')
      .then(response => response.text())
      .then(text => {
        let data = window.repairAndParseJSON(text);
        if (data) {
          document.getElementById('sys_version').innerText = "v" + data.sys.version;
          // document.getElementById('sys_uptime').innerText = data.sys.uptime;
          // Smart Sync Uptime
          let upMatch = data.sys.uptime.match(/(\d+)\s+days,\s+(\d+)\s+hours,\s+(\d+)\s+mins,\s+(\d+)\s+secs/);
          if (upMatch) {
              let serverSecs = parseInt(upMatch[1])*86400 + parseInt(upMatch[2])*3600 + parseInt(upMatch[3])*60 + parseInt(upMatch[4]);
              if (typeof window.localUptimeSecs === 'undefined' || Math.abs(serverSecs - window.localUptimeSecs) > 2 || serverSecs < window.localUptimeSecs) {
                  window.localUptimeSecs = serverSecs;
                  document.getElementById('sys_uptime').innerText = data.sys.uptime;
              }
          }
          let freeRamKB = Math.round(data.sys.heap / 1024);
          let totalRamKB = Math.round(data.sys.heap_size / 1024);
          let ramPercent = Math.round((data.sys.heap / data.sys.heap_size) * 100);
          document.getElementById('sys_ram').innerText = freeRamKB + " / " + totalRamKB + " KB (" + ramPercent + "% Free)";
          
          if (data.sys.psram_size > 0) {
              document.getElementById('sys_psram').innerText = Math.round(data.sys.psram / 1024) + " / " + Math.round(data.sys.psram_size / 1024) + " KB";
          } else {
              document.getElementById('sys_psram').innerText = "Not Enabled";
          }
          document.getElementById('sys_status').innerText = data.sys.status;
          document.getElementById('sys_inv').innerText = data.sys.inv;
          document.getElementById('sys_bat').innerText = data.sys.bat;

          const updateBat = (id, bData) => {
            if(bData.en) {
              document.getElementById('card_' + id).classList.remove('hidden');
              document.getElementById(id + '_soc').innerText = bData.soc;
              document.getElementById(id + '_soh').innerText = bData.soh;
              document.getElementById(id + '_v').innerText = bData.v;
              document.getElementById(id + '_a').innerText = bData.a;
              document.getElementById(id + '_p').innerText = bData.p;
              document.getElementById(id + '_cmin').innerText = bData.cmin;
              document.getElementById(id + '_cmax').innerText = bData.cmax;
              document.getElementById(id + '_tmin').innerText = bData.tmin;
              document.getElementById(id + '_tmax').innerText = bData.tmax;
              
              document.getElementById(id + '_stat').innerText = bData.stat;
              document.getElementById(id + '_mc').innerText = bData.mc;
              document.getElementById(id + '_md').innerText = bData.md;
              if(id === 'b1') {
                document.getElementById(id + '_rem').innerText = bData.rem;
                document.getElementById(id + '_tot').innerText = bData.tot;
                document.getElementById('b1_cont_b').innerText = bData.b_cont ? "YES" : "NO";
                document.getElementById('sys_cont_i').innerText = data.sys.inv_cont ? "YES" : "NO";
              }
              if(bData.fault) document.getElementById('card_' + id).classList.add('fault');
              else document.getElementById('card_' + id).classList.remove('fault');
            }
          };

          let totalPowerW = 0;
          if (data.b1 && data.b1.en) totalPowerW += parseFloat(data.b1.p) || 0;
          if (data.b2 && data.b2.en) totalPowerW += parseFloat(data.b2.p) || 0;
          if (data.b3 && data.b3.en) totalPowerW += parseFloat(data.b3.p) || 0;

          let flowElement = document.getElementById('energy-flow');
          if (totalPowerW > 50) {
            flowElement.className = 'flow-particles status-charging';
            let speed = Math.max(0.2, 2000 / totalPowerW); 
            flowElement.style.animationDuration = speed + 's';
          } else if (totalPowerW < -50) {
            flowElement.className = 'flow-particles status-discharging';
            let speed = Math.max(0.2, 2000 / Math.abs(totalPowerW)); 
            flowElement.style.animationDuration = speed + 's';
          } else {
            flowElement.className = 'flow-particles status-idle';
            flowElement.style.animationDuration = '0s';
          }

          updateBat('b1', data.b1); updateBat('b2', data.b2); updateBat('b3', data.b3);          

          if(data.chg.en) {
            document.getElementById('card_chg').classList.remove('hidden');
            document.getElementById('chg_v').innerText = data.chg.v;
            document.getElementById('chg_a').innerText = data.chg.a;
          }
          document.getElementById('last_update').innerText = new Date().toLocaleTimeString();
        }
      })
      .catch(error => console.error('Error fetching API:', error));
  }
  
  fetchBatteryData();
  setInterval(fetchBatteryData, 2000); 

  setInterval(function() {
    if (typeof window.localUptimeSecs !== 'undefined') {
      window.localUptimeSecs++; 
      
      // convert time format
      // var d = Math.floor(window.localUptimeSecs / 86400);
      // var h = Math.floor((window.localUptimeSecs % 86400) / 3600);
      // var m = Math.floor((window.localUptimeSecs % 3600) / 60);
      // var s = window.localUptimeSecs % 60;
      
      var totalSeconds = window.localUptimeSecs;
      var d = Math.floor(totalSeconds / 86400);
      totalSeconds = totalSeconds - (d * 86400);
      var h = Math.floor(totalSeconds / 3600);
      totalSeconds = totalSeconds - (h * 3600);
      var m = Math.floor(totalSeconds / 60);
      totalSeconds = totalSeconds - (m * 60);
      var s = totalSeconds;


      var timeEl = document.getElementById("sys_uptime");
      if (timeEl) {
        timeEl.innerText = d + " days, " + h + " hours, " + m + " mins, " + s + " secs";
      }
    }
  }, 1000);

</script>
)rawliteral";

const char subpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS System Info</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #121212; color: #ffffff; margin: 0; padding: 20px; }
    button { background-color: #505E67; color: white; border: none; padding: 10px 15px; cursor: pointer; border-radius: 8px; margin-bottom: 20px; font-weight: bold; }
    button:hover { background-color: #3A4A52; }
    table { width: 100%; border-collapse: collapse; background-color: #2D3F2F; border-radius: 10px; overflow: hidden; margin-top: 10px; }
    th, td { border: 1px solid #444; padding: 10px; text-align: left; }
    th { background-color: #1A251B; color: #4CAF50; }
    h2, h3, h4 { color: #4CAF50; }
    a { color: #2196F3; }
  </style>
</head>
<body>
  <button onclick="window.location.href='/'">⬅️ Back to Dashboard</button>
  <div id="content">%X%</div>
</body>
</html>
)rawliteral";

bool is_scanning = false;
String scan_results = "[]";

void wifiScanTask(void *param) {
    is_scanning = true;
    int n = WiFi.scanNetworks(false, false, false, 300); 
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    for (int i = 0; i < n; ++i) {
        JsonObject net = arr.add<JsonObject>();
        net["ssid"] = WiFi.SSID(i);
        net["rssi"] = WiFi.RSSI(i);
    }
    serializeJson(doc, scan_results);
    WiFi.scanDelete(); 
    is_scanning = false;
    vTaskDelete(NULL);
}

int wifi_apply_state = 0; 
String apply_ssid = "";
String apply_pass = "";
String applied_ip = "";
bool keep_ap_mode = true;

void wifiApplyTask(void *param) {
    wifi_mode_t old_mode = WiFi.getMode();
    String old_ssid = WiFi.SSID();
    String old_pass = WiFi.psk();

    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(); 
    vTaskDelay(500 / portTICK_PERIOD_MS);

    WiFi.begin(apply_ssid.c_str(), apply_pass.c_str());

    int attempts = 0;
    while(WiFi.status() != WL_CONNECTED && attempts < 20) { 
        vTaskDelay(500 / portTICK_PERIOD_MS);
        attempts++;
    }

    if(WiFi.status() == WL_CONNECTED) {
        wifi_apply_state = 2; 
        applied_ip = WiFi.localIP().toString();

        BatteryEmulatorSettingsStore settings;
        settings.saveString("SSID", apply_ssid.c_str());
        settings.saveString("PASSWORD", apply_pass.c_str());
        settings.saveBool("WIFIAPENABLED", keep_ap_mode);
        settingsUpdated = true;

        vTaskDelay(6000 / portTICK_PERIOD_MS);
        if(!keep_ap_mode) WiFi.mode(WIFI_STA); 
    } else {
        wifi_apply_state = 3; 
        WiFi.disconnect();
        vTaskDelay(500 / portTICK_PERIOD_MS);
        if(old_ssid.length() > 0) WiFi.begin(old_ssid.c_str(), old_pass.c_str());
        WiFi.mode(old_mode);
    }
    vTaskDelete(NULL);
}

bool use_sd_for_replay = false;
File uploadReplayFile;
size_t current_upload_size = 0;
const size_t MAX_RAM_REPLAY_SIZE = 100 * 1024;  

void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  if (!index) {
    logging.printf("Receiving replay file: %s\n", filename.c_str());
    current_upload_size = 0;
    importedLogs = "";  
    use_sd_for_replay = false;

    uploadReplayFile = SD_MMC.open("/replay_temp.txt", FILE_WRITE);
    if (uploadReplayFile) {
      logging.println("[Hybrid Replay] SD Card detected. Streaming directly to SD.");
      use_sd_for_replay = true;
    } else {
      logging.println("[Hybrid Replay] No SD Card available. Falling back to RAM (Max 100KB limit).");
    }
  }

  current_upload_size += len;

  if (use_sd_for_replay) {
    if (uploadReplayFile) uploadReplayFile.write(data, len);
  } else {
    if (current_upload_size > MAX_RAM_REPLAY_SIZE) {
      if (final) {
        logging.println("ERROR: File too large for RAM! Upload aborted to prevent crash.");
        request->send(400, "text/plain", "File too large! Please insert an SD Card to upload files larger than 100KB.");
      }
      return;  
    }
    importedLogs += String((char*)data).substring(0, len);
  }

  if (final) {
    if (use_sd_for_replay && uploadReplayFile) uploadReplayFile.close();
    if (!use_sd_for_replay && current_upload_size > MAX_RAM_REPLAY_SIZE) return;

    logging.printf("Upload Complete! Total Size: %d bytes\n", current_upload_size);
    request->send(200, "text/plain", "File uploaded successfully");
  }
}

void canReplayTask(void* param) {
  bool hasData = false;

  if (use_sd_for_replay) {
    File f = SD_MMC.open("/replay_temp.txt", FILE_READ);
    if (f && f.size() > 0) {
      hasData = true;
      f.close();
    }
  } else {
    if (importedLogs.length() > 0) hasData = true;
  }

  if (!hasData) {
    logging.println("No replay data found.");
    isReplayRunning = false;
    vTaskDelete(NULL);
    return;
  }

  do {
    float firstTimestamp = -1.0f;
    float lastTimestamp = 0.0f;
    bool firstMessageSent = false;

    File replayFile;
    int ramIndex = 0;  

    if (use_sd_for_replay) {
      replayFile = SD_MMC.open("/replay_temp.txt", FILE_READ);
      if (!replayFile) break;  
    }

    while (true) {
      String line = "";

      if (use_sd_for_replay) {
        if (!replayFile.available()) break;  
        line = replayFile.readStringUntil('\n');
      } else {
        if (ramIndex >= importedLogs.length()) break;  
        int nextIndex = importedLogs.indexOf('\n', ramIndex);
        if (nextIndex == -1) {
          line = importedLogs.substring(ramIndex);
          ramIndex = importedLogs.length();
        } else {
          line = importedLogs.substring(ramIndex, nextIndex);
          ramIndex = nextIndex + 1;
        }
      }

      line.trim();
      if (line.length() == 0) continue;

      int timeStart = line.indexOf("(") + 1;
      int timeEnd = line.indexOf(")");
      if (timeStart == 0 || timeEnd == -1) continue;

      float currentTimestamp = line.substring(timeStart, timeEnd).toFloat();

      if (firstTimestamp < 0) firstTimestamp = currentTimestamp;

      if (!firstMessageSent) {
        firstMessageSent = true;
        firstTimestamp = currentTimestamp;
      } else {
        float deltaT = (currentTimestamp - lastTimestamp) * 1000.0f;
        if (deltaT > 0 && deltaT < 10000) {
          vTaskDelay((int)deltaT / portTICK_PERIOD_MS);
        }
      }

      lastTimestamp = currentTimestamp;

      int interfaceStart = timeEnd + 2;
      int interfaceEnd = line.indexOf(" ", interfaceStart);
      if (interfaceEnd == -1) continue;

      int idStart = interfaceEnd + 1;
      int idEnd = line.indexOf(" [", idStart);
      if (idStart == -1 || idEnd == -1) continue;

      String messageID = line.substring(idStart, idEnd);
      int dlcStart = idEnd + 2;
      int dlcEnd = line.indexOf("]", dlcStart);
      if (dlcEnd == -1) continue;

      String dlc = line.substring(dlcStart, dlcEnd);
      int dataStart = dlcEnd + 2;
      String dataBytes = line.substring(dataStart);

      currentFrame.ID = strtol(messageID.c_str(), NULL, 16);
      currentFrame.DLC = dlc.toInt();

      int byteIndex = 0;
      char* token = strtok((char*)dataBytes.c_str(), " ");
      while (token != NULL && byteIndex < currentFrame.DLC) {
        currentFrame.data.u8[byteIndex++] = strtol(token, NULL, 16);
        token = strtok(NULL, " ");
      }

      currentFrame.FD = (datalayer.system.info.can_replay_interface == CANFD_NATIVE) ||
                        (datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518);
      currentFrame.ext_ID = (currentFrame.ID > 0x7F0);

      transmit_can_frame_to_interface(&currentFrame, (CAN_Interface)datalayer.system.info.can_replay_interface);

    }  

    if (use_sd_for_replay && replayFile) {
      replayFile.close();
    }

  } while (datalayer.system.info.loop_playback);  

  importedLogs = "";  
  isReplayRunning = false;
  vTaskDelete(NULL);  
}

bool checkAuth(AsyncWebServerRequest* request) {
  BatteryEmulatorSettingsStore auth_settings(true);
  bool is_auth_enabled = (auth_settings.getUInt("WEBAUTH", 0) == 1);

  if (!is_auth_enabled) return true;  

  String u = (http_username.length() > 0) ? http_username.c_str() : DEFAULT_WEB_USER;
  String p = (http_password.length() >= 4) ? http_password.c_str() : DEFAULT_WEB_PASS;
  String expectedAuth = "Basic " + base64::encode(u + ":" + p);

  if (request->hasHeader("Authorization")) {
    if (request->getHeader("Authorization")->value().equals(expectedAuth)) return true;
  }
  return false;  
}

void def_route_with_auth(const char* uri, AsyncWebServer& serv, WebRequestMethodComposite method,
                         std::function<void(AsyncWebServerRequest*)> handler) {
  serv.on(uri, method, [handler, uri](AsyncWebServerRequest* request) {
    if (!checkAuth(request)) {
      Serial.printf("[DEBUG] 🛡️ IP: %s invalid password Pop-up...\n", request->client()->remoteIP().toString().c_str());
      AsyncWebServerResponse* response = request->beginResponse(401, "text/plain", "Authentication Required");
      response->addHeader("WWW-Authenticate", "Basic realm=\"Battery_Emulator_Secure\"");
      return request->send(response);
    }
    handler(request);
  });
}

void send_large_page_safely(AsyncWebServerRequest* request, std::function<String(const String&)> processor_func) {
  // String html = String(index_html);
  // String payload = processor_func("X");
  // html.replace("%X%", payload);
  // request->send(200, "text/html", html);
  AsyncWebServerResponse *response = request->beginResponse(200, "text/html", (const uint8_t*)index_html, strlen(index_html), processor_func);
  request->send(response);
}

// =========================================================================
// WebServer Initialization
// =========================================================================

void init_webserver() {

  def_route_with_auth("/startScan", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    if(!is_scanning) xTaskCreate(wifiScanTask, "WiFiScan", 4096, NULL, 1, NULL);
    request->send(200, "text/plain", "Started");
  });
  
  def_route_with_auth("/getScan", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if(is_scanning) request->send(200, "application/json", "{\"status\":\"scanning\"}");
    else request->send(200, "application/json", scan_results);
  });

  def_route_with_auth("/applyWifi", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    if(request->hasParam("ssid", true)) {
        apply_ssid = request->getParam("ssid", true)->value();
        apply_pass = request->hasParam("pass", true) ? request->getParam("pass", true)->value() : "";
        keep_ap_mode = request->hasParam("keep_ap", true) ? (request->getParam("keep_ap", true)->value() == "1") : true;
        wifi_apply_state = 1; 
        xTaskCreate(wifiApplyTask, "WiFiApply", 4096, NULL, 1, NULL); 
        request->send(200, "text/plain", "Started");
    } else {
        request->send(400, "text/plain", "No SSID");
    }
  });
  
  def_route_with_auth("/applyWifiStatus", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String resp = "{\"state\":" + String(wifi_apply_state) + ",\"ip\":\"" + applied_ip + "\"}";
    request->send(200, "application/json", resp);
  });

  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(401, "text/html",
                  "<h2 style='font-family:sans-serif; color:white; background:black; padding:20px;'>Logged Out "
                  "Successfully</h2><p style='color:white; background:black;'>Please close your browser window, or <a "
                  "href='/' style='color:#00ff00;'>click here to login again</a>.</p>");
  });

  server.on("/api/is_auth", HTTP_GET, [](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore auth_settings(true);
    request->send(200, "text/plain", (auth_settings.getUInt("WEBAUTH", 0) == 1) ? "1" : "0");
  });

  def_route_with_auth("/GetFirmwareInfo", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  def_route_with_auth("/", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    ota_active = false;
    auto dashboard_processor = [](const String& var) {
      if (var == "X") return String(dashboard_html);
      return String();
    };
    send_large_page_safely(request, dashboard_processor);
  });

  def_route_with_auth("/advanced", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    send_large_page_safely(request, advanced_battery_processor);
  });

  def_route_with_auth("/canlog", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", can_logger_processor()));
  });

  def_route_with_auth("/canreplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", can_replay_processor()));
  });

  def_route_with_auth("/startReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (isReplayRunning) {
      request->send(400, "text/plain", "Replay already running!");
      return;
    }
    datalayer.system.info.loop_playback = request->hasParam("loop") && request->getParam("loop")->value().toInt() == 1;
    isReplayRunning = true;
    xTaskCreatePinnedToCore(canReplayTask, "CAN_Replay", 8192, NULL, 1, NULL, 1);
    request->send(200, "text/plain", "CAN replay started!");
  });

  def_route_with_auth("/stopReplay", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.loop_playback = false;
    request->send(200, "text/plain", "CAN replay stopped!");
  });

  def_route_with_auth("/setCANInterface", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("interface")) {
      datalayer.system.info.can_replay_interface = request->getParam("interface")->value().toInt();
      request->send(200, "text/plain", "New interface selected");
    } else {
      request->send(400, "text/plain", "Error: updating interface failed");
    }
  });

  def_route_with_auth("/log", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", debug_logger_processor()));
  });

  // API : Start Stream 
  def_route_with_auth("/start_can_logging", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = true;
    if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, (TickType_t)50) == pdTRUE) {
        can_batch_len = 0;
        can_batch_buffer[0] = '\0';
        xSemaphoreGive(can_stream_mutex);
    }
    request->send(200, "text/plain", "CAN Stream Started");
  });

  // 🌟 API : Stop Stream
  server.on("/stop_can_logging", HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = false;
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
    if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, (TickType_t)50) == pdTRUE) {
        can_batch_len = 0;
        can_batch_buffer[0] = '\0';
        xSemaphoreGive(can_stream_mutex);
    }
    request->send(200, "text/plain", "Logging stopped");
  });

  // 🌟 API : High-Speed Poller 
  def_route_with_auth("/api/can_poll", server, HTTP_GET, [](AsyncWebServerRequest* request) {
      if (can_stream_mutex != NULL && xSemaphoreTake(can_stream_mutex, (TickType_t)50) == pdTRUE) {
          if (can_batch_len > 0) {
              request->send(200, "text/plain", can_batch_buffer);
              can_batch_len = 0; // 
              can_batch_buffer[0] = '\0';
          } else {
              request->send(200, "text/plain", "");
          }
          xSemaphoreGive(can_stream_mutex);
      } else {
          request->send(503, "text/plain", ""); // Server Busy
      }
  });

  def_route_with_auth("/clear_log", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
    request->send(200, "text/plain", "Log cleared");
  });

  server.on(
      "/import_can_log", HTTP_POST,
      [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Ready to receive file."); },
      handleFileUpload);

  if (datalayer.system.info.CAN_SD_logging_active) {
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_can_writing();
      request->send(SD_MMC, CAN_LOG_FILE, String(), true);
      resume_can_writing();
    });
    server.on("/delete_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_can_log();
      request->send(200, "text/plain", "Log file deleted");
    });
  } else {
    server.on("/export_can_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(400, "text/plain", "Use frontend Export.");
    });
  }

  if (datalayer.system.info.SD_logging_active) {
    server.on("/delete_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      delete_log();
      request->send(200, "text/plain", "Log file deleted");
    });
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      pause_log_writing();
      request->send(SD_MMC, LOG_FILE, String(), true);
      resume_log_writing();
    });
  } else {
    server.on("/export_log", HTTP_GET, [](AsyncWebServerRequest* request) {
      if (!datalayer.system.info.web_logging_active) {
        AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", "System Logging is DISABLED.");
        response->addHeader("Content-Disposition", "attachment; filename=\"System_Log_Disabled.txt\"");
        return request->send(response);
      }
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0) logs = "No logs available.";
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      char filename[32];
      if (!strftime(filename, sizeof(filename), "log_%H-%M-%S.txt", &timeinfo)) {
        strcpy(filename, "battery_emulator_log.txt");
      }
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", String("attachment; filename=\"") + String(filename) + "\"");
      request->send(response);
    });
  }

  // def_route_with_auth("/cellmonitor", server, HTTP_GET,
  //                    [](AsyncWebServerRequest* request) { send_large_page_safely(request, cellmonitor_processor); });

  // Cell Monitor: use Template Processor RAM 0% 
  def_route_with_auth("/cellmonitor", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", index_html, cellmonitor_processor);
    request->send(response);
  });

  def_route_with_auth("/events", server, HTTP_GET,
                      [](AsyncWebServerRequest* request) { send_large_page_safely(request, events_processor); });

  def_route_with_auth("/clearevents", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    reset_all_events();
    String response = "<html><body><script>window.location.href = '/events';</script></body></html>";
    request->send(200, "text/html", response);
  });

  def_route_with_auth("/factoryReset", server, HTTP_POST, [](AsyncWebServerRequest* request) {
    BatteryEmulatorSettingsStore settings;
    settings.clearAll();
    request->send(200, "text/html", "OK");
  });

  // auto serve_settings_page = [](AsyncWebServerRequest* request, const char* html_array) {
  //     auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);
  //     request->send(200, "text/html", (const uint8_t*)html_array, strlen(html_array), 
  //         [settings](const String& content) { return settings_processor(content, *settings); });
  // };

  auto serve_settings_page = [](AsyncWebServerRequest* request, const char* html_array) {
      auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);
      AsyncWebServerResponse *response = request->beginResponse(200, "text/html", html_array, 
          [settings](const String& content) { return settings_processor(content, *settings); });
      request->send(response);
  };

  def_route_with_auth("/settings", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) { serve_settings_page(request, settings_batt_html); });
  def_route_with_auth("/set_network", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) { serve_settings_page(request, settings_net_html); });
  def_route_with_auth("/set_hardware", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) { serve_settings_page(request, settings_hw_html); });
  def_route_with_auth("/set_web", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) { serve_settings_page(request, settings_web_html); });
  def_route_with_auth("/set_overrides", server, HTTP_GET, [serve_settings_page](AsyncWebServerRequest* request) { serve_settings_page(request, settings_overrides_html); });

  def_route_with_auth("/saveSettings", server, HTTP_POST, [](AsyncWebServerRequest* request) {
        BatteryEmulatorSettingsStore settings;
        String pageUrl = request->hasParam("PAGE_ID", true) ? request->getParam("PAGE_ID", true)->value() : "/settings";

        const char* uintSettingNames[] = {
            "BATTCVMAX", "BATTCVMIN", "MAXPRETIME", "MAXPREFREQ", "WIFICHANNEL", "DCHGPOWER", "CHGPOWER", 
            "LOCALIP1", "LOCALIP2", "LOCALIP3", "LOCALIP4", "GATEWAY1", "GATEWAY2", "GATEWAY3", "GATEWAY4", 
            "SUBNET1", "SUBNET2", "SUBNET3", "SUBNET4", "MQTTPORT", "MQTTTIMEOUT", "SOFAR_ID", "PYLONSEND", 
            "INVCELLS", "INVMODULES", "INVCELLSPER", "INVVLEVEL", "INVCAPACITY", "INVBTYPE", "CANFREQ", 
            "CANFDFREQ", "PRECHGMS", "PWMFREQ", "PWMHOLD", "GTWCOUNTRY", "GTWMAPREG", "GTWCHASSIS", "GTWPACK", 
            "LEDMODE", "GPIOOPT1", "GPIOOPT2", "GPIOOPT3", "INVSUNTYPE", "GPIOOPT4", "LEDTAIL", "LEDCOUNT", 
            "WEBAUTH", "DISPLAYTYPE", "CTVNOM", "CTANOM", "CTATTEN" // 🌟 เพิ่มค่า CT Clamp จาก main
        };

        const char* stringSettingNames[] = {
            "APNAME", "APPASSWORD", "HOSTNAME", "MQTTSERVER", "MQTTUSER",
            "MQTTPASSWORD", "MQTTTOPIC", "MQTTOBJIDPREFIX", "MQTTDEVICENAME", "HADEVICEID",
            "SSID", "PASSWORD", "WEBUSER", "WEBPASS", "CTOFFSET" // 🌟 เพิ่มค่า CT Clamp จาก main
        };

        int numParams = request->params();
        for (int i = 0; i < numParams; i++) {
            auto p = request->getParam(i);
            String pName = p->name();
            String pValue = p->value();

            if (pName == "inverter") settings.saveUInt("INVTYPE", atoi(pValue.c_str()));
            else if (pName == "INVCOMM") settings.saveUInt("INVCOMM", atoi(pValue.c_str()));
            else if (pName == "battery") settings.saveUInt("BATTTYPE", atoi(pValue.c_str()));
            else if (pName == "BATTCHEM") settings.saveUInt("BATTCHEM", atoi(pValue.c_str()));
            else if (pName == "BATTCOMM") settings.saveUInt("BATTCOMM", atoi(pValue.c_str()));
            else if (pName == "BATTPVMAX") settings.saveUInt("BATTPVMAX", (int)(pValue.toFloat() * 10.0f));
            else if (pName == "BATTPVMIN") settings.saveUInt("BATTPVMIN", (int)(pValue.toFloat() * 10.0f));
            else if (pName == "charger") settings.saveUInt("CHGTYPE", atoi(pValue.c_str()));
            else if (pName == "CHGCOMM") settings.saveUInt("CHGCOMM", atoi(pValue.c_str()));
            else if (pName == "EQSTOP") settings.saveUInt("EQSTOP", atoi(pValue.c_str()));
            else if (pName == "BATT2COMM") settings.saveUInt("BATT2COMM", atoi(pValue.c_str()));
            else if (pName == "BATT3COMM") settings.saveUInt("BATT3COMM", atoi(pValue.c_str()));
            else if (pName == "shunttype") settings.saveUInt("SHUNTTYPE", atoi(pValue.c_str())); 
            else if (pName == "SHUNTCOMM") settings.saveUInt("SHUNTCOMM", atoi(pValue.c_str()));
            else if (pName == "MQTTPUBLISHMS") settings.saveUInt("MQTTPUBLISHMS", atoi(pValue.c_str()) * 1000);

            for (const char* uintSetting : uintSettingNames) {
                if (pName == String(uintSetting)) settings.saveUInt(uintSetting, atoi(pValue.c_str()));
            }
            for (const char* stringSetting : stringSettingNames) {
                if (pName == String(stringSetting)) settings.saveString(stringSetting, pValue.c_str());
            }
        }

        std::vector<const char*> activeBools;
        if (pageUrl == "/set_network") activeBools = {"STATICIP", "WIFIAPENABLED", "ESPNOWENABLED", "MQTTENABLED", "MQTTCELLV", "REMBMSRESET", "MQTTTOPICS", "HADISC"};
        else if (pageUrl == "/settings") activeBools = {"INTERLOCKREQ", "DIGITALHVIL", "GTWRHD", "SOCESTIMATED", "DBLBTR", "TRIBTR", "PYLONOFFSET", "PYLONORDER", "DEYEBYD", "INVICNT", "PRIMOGEN24"}; // 🌟 เพิ่ม PRIMOGEN24
        else if (pageUrl == "/set_hardware") activeBools = {"CANFDASCAN", "CNTCTRLDBL", "CNTCTRLTRI", "CNTCTRL", "NCCONTACTOR", "PWMCNTCTRL", "PERBMSRESET", "EXTPRECHARGE", "NOINVDISC", "EPAPREFRESHBTN", "MULTII2C", "I2C_SHT30", "I2C_ATECC", "I2C_RTC", "I2C_IO", "CTINVERT"}; // 🌟 เพิ่ม CTINVERT
        else if (pageUrl == "/set_web") activeBools = {"PERFPROFILE", "CANLOGUSB", "USBENABLED", "WEBENABLED", "CANLOGSD", "SDLOGENABLED"};

        for (const char* boolSetting : activeBools) {
            auto p = request->getParam(boolSetting, true);
            settings.saveBool(boolSetting, p != nullptr && p->value() == "on");
        }

        settingsUpdated = settings.were_settings_updated();
        request->redirect(pageUrl);
    });

  auto update_string = [](const char* route, std::function<void(String)> setter, std::function<bool(String)> validator = nullptr) {
    def_route_with_auth(route, server, HTTP_GET, [=](AsyncWebServerRequest* request) {
      if (request->hasParam("value")) {
        String value = request->getParam("value")->value();
        if (validator && !validator(value)) {
          request->send(400, "text/plain", "Invalid value");
          return;
        }
        setter(value);
        request->send(200, "text/plain", "Updated successfully");
      } else {
        request->send(400, "text/plain", "Bad Request");
      }
    });
  };

  auto update_string_setting = [=](const char* route, std::function<void(String)> setter, std::function<bool(String)> validator = nullptr) {
    update_string(route, [setter](String value) { setter(value); store_settings(); }, validator);
  };

  auto update_int_setting = [=](const char* route, std::function<void(int)> setter) {
    update_string_setting(route, [setter](String value) { setter(value.toInt()); });
  };

  update_int_setting("/updateBatterySize", [](int value) { datalayer.battery.info.total_capacity_Wh = value; });
  update_int_setting("/updateUseScaledSOC", [](int value) { datalayer.battery.settings.soc_scaling_active = value; });
  update_int_setting("/enableRecoveryMode", [](int value) { datalayer.battery.settings.user_requests_forced_charging_recovery_mode = value; });
  update_string_setting("/updateSocMax", [](String value) { datalayer.battery.settings.max_percentage = static_cast<uint16_t>(value.toFloat() * 100); });
  update_int_setting("/set_can_id_cutoff", [](int value) { user_selected_CAN_ID_cutoff_filter = value; });

  update_string("/pause", [](String value) { 
      if (value == "" || value == "toggle") {
          bool current_status = (get_emulator_pause_status() == "PAUSED" || get_emulator_pause_status() == "PAUSING");
          setBatteryPause(!current_status, false); 
      } else {
          setBatteryPause(value == "true" || value == "1", false); 
      }
  });

  update_string("/equipmentStop", [](String value) {
    if (value == "true" || value == "1") setBatteryPause(true, false, true);
    else setBatteryPause(false, false, false);
  });

  // Route for editing SOC Calibration BYD
  update_string_setting("/editCalTargetSOC", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetSOC = static_cast<uint16_t>(value.toFloat());
  });

  // Route for editing AH Calibration BYD
  update_string_setting("/editCalTargetAH", [](String value) {
    datalayer_extended.bydAtto3.calibrationTargetAH = static_cast<uint16_t>(value.toFloat());
  });

  update_string_setting("/updateSocMin", [](String value) { datalayer.battery.settings.min_percentage = static_cast<uint16_t>(value.toFloat() * 100); });
  update_string_setting("/updateMaxChargeA", [](String value) { datalayer.battery.settings.max_user_set_charge_dA = static_cast<uint16_t>(value.toFloat() * 10); });
  update_string_setting("/updateMaxDischargeA", [](String value) { datalayer.battery.settings.max_user_set_discharge_dA = static_cast<uint16_t>(value.toFloat() * 10); });

  for (const auto& cmd : battery_commands) {
    auto route = String("/") + cmd.identifier;
    server.on(route.c_str(), HTTP_PUT,
        [cmd](AsyncWebServerRequest* request) {
          BatteryEmulatorSettingsStore auth_settings(true);
          bool is_auth_enabled = (auth_settings.getUInt("WEBAUTH", 1) == 1);
          if (is_auth_enabled && !checkAuth(request)) return request->requestAuthentication();
        },
        nullptr,
        [cmd](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
          String battIndex = "";
          if (len > 0) battIndex += (char)data[0];
          Battery* batt = battery;
          if (battIndex == "1") batt = battery2;
          if (battIndex == "2") batt = battery3;
          if (batt) cmd.action(batt);
          request->send(200, "text/plain", "Command performed.");
        });
  }

  update_int_setting("/updateUseVoltageLimit", [](int value) { datalayer.battery.settings.user_set_voltage_limits_active = value; });
  update_string_setting("/updateMaxChargeVoltage", [](String value) { datalayer.battery.settings.max_user_set_charge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10); });
  update_string_setting("/updateMaxDischargeVoltage", [](String value) { datalayer.battery.settings.max_user_set_discharge_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10); });
  update_string_setting("/updateBMSresetDuration", [](String value) { datalayer.battery.settings.user_set_bms_reset_duration_ms = static_cast<uint16_t>(value.toFloat() * 1000); });
  update_string_setting("/updateFakeBatteryVoltage", [](String value) { battery->set_fake_voltage(value.toFloat()); });
  update_int_setting("/TeslaBalAct", [](int value) { datalayer.battery.settings.user_requests_balancing = value; });
  update_string_setting("/BalTime", [](String value) { datalayer.battery.settings.balancing_max_time_ms = static_cast<uint32_t>(value.toFloat() * 60000); });
  update_string_setting("/BalFloatPower", [](String value) { datalayer.battery.settings.balancing_float_power_W = static_cast<uint16_t>(value.toFloat()); });
  update_string_setting("/BalMaxPackV", [](String value) { datalayer.battery.settings.balancing_max_pack_voltage_dV = static_cast<uint16_t>(value.toFloat() * 10); });
  update_string_setting("/BalMaxCellV", [](String value) { datalayer.battery.settings.balancing_max_cell_voltage_mV = static_cast<uint16_t>(value.toFloat()); });
  update_string_setting("/BalMaxDevCellV", [](String value) { datalayer.battery.settings.balancing_max_deviation_cell_voltage_mV = static_cast<uint16_t>(value.toFloat()); });

  if (charger) {
    update_string_setting("/updateChargeSetpointV", [](String value) { datalayer.charger.charger_setpoint_HV_VDC = value.toFloat(); }, [](String value) { float val = value.toFloat(); return (val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV) && (val * datalayer.charger.charger_setpoint_HV_IDC <= CHARGER_MAX_POWER); });
    update_string_setting("/updateChargeSetpointA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC = value.toFloat(); }, [](String value) { float val = value.toFloat(); return (val <= CHARGER_MAX_A) && (val <= datalayer.battery.settings.max_user_set_charge_dA) && (val * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER); });
    update_string_setting("/updateChargeEndA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC_END = value.toFloat(); });
    update_int_setting("/updateChargerHvEnabled", [](int value) { datalayer.charger.charger_HV_enabled = (bool)value; });
    update_int_setting("/updateChargerAux12vEnabled", [](int value) { datalayer.charger.charger_aux12V_enabled = (bool)value; });
  }

  def_route_with_auth("/api/data", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    JsonDocument doc;
    doc["sys"]["uptime"] = get_uptime();
    doc["sys"]["heap"] = ESP.getFreeHeap();
    doc["sys"]["heap_size"] = ESP.getHeapSize();
    doc["sys"]["psram"] = ESP.getFreePsram(); 
    doc["sys"]["psram_size"] = ESP.getPsramSize();
    doc["sys"]["status"] = get_emulator_pause_status().c_str();
    doc["sys"]["version"] = version_number;

    doc["sys"]["inv"] = inverter ? String(inverter->name()) + " " + String(datalayer.system.info.inverter_brand) : "None";
    String bat_proto = "None";
    if (battery) {
      bat_proto = String(datalayer.system.info.battery_protocol);
      if (battery3) bat_proto += " (Triple)";
      else if (battery2) bat_proto += " (Double)";
      if (datalayer.battery.info.chemistry == battery_chemistry_enum::LFP) bat_proto += " (LFP)";
    }
    doc["sys"]["bat"] = bat_proto;
    doc["sys"]["inv_cont"] = datalayer.system.status.inverter_allows_contactor_closing;

    auto populate_battery = [&](JsonObject b, Battery* bat, auto& dt_info, auto& dt_status) {
      b["en"] = (bat != nullptr);
      if (bat) {
        b["fault"] = (dt_status.bms_status == FAULT);
        b["soc"] = String((float)dt_status.real_soc / 100.0f, 2);
        b["soh"] = String((float)dt_status.soh_pptt / 100.0f, 2);
        b["v"] = String((float)dt_status.voltage_dV / 10.0f, 1);
        b["a"] = String((float)dt_status.current_dA / 10.0f, 1);
        b["p"] = dt_status.active_power_W;
        b["cmin"] = dt_status.cell_min_voltage_mV;
        b["cmax"] = dt_status.cell_max_voltage_mV;
        b["tmin"] = String((float)dt_status.temperature_min_dC / 10.0f, 1);
        b["tmax"] = String((float)dt_status.temperature_max_dC / 10.0f, 1);

        String stat_str = "UNKNOWN";
        if (dt_status.bms_status == ACTIVE) stat_str = "OK";
        else if (dt_status.bms_status == UPDATING) stat_str = "UPDATING";
        else if (dt_status.bms_status == FAULT) stat_str = "FAULT";
        b["stat"] = stat_str;

        if (dt_status.current_dA == 0) b["act"] = "Idle 💤";
        else if (dt_status.current_dA < 0) b["act"] = "Discharging 🔋⬇️";
        else b["act"] = "Charging 🔋⬆️";

        b["mc"] = dt_status.max_charge_power_W;
        b["md"] = dt_status.max_discharge_power_W;
        b["rem"] = dt_status.remaining_capacity_Wh;
        b["tot"] = dt_info.total_capacity_Wh;
        b["b_cont"] = (dt_status.bms_status != FAULT);
      }
    };

    populate_battery(doc["b1"].to<JsonObject>(), battery, datalayer.battery.info, datalayer.battery.status);
    populate_battery(doc["b2"].to<JsonObject>(), battery2, datalayer.battery2.info, datalayer.battery2.status);
    populate_battery(doc["b3"].to<JsonObject>(), battery3, datalayer.battery3.info, datalayer.battery3.status);

    JsonObject chg = doc["chg"].to<JsonObject>();
    chg["en"] = (charger != nullptr);
    if (charger) {
      chg["v"] = String(charger->HVDC_output_voltage(), 2);
      chg["a"] = String(charger->HVDC_output_current(), 2);
    }

    // String output;
    // serializeJson(doc, output);
    // request->send(200, "application/json", output);
    const char* cType = "application/json";
    AsyncResponseStream *response = request->beginResponseStream(cType);
    serializeJson(doc, *response);
    request->send(response);
  });

  // NEW API : JSON cell battery info
  def_route_with_auth("/api/cells", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    AsyncResponseStream *response = request->beginResponseStream("application/json");
    response->print("{");
    bool firstBat = true;
    
    auto printBat = [&](const char* key, Battery* bat, DATALAYER_BATTERY_TYPE& layer) {
        if (!bat || layer.info.number_of_cells == 0) return;
        if (!firstBat) response->print(",");
        firstBat = false;
        
        response->printf("\"%s\":{\"cv\":[", key);
        int cells = layer.info.number_of_cells;
        if (cells > MAX_AMOUNT_CELLS) cells = MAX_AMOUNT_CELLS; 
        
        for(int i=0; i<cells; i++) {
            response->print(layer.status.cell_voltages_mV[i]);
            if(i < cells-1) response->print(",");
        }
        response->print("],\"cb\":[");
        for(int i=0; i<cells; i++) {
            response->print(layer.status.cell_balancing_status[i] ? 1 : 0); 
            if(i < cells-1) response->print(",");
        }
        response->print("]}");
    };
    
    printBat("b1", battery, datalayer.battery);
    printBat("b2", battery2, datalayer.battery2);
    printBat("b3", battery3, datalayer.battery3);
    
    response->print("}");
    request->send(response);
  });

  def_route_with_auth("/debug", server, HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  def_route_with_auth("/reboot", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Rebooting server...");
    setBatteryPause(true, true, true, false);
    delay(1000);
    ESP.restart();
  });

  def_route_with_auth("/ota", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    String html = String(index_html_header); 
    html += R"rawliteral(
      <style>
        .ota-wrap { display: flex; flex-direction: column; height: 100%; gap: 15px; }
        .ota-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #3498db; padding: 0; overflow: hidden; display: flex; flex-direction: column; height: 80vh; min-height: 600px; }
        .ota-header { padding: 15px 20px; border-bottom: 1px solid #eee; background: #fbfcfc; }
        .ota-header h3 { margin: 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; }
        .ota-header p { margin: 5px 0 0 0; color: #7f8c8d; font-size: 0.9rem; }
        .ota-iframe { width: 100%; height: 100%; border: none; flex: 1; }
      </style>
      <div class="ota-wrap">
        <div class="ota-card">
          <div class="ota-header">
            <h3>🔄 System OTA Update</h3>
            <p>Upload a new firmware (.bin) file. Please do not turn off the power during the update.</p>
          </div>
          <iframe src="/update" class="ota-iframe" title="OTA Update"></iframe>
        </div>
      </div>
    )rawliteral";
    html += String(index_html_footer); 
    request->send(200, "text/html", html);
  });

  init_ElegantOTA();
  server.begin();
}

String getConnectResultString(wl_status_t status) {
  switch (status) {
    case WL_CONNECTED: return "Connected";
    case WL_NO_SHIELD: return "No shield";
    case WL_IDLE_STATUS: return "Idle status";
    case WL_NO_SSID_AVAIL: return "No SSID available";
    case WL_SCAN_COMPLETED: return "Scan completed";
    case WL_CONNECT_FAILED: return "Connect failed";
    case WL_CONNECTION_LOST: return "Connection lost";
    case WL_DISCONNECTED: return "Disconnected";
    default: return "Unknown";
  }
}

void ota_monitor() {
  ElegantOTA.loop(); 
  if (ota_active && ota_timeout_timer.elapsed()) {
    set_event(EVENT_OTA_UPDATE_TIMEOUT, 0);
    onOTAEnd(false);
  }
}

void init_ElegantOTA() {
  ElegantOTA.begin(&server);
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
}

String get_firmware_info_processor(const String& var) {
  if (var == "X") {
    String content = "";
    static JsonDocument doc;
    doc["hardware"] = esp32hal->name();
    doc["firmware"] = String(version_number);
    serializeJson(doc, content);
    return content;
  }
  return String();
}

String get_uptime() {
  uint64_t milliseconds = millis64();
  uint16_t total_days = milliseconds / (1000 * 60 * 60 * 24);
  uint32_t remaining_seconds_in_day = (milliseconds / 1000) % (60 * 60 * 24);
  uint32_t remaining_hours = remaining_seconds_in_day / (60 * 60);
  uint32_t remaining_minutes = (remaining_seconds_in_day % (60 * 60)) / 60;
  uint32_t remaining_seconds = remaining_seconds_in_day % 60;
  return (String)total_days + " days, " + (String)remaining_hours + " hours, " + (String)remaining_minutes + " mins, " + (String)remaining_seconds + " secs";
}

void onOTAStart() {
  setBatteryPause(true, true);
  set_event(EVENT_OTA_UPDATE, 0);
  clear_event(EVENT_OTA_UPDATE_TIMEOUT);
  ota_active = true;
  ota_timeout_timer.reset();
}

void onOTAProgress(size_t current, size_t final) {
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    logging.printf("OTA Progress Current: %u bytes, Final: %u bytes\n", current, final);
    ota_timeout_timer.reset();
  }
}

void onOTAEnd(bool success) {
  ota_active = false;
  clear_event(EVENT_OTA_UPDATE);
  if (success) {
    setBatteryPause(true, true, true, false);
    logging.println("OTA update finished successfully!");
  } else {
    logging.println("There was an error during OTA update!");
    setBatteryPause(false, false);
  }
}

template <typename T>  
String formatPowerValue(String label, T value, String unit, int precision, String color) {
  String result = "<h4 style='color: " + color + ";'>" + label + ": ";
  result += formatPowerValue(value, unit, precision);
  result += "</h4>";
  return result;
}

template <typename T>  
String formatPowerValue(T value, String unit, int precision) {
  String result = "";
  if (std::is_same<T, float>::value || std::is_same<T, uint16_t>::value || std::is_same<T, uint32_t>::value) {
    float convertedValue = static_cast<float>(value);
    if (convertedValue >= 1000.0f || convertedValue <= -1000.0f) {
      result += String(convertedValue / 1000.0f, precision) + " kW";
    } else {
      result += String(convertedValue, 0) + " W";
    }
  }
  result += unit;
  return result;
}