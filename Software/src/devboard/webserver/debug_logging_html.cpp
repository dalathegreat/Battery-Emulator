#include "debug_logging_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

#if defined(DEBUG_VIA_WEB) || defined(LOG_TO_SD)
String debug_logger_processor(void) {
  String content = String(index_html_header);
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
#ifdef DEBUG_VIA_WEB
  content += "<button onclick='refreshPage()'>Refresh data</button> ";
#endif
  content += "<button onclick='exportLog()'>Export to .txt</button> ";
#ifdef LOG_TO_SD
  content += "<button onclick='deleteLog()'>Delete log file</button> ";
#endif
  content += "<button onclick='goToMainPage()'>Back to main page</button>";

  // Start a new block for the debug log messages
  content += "<PRE style='text-align: left'>";
  content += String(datalayer.system.info.logged_can_messages);
  content += "</PRE>";

  // Add JavaScript for navigation
  content += "<script>";
  content += "function refreshPage(){ location.reload(true); }";
  content += "function exportLog() { window.location.href = '/export_log'; }";
#ifdef LOG_TO_SD
  content += "function deleteLog() { window.location.href = '/delete_log'; }";
#endif
  content += "function goToMainPage() { window.location.href = '/'; }";
  content += "</script>";
  content += index_html_footer;
  return content;
}
#endif
