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

void add_can_frame_to_buffer(CAN_frame frame, frameDirection msgDir) {

  if (!sd_card_active)
    return;

  unsigned long currentTime = millis();
  static char messagestr_buffer[32];
  size_t size =
      snprintf(messagestr_buffer + size, sizeof(messagestr_buffer) - size, "(%lu.%03lu) %s %X [%u] ",
               currentTime / 1000, currentTime % 1000, (msgDir == MSG_RX ? "RX0" : "TX1"), frame.ID, frame.DLC);

  if (xRingbufferSend(can_bufferHandle, &messagestr_buffer, size, pdMS_TO_TICKS(2)) != pdTRUE) {
    logging.println("Failed to send message to can ring buffer!");
    return;
  }

  uint8_t i = 0;
  for (i = 0; i < frame.DLC; i++) {
    if (i < frame.DLC - 1)
      size = snprintf(messagestr_buffer, sizeof(messagestr_buffer), "%02X ", frame.data.u8[i]);
    else
      size = snprintf(messagestr_buffer, sizeof(messagestr_buffer), "%02X\n", frame.data.u8[i]);

    if (xRingbufferSend(can_bufferHandle, &messagestr_buffer, size, pdMS_TO_TICKS(2)) != pdTRUE) {
      logging.println("Failed to send message to can ring buffer!");
      return;
    }
  }
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

  if (xRingbufferSend(log_bufferHandle, buffer, size, pdMS_TO_TICKS(1)) != pdTRUE) {
    logging.println("Failed to send message to log ring buffer!");
    return;
  }
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
