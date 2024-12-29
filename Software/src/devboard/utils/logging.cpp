#include "logging.h"
#include "../../datalayer/datalayer.h"
#include "../sdcard/sdcard.h"

bool previous_message_was_newline = true;

size_t Logging::write(const uint8_t* buffer, size_t size) {
#ifdef DEBUG_LOG
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  unsigned long currentTime = millis();
  char timestr[14];
  if (previous_message_was_newline) {
    snprintf(timestr, sizeof(timestr), "%8lu.%03lu ", currentTime / 1000, currentTime % 1000);
    for (int i = 0; i < 13; i++) {
#ifdef LOG_TO_SD
      add_log_to_buffer(timestr[i]);
#endif
#ifdef DEBUG_VIA_USB
      Serial.write(timestr[i]);
#endif
    }
  }
  previous_message_was_newline = buffer[size - 1] == '\n';
#ifdef LOG_TO_SD
  for (size_t i = 0; i < size; i++) {
    add_log_to_buffer(buffer[i]);
  }
#endif
#ifdef DEBUG_VIA_USB
  size_t n = 0;
  while (size--) {
    if (Serial.write(*buffer++))
      n++;
    else
      break;
  }
  return n;
#endif
#ifdef DEBUG_VIA_WEB
  if (datalayer.system.info.can_logging_active) {
    return 0;
  }
  if (offset + size + 13 > sizeof(datalayer.system.info.logged_can_messages)) {
    offset = 0;
  }
  if (previous_message_was_newline) {
    memcpy(message_string + offset, timestr, 13);
    offset += 13;
  }

  memcpy(message_string + offset, buffer, size);
  datalayer.system.info.logged_can_messages_offset = offset + size;  // Update offset in buffer
  return size;
#endif  // DEBUG_VIA_WEB
#endif  // DEBUG_LOG
  return 0;
}

void Logging::printf(const char* fmt, ...) {
#ifdef DEBUG_LOG
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  unsigned long currentTime = millis();
  char timestr[14];
  snprintf(timestr, sizeof(timestr), "%8lu.%03lu ", currentTime / 1000, currentTime % 1000);
  for (int i = 0; i < 13; i++) {
#ifdef LOG_TO_SD
    add_log_to_buffer(timestr[i]);
#endif
#ifdef DEBUG_VIA_USB
    Serial.write(timestr[i]);
#endif
  }

  static char buffer[128];
  va_list(args);
  va_start(args, fmt);
  int size = vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);

  for (int i = 0; i < size; i++) {
#ifdef LOG_TO_SD
    add_log_to_buffer(buffer[i]);
#endif
#ifdef DEBUG_VIA_USB
    Serial.write(buffer[i]);
#endif
  }
#endif

#ifdef DEBUG_VIA_WEB
  if (datalayer.system.info.can_logging_active) {
    return;
  }
  message_string = datalayer.system.info.logged_can_messages;
  offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  message_string_size = sizeof(datalayer.system.info.logged_can_messages);

  if (offset + 128 > sizeof(datalayer.system.info.logged_can_messages)) {
    // Not enough space, reset and start from the beginning
    offset = 0;
  }

  // Add timestamp
  memcpy(message_string + offset, timestr, 13);
  offset += 13;

  memcpy(message_string + offset, buffer, size);
  datalayer.system.info.logged_can_messages_offset = offset + size;  // Update offset in buffer

  previous_message_was_newline = true;
#endif
}
