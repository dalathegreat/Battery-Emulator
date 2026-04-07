#pragma once

#include <Arduino.h>
#include <string>

/**
 * @brief Replaces placeholder with content section in web page
 *
 * @param[in] var
 *
 * @return String
 */
extern const char can_replay_full_html[];
String can_replay_template_processor(const String& var);
