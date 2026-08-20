#include "escape.h"
#include <string>

String html_escape(const String& var) {
  // Built through std::string and converted once: the host test build's
  // String has neither charAt() nor a char operator+=.
  std::string escaped;
  escaped.reserve(var.length() + 6);
  for (const char* p = var.c_str(); *p != '\0'; ++p) {
    switch (*p) {
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
        escaped += *p;
        break;
    }
  }
  return String(escaped.c_str());
}

String js_string_escape(const String& var) {
  std::string out;
  for (const char* p = var.c_str(); *p != '\0'; ++p) {
    // Backtick and $ matter for template literals: `${...}` does not just
    // end the literal, it evaluates. Escaping them here rather than at the
    // call sites means the next template literal someone writes is safe
    // without having to know this.
    if (*p == '\\' || *p == '\'' || *p == '"' || *p == '`' || *p == '$') {
      out += '\\';
      out += *p;
    } else if (*p == '<' && p[1] == '/') {
      out += "<\\/";
      ++p;
    } else if (*p == '\r' || *p == '\n') {
      out += ' ';
    } else {
      out += *p;
    }
  }
  return String(out.c_str());
}
