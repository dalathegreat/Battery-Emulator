#include "webserver.h"
#include <Preferences.h>
#include <ctime>
#include <vector>
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
#include <base64.h>

#include <string>

std::string http_username;
std::string http_password;

// Note: Kept external declaration matching your implementation for seamless UI settings access
extern const char* version_number;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Measure OTA progress
unsigned long ota_progress_millis = 0;

#include "advanced_battery_html.h"
#include "can_logging_html.h"
#include "can_replay_html.h"
#include "cellmonitor_html.h"
#include "debug_logging_html.h"
#include "events_html.h"
#include "index_html.h"
#include "settings_html.h"
#include "../../system_settings.h"

MyTimer ota_timeout_timer = MyTimer(15000);
bool ota_active = false;

const char get_firmware_info_html[] = R"rawliteral(%X%)rawliteral";

String importedLogs = "";      // Store the uploaded logfile contents in RAM
bool isReplayRunning = false;  // Global flag to track replay state

// True when user has updated settings that need a reboot to be effective.
bool settingsUpdated = false;

CAN_frame currentFrame = {.FD = true, .ext_ID = false, .DLC = 64, .ID = 0x12F, .data = {0}};

// 🚨 Forward Declarations
String get_uptime();
String get_firmware_info_processor(const String& var);

// =========================================================================
// 🌐 NEW AJAX DASHBOARD TEMPLATE (Stored in Flash Memory to save RAM)
// =========================================================================
const char dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMS Secure Dashboard</title>
  <style>
    body { font-family: 'Segoe UI', Tahoma, sans-serif; background-color: #121212; color: #ffffff; margin: 0; padding: 20px; text-align: center; }
    .header-box { background-color: #303E47; padding: 15px; border-radius: 15px; margin-bottom: 10px; }
    .proto-box { background-color: #263238; padding: 10px; border-radius: 10px; margin-bottom: 20px; font-size: 0.95em; color: #ccc; }
    h2, h3, h4 { margin: 5px 0; }
    
    .battery-container { display: flex; flex-wrap: wrap; gap: 15px; justify-content: center; margin-bottom: 20px; }
    .bat-card { flex: 1; min-width: 280px; max-width: 400px; background-color: #2D3F2F; padding: 20px; border-radius: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); transition: 0.3s; }
    .bat-card.fault { background-color: #A70107; }
    .bat-card.hidden { display: none; }
    
    /* Style for hidden data */
    .details-box { margin-top: 15px; padding-top: 15px; border-top: 1px dashed #555; display: none; text-align: left; background: rgba(0,0,0,0.2); border-radius: 10px; padding: 15px; }
    .details-box.show { display: block; animation: fadeIn 0.3s; }
    .btn-toggle { background-color: transparent; border: 1px solid #4CAF50; color: #4CAF50; width: 100%; margin-top: 10px; padding: 8px; border-radius: 5px; cursor: pointer; transition: 0.2s; }
    .btn-toggle:hover { background-color: #4CAF50; color: #fff; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(-5px); } to { opacity: 1; transform: translateY(0); } }

    .info-card { background-color: #333; padding: 15px; border-radius: 15px; margin-bottom: 15px; }
    .btn-group { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin-top: 15px; }
    button { background-color: #505E67; color: white; border: none; padding: 10px 15px; cursor: pointer; border-radius: 8px; font-size: 14px; font-weight: bold; transition: 0.2s; }
    button:hover { background-color: #3A4A52; }
    .btn-danger { background-color: #A70107; } .btn-danger:hover { background-color: #850005; }
    .btn-success { background-color: #198754; } .btn-success:hover { background-color: #146c43; }
    .status-text { font-size: 0.9rem; color: #aaa; margin-top: 15px; }

    /* ⚡ Energy Flow Animation ⚡ */
    .flow-wrapper { 
      display: flex; align-items: center; justify-content: space-between; 
      background: #3a4b54; padding: 15px 20px; border-radius: 15px; margin: 15px 0; box-shadow: 0 4px 6px rgba(0,0,0,0.3);
    }
    .flow-box { 
      background: #2a3b45; padding: 10px; border-radius: 8px; font-weight: bold; text-align: center; width: 80px; border: 1px solid #4d5f69;
    }
    .flow-wire { 
      flex-grow: 1; height: 8px; background: #1e2b32; margin: 0 15px; position: relative; overflow: hidden; border-radius: 4px; box-shadow: inset 0 1px 3px rgba(0,0,0,0.5);
    }
    .flow-particles { 
      width: 200%; height: 100%; position: absolute; left: 0; background-size: 40px 100%; 
    }
    
    /* Animation lef or right */
    @keyframes flow-charge { 0% { transform: translateX(-50%); } 100% { transform: translateX(0%); } }
    @keyframes flow-discharge { 0% { transform: translateX(0%); } 100% { transform: translateX(-50%); } }
    
    /* charge/discharge status (charge=green-left-to-right,discharge=orange-right-to-left) */
    .status-charging { 
      animation: flow-charge 1s linear infinite; 
      background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #4CAF50 50%, #81C784 100%); 
      filter: drop-shadow(0 0 4px #4CAF50);
    }
    .status-discharging { 
      animation: flow-discharge 1s linear infinite; 
      background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #FF9800 50%, #FFC107 100%); 
      filter: drop-shadow(0 0 4px #FF9800);
    }
    .status-idle { 
      opacity: 0.1; 
      background-image: linear-gradient(90deg, transparent 0%, transparent 50%, #ffffff 50%, #ffffff 100%); 
    }
  </style>
</head>
<body>

  <div class="header-box">
    <h2>⚡ Battery Emulator <span id="sys_version" style="font-size: 0.8em; color: #aaa;"></span></h2>
    <h4 style="color: #4CAF50;">Uptime: <span id="sys_uptime">--</span> | Free RAM: <span id="sys_ram">--</span></h4>
    <h4>Power Status: <span id="sys_status" style="color: #FFC107;">--</span></h4>
  </div>

  <div class="proto-box">
    🔗 <b>Inverter:</b> <span id="sys_inv" style="color: #4CAF50;">--</span> &nbsp;|&nbsp; 
    <b>Battery:</b> <span id="sys_bat" style="color: #2196F3;">--</span>
  </div>

  <div class="flow-wrapper">
      <div class="flow-box" style="color: #00bcd4;">INVERTER</div>
      <div class="flow-wire">
        <div id="energy-flow" class="flow-particles status-idle"></div>
      </div>
      <div class="flow-box" style="color: #4CAF50;">BATTERY</div>
  </div>

  <div class="battery-container">
    <div id="card_b1" class="bat-card hidden">
      <h3>🔋 Main Battery</h3>
      <h4>SOC: <span id="b1_soc">--</span>% | SOH: <span id="b1_soh">--</span>%</h4>
      <h4><span id="b1_v" style="color: #4CAF50; font-size: 1.2em;">--</span> V &nbsp;&nbsp; <span id="b1_a" style="color: #2196F3; font-size: 1.2em;">--</span> A</h4>
      <h4>Power: <span id="b1_p">--</span> W</h4>
      <h4>Cell (Min/Max): <span id="b1_cmin">--</span> / <span id="b1_cmax">--</span> mV</h4>
      <h4>Temp: <span id="b1_tmin">--</span> / <span id="b1_tmax">--</span> &deg;C</h4>
      
      <button class="btn-toggle" onclick="toggleDetails('b1_more')">🔽 Show / Hide Details</button>
      <div id="b1_more" class="details-box">
        <h4>📡 System Status: <span id="b1_stat" style="color:#FFC107;">--</span></h4>
        <h4>⚡ Activity: <span id="b1_act" style="color:#2196F3;">--</span></h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔋 Total Capacity: <span id="b1_tot">--</span> Wh</h4>
        <h4>🔋 Remaining: <span id="b1_rem">--</span> Wh</h4>
        <h4>⚡ Max Charge: <span id="b1_mc">--</span> W</h4>
        <h4>⚡ Max Discharge: <span id="b1_md">--</span> W</h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔌 Contactor Allowed:</h4>
        <h4 style="margin-left: 20px;">Battery: <span id="b1_cont_b">--</span> | Inverter: <span id="sys_cont_i">--</span></h4>
      </div>
    </div>

    <div id="card_b2" class="bat-card hidden">
      <h3>🔋 Battery 2</h3>
      <h4>SOC: <span id="b2_soc">--</span>% | SOH: <span id="b2_soh">--</span>%</h4>
      <h4><span id="b2_v" style="color: #4CAF50; font-size: 1.2em;">--</span> V &nbsp;&nbsp; <span id="b2_a" style="color: #2196F3; font-size: 1.2em;">--</span> A</h4>
      <h4>Power: <span id="b2_p">--</span> W</h4>
      <h4>Cell (Min/Max): <span id="b2_cmin">--</span> / <span id="b2_cmax">--</span> mV</h4>
      <h4>Temp: <span id="b2_tmin">--</span> / <span id="b2_tmax">--</span> &deg;C</h4>
      
      <button class="btn-toggle" onclick="toggleDetails('b2_more')">🔽 Show / Hide Details</button>
      <div id="b2_more" class="details-box">
        <h4>📡 System Status: <span id="b2_stat" style="color:#FFC107;">--</span></h4>
        <h4>⚡ Activity: <span id="b2_act" style="color:#2196F3;">--</span></h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔋 Total Capacity: <span id="b2_tot">--</span> Wh</h4>
        <h4>🔋 Remaining: <span id="b2_rem">--</span> Wh</h4>
        <h4>⚡ Max Charge: <span id="b2_mc">--</span> W</h4>
        <h4>⚡ Max Discharge: <span id="b2_md">--</span> W</h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔌 Contactor Allowed:</h4>
        <h4 style="margin-left: 20px;">Battery: <span id="b2_cont_b">--</span> | Inverter: <span class="sys_cont_i_dup">--</span></h4>
      </div>
    </div>

    <div id="card_b3" class="bat-card hidden">
      <h3>🔋 Battery 3</h3>
      <h4>SOC: <span id="b3_soc">--</span>% | SOH: <span id="b3_soh">--</span>%</h4>
      <h4><span id="b3_v" style="color: #4CAF50; font-size: 1.2em;">--</span> V &nbsp;&nbsp; <span id="b3_a" style="color: #2196F3; font-size: 1.2em;">--</span> A</h4>
      <h4>Power: <span id="b3_p">--</span> W</h4>
      <h4>Cell (Min/Max): <span id="b3_cmin">--</span> / <span id="b3_cmax">--</span> mV</h4>
      <h4>Temp: <span id="b3_tmin">--</span> / <span id="b3_tmax">--</span> &deg;C</h4>
      
      <button class="btn-toggle" onclick="toggleDetails('b3_more')">🔽 Show / Hide Details</button>
      <div id="b3_more" class="details-box">
        <h4>📡 System Status: <span id="b3_stat" style="color:#FFC107;">--</span></h4>
        <h4>⚡ Activity: <span id="b3_act" style="color:#2196F3;">--</span></h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔋 Total Capacity: <span id="b3_tot">--</span> Wh</h4>
        <h4>🔋 Remaining: <span id="b3_rem">--</span> Wh</h4>
        <h4>⚡ Max Charge: <span id="b3_mc">--</span> W</h4>
        <h4>⚡ Max Discharge: <span id="b3_md">--</span> W</h4>
        <hr style="border: 0; border-top: 1px solid #555; margin: 10px 0;">
        <h4>🔌 Contactor Allowed:</h4>
        <h4 style="margin-left: 20px;">Battery: <span id="b3_cont_b">--</span> | Inverter: <span class="sys_cont_i_dup">--</span></h4>
      </div>
    </div>
  </div>

  <div id="card_chg" class="info-card hidden" style="background-color: #D35400;">
    <h3>🔌 Charger Output</h3>
    <h4><span id="chg_v" style="font-size: 1.2em;">--</span> V &nbsp;&nbsp; <span id="chg_a" style="font-size: 1.2em;">--</span> A</h4>
  </div>

  <div class="info-card">
    <h3>⚙️ Control & Actions</h3>
    <div class="btn-group">
      <button onclick="window.location.href='/settings'">⚙️ Settings</button>
      <button onclick="window.location.href='/advanced'">📊 Adv. Info</button>
      <button onclick="window.location.href='/cellmonitor'">🔋 Cell Monitor</button>
      <button onclick="window.location.href='/canlog'">📝 CAN Log</button>
      <button onclick="window.location.href='/canreplay'">⏪ CAN Replay</button>
      <button onclick="window.location.href='/log'">📄 System Log</button>
      <button onclick="window.location.href='/events'">⚠️ Events</button>
      
      <button onclick="PauseBattery(false)">▶️ Resume</button>
      <button onclick="if(confirm('Pause?')) { PauseBattery(true); }">⏸️ Pause</button>
      <button class="btn-danger" onclick="if(confirm('Open Contactors?')) { estop(true); }">🛑 Open Contactor</button>
      <button class="btn-success" onclick="if(confirm('Close Contactors?')) { estop(false); }">✅ Close Contactor</button>
      
      <button onclick="window.location.href='/update'">🔄 OTA Update</button>
      <button onclick="if(confirm('Reboot?')) { window.location.href='/reboot'; }">🔄 Reboot</button>
      <button class="btn-danger" onclick="logout()">🚪 Logout</button>
    </div>
  </div>
  <div class="status-text">Last Updated: <span id="last_update">Waiting for data...</span></div>

  <script>
    // toggle hide/show
    function toggleDetails(id) {
      document.getElementById(id).classList.toggle('show');
    }
    // 🚀 JavaScript: Fetches JSON API every 2 seconds quietly
    function fetchBatteryData() {
      fetch('/api/data')
        .then(response => response.json())
        .then(data => {

          document.getElementById('sys_version').innerText = "v" + data.sys.version;
          document.getElementById('sys_uptime').innerText = data.sys.uptime;
          document.getElementById('sys_ram').innerText = data.sys.heap + " Bytes";
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
              
              // more Adv. info
              document.getElementById(id + '_stat').innerText = bData.stat;
              document.getElementById(id + '_mc').innerText = bData.mc;
              document.getElementById(id + '_md').innerText = bData.md;
              document.getElementById(id + '_rem').innerText = bData.rem;
              document.getElementById(id + '_tot').innerText = bData.tot;

              if(bData.fault) document.getElementById('card_' + id).classList.add('fault');
              else document.getElementById('card_' + id).classList.remove('fault');
            }
          };

          // ⚡ update Energy Flow Animation
          let totalPowerW = 0;
          if (data.b1 && data.b1.en) {
              totalPowerW += parseFloat(data.b1.p) || 0;
          }
          if (data.b2 && data.b2.en) {
              totalPowerW += parseFloat(data.b2.p) || 0;
          }
          if (data.b3 && data.b3.en) {
              totalPowerW += parseFloat(data.b3.p) || 0;
          }

          let flowElement = document.getElementById('energy-flow');
          
          if (totalPowerW > 50) {
            // Charging the battery (green light moving from left to right).
            flowElement.className = 'flow-particles status-charging';
            
            // Speed varies with power!
            let speed = Math.max(0.2, 2000 / totalPowerW); 
            flowElement.style.animationDuration = speed + 's';
            
          } else if (totalPowerW < -50) {
            // Discharge (orange light flashing from right to left)
            flowElement.className = 'flow-particles status-discharging';
            
            let absPower = Math.abs(totalPowerW);
            let speed = Math.max(0.2, 2000 / absPower); 
            flowElement.style.animationDuration = speed + 's';
            
          } else {
           // Standby (stop running, dim lights)
            flowElement.className = 'flow-particles status-idle';
            flowElement.style.animationDuration = '0s';
          }

          updateBat('b1', data.b1);
          updateBat('b2', data.b2);
          updateBat('b3', data.b3);          

          // Charger Data      
          if(data.chg.en) {
            document.getElementById('card_chg').classList.remove('hidden');
            document.getElementById('chg_v').innerText = data.chg.v;
            document.getElementById('chg_a').innerText = data.chg.a;
          }

          document.getElementById('last_update').innerText = new Date().toLocaleTimeString();
        })
        .catch(error => console.error('Error:', error));
    }

    function PauseBattery(pause){ var xhr=new XMLHttpRequest(); xhr.open('GET','/pause?value='+pause,true); xhr.send(); }
    function estop(stop){ var xhr=new XMLHttpRequest(); xhr.open('GET','/equipmentStop?value='+stop,true); xhr.send(); }
    function logout() { alert('Logged out. Click Cancel if prompt appears.'); var xhr=new XMLHttpRequest(); xhr.open('GET','/logout',true,'logout','logout'); xhr.send(); setTimeout(()=>{window.location.href='/logout';}, 500); }

    fetchBatteryData();
    setInterval(fetchBatteryData, 2000); 
  </script>
</body>
</html>
)rawliteral";

// =========================================================================
// 📄 SUB-PAGE TEMPLATE (For Events, Advanced, CellMonitor)
// =========================================================================
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
    /* Reformat for Dark Mode */
    table { width: 100%; border-collapse: collapse; background-color: #2D3F2F; border-radius: 10px; overflow: hidden; margin-top: 10px; }
    th, td { border: 1px solid #444; padding: 10px; text-align: left; }
    th { background-color: #1A251B; color: #4CAF50; }
    h2, h3, h4 { color: #4CAF50; }
    a { color: #2196F3; }
  </style>
</head>
<body>
  <button onclick="window.location.href='/'">⬅️ Back to Dashboard</button>
  
  <div id="content">
    %X%
  </div>
</body>
</html>
)rawliteral";

// =========================================================================
// 🚀 HYBRID CAN REPLAY SYSTEM (SD Card + RAM Fallback)
// =========================================================================

// Global variables for Hybrid Replay System
bool use_sd_for_replay = false;
File uploadReplayFile;
size_t current_upload_size = 0;
const size_t MAX_RAM_REPLAY_SIZE = 100 * 1024; // 100 KB safety limit for RAM fallback

void handleFileUpload(AsyncWebServerRequest* request, String filename, size_t index, uint8_t* data, size_t len, bool final) {
  if (!index) {
    // 1. Initialization (Run once at the start of upload)
    logging.printf("Receiving replay file: %s\n", filename.c_str());
    current_upload_size = 0;
    importedLogs = ""; // Clear existing RAM buffer
    use_sd_for_replay = false;

    // 2. Try to create a temporary file on the SD card
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
    // --- MODE 1: SD CARD STREAMING (No RAM limit) ---
    if (uploadReplayFile) {
      uploadReplayFile.write(data, len);
    }
  } else {
    // --- MODE 2: RAM FALLBACK (With safety limit) ---
    if (current_upload_size > MAX_RAM_REPLAY_SIZE) {
      if (final) {
        logging.println("ERROR: File too large for RAM! Upload aborted to prevent crash.");
        request->send(400, "text/plain", "File too large! Please insert an SD Card to upload files larger than 100KB.");
      }
      return; // Stop saving to prevent Out-Of-Memory (OOM) crash
    }
    importedLogs += String((char*)data).substring(0, len);
  }

  if (final) {
    // 3. Cleanup and Response
    if (use_sd_for_replay && uploadReplayFile) {
      uploadReplayFile.close();
    }
    
    // If RAM limit exceeded, don't send success message
    if (!use_sd_for_replay && current_upload_size > MAX_RAM_REPLAY_SIZE) {
      return; 
    }
    
    logging.printf("Upload Complete! Total Size: %d bytes\n", current_upload_size);
    request->send(200, "text/plain", "File uploaded successfully");
  }
}

void canReplayTask(void* param) {
  bool hasData = false;
  
  // 1. Check if we have data to replay
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

  // 2. Start the replay loop
  do {
    float firstTimestamp = -1.0f;
    float lastTimestamp = 0.0f;
    bool firstMessageSent = false;
    
    File replayFile;
    int ramIndex = 0; // Pointer for RAM reading

    if (use_sd_for_replay) {
      replayFile = SD_MMC.open("/replay_temp.txt", FILE_READ);
      if (!replayFile) break; // Break if file access fails
    }

    // Process line by line
    while (true) {
      String line = "";
      
      // --- A. Read the next line (Seamlessly from SD or RAM) ---
      if (use_sd_for_replay) {
        if (!replayFile.available()) break; // End of File
        line = replayFile.readStringUntil('\n');
      } else {
        if (ramIndex >= importedLogs.length()) break; // End of String
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

      // --- B. Parse the CAN Message (Same logic as original) ---
      int timeStart = line.indexOf("(") + 1;
      int timeEnd = line.indexOf(")");
      if (timeStart == 0 || timeEnd == -1) continue;

      float currentTimestamp = line.substring(timeStart, timeEnd).toFloat();

      if (firstTimestamp < 0) firstTimestamp = currentTimestamp;

      // --- C. Calculate Delay ---
      if (!firstMessageSent) {
        firstMessageSent = true;
        firstTimestamp = currentTimestamp; 
      } else {
        float deltaT = (currentTimestamp - lastTimestamp) * 1000.0f;
        // Sanity check: Avoid blocking RTOS if delay is unrealistically huge (e.g., > 10 seconds)
        if (deltaT > 0 && deltaT < 10000) { 
          vTaskDelay((int)deltaT / portTICK_PERIOD_MS);
        }
      }

      lastTimestamp = currentTimestamp;

      // --- D. Extract CAN Data ---
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

      // --- E. Transmit ---
      transmit_can_frame_to_interface(&currentFrame, (CAN_Interface)datalayer.system.info.can_replay_interface);
      
    } // End of single file/log loop

    // Close file before next loop iteration
    if (use_sd_for_replay && replayFile) {
      replayFile.close();
    }

  } while (datalayer.system.info.loop_playback); // Repeat if loop is enabled

  // --- 3. Cleanup & Exit Task ---
  importedLogs = ""; // Free RAM immediately after replay completes
  isReplayRunning = false; 
  vTaskDelete(NULL); // Delete the RTOS task
}

// 🛡️ Safe Authentication Check
bool checkAuth(AsyncWebServerRequest *request) {
    BatteryEmulatorSettingsStore auth_settings(true);
    bool is_auth_enabled = (auth_settings.getUInt("WEBAUTH", 0) == 1);

    if (!is_auth_enabled) return true; // Auth disabled, allow access
    
    String u = (http_username.length() > 0) ? http_username.c_str() : DEFAULT_WEB_USER;
    String p = (http_password.length() >= 4) ? http_password.c_str() : DEFAULT_WEB_PASS;
    String expectedAuth = "Basic " + base64::encode(u + ":" + p);
    
    if (request->hasHeader("Authorization")) {
        if (request->getHeader("Authorization")->value().equals(expectedAuth)) return true;
    }
    return false; // Auth failed
}

// Custom route definition wrapper
void def_route_with_auth(const char* uri, AsyncWebServer& serv, WebRequestMethodComposite method,
                         std::function<void(AsyncWebServerRequest*)> handler) {
  serv.on(uri, method, [handler, uri](AsyncWebServerRequest* request) {
    if (!checkAuth(request)) {
      Serial.printf("[DEBUG] 🛡️ IP: %s invalid password Pop-up...\n", request->client()->remoteIP().toString().c_str());
      AsyncWebServerResponse *response = request->beginResponse(401, "text/plain", "Authentication Required");
      response->addHeader("WWW-Authenticate", "Basic realm=\"Battery_Emulator_Secure\"");
      return request->send(response); 
    }
    handler(request);
  });
}

// Prevent Chunked Encoding problem for large pages
void send_large_page_safely(AsyncWebServerRequest* request, std::function<String(const String&)> processor_func) {
    String html = String(index_html);
    String payload = processor_func("X");
    html.replace("%X%", payload);
    request->send(200, "text/html", html);
}

// =========================================================================
// WebServer Initialization
// =========================================================================

void init_webserver() {

  // Logout Route
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest* request) { 
    request->send(401, "text/html", "<h2 style='font-family:sans-serif; color:white; background:black; padding:20px;'>Logged Out Successfully</h2><p style='color:white; background:black;'>Please close your browser window, or <a href='/' style='color:#00ff00;'>click here to login again</a>.</p>");
  });

  // Route for firmware info from ota update page
  def_route_with_auth("/GetFirmwareInfo", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "application/json", get_firmware_info_html, get_firmware_info_processor);
  });

  // -----------------------------------------------------------------------
  // 1️⃣ ROOT ROUTE: Serve static HTML from Flash (PROGMEM)
  // -----------------------------------------------------------------------
  def_route_with_auth("/", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    ota_active = false;
    request->send(200, "text/html", dashboard_html);
  });

  // Route for going to settings web page
  def_route_with_auth("/settings", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    auto settings = std::make_shared<BatteryEmulatorSettingsStore>(true);
    request->send(200, "text/html", settings_html,
                  [settings](const String& content) { return settings_processor(content, *settings); });
  });

  // Route for going to advanced battery info web page
  def_route_with_auth("/advanced", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    send_large_page_safely(request, advanced_battery_processor);
  });

  // Route for going to CAN logging web page
  def_route_with_auth("/canlog", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(request->beginResponse(200, "text/html", can_logger_processor()));
  });

  // Route for going to CAN replay web page
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

  if (datalayer.system.info.web_logging_active || datalayer.system.info.SD_logging_active) {
    server.on("/log", HTTP_GET, [](AsyncWebServerRequest* request) {
      request->send(request->beginResponse(200, "text/html", debug_logger_processor()));
    });
  }

  server.on("/stop_can_logging", HTTP_GET, [](AsyncWebServerRequest* request) {
    datalayer.system.info.can_logging_active = false;
    request->send(200, "text/plain", "Logging stopped");
  });

  server.on("/import_can_log", HTTP_POST, [](AsyncWebServerRequest* request) {
        request->send(200, "text/plain", "Ready to receive file."); 
      }, handleFileUpload);

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
      String logs = String(datalayer.system.info.logged_can_messages);
      if (logs.length() == 0) logs = "No logs available.";
      time_t now = time(nullptr);
      struct tm timeinfo;
      localtime_r(&now, &timeinfo);
      char filename[32];
      if (!strftime(filename, sizeof(filename), "canlog_%H-%M-%S.txt", &timeinfo)) {
        strcpy(filename, "battery_emulator_can_log.txt");
      }
      AsyncWebServerResponse* response = request->beginResponse(200, "text/plain", logs);
      response->addHeader("Content-Disposition", String("attachment; filename=\"") + String(filename) + "\"");
      request->send(response);
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

  def_route_with_auth("/cellmonitor", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    send_large_page_safely(request, cellmonitor_processor);
  });

  def_route_with_auth("/events", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    send_large_page_safely(request, events_processor);
  });

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

  // Settings Definitions
  const char* boolSettingNames[] = {
      "DBLBTR",        "CNTCTRL",      "CNTCTRLDBL",  "PWMCNTCTRL",   "PERBMSRESET",   "SDLOGENABLED", "STATICIP",
      "REMBMSRESET",   "EXTPRECHARGE", "USBENABLED",  "CANLOGUSB",    "WEBENABLED",    "CANFDASCAN",   "CANLOGSD",
      "WIFIAPENABLED", "MQTTENABLED",  "NOINVDISC",   "HADISC",       "MQTTTOPICS",    "MQTTCELLV",    "INVICNT",
      "GTWRHD",        "DIGITALHVIL",  "PERFPROFILE", "INTERLOCKREQ", "SOCESTIMATED",  "PYLONOFFSET",  "PYLONORDER",
      "DEYEBYD",       "NCCONTACTOR",  "TRIBTR",      "CNTCTRLTRI",   "ESPNOWENABLED", "EPAPREFRESHBTN"
  };

  const char* uintSettingNames[] = {
      "BATTCVMAX",  "BATTCVMIN",   "MAXPRETIME", "MAXPREFREQ",  "WIFICHANNEL", "DCHGPOWER", "CHGPOWER",  "LOCALIP1",
      "LOCALIP2",   "LOCALIP3",    "LOCALIP4",   "GATEWAY1",    "GATEWAY2",    "GATEWAY3",  "GATEWAY4",  "SUBNET1",
      "SUBNET2",    "SUBNET3",     "SUBNET4",    "MQTTPORT",    "MQTTTIMEOUT", "SOFAR_ID",  "PYLONSEND", "INVCELLS",
      "INVMODULES", "INVCELLSPER", "INVVLEVEL",  "INVCAPACITY", "INVBTYPE",    "CANFREQ",   "CANFDFREQ", "PRECHGMS",
      "PWMFREQ",    "PWMHOLD",     "GTWCOUNTRY", "GTWMAPREG",   "GTWCHASSIS",  "GTWPACK",   "LEDMODE",   "GPIOOPT1",
      "GPIOOPT2",   "GPIOOPT3",    "INVSUNTYPE", "GPIOOPT4",    "LEDTAIL",     "LEDCOUNT",  "WEBAUTH",   "DISPLAYTYPE"
  };

  const char* stringSettingNames[] = {"APNAME",       "APPASSWORD", "HOSTNAME",        "MQTTSERVER",     "MQTTUSER",
                                      "MQTTPASSWORD", "MQTTTOPIC",  "MQTTOBJIDPREFIX", "MQTTDEVICENAME", "HADEVICEID"};

  def_route_with_auth("/saveSettings", server, HTTP_POST,
            [boolSettingNames, stringSettingNames, uintSettingNames](AsyncWebServerRequest* request) {
              BatteryEmulatorSettingsStore settings;
              int numParams = request->params();
              for (int i = 0; i < numParams; i++) {
                auto p = request->getParam(i);
                if (p->name() == "inverter") {
                  settings.saveUInt("INVTYPE", (int)static_cast<InverterProtocolType>(atoi(p->value().c_str())));
                } else if (p->name() == "INVCOMM") {
                  settings.saveUInt("INVCOMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "battery") {
                  settings.saveUInt("BATTTYPE", (int)static_cast<BatteryType>(atoi(p->value().c_str())));
                } else if (p->name() == "BATTCHEM") {
                  settings.saveUInt("BATTCHEM", (int)static_cast<battery_chemistry_enum>(atoi(p->value().c_str())));
                } else if (p->name() == "BATTCOMM") {
                  settings.saveUInt("BATTCOMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "BATTPVMAX") {
                  settings.saveUInt("BATTPVMAX", (int)(p->value().toFloat() * 10.0f));
                } else if (p->name() == "BATTPVMIN") {
                  settings.saveUInt("BATTPVMIN", (int)(p->value().toFloat() * 10.0f));
                } else if (p->name() == "charger") {
                  settings.saveUInt("CHGTYPE", (int)static_cast<ChargerType>(atoi(p->value().c_str())));
                } else if (p->name() == "CHGCOMM") {
                  settings.saveUInt("CHGCOMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "EQSTOP") {
                  settings.saveUInt("EQSTOP", (int)static_cast<STOP_BUTTON_BEHAVIOR>(atoi(p->value().c_str())));
                } else if (p->name() == "BATT2COMM") {
                  settings.saveUInt("BATT2COMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "BATT3COMM") {
                  settings.saveUInt("BATT3COMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "shunt") {
                  settings.saveUInt("SHUNTTYPE", (int)static_cast<ShuntType>(atoi(p->value().c_str())));
                } else if (p->name() == "SHUNTCOMM") {
                  settings.saveUInt("SHUNTCOMM", (int)static_cast<comm_interface>(atoi(p->value().c_str())));
                } else if (p->name() == "SSID") {
                  settings.saveString("SSID", p->value().c_str());
                  ssid = settings.getString("SSID", "").c_str();
                } else if (p->name() == "PASSWORD") {
                  settings.saveString("PASSWORD", p->value().c_str());
                  password = settings.getString("PASSWORD", "").c_str();
                } else if (p->name() == "WEBAUTH") {
                  int val = atoi(p->value().c_str());
                  settings.saveUInt("WEBAUTH", val);
                } else if (p->name() == "WEBUSER") {
                  if (p->value().length() > 0) {
                    settings.saveString("WEBUSER", p->value().c_str());
                    http_username = p->value().c_str(); 
                  }
                } else if (p->name() == "WEBPASS") {
                  if (p->value().length() >= 4) {
                    settings.saveString("WEBPASS", p->value().c_str());
                    http_password = p->value().c_str();
                  }
                } else if (p->name() == "MQTTPUBLISHMS") {
                  auto interval = atoi(p->value().c_str()) * 1000;  // Convert seconds to milliseconds
                  settings.saveUInt("MQTTPUBLISHMS", interval); 
                } else if (p->name() == "DISPLAYTYPE") {
                  int val = atoi(p->value().c_str());
                  settings.saveUInt("DISPLAYTYPE", val);
                }

                for (auto& uintSetting : uintSettingNames) {
                  if (p->name() == uintSetting) {
                    auto value = atoi(p->value().c_str());
                    if (settings.getUInt(uintSetting, 0) != value) settings.saveUInt(uintSetting, value);
                  }
                }

                for (auto& stringSetting : stringSettingNames) {
                  if (p->name() == stringSetting) {
                    if (settings.getString(stringSetting) != p->value()) settings.saveString(stringSetting, p->value().c_str());
                  }
                }
              }

              for (auto& boolSetting : boolSettingNames) {
                auto p = request->getParam(boolSetting, true);
                const bool default_value = (std::string(boolSetting) == std::string("WIFIAPENABLED"));
                const bool value = p != nullptr && p->value() == "on";
                if (settings.getBool(boolSetting, default_value) != value) settings.saveBool(boolSetting, value);
              }

              settingsUpdated = settings.were_settings_updated();
              request->redirect("/settings");
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
  
  update_string("/pause", [](String value) { setBatteryPause(value == "true" || value == "1", false); });
  update_string("/equipmentStop", [](String value) {
    if (value == "true" || value == "1") setBatteryPause(true, false, true); 
    else setBatteryPause(false, false, false);
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
    update_string_setting("/updateChargeSetpointV", [](String value) { datalayer.charger.charger_setpoint_HV_VDC = value.toFloat(); },
        [](String value) { float val = value.toFloat(); return (val <= CHARGER_MAX_HV && val >= CHARGER_MIN_HV) && (val * datalayer.charger.charger_setpoint_HV_IDC <= CHARGER_MAX_POWER); });
    update_string_setting("/updateChargeSetpointA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC = value.toFloat(); },
        [](String value) { float val = value.toFloat(); return (val <= CHARGER_MAX_A) && (val <= datalayer.battery.settings.max_user_set_charge_dA) && (val * datalayer.charger.charger_setpoint_HV_VDC <= CHARGER_MAX_POWER); });
    update_string_setting("/updateChargeEndA", [](String value) { datalayer.charger.charger_setpoint_HV_IDC_END = value.toFloat(); });
    update_int_setting("/updateChargerHvEnabled", [](int value) { datalayer.charger.charger_HV_enabled = (bool)value; });
    update_int_setting("/updateChargerAux12vEnabled", [](int value) { datalayer.charger.charger_aux12V_enabled = (bool)value; });
  }

  // -----------------------------------------------------------------------
  // 2️⃣ API ROUTE: JSON Data Endpoint (using ArduinoJson v7)
  // -----------------------------------------------------------------------
  def_route_with_auth("/api/data", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    
    // 🚨 Fixed: Use JsonDocument instead of deprecated DynamicJsonDocument in v7                                    
    JsonDocument doc; 

    // System Data
    doc["sys"]["uptime"] = get_uptime();
    doc["sys"]["heap"]   = ESP.getFreeHeap();
    doc["sys"]["status"] = get_emulator_pause_status().c_str();
    doc["sys"]["version"] = version_number;

    // 🔗 Show Protocol (Inverter & Battery)
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

    // Helper macro to populate ALL battery data
    auto populate_battery = [&](JsonObject b, Battery* bat, auto& dt_info, auto& dt_status) {
      b["en"] = (bat != nullptr);
      if (bat) {
        // Basic bat info
        b["fault"] = (dt_status.bms_status == FAULT);
        b["soc"]   = String((float)dt_status.real_soc / 100.0f, 2);
        b["soh"]   = String((float)dt_status.soh_pptt / 100.0f, 2);
        b["v"]     = String((float)dt_status.voltage_dV / 10.0f, 1);
        b["a"]     = String((float)dt_status.current_dA / 10.0f, 1);
        b["p"]     = dt_status.active_power_W;
        b["cmin"]  = dt_status.cell_min_voltage_mV;
        b["cmax"]  = dt_status.cell_max_voltage_mV;
        b["tmin"]  = String((float)dt_status.temperature_min_dC / 10.0f, 1);
        b["tmax"]  = String((float)dt_status.temperature_max_dC / 10.0f, 1);
        
        // Adv. Batt info ( Toggle Show Details)
        String stat_str = "UNKNOWN";
        if (dt_status.bms_status == ACTIVE) stat_str = "OK";
        else if (dt_status.bms_status == UPDATING) stat_str = "UPDATING";
        else if (dt_status.bms_status == FAULT) stat_str = "FAULT";
        b["stat"] = stat_str;
        
        if (dt_status.current_dA == 0) b["act"] = "Idle 💤";
        else if (dt_status.current_dA < 0) b["act"] = "Discharging 🔋⬇️";
        else b["act"] = "Charging 🔋⬆️";

        b["mc"]   = dt_status.max_charge_power_W;
        b["md"]   = dt_status.max_discharge_power_W;
        b["rem"]  = dt_status.remaining_capacity_Wh;
        b["tot"]  = dt_info.total_capacity_Wh;
        b["b_cont"] = (dt_status.bms_status != FAULT);
      }
    };

    // 🚨 Fixed: Use to<JsonObject>() instead of deprecated createNestedObject() in v7
    populate_battery(doc["b1"].to<JsonObject>(), battery, datalayer.battery.info, datalayer.battery.status);
    populate_battery(doc["b2"].to<JsonObject>(), battery2, datalayer.battery2.info, datalayer.battery2.status);
    populate_battery(doc["b3"].to<JsonObject>(), battery3, datalayer.battery3.info, datalayer.battery3.status);
                                          
    // Charger Data
    JsonObject chg = doc["chg"].to<JsonObject>();
    chg["en"] = (charger != nullptr);
    if (charger) {
      chg["v"] = String(charger->HVDC_output_voltage(), 2);
      chg["a"] = String(charger->HVDC_output_current(), 2);
    }

    // Serialize to String and send           
    String output;
    serializeJson(doc, output);
    request->send(200, "application/json", output);
  });

  def_route_with_auth("/debug", server, HTTP_GET, [](AsyncWebServerRequest* request) { request->send(200, "text/plain", "Debug: all OK."); });

  def_route_with_auth("/reboot", server, HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(200, "text/plain", "Rebooting server...");
    setBatteryPause(true, true, true, false);
    delay(1000);
    ESP.restart();
  });

  // Initialize ElegantOTA
  init_ElegantOTA();

  // Start server
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
  ElegantOTA.loop(); // Required by the new ElegantOTA version
  
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

// Brought back from "Incoming" branch to ensure compatibility
template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
String formatPowerValue(String label, T value, String unit, int precision, String color) {
  String result = "<h4 style='color: " + color + ";'>" + label + ": ";
  result += formatPowerValue(value, unit, precision);
  result += "</h4>";
  return result;
}

// Brought back from "Incoming" branch to ensure compatibility
template <typename T>  // This function makes power values appear as W when under 1000, and kW when over
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