#ifndef PARALLEL_SAFETY_H
#define PARALLEL_SAFETY_H

#include <stdint.h>

/* This layer's own mutable state, gathered into one plain struct.
 *
 * It used to live as function-local statics, which is invisible to the unit
 * tests: they run every case in one process, so a latched startup grace or a
 * half-elapsed drift timer left by one case survived into the next and quietly
 * changed its preconditions. Worse, no case could exercise the startup grace
 * at all, because the first one to latch it latched it for the whole run.
 *
 * Data only, no methods - the behaviour stays in the free functions below,
 * the same shape the datalayer types use. Gathering the variables is the
 * point: reinitialising is one assignment of a fresh instance rather than a
 * list of fields that the next one added would silently escape.
 */
struct ParallelJoinState {
  /* Startup grace. 3700 dV is the datalayer's "not decoded yet" default and
     also an ordinary reading for a pack at 370.0 V, so the skip latches rather
     than applying forever: once a joiner and the main pack have both been seen
     off it, the 1.5 V check runs from then on. */
  bool voltages_seen_battery2 = false;
  bool voltages_seen_battery3 = false;

  // Drift timers. They accumulate across calls by design, which in a
  // single-process test binary means across tests too.
  uint8_t seconds_out_of_sync_battery2 = 0;
  uint8_t seconds_out_of_sync_battery3 = 0;
};

/* Declared here rather than kept private to the .cpp, matching SafetyState:
   the tests are the reason the state was gathered in the first place, and
   reaching a precondition by driving the check through N ticks is exactly the
   indirection that makes these cases hard to follow. A test can set the latch
   or the drift counter directly and then assert one thing. */
extern ParallelJoinState join_state;

/**
 * @brief Safety checks for parallel-connected secondary batteries.
 *
 * Called once per second. Verifies that the specified secondary battery
 * is safe to remain connected in parallel with the primary battery.
 * Currently checks voltage synchronization — if voltages drift apart
 * by more than 1.5V for longer than 10 seconds, the secondary battery
 * is disconnected.
 *
 * @param[in] batteryNumber The battery to check (2 or 3)
 */
void check_parallel_battery_safety(uint8_t batteryNumber);

#ifdef UNIT_TEST
/**
 * @brief Put the module back into its power-on state. Test-only.
 *
 * The sentinel latch and the out-of-sync second counters persist for the
 * lifetime of the process, which is correct in firmware and untestable in a
 * test binary: without this, every case after the first inherits whatever
 * latch state its predecessors left behind, and no case can exercise the
 * startup grace at all.
 */
void reset_parallel_safety_state();
#endif

#endif
