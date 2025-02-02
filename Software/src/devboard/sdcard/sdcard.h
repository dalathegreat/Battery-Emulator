#ifndef SDCARD_H
#define SDCARD_H

#include <SD_MMC.h>
#include "../../communication/can/comm_can.h"
#include "../hal/hal.h"
#include "../utils/events.h"

#if defined(SD_CS_PIN) && defined(SD_SCLK_PIN) && defined(SD_MOSI_PIN) && \
    defined(SD_MISO_PIN)  // ensure code is only compiled if all SD card pins are defined
#define CAN_LOG_FILE "/canlog.txt"
#define LOG_FILE "/log.txt"

void init_logging_buffers();

void init_sdcard();
void log_sdcard_details();

void add_can_frame_to_buffer(CAN_frame frame, frameDirection msgDir);
void write_can_frame_to_sdcard();

void pause_can_writing();
void resume_can_writing();
void delete_can_log();
void delete_log();
void resume_log_writing();
void pause_log_writing();

void add_log_to_buffer(const uint8_t* buffer, size_t size);
void write_log_to_sdcard();

#endif  // defined(SD_CS_PIN) && defined(SD_SCLK_PIN) && defined(SD_MOSI_PIN) && defined(SD_MISO_PIN)
#endif  // SDCARD_H
