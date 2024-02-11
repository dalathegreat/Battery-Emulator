#ifndef EVENTS_H
#define EVENTS_H

#include "../utils/events.h"

extern EVENTS_STRUCT_TYPE entries[EVENT_NOF_EVENTS];

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
String events_processor(const String& var);

#endif
