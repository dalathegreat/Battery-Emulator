#include "html_escape.h"

String html_escape(const String& var) {
  String escaped;
  // Reserve the same length plus a bit for potential escaping.
  escaped.reserve(var.length() + 6);
  for (unsigned int i = 0; i < var.length(); i++) {
    char c = var.charAt(i);
    switch (c) {
      case '&':
        escaped += "&amp;";
        break;
      case '<':
        escaped += "&lt;";
        break;
      case '>':
        escaped += "&gt;";
        break;
      case '\"':
        escaped += "&quot;";
        break;
      case '\'':
        escaped += "&#39;";
        break;
      default:
        // All other characters (including UTF-8 bytes) are added as-is.
        escaped += c;
        break;
    }
  }
  return escaped;
}
