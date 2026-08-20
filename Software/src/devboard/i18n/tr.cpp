#include "tr.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>
#include "../utils/escape.h"
#include "i18n.h"
#include "i18n_store.h"

struct PackIndexEntry {
  uint16_t id;
  uint32_t offset;
};

struct Pack {
  bool loaded = false;
  char filename[24] = {0};
  uint16_t count = 0;
  uint16_t max_id = 0;
  uint32_t blob_start = 0;
  uint32_t file_length = 0;
  std::vector<PackIndexEntry> index;
};

static Pack active_pack;
static Pack english_pack;

static bool parse_pack(Pack& pack, const char* lang) {
  String filename = String(lang) + ".blp";
  int32_t total = i18n_store().length(filename.c_str());
  if (total < 12) {
    return false;
  }
  uint8_t header[12];
  if (!i18n_store().read(filename.c_str(), 0, header, sizeof(header))) {
    return false;
  }
  uint16_t version = 0;
  if (memcmp(header, "BLP1", 4) != 0) {
    return false;
  }
  memcpy(&version, header + 4, 2);
  memcpy(&pack.count, header + 6, 2);
  memcpy(&pack.max_id, header + 8, 2);
  if (version != 1 || pack.count == 0) {
    return false;
  }
  /* Header fields come from an uploaded file and size two heap allocations
   * (the raw index read and the parsed index), so they are bounded before
   * either happens. Without this a crafted header sizes ~340 KB of
   * allocations on a device with a fraction of that free - and because
   * i18n_activate() runs from setup(), the pack that does it is reloaded on
   * every boot.
   *
   * The bound is TR_MAX_ID, not TR_KEY_COUNT. Retiring a key tombstones its
   * id instead of reusing it, so the live-key count sits below the highest
   * id and every catalog we generate carries ids in the holes
   * (count 1395, max_id 1399 today). Bounding by the count rejected all of
   * them and left the device rendering [T<id>] markers. */
  if (pack.count > TR_MAX_ID + 1 || pack.max_id > TR_MAX_ID || pack.count > pack.max_id + 1) {
    return false;
  }
  uint32_t index_bytes = pack.count * 6u;
  if ((uint32_t)total < 12 + index_bytes) {
    return false;
  }
  std::vector<uint8_t> raw(index_bytes);
  if (!i18n_store().read(filename.c_str(), 12, raw.data(), raw.size())) {
    return false;
  }
  pack.index.resize(pack.count);
  for (uint16_t i = 0; i < pack.count; i++) {
    memcpy(&pack.index[i].id, raw.data() + i * 6, 2);
    memcpy(&pack.index[i].offset, raw.data() + i * 6 + 2, 4);
    // find_index() binary-searches this array, which is only meaningful if
    // the ids ascend. An unsorted pack would not crash - it would quietly
    // return the wrong string for a key, and the keys here include fault
    // messages and contactor-state labels. The generator always sorts;
    // nothing guarantees a third-party or hand-built pack does.
    if (i > 0 && pack.index[i].id <= pack.index[i - 1].id) {
      return false;
    }
    /* Every stored id must be within the declared max_id. find_index() takes
     * the dense-direct path when count == max_id + 1 and then uses the id AS
     * the slot subscript; without this, a pack declaring max_id 2 with ids
     * {0, 1, 99} takes that path and serves slot 2's string for key 2 -
     * silently the wrong text, which for these keys means the wrong fault
     * message. Ascending + unique + in-range + count == max_id + 1 together
     * force the ids to be exactly 0..max_id. */
    if (pack.index[i].id > pack.max_id) {
      return false;
    }
  }
  strncpy(pack.filename, filename.c_str(), sizeof(pack.filename) - 1);
  pack.blob_start = 12 + index_bytes;
  pack.file_length = (uint32_t)total;
  pack.loaded = true;
  return true;
}

/* Publish a parsed pack with a single assignment.
 *
 * Readers are not all on the writer's task: i18n_activate() runs on the async
 * web task, while TR_RAW() is reached from the battery/core tasks through
 * set_event()'s logging and from the MQTT task. On a dual-core part those are
 * genuinely concurrent, so rebuilding pack.index in place - clear() then
 * resize() spanning a multi-millisecond parse that reads flash - freed the
 * vector under a reader mid-lookup.
 *
 * Parsing into a local and move-assigning once shrinks that to a single
 * assignment. It does NOT make publication atomic in the memory-model sense:
 * the vector pointer and the loaded flag are separate words, and the old
 * buffer is still freed by the assignment. Closing it properly needs a
 * shared_ptr<const Pack> swapped under a short critical section, with readers
 * taking a local copy - deliberately not done in this pass, recorded as a
 * known residual.
 *
 * A failed parse publishes an EMPTY pack rather than leaving the previous one
 * in place: selecting a language whose pack is missing or rejected must fall
 * back to English, not keep serving the language the user just switched away
 * from. */
static bool load_pack(Pack& pack, const char* lang) {
  Pack staged;
  bool ok = parse_pack(staged, lang);
  if (!ok) {
    staged = Pack();
  }
  pack = std::move(staged);
  return ok;
}

bool i18n_activate(const char* lang) {
  load_pack(english_pack, "en");
  if (lang[0] == '\0' || strcmp(lang, "en") == 0) {
    active_pack.loaded = false;  // English pack serves directly
    return english_pack.loaded;
  }
  return load_pack(active_pack, lang);
}

// Dense-direct when the pack covers every id, binary search otherwise.
// (An interpolation middle tier was considered and cut: lookups run per
// page render, and binary search over <=2k ids is ~11 probes.)
static int find_index(const Pack& pack, uint16_t id) {
  if (id > pack.max_id) {
    return -1;
  }
  if (pack.count == (uint16_t)(pack.max_id + 1)) {
    return id;
  }
  int low = 0;
  int high = (int)pack.count - 1;
  while (low <= high) {
    int mid = (low + high) / 2;
    if (pack.index[mid].id == id) {
      return mid;
    }
    if (pack.index[mid].id < id) {
      low = mid + 1;
    } else {
      high = mid - 1;
    }
  }
  return -1;
}

// A single UI label. Generously above the longest real string (~200 bytes),
// low enough that a crafted pack cannot turn every lookup into a large
// allocation on the render path.
static constexpr uint32_t MAX_STRING_LEN = 1024;

static bool read_string(const Pack& pack, int slot, String* out) {
  if (pack.index[slot].offset >= pack.file_length) {
    return false;
  }
  uint32_t start = pack.blob_start + pack.index[slot].offset;
  uint32_t end = (slot + 1 < (int)pack.count) ? pack.blob_start + pack.index[slot + 1].offset : pack.file_length;
  if (end <= start + 1 || end > pack.file_length || end - start - 1 > MAX_STRING_LEN) {
    return false;
  }
  uint32_t len = end - start - 1;  // Strip the NUL
  std::vector<char> buf(len + 1, 0);
  if (!i18n_store().read(pack.filename, start, buf.data(), len)) {
    return false;
  }
  *out = String(buf.data());
  return true;
}

String TR(TrKey key) {
  return html_escape(TR_RAW(key));
}

String TR_JS(TrKey key) {
  return js_string_escape(TR_RAW(key));
}

String TR_JS(TrKey key, const String& arg0) {
  // Expand FIRST, then escape the whole result: the substituted value needs
  // escaping too, and escaping the template before substitution would leave
  // the argument raw inside an escaped string.
  return js_string_escape(tr_expand(TR_RAW(key), arg0));
}

String TR_JS(TrKey key, const String& arg0, const String& arg1) {
  // Same order as the one-argument form: expand, then escape the result.
  return js_string_escape(tr_expand(TR_RAW(key), arg0, arg1));
}

String TR_RAW(TrKey key) {
  uint16_t id = (uint16_t)key;
  String out;
  if (active_pack.loaded) {
    int slot = find_index(active_pack, id);
    if (slot >= 0 && read_string(active_pack, slot, &out)) {
      return out;
    }
  }
  if (english_pack.loaded) {
    int slot = find_index(english_pack, id);
    if (slot >= 0 && read_string(english_pack, slot, &out)) {
      return out;
    }
  }
  for (uint16_t i = 0; i < TR_BOOTSTRAP_COUNT; i++) {
    if (TR_BOOTSTRAP[i].id == id) {
      return String(TR_BOOTSTRAP[i].text);
    }
  }
  // Last resort: visibly broken on purpose
  char marker[12];
  snprintf(marker, sizeof(marker), "[T%u]", (unsigned)id);
  return String(marker);
}

// Literal single-pass substitution - translated text never reaches printf,
// and substituted values are never re-scanned for tokens
static String expand(const String& text, const String* args, int arg_count) {
  std::string out;
  const char* s = text.c_str();
  while (*s) {
    if (s[0] == '{' && s[1] >= '0' && s[1] < '0' + arg_count && s[2] == '}') {
      out.append(args[s[1] - '0'].c_str());
      s += 3;
    } else {
      out += *s++;
    }
  }
  return String(out.c_str());
}

String tr_expand(const String& text, const String& arg0) {
  return expand(text, &arg0, 1);
}

String tr_expand(const String& text, const String& arg0, const String& arg1) {
  String args[2] = {arg0, arg1};
  return expand(text, args, 2);
}

String& tr_h4(String& out, TrKey key) {
  out += "<h4>";
  out += TR(key);
  out += "</h4>";
  return out;
}

String& tr_h4(String& out, TrKey key, const String& value) {
  out += "<h4>";
  out += TR(key);
  out += ' ';
  out += value;
  out += "</h4>";
  return out;
}

String& tr_h4(String& out, TrKey key, const String& value, const char* suffix) {
  out += "<h4>";
  out += TR(key);
  out += ' ';
  out += value;
  out += suffix;
  out += "</h4>";
  return out;
}

String& tr_h4_open(String& out, TrKey key) {
  out += "<h4>";
  out += TR(key);
  out += ' ';
  return out;
}

String& tr_h4_start(String& out, TrKey key) {
  out += "<h4>";
  out += TR(key);
  return out;
}

String& tr_h4_end(String& out, TrKey key) {
  out += TR(key);
  out += "</h4>";
  return out;
}

String& tr_h4_pair(String& out, TrKey k1, const char* sep1, const String& v1, const char* unit1, TrKey k2,
                   const char* sep2, const String& v2, const char* unit2) {
  out += "<h4>";
  out += TR(k1);
  out += sep1;
  out += v1;
  out += unit1;
  out += " &nbsp; ";
  out += TR(k2);
  out += sep2;
  out += v2;
  out += unit2;
  out += "</h4>";
  return out;
}

String& tr_h3(String& out, TrKey key) {
  out += "<h3>";
  out += TR(key);
  out += "</h3>";
  return out;
}

String& tr_h3(String& out, TrKey key, const String& value) {
  out += "<h3>";
  out += TR(key);
  out += ' ';
  out += value;
  out += "</h3>";
  return out;
}

String& tr_h4_colon(String& out, TrKey key, const String& value) {
  out += "<h4>";
  out += TR(key);
  out += ": ";
  out += value;
  out += "</h4>";
  return out;
}
