#include "can_replay_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

#ifdef HW_UNIFIED_S3
#include <vector>
#include "../hal/hal.h"
#include "html_escape.h"

namespace {

// A CAN bus the board configuration declared, with the name its port gave it.
struct CanPort {
  CAN_Interface can;
  String name;
};

// The settings enum and the runtime enum number interfaces differently. The
// dump numbers channels by the runtime one, so that is the one that matters here.
CAN_Interface can_interface_for(comm_interface comm) {
  switch (comm) {
    case comm_interface::CanNative:
      return CAN_NATIVE;
    case comm_interface::CanFdNative:
      return CANFD_NATIVE;
    case comm_interface::CanAddonMcp2515:
      return CAN_ADDON_MCP2515;
    case comm_interface::CanFdAddonMcp2518:
      return CANFD_ADDON_MCP2518;
    case comm_interface::CanFdAddonMcp2518_2:
      return CANFD_ADDON_MCP2518_2;
    default:
      return NO_CAN_INTERFACE;  // RS485 and Modbus carry no CAN traffic
  }
}

std::vector<CanPort> configured_can_ports() {
  std::vector<CanPort> ports;
  for (comm_interface comm : esp32hal->available_interfaces()) {
    CAN_Interface can = can_interface_for(comm);
    if (can != NO_CAN_INTERFACE) {
      ports.push_back({can, html_escape(esp32hal->name_for_comm_interface(comm))});
    }
  }
  return ports;
}

bool is_fd_capable(CAN_Interface can) {
  return can == CANFD_NATIVE || can == CANFD_ADDON_MCP2518 || can == CANFD_ADDON_MCP2518_2;
}

// format_can_frame() in comm_can.cpp writes rx as '0' + interface*2 and tx as
// '1' + interface*2, so the channel numbers follow from the runtime enum.
String channel_pair(CAN_Interface can, bool fd) {
  int n = (int)can * 2;
  return String(fd ? "RX" : "rx") + String(n) + "/" + String(fd ? "TX" : "tx") + String(n + 1);
}

// "The dump will contain data from interface CAN denoted as rx0/tx1, and from
// CAN FD 1 denoted as rx6/tx7." One clause per configured bus, in file order.
String dump_channel_sentence(const std::vector<CanPort>& ports) {
  if (ports.empty()) {
    return "This board has no CAN interfaces configured, so a dump will be empty.";
  }
  String s = "The dump will contain data from interface ";
  const CanPort* fd = nullptr;
  for (size_t i = 0; i < ports.size(); i++) {
    if (i > 0) {
      s += (i == ports.size() - 1) ? ", and from " : ", from ";
    }
    s += "<b>" + ports[i].name + "</b> denoted as " + channel_pair(ports[i].can, false);
    if (fd == nullptr && is_fd_capable(ports[i].can)) {
      fd = &ports[i];
    }
  }
  s += ".";
  // The case follows the frame, not the bus: an FD controller sending classic
  // frames still writes lower case, which is why the sample dump shows rx6.
  if (fd != nullptr) {
    s += " Frames carried as CAN FD are written in capitals, such as " + channel_pair(fd->can, true) + ".";
  }
  return s;
}

}  // namespace
#endif  // HW_UNIFIED_S3

String can_replay_processor(void) {
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
  content += "<button onclick='home()'>Back to main page</button>";

  // CAN dump card
  content +=
      "<div style='background-color: #303E47; padding: 20px; border-radius: 15px; margin-bottom: 20px; text-align: "
      "center'>";
  content += "<h3>CAN dump</h3>";
  content +=
      "<p>CAN traffic will open in a new window. Let it run for the required amount of time and save the file.</p>";
#ifdef HW_UNIFIED_S3
  content += "<p>" + dump_channel_sentence(configured_can_ports()) + "</p>";
#endif  // HW_UNIFIED_S3
  content += "<button onclick='startDump()'>Start dump</button>";
#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    content += "<hr style='border: 0; border-top: 1px solid #505E67; margin: 0 0 20px'>";
    content += "<button onclick='exportCANLog()'>Export SD card CAN log</button> ";
    content += "<button onclick='deleteCANLog()'>Delete SD card CAN log</button>";
  }
#endif  // SDCARD
  content += "</div>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";
  content += "<h3>CAN replay</h3>";

  // Ask user to select which CAN interface log should be sent to
  content += "<h4>Step 1: Select CAN Interface for Playback</h4>";

  // Dropdown with choices
  content += "<label for='canInterface'>CAN Interface:</label>";
  content += "<select id='canInterface' name='canInterface'>";
#ifdef HW_UNIFIED_S3
  // Only the CAN buses the board configuration declared, under the names it
  // gave them. A fixed list would offer buses this board does not have, and a
  // replay sent to one of those goes nowhere.
  std::vector<CanPort> ports = configured_can_ports();
  if (ports.empty()) {
    content += "<option disabled selected>No CAN interfaces configured</option>";
  }
  for (const CanPort& port : ports) {
    content += "<option value='" + String((int)port.can) + "'" +
               (datalayer.system.info.can_replay_interface == port.can ? " selected" : "") + ">" + port.name +
               "</option>";
  }
#else
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
#endif  // HW_UNIFIED_S3

  content += "</select>";

  // Add a button to submit the selected CAN interface
  // This function writes the selection to datalayer.system.info.can_replay_interface
  content += "<button onclick='sendCANSelection()'>Apply</button>";

  content += "<h4>Step 2: Upload CAN Log File</h4>";
  content += "<p>Click Browse to select a .txt CANdump log file to upload</p>";
  content += "<input type='file' id='file-input' accept='.txt'>";
  content += "<button id='upload-btn'>Upload</button>";

  content += "<h4>Step 3: Playback control</h4>";

  //Checkbox to see if the user wants the log to repeat once it reaches the end
  content += "<input type=\"checkbox\" id=\"loopCheckbox\"> Loop ";

  // Add a button to start playing the log
  content += "<button onclick='startReplay()'>Start</button> ";

  // Add a button to stop playing the log
  content += "<button onclick='stopReplay()'>Stop</button> ";

  // Status indicator
  content += "<span id='statusIndicator' style='margin-left:10px; font-weight:bold;'>Stopped</span> ";

  content += "<h4>Uploaded Log Preview:</h4>";
  content += "<pre id='file-content'></pre>";

  content += "<script>";
  content += "const fileInput = document.getElementById('file-input');";
  content += "const uploadBtn = document.getElementById('upload-btn');";
  content += "const fileContent = document.getElementById('file-content');";
  content += "let selectedFile = null;";

  content += "fileInput.addEventListener('change', () => { selectedFile = fileInput.files[0]; });";

  content += "uploadBtn.addEventListener('click', () => {";
  content += "if (!selectedFile) { alert('Please select a file first!'); return; }";
  content += "const formData = new FormData();";
  content += "formData.append('file', selectedFile);";
  content += "const xhr = new XMLHttpRequest();";
  content += "xhr.open('POST', '/import_can_log', true);";
  content +=
      "xhr.onload = () => { if (xhr.status === 200) { alert('File uploaded successfully!'); const reader = new "
      "FileReader(); reader.onload = function (e) { fileContent.textContent = e.target.result; }; "
      "reader.readAsText(selectedFile); } else { alert('Upload failed! Server error.'); }};";
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
  content += "      document.getElementById('statusIndicator').innerText = 'Running...';";
  content += "      document.getElementById('statusIndicator').style.color = 'green';";
  content += "      if (loop === 0) {";  // If loop is not checked
  content += "        setTimeout(() => {";
  content += "          document.getElementById('statusIndicator').innerText = 'Completed';";
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
  content += "      document.getElementById('statusIndicator').innerText = 'Stopped';";
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
  content += "        alert('Success: ' + xhr.responseText);";
  content += "      } else {";
  content += "        alert('Error: ' + xhr.responseText);";
  content += "      }";
  content += "    }";
  content += "  };";
  content += "  xhr.send();";
  content += "}";
  content += "function startDump() { window.open('/dump_can', '_blank'); }";
#ifdef SDCARD
  if (datalayer.system.info.CAN_SD_logging_active) {
    content += "function exportCANLog() { window.location.href = '/export_can_log'; }";
    content += "function deleteCANLog() { window.location.href = '/delete_can_log'; }";
  }
#endif  // SDCARD
  content += "function home() { window.location.href = '/'; }";
  content += "</script>";
  content += index_html_footer;
  return content;
}
