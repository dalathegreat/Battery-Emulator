#include "can_replay_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

String can_replay_processor(void) {
  if (!datalayer.system.info.can_logging_active) {
    datalayer.system.info.logged_can_messages_offset = 0;
    datalayer.system.info.logged_can_messages[0] = '\0';
  }
  datalayer.system.info.can_logging_active =
      true;  // Signal to main loop that we should log messages. Disabled by default for performance reasons
  
  String content = index_html_header;
  
  // ==========================================
  // CSS STYLES
  // ==========================================
  content += "<style>";
  content += "body { background-color: #121212; color: #ffffff; font-family: 'Segoe UI', Tahoma, sans-serif; }";
  
  // Settings Card Style
  content += ".settings-card { background-color: #303E47; padding: 20px; margin-bottom: 20px; border-radius: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.3); text-align: left; }";
  content += ".settings-card h3 { color: #4CAF50; margin-top: 0; border-bottom: 1px solid #4d5f69; padding-bottom: 10px; }";
  
  // Button Styles
  content += "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; cursor: pointer; border-radius: 8px; font-weight: bold; transition: 0.2s; margin-right: 10px; }";
  content += "button:hover:not(:disabled) { background-color: #3A4A52; filter: brightness(1.2); }";
  content += "button:disabled { background-color: #333333; color: #777777; cursor: not-allowed; }";
  content += ".btn-success { background-color: #198754; }";
  content += ".btn-danger { background-color: #A70107; }";
  content += ".btn-info { background-color: #0dcaf0; color: #000; }";
  
  // Form Controls
  content += "select, input[type='file'] { padding: 8px; border-radius: 5px; border: 1px solid #555; background-color: #263238; color: white; font-size: 14px; margin-right: 10px; }";
  content += ".flex-row { display: flex; align-items: center; flex-wrap: wrap; gap: 10px; margin-top: 15px; }";
  
  // Modal (Popup) Styles
  content += ".modal { display: none; position: fixed; z-index: 1000; left: 0; top: 0; width: 100%; height: 100%; background-color: rgba(0,0,0,0.8); backdrop-filter: blur(5px); }";
  content += ".modal-content { background-color: #263238; margin: 5% auto; padding: 20px; border: 1px solid #4CAF50; width: 85%; max-width: 800px; border-radius: 12px; box-shadow: 0 5px 15px rgba(0,0,0,0.5); }";
  content += ".close-btn { color: #aaa; float: right; font-size: 28px; font-weight: bold; cursor: pointer; line-height: 1; }";
  content += ".close-btn:hover { color: #fff; }";
  content += "pre.log-preview { background-color: #1e2b32; padding: 15px; border-radius: 8px; color: #00FF00; max-height: 50vh; overflow-y: auto; white-space: pre-wrap; word-wrap: break-word; font-family: monospace; border: 1px solid #4d5f69; }";
  content += "</style>";

  // ==========================================
  // HTML BODY
  // ==========================================
  content += "<div style='max-width: 900px; margin: 0 auto; text-align: center;'>";
  content += "<button style='margin-bottom: 20px;' onclick='home()'>⬅️ Back to Dashboard</button>";

  // --- Step 1 Card ---
  content += "<div class='settings-card'>";
  content += "<h3>Step 1: Select CAN Interface</h3>";
  content += "<p style='color: #ccc; font-size: 0.9em; margin-bottom: 0;'>Select the interface where the replay data should be transmitted.</p>";
  content += "<div class='flex-row'>";
  content += "<label for='canInterface' style='font-weight:bold;'>Interface:</label>";
  content += "<select id='canInterface' name='canInterface'>";
  content += "<option value='" + String(CAN_NATIVE) + "' " + (datalayer.system.info.can_replay_interface == CAN_NATIVE ? "selected" : "") + ">CAN Native</option>";
  content += "<option value='" + String(CANFD_NATIVE) + "' " + (datalayer.system.info.can_replay_interface == CANFD_NATIVE ? "selected" : "") + ">CANFD Native</option>";
  content += "<option value='" + String(CAN_ADDON_MCP2515) + "' " + (datalayer.system.info.can_replay_interface == CAN_ADDON_MCP2515 ? "selected" : "") + ">CAN Addon MCP2515</option>";
  content += "<option value='" + String(CANFD_ADDON_MCP2518) + "' " + (datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518 ? "selected" : "") + ">CANFD Addon MCP2518</option>";
  content += "</select>";
  content += "<button class='btn-info' onclick='sendCANSelection()'>Apply Interface</button>";
  content += "</div>";
  content += "</div>";

  // --- Step 2 Card ---
  content += "<div class='settings-card'>";
  content += "<h3>Step 2: Upload CAN Log File</h3>";
  content += "<p style='color: #ccc; font-size: 0.9em; margin-bottom: 0;'>Select a .txt CANdump file. (Max 100KB if no SD Card is present)</p>";
  content += "<div class='flex-row'>";
  content += "<input type='file' id='file-input' accept='.txt'>";
  content += "<button id='upload-btn'>📤 Upload File</button>";
  content += "<button id='view-preview-btn' class='btn-info' style='display:none;' onclick='openModal()'>👁️ View Preview</button>";
  content += "</div>";
  content += "</div>";

  // --- Step 3 Card ---
  content += "<div class='settings-card'>";
  content += "<h3>Step 3: Playback Control</h3>";
  content += "<div class='flex-row' style='background-color: rgba(0,0,0,0.2); padding: 15px; border-radius: 8px;'>";
  content += "<label style='cursor: pointer; font-weight: bold;'><input type='checkbox' id='loopCheckbox' style='transform: scale(1.2); margin-right: 8px;'> Loop Playback</label>";
  content += "<span style='flex-grow: 1;'></span>"; // Spacer
  content += "<button id='start-btn' class='btn-success' onclick='startReplay()'>▶️ Start</button>";
  content += "<button id='stop-btn' class='btn-danger' onclick='stopReplay()'>⏹️ Stop</button>";
  content += "<div style='background-color: #1e2b32; padding: 10px 20px; border-radius: 8px; border: 1px solid #4d5f69;'>";
  content += "Status: <span id='statusIndicator' style='margin-left:5px; font-weight:bold; color:white;'>Ready</span>";
  content += "</div>";
  content += "</div>";
  content += "</div>";
  
  content += "</div>"; // End Main Container

  // ==========================================
  // MODAL (POPUP) HTML
  // ==========================================
  content += "<div id='previewModal' class='modal'>";
  content += "  <div class='modal-content'>";
  content += "    <span class='close-btn' onclick='closeModal()'>&times;</span>";
  content += "    <h3 style='color: #4CAF50; margin-top: 0;'>📄 Uploaded Log Preview</h3>";
  content += "    <pre id='file-content' class='log-preview'>No data uploaded yet.</pre>";
  content += "  </div>";
  content += "</div>";

  // ==========================================
  // JAVASCRIPT
  // ==========================================
  content += "<script>";
  // Modal Functions
  content += "function openModal() { document.getElementById('previewModal').style.display = 'block'; }";
  content += "function closeModal() { document.getElementById('previewModal').style.display = 'none'; }";
  content += "window.onclick = function(event) { if (event.target == document.getElementById('previewModal')) { closeModal(); } };";

  // Upload Logic
  content += "const fileInput = document.getElementById('file-input');";
  content += "const uploadBtn = document.getElementById('upload-btn');";
  content += "const startBtn = document.getElementById('start-btn');";
  content += "const stopBtn = document.getElementById('stop-btn');";
  content += "const fileContent = document.getElementById('file-content');";
  content += "const viewBtn = document.getElementById('view-preview-btn');";
  content += "let selectedFile = null;";

  content += "fileInput.addEventListener('change', () => { selectedFile = fileInput.files[0]; });";

  content += "function resetUploadUI() {";
  content += "  uploadBtn.disabled = false;";
  content += "  uploadBtn.innerText = '📤 Upload File';";
  content += "  fileInput.disabled = false;";
  content += "  startBtn.disabled = false;";
  content += "  stopBtn.disabled = false;";
  content += "  document.getElementById('statusIndicator').innerText = 'Ready';";
  content += "  document.getElementById('statusIndicator').style.color = 'white';";
  content += "}";

  content += "uploadBtn.addEventListener('click', () => {";
  content += "  if (!selectedFile) { alert('Please select a file first!'); return; }";
  
  content += "  if (selectedFile.size > 102400) {";
  content += "    if (!confirm('Warning: File is larger than 100KB!\\n\\nIf you do NOT have an SD Card inserted, the server will reject this upload to protect the system memory.\\n\\nDo you want to continue?')) { return; }";
  content += "  }";

  content += "  uploadBtn.disabled = true;";
  content += "  fileInput.disabled = true;";
  content += "  startBtn.disabled = true;";
  content += "  stopBtn.disabled = true;";
  content += "  viewBtn.style.display = 'none';";
  content += "  uploadBtn.innerText = '⏳ Uploading...';";
  content += "  document.getElementById('statusIndicator').innerText = 'Uploading...';";
  content += "  document.getElementById('statusIndicator').style.color = 'orange';";
  content += "  fileContent.textContent = 'Loading preview...';";

  content += "  const formData = new FormData();";
  content += "  formData.append('file', selectedFile);";
  content += "  const xhr = new XMLHttpRequest();";
  content += "  xhr.open('POST', '/import_can_log', true);";
  
  content += "  xhr.onload = () => {";
  content += "    resetUploadUI();";
  content += "    if (xhr.status === 200) {";
  content += "      alert('✅ File uploaded successfully!');";
  content += "      viewBtn.style.display = 'inline-block';";
  content += "      const reader = new FileReader();";
  content += "      reader.onload = function (e) { ";
  content += "        let text = e.target.result;";
  content += "        if(selectedFile.size > 5000) text += '\\n\\n... [Preview Truncated to prevent Browser lag] ...';";
  content += "        fileContent.textContent = text;";
  content += "      };";
  content += "      reader.readAsText(selectedFile.slice(0, 5000));";
  content += "    } else if (xhr.status === 400) {";
  content += "      alert('❌ Upload Rejected: ' + xhr.responseText);";
  content += "      fileContent.textContent = 'Upload failed: ' + xhr.responseText;";
  content += "    } else {";
  content += "      alert('❌ Upload failed! Server returned code ' + xhr.status);";
  content += "      fileContent.textContent = 'Error during upload. Check server logs.';";
  content += "    }";
  content += "  };";
  
  content += "  xhr.onerror = () => {";
  content += "    resetUploadUI();";
  content += "    alert('❌ Network error during upload.');";
  content += "    fileContent.textContent = 'Network disconnected or board restarted.';";
  content += "  };";
  
  content += "  xhr.send(formData);";
  content += "});";

  // Playback Control Logic
  content += "function startReplay() {";
  content += "  let loop = document.getElementById('loopCheckbox').checked ? 1 : 0;";
  content += "  fetch('/startReplay?loop=' + loop, { method: 'GET' })";
  content += "    .then(response => response.text())";
  content += "    .then(data => {";
  content += "      document.getElementById('statusIndicator').innerText = '▶️ Running';";
  content += "      document.getElementById('statusIndicator').style.color = '#00FF00';";
  content += "      if (loop === 0) {";
  content += "        setTimeout(() => {";
  content += "          document.getElementById('statusIndicator').innerText = '✅ Completed';";
  content += "          document.getElementById('statusIndicator').style.color = 'white';";
  content += "        }, 5000);";
  content += "      }";
  content += "    })";
  content += "    .catch(error => console.error('Error:', error));";
  content += "}";

  content += "function stopReplay() {";
  content += "  fetch('/stopReplay', { method: 'GET' })";
  content += "    .then(response => response.text())";
  content += "    .then(data => {";
  content += "      document.getElementById('statusIndicator').innerText = '⏹️ Stopped';";
  content += "      document.getElementById('statusIndicator').style.color = '#FF5555';";
  content += "    })";
  content += "    .catch(error => console.error('Error:', error));";
  content += "}";

  content += "function sendCANSelection() {";
  content += "  var selectedInterface = document.getElementById('canInterface').value;";
  content += "  var xhr = new XMLHttpRequest();";
  content += "  xhr.open('GET', '/setCANInterface?interface=' + selectedInterface, true);";
  content += "  xhr.onreadystatechange = function() {";
  content += "    if (xhr.readyState === 4) {";
  content += "      if (xhr.status === 200) { alert('✅ Success: Interface Updated'); }";
  content += "      else { alert('❌ Error: ' + xhr.responseText); }";
  content += "    }";
  content += "  };";
  content += "  xhr.send();";
  content += "}";

  content += "function home() { window.location.href = '/'; }";
  content += "</script>";
  
  content += index_html_footer;
  return content;
}