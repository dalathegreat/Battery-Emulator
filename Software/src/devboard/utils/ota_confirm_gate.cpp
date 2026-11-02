#include "ota_confirm_gate.h"

namespace {

/* Two flags, written from different tasks, and no lock - which is safe here
 * because of what they are, not because it is usually fine.
 *
 * Each transition has exactly one writer (the CHECK side only ever sets `owed`,
 * the WRITE side only ever clears it and sets `taken`), each is a single
 * aligned word, and neither ever moves back. The worst a torn read could do is
 * miss the flag for one pass, and the next pass is a millisecond away.
 */
volatile bool confirmation_owed = false;
volatile bool confirmation_taken = false;

}  // namespace

void ota_confirm_check(uint32_t uptime_ms) {
  if (!confirmation_taken && uptime_ms >= OTA_CONFIRM_UPTIME_MS) {
    confirmation_owed = true;
  }
}

void ota_confirm_request(void) {
  if (!confirmation_taken) {
    confirmation_owed = true;
  }
}

bool ota_confirm_take_pending(void) {
  if (!confirmation_owed) {
    return false;
  }
  confirmation_owed = false;
  confirmation_taken = true;
  return true;
}

#ifdef UNIT_TEST
void ota_confirm_gate_reset(void) {
  confirmation_owed = false;
  confirmation_taken = false;
}
#endif
