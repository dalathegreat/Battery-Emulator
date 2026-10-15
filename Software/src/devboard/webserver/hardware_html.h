#ifndef HARDWARE_HTML_H
#define HARDWARE_HTML_H

#ifdef HW_UNIFIED_S3

#include <Arduino.h>
#include "index_html.h"

// Page listing the active board configuration, the boot-time SPI CAN probe
// result and any validation findings, with a form to upload a new file.
extern const char hardware_html[];

String hardware_processor(const String& var);

#endif  // HW_UNIFIED_S3
#endif  // HARDWARE_HTML_H
