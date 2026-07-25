#include "logging.h"
#include "../../datalayer/datalayer.h"
#include "../sdcard/sdcard.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include "../wifi/wifi.h"  // custom_hostname, default_hostname()

#define MAX_LINE_LENGTH_PRINTF 128
#define MAX_LENGTH_TIME_STR 14

bool previous_message_was_newline = true;

// ---- USB debug-log sink ----
// Writes only when the TX ring buffer has room. This path runs in the core task
// (and others): it must never block on a host that stops draining the port
// (EVENT_TASK_OVERRUN). Lost output is reported with a marker once the port
// recovers, and the marker is always flushed before output resumes so the gap
// is visible at the right place. CAN frame logging has its own equivalent
// implementation of this pattern in comm_can.cpp.
// The drop counter is not atomic; concurrent writers can at worst miscount
// drops slightly, which only affects the marker, never the data path.
static uint32_t usb_log_chunks_dropped = 0;

static void usb_log_write(const uint8_t* buffer, size_t size) {
  static const char marker[] = "\n[USB log output dropped, host not draining port]\n";
  size_t needed = size;
  if (usb_log_chunks_dropped > 0) {
    needed += sizeof(marker) - 1;
  }
  if ((size_t)Serial.availableForWrite() < needed) {
    usb_log_chunks_dropped++;
    return;
  }
  if (usb_log_chunks_dropped > 0) {
    Serial.write((const uint8_t*)marker, sizeof(marker) - 1);
    usb_log_chunks_dropped = 0;
  }
  Serial.write(buffer, size);
}

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

// ---- Syslog queue + sender task ----
// Every assembled line is queued; a dedicated low-priority task does ALL of the UDP sending.
//
// The logging path (which runs in the core task, the MQTT task, arduino_events, ...) therefore
// never touches the network and never blocks -> it cannot cause EVENT_TASK_OVERRUN.
// A single consumer draining a single FIFO also guarantees that lines arrive in order and that
// two tasks can never interleave inside syslogUdp (which produced corrupted packets before).
//
// Lines logged before we have connectivity simply wait in the queue and are sent once the sender
// task sees the link come up. Record layout: [uint8 severity][text\0]
#define SYSLOG_QUEUE_MAX 4096
#define SYSLOG_MSG_MAX (SYSLOG_LINE_MAX + 24)  // room for the "[boot +N.NNNs] " prefix
static char* syslogQueue = nullptr;
static size_t syslogQueueLen = 0;  // write cursor
static size_t syslogQueuePos = 0;  // read cursor
static uint16_t syslogDropped = 0;

// Guards syslogQueue/Len/Pos only. It is NEVER held while sending, so it is only ever
// owned for a few microseconds (a memcpy) and contention is effectively zero.
static SemaphoreHandle_t syslogMutex = xSemaphoreCreateMutex();

// Same rule as init_WiFi(): custom hostname if set, otherwise the MAC-derived default.
// Cached — default_hostname() re-reads eFuse and allocates a String on every call.
// Only the default is cached, so a call made before settings are loaded cannot freeze a wrong name in.
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

static bool syslog_online(void) {
  // Sendable when joined to a network (STA) OR when a client is on our SoftAP.
  return (WiFi.status() == WL_CONNECTED) || (WiFi.softAPgetStationNum() > 0);
}

// Called ONLY from syslog_task, and never with the mutex held.
static void syslog_send(uint8_t sev, const char* proc, const char* msg) {
  IPAddress dst;
  if (!dst.fromString(syslog_ip.c_str())) {  // empty or invalid IP -> skip
    return;
  }
  uint8_t pri = (uint8_t)((syslog_facility & 0x1F) * 8 + (sev & 0x07));
  if (syslogUdp.beginPacket(dst, syslog_port)) {
    // RFC 5424: <PRI>1 TIMESTAMP HOSTNAME APP PROCID MSGID MSG
    // NILVALUE '-' timestamp -> the syslog server stamps on receipt.
    // APP-NAME carries the FreeRTOS task that produced the line.
    syslogUdp.printf("<%u>1 - %s %s - - - %s", pri, syslog_hostname(), proc, msg);
    syslogUdp.endPacket();
  }
}

// Called from the logging path, in whatever task did the logging. Does no I/O.
static void syslog_queue_push(uint8_t sev, const char* msg) {
  // MUST be read here: we are in the caller's task. Reading it in syslog_task
  // would label every line "syslog".
  const char* proc = pcTaskGetName(nullptr);
  if (proc == nullptr || proc[0] == '\0') {
    proc = "-";  // NILVALUE
  }
  size_t plen = strlen(proc);
  char rec[SYSLOG_MSG_MAX];
  int n;
  if (syslog_online()) {
    n = snprintf(rec, sizeof(rec), "%s", msg);
  } else {
    // No wall clock this early and the server stamps on receipt, so carry the uptime in MSG.
    // Without this, everything logged before the link came up looks simultaneous.
    unsigned long ms = millis();
    n = snprintf(rec, sizeof(rec), "[boot +%lu.%03lus] %s", ms / 1000, ms % 1000, msg);
  }
  if (n < 0 || syslogMutex == nullptr) {
    return;
  }
  // Short timeout: the holder only ever does a memcpy, so this never actually waits.
  if (xSemaphoreTake(syslogMutex, pdMS_TO_TICKS(2)) != pdTRUE) {
    return;
  }

  if (syslogQueue == nullptr) {
    IPAddress dst;
    if (!dst.fromString(syslog_ip.c_str())) {
      xSemaphoreGive(syslogMutex);  // no syslog server configured -> never allocate
      return;
    }
    syslogQueue = (char*)malloc(SYSLOG_QUEUE_MAX);
    if (syslogQueue == nullptr) {
      xSemaphoreGive(syslogMutex);  // out of heap -> drop; logging must never be fatal
      return;
    }
  }

  size_t need = 1 + plen + 1 + (size_t)n + 1;
  if (syslogQueueLen + need > SYSLOG_QUEUE_MAX && syslogQueuePos > 0) {
    // Tail hit the cap but the head is already sent: slide the pending region down.
    memmove(syslogQueue, syslogQueue + syslogQueuePos, syslogQueueLen - syslogQueuePos);
    syslogQueueLen -= syslogQueuePos;
    syslogQueuePos = 0;
  }
  if (syslogQueueLen + need > SYSLOG_QUEUE_MAX) {
    if (syslogDropped < UINT16_MAX) {
      syslogDropped++;  // genuinely full -> keep the earliest lines
    }
    xSemaphoreGive(syslogMutex);
    return;
  }

  syslogQueue[syslogQueueLen++] = (char)sev;
  memcpy(syslogQueue + syslogQueueLen, proc, plen + 1);
  syslogQueueLen += plen + 1;
  memcpy(syslogQueue + syslogQueueLen, rec, n + 1);
  syslogQueueLen += n + 1;
  xSemaphoreGive(syslogMutex);
}

// Pops one record under the lock, then sends it with the lock released.
static void syslog_task(void* arg) {
  char msg[SYSLOG_MSG_MAX];
  char proc[configMAX_TASK_NAME_LEN];
  uint8_t sev = 0;
  uint16_t dropped = 0;

  for (;;) {
    bool have = false;

    if (syslog_online() && syslogMutex != nullptr && xSemaphoreTake(syslogMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      if (syslogQueue != nullptr && syslogQueuePos < syslogQueueLen) {
        sev = (uint8_t)syslogQueue[syslogQueuePos++];
        const char* rec = &syslogQueue[syslogQueuePos];
        snprintf(proc, sizeof(proc), "%s", rec);  // task name
        syslogQueuePos += strlen(rec) + 1;
        rec = &syslogQueue[syslogQueuePos];
        snprintf(msg, sizeof(msg), "%s", rec);  // message; copy out so we can send unlocked
        syslogQueuePos += strlen(rec) + 1;
        have = true;

        if (syslogQueuePos >= syslogQueueLen) {  // drained -> rewind
          syslogQueuePos = 0;
          syslogQueueLen = 0;
          dropped = syslogDropped;
          syslogDropped = 0;
        }
      }
      xSemaphoreGive(syslogMutex);
    }

    if (have) {
      syslog_send(sev, proc, msg);  // UDP happens OUTSIDE the lock
    }
    if (dropped) {
      char note[64];
      snprintf(note, sizeof(note), "syslog queue was full, %u line(s) dropped", dropped);
      syslog_send(4, "syslog", note);  // severity 4 = warning
      dropped = 0;
    }

    // 2 ms between packets paces the boot replay without touching the core loop.
    vTaskDelay(pdMS_TO_TICKS(have ? 2 : 50));
  }
}

// Starts the sender task. Safe to call more than once.
void syslog_start(void) {
  static bool started = false;
  if (!started) {
    started = true;
    xTaskCreate(syslog_task, "syslog", 3072, nullptr, tskIDLE_PRIORITY + 1, nullptr);
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
  syslog_queue_push(sev, syslogLine);
}

static void syslog_emit(const uint8_t* buffer, size_t size) {
  if (!datalayer.system.info.syslog_logging_active) {
    return;
  }
  // No connectivity check: lines logged before the link is up are queued and sent later.
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
    usb_log_write((const uint8_t*)timestr, strlen(timestr));
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
    usb_log_write(buffer, size);
  }

  syslog_emit(buffer, size);

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
    usb_log_write((const uint8_t*)message_buffer, size);
  }

  syslog_emit((const uint8_t*)message_buffer, size);

  if (datalayer.system.info.web_logging_active && !datalayer.system.info.can_logging_active) {
    // Data was already added to buffer, just move offset
    datalayer.system.info.logged_can_messages_offset =
        offset + size;  // Keeps track of the current position in the buffer
  }

  previous_message_was_newline = message_buffer[size - 1] == '\n';
}
