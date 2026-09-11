#ifndef _I18N_H_
#define _I18N_H_

#include <string>
#include <vector>
#include "i18n_store.h"
#include "i18n_util.h"

/* i18n catalog storage: a raw slot store on the (previously unused) spiffs
 * partition - see i18n_store.h. ~4 KB of code on every board, no filesystem
 * driver. Mount failure is tolerated: the feature is simply off until the
 * user formats the storage from the Languages block. */

extern std::string user_selected_language;  // "" = built-in English

bool init_i18n_storage();
bool i18n_storage_available();
bool format_i18n_storage();

// Stored catalog filenames (directory listing, unvalidated)
std::vector<String> i18n_stored_files();

// The store itself, for the endpoints (serve/upload/delete)
I18nStore& i18n_store();

#endif
