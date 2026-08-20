#include "can_logging_html.h"
#include <Arduino.h>
#include "../../communication/can/comm_can.h"
#include "../../datalayer/datalayer.h"
#include "../i18n/tr.h"
#include "html_escape.h"
#include "index_html.h"

String can_logger_processor(void) {
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
  content += ".config-item { margin: 15px 0; }";
  content += "</style>";

  // Configuration section
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px; margin-bottom: 20px;'>";
  tr_h3(content, TrKey::UI_CAN_LOGGER_CONFIGURATION);
  content += "<div class='config-item'><span>" + TR(TrKey::UI_CAN_ID_CUTOFF_FILTER) + " " +
             String(user_selected_CAN_ID_cutoff_filter) + "</span> <button onclick='editCANIDCutoff()'>" +
             TR(TrKey::UI_EDIT) + "</button></div>";
  content += "<button onclick='refreshPage()'>" + TR(TrKey::UI_REFRESH_DATA) + "</button> ";
  content += "<button onclick='exportLog()'>" + TR(TrKey::UI_EXPORT_TXT) + "</button> ";
#ifdef SDCARD
  content += "<button onclick='deleteLogFile()'>" + TR(TrKey::UI_DELETE_LOG_FILE) + "</button> ";
#endif
  content += "<button onclick='stopLoggingAndGoToMainPage()'>" + TR(TrKey::UI_STOP_BACK_MAIN_PAGE) + "</button>";
  content += "</div>";

  // Start a new block for the CAN messages
  content += "<div style='background-color: #303E47; padding: 20px; border-radius: 15px'>";

  // Check for messages
  if (datalayer.system.info.logged_can_messages[0] == 0) {
    content += TR(TrKey::UI_CAN_LOGGER_STARTED_REFRESH_PAGE_DISPLAY_INCOMING_RX_OUTGOING_TX_MESSAGES);
  } else {
    // Split the messages using the newline character
    String messages = String(datalayer.system.info.logged_can_messages);
    int startIndex = 0;
    int endIndex = messages.indexOf('\n');
    while (endIndex != -1) {
      // Extract a single message and wrap it in a styled div
      String singleMessage = messages.substring(startIndex, endIndex);
      content += "<div class='can-message'>" + singleMessage + "</div>";
      startIndex = endIndex + 1;  // Move past the newline character
      endIndex = messages.indexOf('\n', startIndex);
    }
  }

  content += "</div>";

  // Add JavaScript for navigation and configuration
  content += "<script>";
  content += "function refreshPage(){ location.reload(true); }";
  content += "function exportLog() { window.location.href = '/export_can_log'; }";
#ifdef SDCARD
  content += "function deleteLogFile() { window.location.href = '/delete_can_log'; }";
#endif
  content += "function stopLoggingAndGoToMainPage() {";
  content += "  fetch('/stop_can_logging').then(() => window.location.href = '/');";
  content += "}";
  content += "function editCANIDCutoff() {";
  content += "  var value = prompt('" +
             TR_JS(TrKey::UI_CAN_IDS_IN_DECIMAL_BELOW_THIS_VALUE_WILL_NOT_BE_LOGGED_0_65535) + "', '" +
             String(user_selected_CAN_ID_cutoff_filter) + "');";
  content += "  if (value !== null) {";
  content += "    if (value >= 0 && value <= 65535) {";
  content += "      var xhr = new XMLHttpRequest();";
  content += "      xhr.onload = function() { location.reload(true); };";
  content += "      xhr.onerror = function() { alert('" + TR_JS(TrKey::UI_ERROR_UPDATING_CAN_ID_CUTOFF) + "'); };";
  content += "      xhr.open('GET', '/set_can_id_cutoff?value=' + value, true);";
  content += "      xhr.send();";
  content += "    } else {";
  content += "      alert('" + TR_JS(TrKey::UI_ALERT_INVALID_VALUE_PLEASE_ENTER_VALUE_BETWEEN_0_65535) + "');";
  content += "    }";
  content += "  }";
  content += "}";
  content += "</script>";
  content += index_html_footer;
  return content;
}
