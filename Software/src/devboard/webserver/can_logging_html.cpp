#include "can_logging_html.h"
#include <Arduino.h>
#include "../../communication/can/comm_can.h"
#include "../../datalayer/datalayer.h"
#include "index_html.h"

const char can_logger_full_html[] PROGMEM = INDEX_HTML_HEADER R"rawliteral(
  <style>
    .can-wrap { display: flex; flex-direction: column; gap: 20px; }
    .control-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #9b59b6; padding: 20px; }
    .control-card h3 { margin: 0 0 15px 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; border-bottom: 1px solid #eee; padding-bottom: 10px; }
    .filter-box { background: #f8f9fa; padding: 12px 15px; border-radius: 6px; border: 1px solid #ddd; display: inline-flex; align-items: center; gap: 15px; margin-bottom: 15px; flex-wrap: wrap; }
    .filter-text { font-size: 1rem; color: #333; font-weight: 600; }
    .filter-val { color: #e74c3c; font-size: 1.2rem; font-weight: bold; }
    .btn-group { display: flex; flex-wrap: wrap; gap: 10px; }
    .btn-cmd { color: white; border: none; padding: 10px 20px; border-radius: 6px; font-weight: bold; cursor: pointer; transition: 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.1); font-size: 0.95rem; text-decoration: none; text-align: center; }
    .btn-cmd:hover { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }
    .btn-blue { background: #3498db; } .btn-blue:hover { background: #2980b9; }
    .btn-green { background: #2ecc71; } .btn-green:hover { background: #27ae60; }
    .btn-red { background: #e74c3c; } .btn-red:hover { background: #c0392b; }
    .btn-gray { background: #7f8c8d; } .btn-gray:hover { background: #6c757d; }
    .term-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); border-top: 4px solid #34495e; padding: 20px; }
    .terminal { background: #1e1e1e; color: #00ff00; font-family: 'Courier New', Courier, monospace; font-size: 0.9rem; padding: 15px; border-radius: 6px; height: 500px; overflow-y: auto; white-space: pre-wrap; box-shadow: inset 0 0 10px rgba(0,0,0,0.8); line-height: 1.5; word-break: break-all; }
    #exitOverlay { display: none; position: fixed; top: 0; left: 0; width: 100vw; height: 100vh; background: rgba(0,0,0,0.85); z-index: 99999; justify-content: center; align-items: center; flex-direction: column; backdrop-filter: blur(5px); }
    .exit-spinner { border: 5px solid #f3f3f3; border-top: 5px solid #e74c3c; border-radius: 15px; width: 50px; height: 50px; animation: spin 1s linear infinite; margin-bottom: 20px; }
  </style>

  <div id="exitOverlay">
    <div class="exit-spinner"></div>
    <h2 style="color:white; margin:0; font-family:sans-serif;">⏹️ Exiting Live Stream...</h2>
  </div>

  <div class="can-wrap">
    <div class="control-card">
      <h3>⚙️ Live CAN Stream</h3>
      <div class="filter-box">
        <span class="filter-text">CAN ID Cutoff Filter:</span>
        <span class="filter-val">%CUTOFF%</span>
        <button class="btn-cmd btn-gray" style="padding: 6px 12px; font-size: 0.85rem;" onclick="editCANIDCutoff()">✏️ Edit</button>
      </div>

      <div class="btn-group">
        <button class="btn-cmd btn-blue" id="btnStream" onclick="toggleStream()">▶️ Start Stream</button>
        <button class="btn-cmd" id="btnExport" disabled style="background-color: #bdc3c7; color: #fff; cursor: not-allowed;" onclick="exportLog()">📥 Export to .txt (No Data)</button>
        <button class="btn-cmd btn-red" onclick="clearTerminal()">🗑️ Clear Screen</button>
      </div>
    </div>

    <div class="term-card">
      <h3>🖥️ Real-time Traffic <span id="statusIcon" style="color:gray; font-size:1rem;">(Offline)</span></h3>
      <div class="terminal" id="log_terminal">> Waiting for stream to start...</div>
    </div>
  </div>

  <script>
    window.skipAutoStop = false;
    let isStreaming = false;
    let clientLogBuffer = [];

    const term = document.getElementById("log_terminal");
    const btnStream = document.getElementById("btnStream");
    const btnExport = document.getElementById("btnExport");
    const statusIcon = document.getElementById("statusIcon");

    function toggleStream() {
      if(isStreaming) stopStream();
      else startStream();
    }

    async function pollLogData() {
        if (!isStreaming) return;

        try {
            let res = await fetch('/api/can_poll');
            if (res.ok) {
                let data = await res.text();
                if(data.length > 0) {
                    term.appendChild(document.createTextNode(data));
                    clientLogBuffer.push(data);

                    if(term.childNodes.length > 1000) {
                        term.removeChild(term.firstChild);
                    }
                    term.scrollTop = term.scrollHeight;

                    if(btnExport.disabled) {
                        btnExport.disabled = false;
                        btnExport.style.backgroundColor = "#2ecc71";
                        btnExport.style.cursor = "pointer";
                        btnExport.innerHTML = "📥 Export to .txt";
                    }
                }
            }
        } catch(e) {
            console.log('WiFi Hiccough... giving ESP32 some breathing room.');
            if (isStreaming) {
                setTimeout(pollLogData, 2000);
            }
            return;
        }

        if (isStreaming) {
            setTimeout(pollLogData, 500);
        }
    }

    function startStream() {
      if(isStreaming) return;

      fetch("/start_can_logging").then(() => {
        isStreaming = true;
        btnStream.innerHTML = "⏹️ Stop Stream";
        btnStream.classList.replace("btn-blue", "btn-red");
        statusIcon.innerHTML = "🔴 (Live)";
        statusIcon.style.color = "red";
        term.innerHTML = "> Stream connected. Receiving data...\n";

        pollLogData();
      });
    }

    function stopStream() {
      isStreaming = false;
      btnStream.innerHTML = "▶️ Start Stream";
      btnStream.classList.replace("btn-red", "btn-blue");
      statusIcon.innerHTML = "(Offline)";
      statusIcon.style.color = "gray";
      term.appendChild(document.createTextNode("\n> Stream stopped.\n"));
      fetch("/stop_can_logging").catch(e=>{});
    }

    function clearTerminal() {
      clientLogBuffer = [];
      term.innerHTML = "> Screen cleared.\n";
      btnExport.disabled = true;
      btnExport.style.backgroundColor = "#bdc3c7";
      btnExport.style.cursor = "not-allowed";
      btnExport.innerHTML = "📥 Export to .txt (No Data)";
    }

    function exportLog() {
      if(clientLogBuffer.length === 0) return;
      window.skipAutoStop = true;

      const blob = new Blob([clientLogBuffer.join('')], { type: 'text/plain' });
      const url = window.URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = "CAN_Live_Log.txt";
      document.body.appendChild(a);
      a.click();
      document.body.removeChild(a);
      window.URL.revokeObjectURL(url);

      setTimeout(() => { window.skipAutoStop = false; }, 1000);
    }

    function editCANIDCutoff() {
      window.skipAutoStop = true;
      var value = prompt('CAN IDs in decimal, below this value will not be logged (0-65535):', '%CUTOFF%');
      if (value !== null) {
        if (value >= 0 && value <= 65535) {
          fetch('/set_can_id_cutoff?value=' + value).then(() => location.reload());
        } else {
          alert('Invalid value. Please enter a value between 0 and 65535');
          window.skipAutoStop = false;
        }
      } else { window.skipAutoStop = false; }
    }

    window.addEventListener('beforeunload', function() {
      if (isStreaming) {
        fetch('/stop_can_logging', { keepalive: true }).catch(e => {});
      }
    });

    document.addEventListener('DOMContentLoaded', function() {
      document.querySelectorAll('a').forEach(function(link) {
        link.addEventListener('click', function(e) {
          var href = link.getAttribute('href');
          var onclick = link.getAttribute('onclick');

          if (href && href !== '#' && !onclick && !href.startsWith('javascript:')) {
            e.preventDefault();
            document.getElementById('exitOverlay').style.display = 'flex';
            stopStream();
            setTimeout(() => { window.location.href = href; }, 500);
          }
        });
      });
    });

  </script>
)rawliteral" INDEX_HTML_FOOTER;

String can_logger_processor(const String& var) {
  if (var == "CUTOFF") {
    return String(user_selected_CAN_ID_cutoff_filter);
  }
  return String();
}