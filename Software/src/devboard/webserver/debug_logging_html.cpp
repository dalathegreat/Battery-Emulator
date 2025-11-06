#include "debug_logging_html.h"
#include <Arduino.h>
#include "../../datalayer/datalayer.h"
#include "index_html.h"

char* strnchr(const char* s, int c, size_t n) {
  // Like strchr, but only searches the first 'n' bytes of the string.

  if (s == NULL) {
    return NULL;
  }

  // Iterate through the string up to 'n' characters or until a null terminator is found.
  for (size_t i = 0; i < n && s[i] != '\0'; ++i) {
    if (s[i] == c) {
      // Character found, return a pointer to it.
      return (char*)&s[i];
    }
  }

  // If the character to be found is the null terminator, and we haven't exceeded
  // 'n' bytes, check if the null terminator is at the current position.
  if (c == '\0') {
    for (size_t i = 0; i < n; ++i) {
      if (s[i] == '\0') {
        return (char*)&s[i];
      }
    }
  }

  // Character not found within the first 'n' bytes.
  return NULL;
}

String debug_logger_processor(void) {
  String content = String();
  // Reserve enough space for the content to avoid reallocations.
  if (!content.reserve(1000 + sizeof(datalayer.system.info.logged_can_messages))) {
    if (content.reserve(15)) {
      content += "Out of memory.";
    }
    return content;
  }
  content += index_html_header;
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
  if (datalayer.system.info.web_logging_active) {
    content += "<button onclick='refreshPage()'>Refresh data</button> ";
  }
  content += "<button onclick='exportLog()'>Export to .txt</button> ";
  if (datalayer.system.info.SD_logging_active) {
    content += "<button onclick='deleteLog()'>Delete log file</button> ";
  }
  content += "<button onclick='goToMainPage()'>Back to main page</button>";

  // Start a new block for the debug log messages
  content += "<PRE style='text-align: left'>";
  size_t offset = datalayer.system.info.logged_can_messages_offset;
  // If we're mid-buffer, print the older part first.
  if (offset > 0 && offset < (sizeof(datalayer.system.info.logged_can_messages) - 1)) {
    // Find the next newline after the current offset. The offset will always be
    // before the penultimate character, so (offset + 1) will be the final '\0'
    // or earlier.

    char* next_newline = strnchr(&datalayer.system.info.logged_can_messages[offset + 1], '\n',
                                 sizeof(datalayer.system.info.logged_can_messages) - offset - 1);

    if (next_newline != NULL) {
      // We found a newline, so append from the character after that. We check
      // the string length to ensure we don't add any intermediate '\0'
      // characters.
      content.concat(next_newline + 1,
                     strnlen(next_newline + 1, sizeof(datalayer.system.info.logged_can_messages) - offset - 2));
    } else {
      // No newline found, so append from the next character after the offset to
      // the end of the buffer. We check the string length to ensure we don't
      // add any intermediate '\0' characters.
      content.concat(&datalayer.system.info.logged_can_messages[offset + 1],
                     strnlen(&datalayer.system.info.logged_can_messages[offset + 1],
                             sizeof(datalayer.system.info.logged_can_messages) - offset - 1));
    }
  }
  // Append the first part of the buffer up to the current write offset (which
  // points to the first \0).
  content.concat(datalayer.system.info.logged_can_messages, offset);
  content += "</PRE>";

  // Add JavaScript for navigation
  content += "<script>";
  content += "function refreshPage(){ location.reload(true); }";
  content += "function exportLog() { window.location.href = '/export_log'; }";
  if (datalayer.system.info.SD_logging_active) {
    content += "function deleteLog() { window.location.href = '/delete_log'; }";
  }
  content += "function goToMainPage() { window.location.href = '/'; }";
  content += "</script>";
  content += index_html_footer;
  return content;
}
