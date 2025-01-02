#ifndef SDCARD_H
#define SDCARD_H

#include <SD.h>
#include <SPI.h>
#include "../../communication/can/comm_can.h"
#include "../hal/hal.h"

#if defined(SD_CS_PIN) && defined(SD_SCLK_PIN) && defined(SD_MOSI_PIN) && \
    defined(SD_MISO_PIN)  // ensure code is only compiled if all SD card pins are defined
#define CAN_LOG_FILE "/canlog.txt"

void init_logging_buffer();

void init_sdcard();
void print_sdcard_details();

void add_can_frame_to_buffer(CAN_frame frame, frameDirection msgDir);
void write_can_frame_to_sdcard();

void pause_can_writing();
void resume_can_writing();
void delete_can_log();
#endif  // defined(SD_CS_PIN) && defined(SD_SCLK_PIN) && defined(SD_MOSI_PIN) && defined(SD_MISO_PIN)

#endif  // SDCARD_H
