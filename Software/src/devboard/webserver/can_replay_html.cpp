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
  // Page format
  content += "<style>";
  content += "body { background-color: black; color: white; font-family: Arial, sans-serif; }";
  content +=
      "button { background-color: #505E67; color: white; border: none; padding: 10px 20px; margin-bottom: 20px; "
      "cursor: pointer; border-radius: 10px; }";
  content += "button:hover { background-color: #3A4A52; }";
  content +=
      ".can-message { background-color: #404E57; margin-bottom: 5px; padding: 10px; border-radius: 5px; font-family: "
      "monospace; }";
  content += "</style>";
  content += "<button onclick='stopPlaybackAndGoToMainPage()'>Stop &amp; Back to main page</button>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";

  // Ask user to select which CAN interface log should be sent to
  content += "<h3>Step 1: Select CAN Interface for Playback</h3>";

  // Dropdown with choices
  content += "<label for='canInterface'>CAN Interface:</label>";
  content += "<select id='canInterface' name='canInterface'>";
  content += "<option value='" + String(CAN_NATIVE) + "' " + 
           (datalayer.system.info.can_replay_interface == CAN_NATIVE ? "selected" : "") + 
           ">CAN Native</option>";
  content += "<option value='" + String(CANFD_NATIVE) + "' " + 
           (datalayer.system.info.can_replay_interface == CANFD_NATIVE ? "selected" : "") + 
           ">CANFD Native</option>";
  content += "<option value='" + String(CAN_ADDON_MCP2515) + "' " + 
           (datalayer.system.info.can_replay_interface == CAN_ADDON_MCP2515 ? "selected" : "") + 
           ">CAN Addon MCP2515</option>";
  content += "<option value='" + String(CANFD_ADDON_MCP2518) + "' " + 
           (datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518 ? "selected" : "") + 
           ">CANFD Addon MCP2518</option>";

  content += "</select>";

  // Add a button to submit the selected CAN interface
  // This function writes the selection to datalayer.system.info.can_replay_interface
  content += "<button onclick='sendCANSelection()'>Apply</button>";

  content += "<h3>Step 2: Upload CAN Log File</h3>";

content += "<div id='drop-area' onclick=\"document.getElementById('file-input').click()\">";
content += "<p>Drag & drop a .txt file here, or click Browse to select one.</p>";
content += "<input type='file' id='file-input' accept='.txt'>";
content += "</div>";

content += "<div id='progress'><div id='progress-bar'></div></div>";

content += "<button id='upload-btn'>Upload</button>";

  content += "<h3>Step 3: Playback control</h3>";

  // Add a button to start playing the log
  content += "<button onclick='startReplay()'>Start</button> ";

  // Add a button to stop playing the log
  content += "<button onclick='startReplay()'>Stop</button> ";

content += "<h3>Uploaded Log Preview:</h3>";
content += "<pre id='file-content'></pre>";

content += "<script>";
content += "const fileInput = document.getElementById('file-input');";
content += "const uploadBtn = document.getElementById('upload-btn');";
content += "const fileContent = document.getElementById('file-content');";
content += "const dropArea = document.getElementById('drop-area');";
content += "const progressBar = document.getElementById('progress-bar');";
content += "const progressContainer = document.getElementById('progress');";
content += "let selectedFile = null;";

content += "dropArea.addEventListener('dragover', (e) => { e.preventDefault(); dropArea.style.background = '#f0f0f0'; });";
content += "dropArea.addEventListener('dragleave', () => { dropArea.style.background = 'white'; });";
content += "dropArea.addEventListener('drop', (e) => { e.preventDefault(); dropArea.style.background = 'white'; if (e.dataTransfer.files.length > 0) { fileInput.files = e.dataTransfer.files; selectedFile = fileInput.files[0]; }});";

content += "fileInput.addEventListener('change', () => { selectedFile = fileInput.files[0]; });";

content += "uploadBtn.addEventListener('click', () => {";
content += "if (!selectedFile) { alert('Please select a file first!'); return; }";
content += "const formData = new FormData();";
content += "formData.append('file', selectedFile);";
content += "const xhr = new XMLHttpRequest();";
content += "xhr.open('POST', '/import_can_log', true);";
content += "xhr.upload.onprogress = (event) => { if (event.lengthComputable) { const percent = (event.loaded / event.total) * 100; progressContainer.style.display = 'block'; progressBar.style.width = percent + '%'; }};";
content += "xhr.onload = () => { if (xhr.status === 200) { alert('File uploaded successfully!'); progressBar.style.width = '100%'; const reader = new FileReader(); reader.onload = function (e) { fileContent.textContent = e.target.result; }; reader.readAsText(selectedFile); } else { alert('Upload failed! Server error.'); }};";
content += "xhr.send(formData);";
content += "});";
content += "</script>";

  content += "</div>";

  // Add JavaScript for navigation
  content += "<script>";
    content += "function startReplay() {";
  content += "  var xhr = new XMLHttpRequest();";
  content += "  xhr.open('GET', '/startReplay', true);";
  content += "  xhr.send();";
  content += "}";
  content += "function sendCANSelection() {";
  content += "  var selectedInterface = document.getElementById('canInterface').value;";
  content += "  var xhr = new XMLHttpRequest();";
  content += "  xhr.open('GET', '/setCANInterface?interface=' + selectedInterface, true);";
  content += "  xhr.send();";
  content += "}";
  content += "function stopPlaybackAndGoToMainPage() {";
  content += "  fetch('/stop_can_logging').then(() => window.location.href = '/');";
  content += "}";
  content += "</script>";
  content += index_html_footer;
  return content;
}
