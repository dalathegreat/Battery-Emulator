#include "sdcard.h"
#include "freertos/ringbuf.h"

File can_log_file;
RingbufHandle_t can_bufferHandle;

bool can_logging_paused = false;
bool can_file_open = false;
bool delete_can_file = false;
bool sd_card_active = false;

void delete_can_log() {
  can_logging_paused = true;
  delete_can_file = true;
}

void resume_can_writing() {
  can_logging_paused = false;
  can_log_file = SD.open(CAN_LOG_FILE, FILE_APPEND);
  can_file_open = true;
}

void pause_can_writing() {
  can_logging_paused = true;
}

void add_can_frame_to_buffer(CAN_frame frame, frameDirection msgDir) {

  if (!sd_card_active)
    return;

  CAN_log_frame log_frame = {frame, msgDir};
  if (xRingbufferSend(can_bufferHandle, &log_frame, sizeof(log_frame), 0) != pdTRUE) {
    Serial.println("Failed to send CAN frame to ring buffer!");
    return;
  }
}

void write_can_frame_to_sdcard() {

  if (!sd_card_active)
    return;

  size_t receivedMessageSize;
  CAN_log_frame* log_frame =
      (CAN_log_frame*)xRingbufferReceive(can_bufferHandle, &receivedMessageSize, pdMS_TO_TICKS(10));

  if (log_frame != NULL) {

    if (can_logging_paused) {
      if (can_file_open) {
        can_log_file.close();
        can_file_open = false;
      }
      if (delete_can_file) {
        SD.remove(CAN_LOG_FILE);
        delete_can_file = false;
        can_logging_paused = false;
      }
      vRingbufferReturnItem(can_bufferHandle, (void*)log_frame);
      return;
    }

    if (can_file_open == false) {
      can_log_file = SD.open(CAN_LOG_FILE, FILE_APPEND);
      can_file_open = true;
    }

    uint8_t i = 0;
    can_log_file.print("(");
    can_log_file.print(millis() / 1000.0);
    (log_frame->direction == MSG_RX) ? can_log_file.print(") RX0 ") : can_log_file.print(") TX1 ");
    can_log_file.print(log_frame->frame.ID, HEX);
    can_log_file.print(" [");
    can_log_file.print(log_frame->frame.DLC);
    can_log_file.print("] ");
    for (i = 0; i < log_frame->frame.DLC; i++) {
      can_log_file.print(log_frame->frame.data.u8[i] < 16 ? "0" : "");
      can_log_file.print(log_frame->frame.data.u8[i], HEX);
      if (i < log_frame->frame.DLC - 1)
        can_log_file.print(" ");
    }
    can_log_file.println("");

    vRingbufferReturnItem(can_bufferHandle, (void*)log_frame);
  }
}

void init_logging_buffer() {
  can_bufferHandle = xRingbufferCreate(64 * 1024, RINGBUF_TYPE_BYTEBUF);
  if (can_bufferHandle == NULL) {
    Serial.println("Failed to create CAN ring buffer!");
    return;
  }
}

void init_sdcard() {

  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(SD_SCLK_PIN, OUTPUT);
  pinMode(SD_MOSI_PIN, OUTPUT);
  pinMode(SD_MISO_PIN, INPUT);

  SPI.begin(SD_SCLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card initialization failed!");
    return;
  }

  Serial.println("SD Card initialization successful.");
  sd_card_active = true;

  print_sdcard_details();
}

void print_sdcard_details() {

  Serial.print("SD Card Type: ");
  switch (SD.cardType()) {
    case CARD_MMC:
      Serial.println("MMC");
      break;
    case CARD_SD:
      Serial.println("SD");
      break;
    case CARD_SDHC:
      Serial.println("SDHC");
      break;
    case CARD_UNKNOWN:
      Serial.println("UNKNOWN");
      break;
    case CARD_NONE:
      Serial.println("No SD Card found");
      break;
  }

  if (SD.cardType() != CARD_NONE) {
    Serial.print("SD Card Size: ");
    Serial.print(SD.cardSize() / 1024 / 1024);
    Serial.println(" MB");

    Serial.print("Total space: ");
    Serial.print(SD.totalBytes() / 1024 / 1024);
    Serial.println(" MB");

    Serial.print("Used space: ");
    Serial.print(SD.usedBytes() / 1024 / 1024);
    Serial.println(" MB");
  }
}
