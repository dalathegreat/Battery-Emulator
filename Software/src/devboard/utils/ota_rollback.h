#ifndef OTA_ROLLBACK_H
#define OTA_ROLLBACK_H

/* App rollback: keeping a firmware that cannot boot from becoming permanent.
 *
 * The bootloader already runs a freshly updated image once with its otadata
 * state set to PENDING_VERIFY, and already restores the previous image if that
 * boot resets before the app confirms itself. None of that needed enabling -
 * CONFIG_APP_ROLLBACK_ENABLE is on in the framework's own config.
 *
 * What was missing is that arduino-esp32 confirms the image inside
 * initArduino(), before setup() runs, so the window closed before this
 * firmware had done anything at all. The .cpp overrides the framework's
 * verifyRollbackLater() hook to defer that, and these two calls are then the
 * app's side of the contract.
 *
 * All of these are no-ops when rollback is not enabled, so they can be called
 * unconditionally.
 *
 * WHEN the confirmation happens is not decided here - see ota_confirm_gate.h.
 * The short version: not at the end of setup(), but 42 s into a running system,
 * so the crashes that only appear once the board is talking to something are
 * still inside the window.
 */

// Report at boot whether the previous update failed to run. Call it early -
// after init_events(), before anything that might itself fail - so the reason
// is visible even if this boot goes badly too.
void report_ota_rollback(void);

// The WRITE side of the gate, for ordinary main-task context: performs the
// confirmation on the one pass where the gate says it is owed, and costs a
// single flag read on every other. Call it from loop().
void ota_confirm_service(void);

// "This image works." Writes otadata, so it belongs on the runtime flash-write
// path like every other one; ota_confirm_service() is what decides when. Public
// because the gate's decision and the write are deliberately separable, not
// because setup() should call it - a boot that confirms itself before it has
// run is the whole defect this exists to fix.
void mark_ota_image_valid(void);

#endif  // OTA_ROLLBACK_H
