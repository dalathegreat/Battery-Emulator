#include "logging.h"
#include "../../datalayer/datalayer.h"
#include "../sdcard/sdcard.h"

#define MAX_LINE_LENGTH_PRINTF 128
#define MAX_LENGTH_TIME_STR 14

bool previous_message_was_newline = true;

void Logging::add_timestamp(size_t size) {
  // Check if any logging is enabled at runtime
  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.usb_logging_active) {
    return;
  }

  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  unsigned long currentTime = millis();
  char* timestr;
  static char timestr_buffer[MAX_LENGTH_TIME_STR];

  if (datalayer.system.info.web_logging_active) {
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
  } else {
    timestr = timestr_buffer;
  }

  offset += min(MAX_LENGTH_TIME_STR - 1,
                snprintf(timestr, MAX_LENGTH_TIME_STR, "%8lu.%03lu ", currentTime / 1000, currentTime % 1000));

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
  }

  // LOG_TO_SD remains as compile-time option for now
#ifdef LOG_TO_SD
  add_log_to_buffer((uint8_t*)timestr, MAX_LENGTH_TIME_STR);
#endif  // LOG_TO_SD

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(timestr);
  }
}

size_t Logging::write(const uint8_t* buffer, size_t size) {
  // Check if any logging is enabled at runtime
  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.usb_logging_active) {
    return 0;
  }

  if (previous_message_was_newline) {
    add_timestamp(size);
  }

#ifdef LOG_TO_SD
  add_log_to_buffer(buffer, size);
#endif

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(buffer, size);
  }

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    char* message_string = datalayer.system.info.logged_can_messages;
    int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
    size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);

    if (offset + size > message_string_size) {
      offset = 0;
    }
    memcpy(message_string + offset, buffer, size);
    datalayer.system.info.logged_can_messages_offset = offset + size;  // Update offset in buffer
  }

  previous_message_was_newline = buffer[size - 1] == '\n';
  return size;
}

void Logging::printf(const char* fmt, ...) {
  // Check if any logging is enabled at runtime
  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.usb_logging_active) {
    return;
  }

  if (previous_message_was_newline) {
    add_timestamp(MAX_LINE_LENGTH_PRINTF);
  }

  char* message_string = datalayer.system.info.logged_can_messages;
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  static char buffer[MAX_LINE_LENGTH_PRINTF];
  char* message_buffer;

  if (datalayer.system.info.web_logging_active) {
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
  } else {
    message_buffer = buffer;
  }

  va_list(args);
  va_start(args, fmt);
  int size = min(MAX_LINE_LENGTH_PRINTF - 1, vsnprintf(message_buffer, MAX_LINE_LENGTH_PRINTF, fmt, args));
  va_end(args);

#ifdef LOG_TO_SD
  add_log_to_buffer((uint8_t*)message_buffer, size);
#endif  // LOG_TO_SD

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(message_buffer, size);
  }

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    // Data was already added to buffer, just move offset
    datalayer.system.info.logged_can_messages_offset =
        offset + size;  // Keeps track of the current position in the buffer
  }

  previous_message_was_newline = message_buffer[size - 1] == '\n';
}
