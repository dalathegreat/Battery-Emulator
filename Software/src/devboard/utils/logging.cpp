#include "logging.h"
#include "../../datalayer/datalayer.h"

size_t Logging::write(const uint8_t *buffer, size_t size) {
#ifdef DEBUG_LOG
  char* message_string = datalayer.system.info.logged_can_messages;
  int offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  unsigned long currentTime = millis();
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
  if (buffer[0] != '\r' && buffer[0] != '\n' && 
     (offset == 0 || message_string[offset-1] == '\r' || message_string[offset-1] == '\n')){
    offset += snprintf(message_string + offset, message_string_size - offset - 1, "%8lu.%03lu ", currentTime / 1000,
                       currentTime % 1000);
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
#ifdef DEBUG_VIA_USB
  static char buf[128];
  message_string = buf;
  offset = 0;
  message_string_size = sizeof(buf);
#endif
#ifdef DEBUG_VIA_WEB
  if (datalayer.system.info.can_logging_active){
    return;
  }
  message_string = datalayer.system.info.logged_can_messages;
  offset = datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
  message_string_size = sizeof(datalayer.system.info.logged_can_messages);
#endif    
  if (offset + 128 > sizeof(datalayer.system.info.logged_can_messages)) {
    // Not enough space, reset and start from the beginning
    offset = 0;
  }
  unsigned long currentTime = millis();
  // Add timestamp
#ifdef NTP
  if (timeClient && timeClient->isTimeSet()){
    offset += snprintf(message_string + offset, message_string_size - offset - 1, "%02u:%02u:%02u.%03lu ", 
                      timeClient->getHours(), timeClient->getMinutes(), timeClient->getSeconds(),
                      (currentTime /*- timeClient->_lastUpdate*/) % 1000);
  } else {
#endif
    offset += snprintf(message_string + offset, message_string_size - offset - 1, "%8lu.%03lu ", currentTime / 1000,
                      currentTime % 1000);
#ifdef NTP
  }
#endif

  va_list(args);
  va_start (args, fmt);
  offset +=
      vsnprintf(message_string + offset, message_string_size - offset - 1, fmt, args);
  va_end (args);    

  if (datalayer.system.info.can_logging_active){
    size_t size = offset;
    size_t n = 0;
    while (size--) {
      if (Serial.write(*message_string++)) 
        n++;
      else 
        break;
    }
  } else {
    datalayer.system.info.logged_can_messages_offset = offset;  // Update offset in buffer
  }
#endif  // DEBUG_LOG
}
