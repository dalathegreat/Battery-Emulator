#ifndef _HTML_ESCAPE_H_
#define _HTML_ESCAPE_H_

// Escaping moved to devboard/utils/escape.h so the i18n runtime can use the
// same implementation without depending on the webserver. Kept as a forwarder
// rather than churning the existing includers.
#include "../utils/escape.h"

#endif
