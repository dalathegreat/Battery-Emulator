#include "logging.h"
#include "../../datalayer/datalayer.h"
#include "../sdcard/sdcard.h"

#define MAX_LINE_LENGTH_PRINTF 128
#define MAX_LENGTH_TIME_STR 14

bool previous_message_was_newline = true;

void Logging::add_timestamp(size_t size) {
#ifdef DEBUG_LOG
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  unsigned long currentTime = millis();
  char* timestr;
  static char timestr_buffer[MAX_LENGTH_TIME_STR];

#ifdef DEBUG_VIA_WEB
  if (!datalayer.system.info.can_logging_active) {
    /* If web debug is active and can logging is inactive, 
     * we use the debug logging memory directly for writing the timestring */
    if (offset + size + MAX_LENGTH_TIME_STR > message_string_size) {
      offset = 0;
    }
    timestr = datalayer.system.info.logged_can_messages + offset;
  } else {
    timestr = timestr_buffer;
  }
#else
  timestr = timestr_buffer;
#endif  // DEBUG_VIA_WEB

  offset += min(MAX_LENGTH_TIME_STR - 1,
                snprintf(timestr, MAX_LENGTH_TIME_STR, "%8lu.%03lu ", currentTime / 1000, currentTime % 1000));

#ifdef DEBUG_VIA_WEB
  if (!datalayer.system.info.can_logging_active) {
    datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
  }
#endif  // DEBUG_VIA_WEB

#ifdef LOG_TO_SD
  add_log_to_buffer((uint8_t*)timestr, MAX_LENGTH_TIME_STR);
#endif  // LOG_TO_SD

#ifdef DEBUG_VIA_USB
  Serial.write(timestr);
#endif  // DEBUG_VIA_USB

#endif  // DEBUG_LOG
}

size_t Logging::write(const uint8_t* buffer, size_t size) {
#ifdef DEBUG_LOG
  if (previous_message_was_newline) {
    add_timestamp(size);
  }

#ifdef LOG_TO_SD
  add_log_to_buffer(buffer, size);
#endif
#ifdef DEBUG_VIA_USB
  Serial.write(buffer, size);
#endif
#ifdef DEBUG_VIA_WEB
  if (!datalayer.system.info.can_logging_active) {
    char* message_string = datalayer.system.info.logged_can_messages;
    int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
    size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

    if (offset + size > message_string_size) {
      offset = 0;
    }
    memcpy(message_string + offset, buffer, size);
    datalayer.system.info.logged_can_messages_offset = offset + size;  // Update offset in buffer
  }
#endif  // DEBUG_VIA_WEB

  previous_message_was_newline = buffer[size - 1] == '\n';
  return size;
#endif  // DEBUG_LOG
  return 0;
}

void Logging::printf(const char* fmt, ...) {
#ifdef DEBUG_LOG
  if (previous_message_was_newline) {
    add_timestamp(MAX_LINE_LENGTH_PRINTF);
  }

  char* message_string = datalayer.system.info.logged_can_messages;
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  static char buffer[MAX_LINE_LENGTH_PRINTF];
  char* message_buffer;
#ifdef DEBUG_VIA_WEB
  if (!datalayer.system.info.can_logging_active) {
    /* If web debug is active and can logging is inactive, 
     * we use the debug logging memory directly for writing the output */
    if (offset + MAX_LINE_LENGTH_PRINTF > message_string_size) {
      // Not enough space, reset and start from the beginning
      offset = 0;
    }
    message_buffer = message_string + offset;
  } else {
    message_buffer = buffer;
  }
#else
  message_buffer = buffer;
#endif  // DEBUG_VIA_WEB

  va_list(args);
  va_start(args, fmt);
  int size = min(MAX_LINE_LENGTH_PRINTF - 1, vsnprintf(message_buffer, MAX_LINE_LENGTH_PRINTF, fmt, args));
  va_end(args);

#ifdef LOG_TO_SD
  add_log_to_buffer((uint8_t*)message_buffer, size);
#endif  // LOG_TO_SD

#ifdef DEBUG_VIA_USB
  Serial.write(message_buffer, size);
#endif  // DEBUG_VIA_USB

#ifdef DEBUG_VIA_WEB
  if (!datalayer.system.info.can_logging_active) {
    // Data was already added to buffer, just move offset
    datalayer.system.info.logged_can_messages_offset =
        offset + size;  // Keeps track of the current position in the buffer
  }
#endif  // DEBUG_VIA_WEB

  previous_message_was_newline = message_buffer[size - 1] == '\n';
#endif  // DEBUG_LOG
}
