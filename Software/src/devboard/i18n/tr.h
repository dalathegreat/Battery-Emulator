#ifndef _I18N_TR_H_
#define _I18N_TR_H_

#include <WString.h>
#include "i18n_bootstrap.h"
#include "i18n_keys.h"

// "No string" sentinel for optional TrKey fields (e.g. commands without a
// confirmation prompt). Never a valid generated id.
constexpr TrKey TR_NONE = static_cast<TrKey>(0xFFFF);

/* TR() runtime: server-side rendering reads the ACTIVE language pack (.blp in
 * the slot store). Activation loads only the pack's index into heap (~6 B per
 * key); strings are seek-read on demand. Lookup is dense-direct when the pack
 * covers every id (count == max_id + 1, the freshly-generated common case),
 * binary search otherwise. Fallback chain: active pack -> en.blp -> bootstrap
 * table -> "[T<id>]" (visibly broken on purpose).
 *
 * Translated text NEVER passes through printf: token expansion is literal
 * substitution of {0}/{1}/... via tr_expand().
 *
 * TR() returns HTML-ESCAPED text, because almost every caller concatenates it
 * straight into markup. That is a safety property - a pack is uploaded data
 * and must not be able to inject markup or script into the admin UI - but it
 * is also plain correctness: real catalogs contain literal metacharacters
 * ("U<20V", "SOC is < 15 or > 90", "UK & N Ireland") that a browser would
 * otherwise swallow as a tag. Use TR_RAW() for the few consumers that are not
 * HTML (MQTT payloads, logs, JS string literals - the last still needs
 * js_escape()). */

// (Re)load pack indexes for the given language ("" = English only)
bool i18n_activate(const char* lang);

String TR(TrKey key);

/* Escaped for a JavaScript string literal. Use this rather than composing
 * js_string_escape(TR_RAW(...)) by hand: composing it leaves two ways to get
 * it wrong that nothing catches - js_string_escape(TR(...)) double-escapes,
 * and omitting the wrapper puts HTML-escaped text into a <script>, which is
 * how a translation once broke out of a template literal. The two-argument
 * form expands {0} and escapes the result, so the substituted value is
 * escaped as well. */
String TR_JS(TrKey key);
String TR_JS(TrKey key, const String& arg0);
String TR_JS(TrKey key, const String& arg0, const String& arg1);

/* Unescaped lookup. Correct ONLY where the sink is not markup and not
 * JavaScript: text/plain bodies, JSON values, MQTT payloads, logs. If the
 * value is going into a page, it wants TR() or TR_JS(). */
String TR_RAW(TrKey key);

String tr_expand(const String& text, const String& arg0);
String tr_expand(const String& text, const String& arg0, const String& arg1);

/* Compact append helpers for the battery-status pages: one out-of-line call
 * per row instead of an inlined String operator+ chain at every site (the
 * chains cost more flash than the removed literals saved). */
String& tr_h4(String& out, TrKey key);                       // <h4>{key}</h4>
String& tr_h4(String& out, TrKey key, const String& value);  // <h4>{key} {value}</h4>
String& tr_h4(String& out, TrKey key, const String& value, const char* suffix);
String& tr_h4_open(String& out, TrKey key);   // "<h4>{key} " - caller appends value and </h4>
String& tr_h4_start(String& out, TrKey key);  // "<h4>{key}" - key text carries its own separator
String& tr_h4_end(String& out, TrKey key);
String& tr_h4_colon(String& out, TrKey key, const String& value);  // <h4>{key}: {value}</h4>
String& tr_h3(String& out, TrKey key);                             // <h3>{key}</h3>
String& tr_h3(String& out, TrKey key,
              const String& value);  // <h3>{key} {value}</h3>    // "{key}</h4>" - closes a row a branch opened
// "<h4>{k1}{sep1}{v1}{unit1} &nbsp; {k2}{sep2}{v2}{unit2}</h4>" - paired-label row
String& tr_h4_pair(String& out, TrKey k1, const char* sep1, const String& v1, const char* unit1, TrKey k2,
                   const char* sep2, const String& v2, const char* unit2);


/* Bounds-checked catalog lookup for value tables, replacing per-value switch
 * chains. Adopted from upstream's enum_str (PR #1941 era) and kept table-driven
 * for the same reason: taking the array by reference deduces N, so a table and
 * its length can never drift apart. The entries are catalog keys rather than
 * literals, so the pooling upstream did by splitting label from value is
 * subsumed by the catalog - one stored copy per string, and translatable.
 * Out-of-range reproduces the "?" of the switches this replaces. */
template <size_t N>
String tr_enum(const TrKey (&table)[N], uint8_t value) {
  return value < N ? TR(table[value]) : String("?");
}

#endif
