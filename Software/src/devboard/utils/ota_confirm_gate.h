#ifndef OTA_CONFIRM_GATE_H
#define OTA_CONFIRM_GATE_H

#include <stdint.h>

/* WHEN a freshly updated image has earned confirmation, and WHO performs it.
 *
 * The decision and the flash write are deliberately split across two contexts,
 * and the split is about routing, not cost:
 *
 *   CHECK  - the 1 ms core tick, unconditionally. That tick alternating for 42 s
 *            is the strongest liveness witness this firmware has: CAN is being
 *            received, the 10 ms and 1 s sub-tasks are both being reached, and
 *            the task watchdog has been fed the whole time. All this side does
 *            is set a flag.
 *   WRITE  - ordinary main-task context. Confirming an image writes otadata,
 *            which is a runtime flash write like any other and belongs on the
 *            same path as every other one (the planned flash-write broker in
 *            the end state: pre-drain, op, drainage gap, so the cache-off
 *            window lands on empty CAN FIFOs). Initiating it from the 1 ms
 *            tick would bypass
 *            that discipline. It would NOT save the tick a stall - during a
 *            flash op the scheduler is suspended whoever started it - so
 *            routing is the whole of the argument.
 *
 * Nothing here touches esp_ota_*: this is the policy, and it holds no opinion
 * about how the confirmation is written. That is what lets the 42 s boundary be
 * tested on the host, where ota_rollback.cpp cannot be built.
 */

// The uptime a fresh image must survive before it is confirmed.
//
// 42 s clears STA association, the TLS MQTT handshake and autodiscovery, so the
// crashes that only happen on first contact - the first frames parsed, the
// first MQTT or HTTP exchange, a watchdog trip under real load - still fall
// inside the window and still roll back. Criteria that sound stronger were
// rejected on purpose: "all RTOS tasks started" measures starting, not
// surviving, and anything externally dependent (CAN frames seen, WiFi up) would
// never confirm on a bench board with no bus or an offline install, so every
// reboot would revert a perfectly good image forever.
constexpr uint32_t OTA_CONFIRM_UPTIME_MS = 42 * 1000;

// CHECK side. Call unconditionally from a steady-state tick, passing the uptime
// in ms (millis(), not a delta). One guarded compare; sets a flag and no more.
//
// The comparison is against absolute uptime rather than an interval, so the
// millis() wrap at ~49.7 days is not a hazard: the flag latches 42 s into the
// first boot, tens of thousands of wraps before the counter comes back round.
void ota_confirm_check(uint32_t uptime_ms);

// A deliberate restart is its own liveness witness - the board was well enough
// to serve the request - so an intentional reboot inside the window keeps the
// update instead of reverting it. Called when the restart is REQUESTED, not
// when it is enacted: the 5-10 s the graceful restart spends pausing
// charge/discharge is what lets the write take the ordinary-context route
// above. A board whose main task cannot complete one pass in those seconds is
// not healthy, and rolling it back is the right answer, not a gap.
void ota_confirm_request(void);

// WRITE side. True exactly once, on the first call after a confirmation becomes
// owed; false forever after. The caller performs the write.
//
// Once taken, no further confirmation is ever owed - including when the write
// itself fails. There is nothing useful to retry (a failure here means otadata
// is unwritable), and re-arming would repeat it on every pass of a loop that
// runs flat out. The failing write says so in the log instead.
bool ota_confirm_take_pending(void);

#ifdef UNIT_TEST
// Test seam only: the gate is a boot-lifetime latch, and the host tests need to
// run more than one boot.
void ota_confirm_gate_reset(void);
#endif

#endif  // OTA_CONFIRM_GATE_H
