#include "logging.h"
#include "../../datalayer/datalayer.h"
#include "../sdcard/sdcard.h"

#ifndef SMALL_FLASH_DEVICE
#include <WiFi.h>
#include <WiFiUdp.h>
#include "../wifi/wifi.h"  // custom_hostname
#endif

#define MAX_LINE_LENGTH_PRINTF 128
#define MAX_LENGTH_TIME_STR 14

bool previous_message_was_newline = true;

#ifndef SMALL_FLASH_DEVICE
// ---- Remote syslog sink ----
std::string syslog_ip;
uint16_t syslog_port = 514;
uint8_t syslog_facility = 1;  // 1 = user-level

static const uint8_t SYSLOG_DEFAULT_SEVERITY = 7;  // debug — for messages without an event level
static int syslog_next_severity = -1;              // -1 = use default; set per-line by set_next_severity()
static WiFiUDP syslogUdp;
#define SYSLOG_LINE_MAX 240
static char syslogLine[SYSLOG_LINE_MAX];
static size_t syslogLineLen = 0;

// ---- Pre-connectivity backlog ----
// Lines logged before we can send are buffered here and replayed on connect.
// Heap-allocated on first use, freed once drained. Record layout: [uint8 severity][text\0]
#define SYSLOG_BACKLOG_MAX 4096
#define SYSLOG_BACKLOG_BURST 4  // records per flush call — keeps the core task inside its 10 ms budget 
static char* syslogBacklog = nullptr;
static size_t syslogBacklogLen = 0;
static size_t syslogBacklogPos = 0;

// Same rule as init_WiFi(): custom hostname if set, otherwise the MAC-derived default.
// Cached — default_hostname() re-reads eFuse and allocates a String on every call.
static const char* syslog_hostname(void) {
  if (!custom_hostname.empty()) {
    return custom_hostname.c_str();
  }
  static String fallback;
  if (fallback.isEmpty()) {
    fallback = default_hostname();
  }
  return fallback.c_str();
}

static void syslog_send(uint8_t sev, const char* msg) {
  IPAddress dst;
  if (!dst.fromString(syslog_ip.c_str())) {  // empty or invalid IP -> skip
    return;
  }
  uint8_t pri = (uint8_t)((syslog_facility & 0x1F) * 8 + (sev & 0x07));
  if (syslogUdp.beginPacket(dst, syslog_port)) {
    // RFC 5424: <PRI>1 TIMESTAMP HOSTNAME APP PROCID MSGID MSG
    // NILVALUE '-' timestamp -> the syslog server stamps on receipt.
    syslogUdp.printf("<%u>1 - %s BatteryEmulator - - - %s", pri, syslog_hostname(), msg);
    syslogUdp.endPacket();
  }
}

static void syslog_backlog_push(uint8_t sev, const char* msg) {
  char rec[SYSLOG_LINE_MAX + 24];
  unsigned long ms = millis();
  // No wall clock at boot: carry the uptime in MSG, else replayed lines all look simultaneous.
  int n = snprintf(rec, sizeof(rec), "[boot +%lu.%03lus] %s", ms / 1000, ms % 1000, msg);
  if (n < 0) {
    return;
  }
  if (syslogBacklog == nullptr) {
    IPAddress dst;
    if (!dst.fromString(syslog_ip.c_str())) {
      return;  // no syslog server configured -> never allocate
    }
    syslogBacklog = (char*)malloc(SYSLOG_BACKLOG_MAX);
    if (syslogBacklog == nullptr) {
      return;  // out of heap -> drop; logging must never be fatal
    }
  }
  if (syslogBacklogLen + 1 + (size_t)n + 1 > SYSLOG_BACKLOG_MAX) {
    return;  // full -> keep the earliest lines, they are the point of buffering
  }
  syslogBacklog[syslogBacklogLen++] = (char)sev;
  memcpy(syslogBacklog + syslogBacklogLen, rec, n + 1);
  syslogBacklogLen += n + 1;
}

void syslog_backlog_flush(void) {
  if (syslogBacklog == nullptr) {
    return;
  }
  if (WiFi.status() != WL_CONNECTED && WiFi.softAPgetStationNum() == 0) {
    return;
  }
  uint8_t sent = 0;
  while (syslogBacklogPos < syslogBacklogLen && sent < SYSLOG_BACKLOG_BURST) {
    uint8_t sev = (uint8_t)syslogBacklog[syslogBacklogPos++];
    const char* msg = &syslogBacklog[syslogBacklogPos];
    syslog_send(sev, msg);
    syslogBacklogPos += strlen(msg) + 1;
    sent++;
  }
  if (syslogBacklogPos >= syslogBacklogLen) {  // drained -> give the RAM back
    free(syslogBacklog);
    syslogBacklog = nullptr;
    syslogBacklogLen = 0;
    syslogBacklogPos = 0;
  }
}

void Logging::set_next_severity(uint8_t sev) {
  syslog_next_severity = (int)sev;
}

static void syslog_flush_line(void) {
  int sev_override = syslog_next_severity;
  syslog_next_severity = -1;  // consume regardless of outcome
  size_t len = syslogLineLen;
  syslogLineLen = 0;
  if (len == 0) {
    return;
  }
  syslogLine[len] = '\0';
  uint8_t sev = (sev_override >= 0) ? (uint8_t)sev_override : SYSLOG_DEFAULT_SEVERITY;
  if (WiFi.status() == WL_CONNECTED || WiFi.softAPgetStationNum() > 0) {
    syslog_backlog_flush();  // replay buffered lines first, keeps ordering
    syslog_send(sev, syslogLine);
  } else {
    syslog_backlog_push(sev, syslogLine);
  }
}

static void syslog_emit(const uint8_t* buffer, size_t size) {
  if (!datalayer.system.info.syslog_logging_active) {
    return;
  }
  for (size_t i = 0; i < size; i++) {
    char c = (char)buffer[i];
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      syslog_flush_line();
    } else {
      syslogLine[syslogLineLen++] = c;
      if (syslogLineLen >= SYSLOG_LINE_MAX - 1) {
        syslog_flush_line();
      }
    }
  }
}
#endif

void Logging::add_timestamp(size_t size) {

  size_t offset =
      datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
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

  if (datalayer.system.info.SD_logging_active) {
    add_log_to_buffer((uint8_t*)timestr, MAX_LENGTH_TIME_STR);
  }

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(timestr);
  }
}

size_t Logging::write(const uint8_t* buffer, size_t size) {
  // Check if any logging is enabled at runtime
  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.usb_logging_active &&
      !datalayer.system.info.SD_logging_active && !datalayer.system.info.syslog_logging_active) {
    return 0;
  }

  // If size is 0, we can skip all the processing and just return
  if (size == 0) {
    return 0;
  }

  if (previous_message_was_newline) {
    add_timestamp(size);
  }

  if (datalayer.system.info.SD_logging_active) {
    add_log_to_buffer(buffer, size);
  }

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(buffer, size);
  }

#ifndef SMALL_FLASH_DEVICE
  syslog_emit(buffer, size);
#endif

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    char* message_string = datalayer.system.info.logged_can_messages;
    size_t offset =
        datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
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
  if (!datalayer.system.info.web_logging_active && !datalayer.system.info.usb_logging_active &&
      !datalayer.system.info.SD_logging_active && !datalayer.system.info.syslog_logging_active) {
    return;
  }

  if (previous_message_was_newline) {
    add_timestamp(MAX_LINE_LENGTH_PRINTF);
  }

  char* message_string = datalayer.system.info.logged_can_messages;
  size_t message_string_size = sizeof(datalayer.system.info.logged_can_messages);
  size_t offset =
      datalayer.system.info.logged_can_messages_offset;  // Keeps track of the current position in the buffer
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

  va_list args;
  va_start(args, fmt);
  int size = min(MAX_LINE_LENGTH_PRINTF - 1, vsnprintf(message_buffer, MAX_LINE_LENGTH_PRINTF, fmt, args));
  va_end(args);

  if (datalayer.system.info.SD_logging_active) {
    add_log_to_buffer((uint8_t*)message_buffer, size);
  }

  if (datalayer.system.info.usb_logging_active) {
    Serial.write(message_buffer, size);
  }

#ifndef SMALL_FLASH_DEVICE
  syslog_emit((const uint8_t*)message_buffer, size);
#endif

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    // Data was already added to buffer, just move offset
    datalayer.system.info.logged_can_messages_offset =
        offset + size;  // Keeps track of the current position in the buffer
  }

  previous_message_was_newline = message_buffer[size - 1] == '\n';
}
