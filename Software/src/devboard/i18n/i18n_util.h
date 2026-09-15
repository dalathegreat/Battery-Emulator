#ifndef _I18N_UTIL_H_
#define _I18N_UTIL_H_

#include <WString.h>
#include <vector>

/* Host-testable i18n helpers, no filesystem dependencies. */

/* True for catalog filenames of the form ll.json.gz, ll-CC.json.gz,
 * ll.blp or ll-CC.blp (ISO language, optional region). */
bool is_valid_i18n_filename(const String& name);

/* True for a bare language code: ll or ll-CC. The empty string is NOT a
 * valid code - callers that accept "" as "built-in English" test for it
 * separately, so that the accept-empty decision is always explicit. */
bool is_valid_language_code(const String& code);

/* Language code (e.g. "sv" or "pt-BR") from a valid catalog filename. */
String i18n_language_from_filename(const String& name);

/* JSON body for GET /api/i18n: unique language codes from the stored
 * catalog filenames (invalid names ignored) plus the active language.
 * English is the built-in default and is not a stored catalog. */
String i18n_list_json(const std::vector<String>& filenames, const String& active_language);

#endif
