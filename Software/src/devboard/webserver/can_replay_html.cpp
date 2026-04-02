#include "can_replay_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

const char can_replay_full_html[] PROGMEM = INDEX_HTML_HEADER R"rawliteral(
  <style>
    .can-wrap { display: flex; flex-direction: column; gap: 20px; }
    .control-card { background: #fff; border-radius: 8px; box-shadow: 0 4px 10px rgba(0,0,0,0.05); padding: 20px; }
    .control-card h3 { margin: 0 0 10px 0; color: #2c3e50; font-size: 1.25rem; font-weight: 800; border-bottom: 1px solid #eee; padding-bottom: 10px; }
    .control-card p { color: #7f8c8d; font-size: 0.9rem; margin-top: 0; margin-bottom: 15px; }
    .flex-row { display: flex; align-items: center; flex-wrap: wrap; gap: 15px; }
    select, input[type='file'] { padding: 8px 12px; border-radius: 6px; border: 1px solid #ccc; background-color: #f8f9fa; color: #333; font-size: 0.95rem; }
    .btn-cmd { color: white; border: none; padding: 10px 20px; border-radius: 6px; font-weight: bold; cursor: pointer; transition: 0.2s; box-shadow: 0 2px 4px rgba(0,0,0,0.1); font-size: 0.95rem; }
    .btn-cmd:hover:not(:disabled) { transform: translateY(-2px); box-shadow: 0 4px 8px rgba(0,0,0,0.15); }
    .btn-cmd:disabled { background: #bdc3c7; cursor: not-allowed; transform: none; box-shadow: none; color: #fff; }
    .btn-blue { background: #3498db; } .btn-blue:hover { background: #2980b9; }
    .btn-green { background: #2ecc71; } .btn-green:hover { background: #27ae60; }
    .btn-red { background: #e74c3c; } .btn-red:hover { background: #c0392b; }
    .btn-purple { background: #9b59b6; } .btn-purple:hover { background: #8e44ad; }
    .status-box { background: #f8f9fa; border: 1px solid #ddd; padding: 10px 15px; border-radius: 6px; font-weight: bold; color: #333; margin-left: auto; }
    .modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100vw; height: 100vh; background-color: rgba(0,0,0,0.5); backdrop-filter: blur(3px); }
    .modal-content { background-color: #fff; margin: 5vh auto; padding: 25px; border-top: 5px solid #3498db; width: 85vw; max-width: 800px; border-radius: 8px; box-shadow: 0 10px 25px rgba(0,0,0,0.2); }
    .close-btn { color: #aaa; float: right; font-size: 28px; font-weight: bold; cursor: pointer; line-height: 1; margin-top: -5px; }
    .close-btn:hover { color: #333; }
    pre.log-preview { background-color: #1e1e1e; padding: 15px; border-radius: 6px; color: #00ff00; max-height: 50vh; overflow-y: auto; white-space: pre-wrap; word-wrap: break-word; font-family: 'Courier New', Courier, monospace; box-shadow: inset 0 0 10px rgba(0,0,0,0.8); }
    @media (max-width: 768px) { .status-box { margin-left: 0; width: auto; flex: 1 1 250px; text-align: center; } }
  </style>

  <div class="can-wrap">
    <div class="control-card" style="border-top-color: #3498db;">
      <h3>🔌 Step 1: Select CAN Interface</h3>
      <p>Select the interface where the replay data should be transmitted.</p>
      <div class="flex-row">
        <select id="canInterface">
          %CAN_OPTIONS%
        </select>
        <button class="btn-cmd btn-blue" onclick="sendCANSelection()">Apply Interface</button>
      </div>
    </div>

    <div class="control-card" style="border-top-color: #9b59b6;">
      <h3>📂 Step 2: Upload CAN Log File</h3>
      <p>Select a .txt CANdump file. (Max 100KB if no SD Card is present)</p>
      <div class="flex-row">
        <input type="file" id="file-input" accept=".txt">
        <button id="upload-btn" class="btn-cmd btn-purple">📤 Upload File</button>
        <button id="view-preview-btn" class="btn-cmd btn-blue" style="display:none;" onclick="openModal()">👁️ View Preview</button>
      </div>
    </div>

    <div class="control-card" style="border-top-color: #2ecc71;">
      <h3>▶️ Step 3: Playback Control</h3>
      <div class="flex-row">
        <label style="cursor: pointer; font-weight: bold; display: flex; align-items: center; gap: 8px;">
          <input type="checkbox" id="loopCheckbox" style="transform: scale(1.3);"> Loop Playback
        </label>
        <button id="start-btn" class="btn-cmd btn-green" onclick="startReplay()">▶️ Start</button>
        <button id="stop-btn" class="btn-cmd btn-red" onclick="stopReplay()">⏹️ Stop</button>

        <div class="status-box">
          Status: <span id="statusIndicator" style="color: #7f8c8d; margin-left: 5px;">Ready</span>
        </div>
      </div>
    </div>

  </div>

  <div id="previewModal" class="modal">
    <div class="modal-content">
      <span class="close-btn" onclick="closeModal()">&times;</span>
      <h3 style="color: #2c3e50; margin-top: 0; border-bottom: 1px solid #eee; padding-bottom: 10px;">📄 Uploaded Log Preview</h3>
      <pre id="file-content" class="log-preview">No data uploaded yet.</pre>
    </div>
  </div>

  <script>
    function openModal() { document.getElementById('previewModal').style.display = 'block'; }
    function closeModal() { document.getElementById('previewModal').style.display = 'none'; }
    window.onclick = function(event) { if (event.target == document.getElementById('previewModal')) closeModal(); };

    const fileInput = document.getElementById('file-input');
    const uploadBtn = document.getElementById('upload-btn');
    const startBtn = document.getElementById('start-btn');
    const stopBtn = document.getElementById('stop-btn');
    const fileContent = document.getElementById('file-content');
    const viewBtn = document.getElementById('view-preview-btn');
    const statInd = document.getElementById('statusIndicator');
    let selectedFile = null;

    fileInput.addEventListener('change', () => { selectedFile = fileInput.files[0]; });

    function resetUploadUI() {
      uploadBtn.disabled = false;
      uploadBtn.innerText = '📤 Upload File';
      fileInput.disabled = false;
      startBtn.disabled = false;
      stopBtn.disabled = false;
      statInd.innerText = 'Ready';
      statInd.style.color = '#7f8c8d';
    }

    uploadBtn.addEventListener('click', () => {
      if (!selectedFile) { alert('Please select a file first!'); return; }

      if (selectedFile.size > 102400) {
        if (!confirm('Warning: File is larger than 100KB!\n\nIf you do NOT have an SD Card inserted, the server will reject this upload to protect system memory.\n\nContinue?')) return;
      }

      uploadBtn.disabled = true;
      fileInput.disabled = true;
      startBtn.disabled = true;
      stopBtn.disabled = true;
      viewBtn.style.display = 'none';
      uploadBtn.innerText = '⏳ Uploading...';
      statInd.innerText = 'Uploading...';
      statInd.style.color = '#f39c12';
      fileContent.textContent = 'Loading preview...';

      const formData = new FormData();
      formData.append('file', selectedFile);
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/import_can_log', true);

      xhr.onload = () => {
        resetUploadUI();
        if (xhr.status === 200) {
          alert('✅ File uploaded successfully!');
          viewBtn.style.display = 'inline-block';
          const reader = new FileReader();
          reader.onload = function (e) {
            let text = e.target.result;
            if(selectedFile.size > 5000) text += '\n\n... [Preview Truncated to prevent Browser lag] ...';
            fileContent.textContent = text;
          };
          reader.readAsText(selectedFile.slice(0, 5000));
        } else if (xhr.status === 400) {
          alert('❌ Upload Rejected: ' + xhr.responseText);
          fileContent.textContent = 'Upload failed: ' + xhr.responseText;
        } else {
          alert('❌ Upload failed! Server returned code ' + xhr.status);
          fileContent.textContent = 'Error during upload. Check server logs.';
        }
      };

      xhr.onerror = () => {
        resetUploadUI();
        alert('❌ Network error during upload.');
        fileContent.textContent = 'Network disconnected or board restarted.';
      };

      xhr.send(formData);
    });

    function startReplay() {
      let loop = document.getElementById('loopCheckbox').checked ? 1 : 0;
      fetch('/startReplay?loop=' + loop, { method: 'GET' })
        .then(response => response.text())
        .then(data => {
          statInd.innerText = '▶️ Running';
          statInd.style.color = '#2ecc71';
          if (loop === 0) {
            setTimeout(() => {
              statInd.innerText = '✅ Completed';
              statInd.style.color = '#7f8c8d';
            }, 5000);
          }
        })
        .catch(error => console.error('Error:', error));
    }

    function stopReplay() {
      fetch('/stopReplay', { method: 'GET' })
        .then(response => response.text())
        .then(data => {
          statInd.innerText = '⏹️ Stopped';
          statInd.style.color = '#e74c3c';
        })
        .catch(error => console.error('Error:', error));
    }

    function sendCANSelection() {
      var selectedInterface = document.getElementById('canInterface').value;
      var xhr = new XMLHttpRequest();
      xhr.open('GET', '/setCANInterface?interface=' + selectedInterface, true);
      xhr.onreadystatechange = function() {
        if (xhr.readyState === 4) {
          if (xhr.status === 200) { alert('✅ Success: Interface Updated'); }
          else { alert('❌ Error: ' + xhr.responseText); }
        }
      };
      xhr.send();
    }
  </script>
)rawliteral" INDEX_HTML_FOOTER;

String can_replay_template_processor(const String& var) {
  if (var == "CAN_OPTIONS") {
    String opts = "";
    opts += "<option value=\"" + String(CAN_NATIVE) + "\" " + ((datalayer.system.info.can_replay_interface == CAN_NATIVE) ? "selected" : "") + ">CAN Native</option>";
    opts += "<option value=\"" + String(CANFD_NATIVE) + "\" " + ((datalayer.system.info.can_replay_interface == CANFD_NATIVE) ? "selected" : "") + ">CANFD Native</option>";
    opts += "<option value=\"" + String(CAN_ADDON_MCP2515) + "\" " + ((datalayer.system.info.can_replay_interface == CAN_ADDON_MCP2515) ? "selected" : "") + ">CAN Addon MCP2515</option>";
    opts += "<option value=\"" + String(CANFD_ADDON_MCP2518) + "\" " + ((datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518) ? "selected" : "") + ">CANFD Addon MCP2518</option>";
    return opts;
  }
  return String();
}
