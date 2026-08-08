#include "i18n_util.h"
#include <cstring>

static bool is_lower(char c) {
  return c >= 'a' && c <= 'z';
}

static bool is_upper(char c) {
  return c >= 'A' && c <= 'Z';
}

// Length of the language-code prefix (2 or 5), or 0 if invalid
static unsigned int language_code_length(const char* name) {
  if (!is_lower(name[0]) || !is_lower(name[1])) {
    return 0;
  }
  if (name[2] == '-') {
    if (!is_upper(name[3]) || !is_upper(name[4])) {
      return 0;
    }
    return 5;
  }
  return 2;
}

// ^[a-z]{2}(-[A-Z]{2})?\.(json\.gz|blp)$ without regex
bool is_valid_i18n_filename(const String& name) {
  const char* s = name.c_str();
  if (name.length() < 6) {
    return false;
  }
  unsigned int code_len = language_code_length(s);
  if (code_len == 0) {
    return false;
  }
  const char* suffix = s + code_len;
  return strcmp(suffix, ".json.gz") == 0 || strcmp(suffix, ".blp") == 0;
}

bool is_valid_language_code(const String& code) {
  if (code.length() != 2 && code.length() != 5) {
    return false;
  }
  return language_code_length(code.c_str()) == code.length();
}

String i18n_language_from_filename(const String& name) {
  if (!is_valid_i18n_filename(name)) {
    return "";
  }
  char code[6] = {0};
  unsigned int code_len = language_code_length(name.c_str());
  memcpy(code, name.c_str(), code_len);
  return String(code);
}

String i18n_list_json(const std::vector<String>& filenames, const String& active_language) {
  std::vector<String> languages;
  for (const String& name : filenames) {
    String lang = i18n_language_from_filename(name);
    if (lang == "") {
      continue;
    }
    bool seen = false;
    for (const String& existing : languages) {
      if (existing == lang) {
        seen = true;
      }
    }
    if (!seen) {
      languages.push_back(lang);
    }
  }
  // Never interpolate the stored value raw: it reaches the page as JSON and
  // a stale/hand-written NVS value must not be able to break out of the
  // string literal. Anything that is not a well-formed code reads as "".
  String active = is_valid_language_code(active_language) ? active_language : String("");
  String json = "{\"active\":\"" + active + "\",\"languages\":[";
  for (unsigned int i = 0; i < languages.size(); i++) {
    if (i > 0) {
      json += ",";
    }
    json += "\"" + languages[i] + "\"";
  }
  json += "]}";
  return json;
}
