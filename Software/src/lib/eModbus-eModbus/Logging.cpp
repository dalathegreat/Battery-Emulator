// =================================================================================================
// eModbus: Copyright 2020 by Michael Harwerth, Bert Melis and the contributors to eModbus
//               MIT license - see license.md for details
// =================================================================================================
#include "Logging.h"
#include <cinttypes>

int MBUlogLvl = LOG_LEVEL;
#if IS_LINUX
#define PrintOut printf

void logHexDump(const char *letter, const char *label, const uint8_t *data, const size_t length) {
#else
Print *LOGDEVICE = &Serial;
#define PrintOut output->printf

void logHexDump(Print *output, const char *letter, const char *label, const uint8_t *data, const size_t length) {
#endif
  size_t cnt = 0;
  size_t step = 0;
  char limiter = '|';
  // Use line buffer to speed up output
  const uint16_t BUFLEN(80);
  const uint16_t ascOffset(61);
  char linebuf[BUFLEN];
  char *cp = linebuf;
  const char HEXDIGIT[] = "0123456789ABCDEF";

  // Print out header
  PrintOut ("[%s] %s: @%" PRIXPTR "/%" PRIu32 ":\n", letter, label, (uintptr_t)data, (uint32_t)(length & 0xFFFFFFFF));

  // loop over data in steps of 16
  for (cnt = 0; cnt < length; ++cnt) {
    step = cnt % 16;
    // New line?
    if (step == 0) {
      // Yes. Clear line and print address header
      memset(linebuf, ' ', BUFLEN);
      linebuf[60] = limiter;
      linebuf[77] = limiter;
      linebuf[78] = '\n';
      linebuf[BUFLEN - 1] = 0;
      snprintf(linebuf, BUFLEN, "  %c %04X: ", limiter, (uint16_t)(cnt & 0xFFFF));
      cp = linebuf + strlen(linebuf);
      // No, but first block of 8 done?
    } else if (step == 8) {
      // Yes, put out additional space
      cp++;
    }
    // Print data byte
    uint8_t c = data[cnt];
    *cp++ = HEXDIGIT[(c >> 4) & 0x0F];
    *cp++ = HEXDIGIT[c & 0x0F];
    *cp++ = ' ';
    if (c >= 32 && c <= 127) linebuf[ascOffset + step] = c;
    else                     linebuf[ascOffset + step] = '.';
    // Line end?
    if (step == 15) {
      // Yes, print line
      PrintOut ("%s", linebuf);
    }
  }
  // Unfinished line?
  if (length && step != 15) {
      // Yes, print line
      PrintOut ("%s", linebuf);
  }
}
