#include "debug_logging_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"

#ifdef DEBUG_VIA_WEB
String debug_logger_processor(const String& var) {
  String content = "";
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
  content += "<button onclick='refreshPage()'>Refresh data</button> ";
  content += "<button onclick='exportLog()'>Export to .txt</button> ";
  content += "<button onclick='goToMainPage()'>Back to main page</button>";

  // Start a new block for the debug log messages
  content += "<PRE style='text-align: left'>";
  content += String(datalayer.system.info.logged_can_messages);
  content += "</PRE>";

  // Add JavaScript for navigation
  content += "<script>";
  content += "function refreshPage(){ location.reload(true); }";
  content += "function exportLog() { window.location.href = '/export_log'; }";
  content += "function goToMainPage() { window.location.href = '/'; }";
  content += "</script>";
  return content;

}
#endif  // DEBUG_VIA_WEB
