#include "can_replay_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "../i18n/tr.h"
#include "html_escape.h"
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
  content += "<button onclick='home()'>" + TR(TrKey::UI_BACK_MAIN_PAGE) + "</button>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";

  // Ask user to select which CAN interface log should be sent to
  tr_h3(content, TrKey::UI_STEP_1_SELECT_CAN_INTERFACE_PLAYBACK);

  // Dropdown with choices
  content += "<label for='canInterface'>" + TR(TrKey::UI_CAN_INTERFACE) + "</label>";
  content += "<select id='canInterface' name='canInterface'>";
  content += "<option value='" + String(CAN_NATIVE) + "' " +
             (datalayer.system.info.can_replay_interface == CAN_NATIVE ? "selected" : "") + ">CAN Native</option>";
  content += "<option value='" + String(CANFD_NATIVE) + "' " +
             (datalayer.system.info.can_replay_interface == CANFD_NATIVE ? "selected" : "") + ">CANFD Native</option>";
  content += "<option value='" + String(CAN_ADDON_MCP2515) + "' " +
             (datalayer.system.info.can_replay_interface == CAN_ADDON_MCP2515 ? "selected" : "") +
             ">CAN Addon MCP2515</option>";
  content += "<option value='" + String(CANFD_ADDON_MCP2518) + "' " +
             (datalayer.system.info.can_replay_interface == CANFD_ADDON_MCP2518 ? "selected" : "") +
             ">CANFD Addon MCP2518</option>";

  content += "</select>";

  // Add a button to submit the selected CAN interface
  // This function writes the selection to datalayer.system.info.can_replay_interface
  content += "<button onclick='sendCANSelection()'>" + TR(TrKey::UI_APPLY) + "</button>";

  tr_h3(content, TrKey::UI_STEP_2_UPLOAD_CAN_LOG_FILE);
  content += "<p>" + TR(TrKey::UI_CLICK_BROWSE_SELECT_TXT_CANDUMP_LOG_FILE_UPLOAD) + "</p>";
  content += "<input type='file' id='file-input' accept='.txt'>";
  content += "<button id='upload-btn'>" + TR(TrKey::UI_UPLOAD) + "</button>";

  tr_h3(content, TrKey::UI_STEP_3_PLAYBACK_CONTROL);

  //Checkbox to see if the user wants the log to repeat once it reaches the end
  content += "<input type=\"checkbox\" id=\"loopCheckbox\"> " + TR(TrKey::UI_LOOP) + " ";

  // Add a button to start playing the log
  content += "<button onclick='startReplay()'>" + TR(TrKey::UI_START) + "</button> ";

  // Add a button to stop playing the log
  content += "<button onclick='stopReplay()'>" + TR(TrKey::UI_STOP) + "</button> ";

  // Status indicator
  content +=
      "<span id='statusIndicator' style='margin-left:10px; font-weight:bold;'>" + TR(TrKey::DRV_STOPPED) + "</span> ";

  tr_h3(content, TrKey::UI_UPLOADED_LOG_PREVIEW);
  content += "<pre id='file-content'></pre>";

  content += "<script>";
  content += "const fileInput = document.getElementById('file-input');";
  content += "const uploadBtn = document.getElementById('upload-btn');";
  content += "const fileContent = document.getElementById('file-content');";
  content += "let selectedFile = null;";

  content += "fileInput.addEventListener('change', () => { selectedFile = fileInput.files[0]; });";

  content += "uploadBtn.addEventListener('click', () => {";
  content += "if (!selectedFile) { alert('" + TR_JS(TrKey::UI_PLEASE_SELECT_A_FILE_FIRST) + "'); return; }";
  content += "const formData = new FormData();";
  content += "formData.append('file', selectedFile);";
  content += "const xhr = new XMLHttpRequest();";
  content += "xhr.open('POST', '/import_can_log', true);";
  content += "xhr.onload = () => { if (xhr.status === 200) { alert('" + TR_JS(TrKey::UI_FILE_UPLOADED_SUCCESSFULLY) +
             "'); const reader = new "
             "FileReader(); reader.onload = function (e) { fileContent.textContent = e.target.result; }; "
             "reader.readAsText(selectedFile); } else { alert('" +
             TR_JS(TrKey::UI_UPLOAD_FAILED_SERVER_ERROR) + "'); }};";
  content += "xhr.send(formData);";
  content += "});";
  content += "</script>";

  content += "</div>";

  // Add JavaScript for updating status
  content += "<script>";
  content += "function startReplay() {";
  content += "  let loop = document.getElementById('loopCheckbox').checked ? 1 : 0;";
  content += "  fetch('/startReplay?loop=' + loop, { method: 'GET' })";
  content += "    .then(response => response.text())";
  content += "    .then(data => {";
  content += "      console.log(data);";
  content += "      document.getElementById('statusIndicator').innerText = '" + TR_JS(TrKey::UI_RUNNING) + "';";
  content += "      document.getElementById('statusIndicator').style.color = 'green';";
  content += "      if (loop === 0) {";  // If loop is not checked
  content += "        setTimeout(() => {";
  content += "          document.getElementById('statusIndicator').innerText = '" + TR_JS(TrKey::UI_COMPLETED) + "';";
  content += "          document.getElementById('statusIndicator').style.color = 'white';";
  content += "        }, 5000);";  // 5-second timeout before reverting the text
  content += "      }";
  content += "    })";
  content += "    .catch(error => console.error('Error:', error));";
  content += "}";
  content += "function stopReplay() {";
  content += "  fetch('/stopReplay', { method: 'GET' })";
  content += "    .then(response => response.text())";
  content += "    .then(data => {";
  content += "      console.log(data);";
  content += "      document.getElementById('statusIndicator').innerText = '" + TR_JS(TrKey::UI_STOPPED) + "';";
  content += "      document.getElementById('statusIndicator').style.color = 'red';";
  content += "    })";
  content += "    .catch(error => console.error('Error:', error));";
  content += "}";
  content += "function sendCANSelection() {";
  content += "  var selectedInterface = document.getElementById('canInterface').value;";
  content += "  var xhr = new XMLHttpRequest();";
  content += "  xhr.open('GET', '/setCANInterface?interface=' + selectedInterface, true);";
  content += "  xhr.onreadystatechange = function() {";
  content += "    if (xhr.readyState === 4) {";
  content += "      if (xhr.status === 200) {";
  content += "        alert('" + TR_JS(TrKey::UI_ALERT_SUCCESS_XHR_RESPONSETEXT) + " ' + xhr.responseText);";
  content += "      } else {";
  content += "        alert('" + TR_JS(TrKey::UI_ALERT_ERROR_XHR_RESPONSETEXT) + " ' + xhr.responseText);";
  content += "      }";
  content += "    }";
  content += "  };";
  content += "  xhr.send();";
  content += "}";
  content += "function home() { window.location.href = '/'; }";
  content += "</script>";
  content += index_html_footer;
  return content;
}
