#include "sdcard.h"
#include "freertos/ringbuf.h"

File can_log_file;
File log_file;
RingbufHandle_t can_bufferHandle;
RingbufHandle_t log_bufferHandle;

bool can_logging_paused = false;
bool can_file_open = false;
bool delete_can_file = false;

bool logging_paused = false;
bool log_file_open = false;
bool delete_log_file = false;

bool sd_card_active = false;

void delete_can_log() {
  can_logging_paused = true;
  delete_can_file = true;
}

void resume_can_writing() {
  can_logging_paused = false;
  can_log_file = SD_MMC.open(CAN_LOG_FILE, FILE_APPEND);
  can_file_open = true;
}

void pause_can_writing() {
  can_logging_paused = true;
}

void delete_log() {
  logging_paused = true;
  if (log_file_open) {
    log_file.close();
    log_file_open = false;
  }
  SD_MMC.remove(LOG_FILE);
  logging_paused = false;
}

void resume_log_writing() {
  logging_paused = false;
  log_file = SD_MMC.open(LOG_FILE, FILE_APPEND);
  log_file_open = true;
}

void pause_log_writing() {
  logging_paused = true;
}

// Number of frames lost because the ring buffer was full (SD writer stalled).
// Reported as a gap marker in the log once the buffer has room again.
static uint32_t can_frames_dropped = 0;

void add_can_frame_to_buffer(CAN_frame frame, frameDirection msgDir) {

  if (!sd_card_active)
    return;

  unsigned long currentTime = millis();
  // Sized for the worst case: gap marker + header + 64 data bytes (CAN-FD) at 3 chars each
  static char messagestr_buffer[320];
  size_t size = 0;

  if (can_frames_dropped > 0) {
    // Frames were lost while the SD writer was stalled. Record the gap so the
    // log stays honest, then continue with the current frame in the same send.
    size += snprintf(messagestr_buffer + size, sizeof(messagestr_buffer) - size,
                     "[%lu CAN frames dropped, SD buffer full]\n", (unsigned long)can_frames_dropped);
  }

  size += snprintf(messagestr_buffer + size, sizeof(messagestr_buffer) - size, "(%lu.%03lu) %s %lX [%u] ",
                   currentTime / 1000, currentTime % 1000, (msgDir == MSG_RX ? "RX0" : "TX1"), frame.ID, frame.DLC);

  for (uint8_t i = 0; i < frame.DLC; i++) {
    size += snprintf(messagestr_buffer + size, sizeof(messagestr_buffer) - size,
                     (i < frame.DLC - 1) ? "%02X " : "%02X\n", frame.data.u8[i]);
  }
  if (frame.DLC == 0) {  // Frames without payload still need to terminate the line
    size += snprintf(messagestr_buffer + size, sizeof(messagestr_buffer) - size, "\n");
  }

  // One send per frame, zero timeout: this runs in the core task and must never
  // block. If the buffer is full the SD card is stalled anyway - waiting here
  // would not save the frame, it would only delay the 10ms tasks (EVENT_TASK_OVERRUN).
  if (xRingbufferSend(can_bufferHandle, messagestr_buffer, size, 0) != pdTRUE) {
    can_frames_dropped++;
    return;
  }
  can_frames_dropped = 0;
}

void write_can_frame_to_sdcard() {

  if (!sd_card_active)
    return;

  size_t receivedMessageSize;
  uint8_t* buffer = (uint8_t*)xRingbufferReceive(can_bufferHandle, &receivedMessageSize, pdMS_TO_TICKS(10));

  if (buffer != NULL) {

    if (can_logging_paused) {
      if (can_file_open) {
        can_log_file.close();
        can_file_open = false;
      }
      if (delete_can_file) {
        SD_MMC.remove(CAN_LOG_FILE);
        delete_can_file = false;
        can_logging_paused = false;
      }
      vRingbufferReturnItem(can_bufferHandle, (void*)buffer);
      return;
    }

    if (can_file_open == false) {
      can_log_file = SD_MMC.open(CAN_LOG_FILE, FILE_APPEND);
      can_file_open = true;
    }

    can_log_file.write(buffer, receivedMessageSize);
    can_log_file.flush();

    vRingbufferReturnItem(can_bufferHandle, (void*)buffer);
  }
}

void add_log_to_buffer(const uint8_t* buffer, size_t size) {

  if (!sd_card_active)
    return;

  // Zero timeout: called from the logging path of any task, must never block.
  // NOTE: do not log from the failure path here. logging.println() would call
  // Logging::write() -> add_log_to_buffer() again while the buffer is still
  // full, recursing until the stack overflows.
  xRingbufferSend(log_bufferHandle, buffer, size, 0);
}

void write_log_to_sdcard() {

  if (!sd_card_active)
    return;

  size_t receivedMessageSize;
  uint8_t* buffer = (uint8_t*)xRingbufferReceive(log_bufferHandle, &receivedMessageSize, pdMS_TO_TICKS(10));

  if (buffer != NULL) {

    if (logging_paused) {
      vRingbufferReturnItem(log_bufferHandle, (void*)buffer);
      return;
    }

    if (log_file_open == false) {
      log_file = SD_MMC.open(LOG_FILE, FILE_APPEND);
      log_file_open = true;
    }

    log_file.write(buffer, receivedMessageSize);
    log_file.flush();
    vRingbufferReturnItem(log_bufferHandle, (void*)buffer);
  }
}

void init_logging_buffers() {

  if (datalayer.system.info.CAN_SD_logging_active) {
    can_bufferHandle = xRingbufferCreate(32 * 1024, RINGBUF_TYPE_BYTEBUF);
    if (can_bufferHandle == NULL) {
      logging.println("Failed to create CAN ring buffer!");
      return;
    }
  }

  if (datalayer.system.info.SD_logging_active) {
    log_bufferHandle = xRingbufferCreate(1024, RINGBUF_TYPE_BYTEBUF);
    if (log_bufferHandle == NULL) {
      logging.println("Failed to create log ring buffer!");
      return;
    }
  }
}

void deinit_logging_buffers() {
  if ((!datalayer.system.info.CAN_SD_logging_active) && (!datalayer.system.info.CAN_SD_logging_active)) {
    if (can_bufferHandle != NULL) {
      vRingbufferDelete(can_bufferHandle);
    }
    if (log_bufferHandle != NULL) {
      vRingbufferDelete(log_bufferHandle);
    }
  }
}

bool init_sdcard() {
  auto miso_pin = esp32hal->SD_MISO_PIN();
  auto mosi_pin = esp32hal->SD_MOSI_PIN();
  auto sclk_pin = esp32hal->SD_SCLK_PIN();

  if (!esp32hal->alloc_pins("SD Card", miso_pin, mosi_pin, sclk_pin)) {
    return false;
  }

  pinMode(miso_pin, INPUT_PULLUP);

  SD_MMC.setPins(sclk_pin, mosi_pin, miso_pin);
  if (!SD_MMC.begin("/root", true, true, SDMMC_FREQ_HIGHSPEED)) {
    set_event_latched(EVENT_SD_INIT_FAILED, 0);
    logging.println("SD Card initialization failed!");
    return false;
  }

  clear_event(EVENT_SD_INIT_FAILED);
  logging.println("SD Card initialization successful.");

  sd_card_active = true;

  log_sdcard_details();

  return true;
}

void log_sdcard_details() {

  logging.print("SD Card Type: ");
  switch (SD_MMC.cardType()) {
    case CARD_MMC:
      logging.println("MMC");
      break;
    case CARD_SD:
      logging.println("SD");
      break;
    case CARD_SDHC:
      logging.println("SDHC");
      break;
    case CARD_UNKNOWN:
      logging.println("UNKNOWN");
      break;
    case CARD_NONE:
      logging.println("No SD Card found");
      break;
  }

  if (SD_MMC.cardType() != CARD_NONE) {
    logging.print("SD Card Size: ");
    logging.print(SD_MMC.cardSize() / 1024 / 1024);
    logging.println(" MB");

    logging.print("Total space: ");
    logging.print(SD_MMC.totalBytes() / 1024 / 1024);
    logging.println(" MB");

    logging.print("Used space: ");
    logging.print(SD_MMC.usedBytes() / 1024 / 1024);
    logging.println(" MB");
  }
}
