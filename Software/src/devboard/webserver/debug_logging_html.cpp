#include "debug_logging_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

// Native function for character search.
char* strnchr(const char* s, int c, size_t n) {
  if (s == NULL) return NULL;
  for (size_t i = 0; i < n && s[i] != '\0'; ++i) {
    if (s[i] == c) return (char*)&s[i];
  }
  if (c == '\0') {
    for (size_t i = 0; i < n; ++i) {
      if (s[i] == '\0') return (char*)&s[i];
    }
  }
  return NULL;
}

void print_debug_logger_html(AsyncResponseStream* response) {
  response->print(R"rawliteral(
  <style>
    .log-wrap { display: flex; flex-direction: column; gap: 20px; }
    .control-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #f39c12; padding: 20px; }
    .control-card h3 { margin: 0 0 15px 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; border-bottom: 1px solid #eee; padding-bottom: 10px; }
    .btn-group { display: flex; flex-wrap: wrap; gap: 10px; }
    .btn-cmd { color: white; border: none; padding: 10px 20px; border-radius: 6px; font-weight: bold; cursor: pointer; transition: 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.1); font-size: 0.95rem; }
    .btn-cmd:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }
    .btn-blue { background: #3498db; } .btn-blue:hover { background: #2980b9; }
    .btn-green { background: #2ecc71; } .btn-green:hover { background: #27ae60; }
    .btn-dark { background: #34495e; } .btn-dark:hover { background: #2c3e50; }
    .btn-red { background: #e93232; } .btn-red:hover { background: #c00303; }
    .term-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #34495e; padding: 20px; }
    .term-card h3 { margin: 0 0 15px 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; border-bottom: 1px solid #eee; padding-bottom: 10px; }
    .terminal { background: #1e1e1e; color: #00ff00; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; padding: 15px; border-radius: 6px; height: 60vh; overflow-y: auto; white-space: pre-wrap; box-shadow: inset 0 0 10px rgba(0,0,0,0.8); line-height: 1.4; word-break: break-all; }
  </style>

  <div class="log-wrap">
    <div class="control-card">
      <h3>📄 System Debug Log</h3>
      <div class="btn-group">
  )rawliteral");

  if (datalayer.system.info.web_logging_active || datalayer.system.info.SD_logging_active) {
    if (datalayer.system.info.web_logging_active) {
      response->print("<button class='btn-cmd btn-blue' onclick='location.reload(true)'>🔄 Refresh Data</button>\n");
      response->print("<button class='btn-cmd btn-red' onclick='fetch(\"/clear_log\").then(()=>location.reload(true))'>🧹 Clear Display</button>\n");
    }
    response->print("<button class='btn-cmd btn-green' onclick='window.location.href=\"/export_log\"'>📥 Export to .txt</button>\n");
    if (datalayer.system.info.SD_logging_active) {
      response->print("<button class='btn-cmd btn-dark' onclick='window.location.href=\"/delete_log\"'>🗑️ Delete log file</button>\n");
    }
  }

  response->print(R"rawliteral(
      </div>
    </div>
    <div class="term-card">
      <h3>🖥️ Live Output</h3>
      <div class="terminal" id="sys_terminal">)rawliteral");

  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.SD_logging_active) {
    response->print("> ⚠️ System Logging is currently DISABLED.\n> Please go to 'Settings -> Web Settings' to enable Web Logging.");
  } else {
    if (datalayer.system.info.logged_can_messages[0] == '\0') {
      response->print("> 🟢 System Log buffer is clean and ready.\n> Waiting for new system events to occur...\n>\n> 💡 Note: To save system RAM, this board uses a shared memory buffer.\n> Whenever you view CAN Logs, the System Log history is automatically cleared.");
    } else {
      size_t offset = datalayer.system.info.logged_can_messages_offset;
      if (offset > 0 && offset < (sizeof(datalayer.system.info.logged_can_messages) - 1)) {
        char* next_newline = strnchr(&datalayer.system.info.logged_can_messages[offset + 1], '\n',
                                     sizeof(datalayer.system.info.logged_can_messages) - offset - 1);
        if (next_newline != NULL) {
          response->write((uint8_t*)(next_newline + 1), strnlen(next_newline + 1, sizeof(datalayer.system.info.logged_can_messages) - offset - 2));
        } else {
          response->write((uint8_t*)&datalayer.system.info.logged_can_messages[offset + 1],
                          strnlen(&datalayer.system.info.logged_can_messages[offset + 1], sizeof(datalayer.system.info.logged_can_messages) - offset - 1));
        }
      }
      response->write((uint8_t*)datalayer.system.info.logged_can_messages, offset);
    }
  }

  response->print(R"rawliteral(</div>
    </div>
  </div>
  <script>
    var term = document.getElementById("sys_terminal");
    if(term) term.scrollTop = term.scrollHeight;
    fetch('/stop_can_logging').catch(e => {});
  </script>
  )rawliteral");
}