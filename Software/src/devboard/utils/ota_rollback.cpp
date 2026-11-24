#include "ota_rollback.h"

#include <esp_app_format.h>
#include <esp_image_format.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>

#include "events.h"
#include "logging.h"
#include "ota_confirm_gate.h"

/* Take the confirmation away from initArduino() and do it ourselves.
 *
 * Rollback is already enabled in this build (CONFIG_APP_ROLLBACK_ENABLE comes
 * from the framework's own config, not from anything here), and the bootloader
 * already runs a freshly updated image as PENDING_VERIFY. What closes the
 * window is arduino-esp32: initArduino() calls
 * esp_ota_mark_app_valid_cancel_rollback() BEFORE setup() begins, so an update
 * is confirmed before a single line of this firmware has run. Every crash the
 * field reports - stored settings meeting new code, a driver that dies on
 * init - happens after that point, with the safety net already folded away.
 *
 * This weak hook is the framework's own way out: returning true defers the
 * decision to the application. It means the responsibility is now entirely
 * ours - if mark_ota_image_valid() is not reached, the next reset undoes the
 * update - which is why every steady state this firmware can be in reaches the
 * gate in ota_confirm_gate.h, and why failing to confirm is logged rather than
 * passed over.
 */
extern "C" bool verifyRollbackLater(void) {
  return true;
}

#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
/* An image's BUILD_VERSION, read from the record that follows its esp_app_desc_t
 * (OTA_BUILD_VERSION_MAGIC in ota_rollback.h). False for an image built before
 * the record existed: the bytes there are then just its first rodata. */
static bool read_build_version(const esp_partition_t* image, char* out, size_t size) {
  const size_t at = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t);
  uint32_t magic = 0;
  if (esp_partition_read(image, at, &magic, sizeof(magic)) != ESP_OK || magic != OTA_BUILD_VERSION_MAGIC) {
    return false;
  }
  if (esp_partition_read(image, at + sizeof(magic), out, size - 1) != ESP_OK) {
    return false;
  }
  out[size - 1] = '\0';  // A longer string is cut short, never left unterminated
  return out[0] != '\0';
}
#endif  // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE

void report_ota_rollback(void) {
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
  /* The image the bootloader gave up on is the OTHER slot, and its otadata
   * state is the record of what happened: ABORTED when it reset before
   * confirming itself, INVALID when it (or a previous boot of it) was rejected
   * outright. Both mean the same thing to a user - the update did not run, and
   * this older firmware is what is running instead.
   *
   * Reported on EVERY boot, not once. The device really is running behind, and
   * the state stays until a later update succeeds, so a line that appeared once
   * and then vanished would be the less honest of the two. */
  const esp_partition_t* other = esp_ota_get_next_update_partition(NULL);
  if (other == NULL) {
    return;
  }
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(other, &state) != ESP_OK) {
    return;
  }
  if (state != ESP_OTA_IMG_ABORTED && state != ESP_OTA_IMG_INVALID) {
    return;
  }

  /* Naming the version it rolled back FROM is the difference between "something
   * went wrong once" and a user knowing which release not to install again. It
   * is read from the failed image's own header, so it is right even when the
   * failure happened several boots ago. Its BUILD_VERSION record is preferred,
   * so both versions read the same way as on the web UI; only an image older
   * than that record falls back to the IDF project version. */
  esp_app_desc_t failed;
  if (esp_ota_get_partition_description(other, &failed) == ESP_OK) {
    char failed_version[64];
    const char* failed_name =
        read_build_version(other, failed_version, sizeof(failed_version)) ? failed_version : failed.version;
    LOG_SET_NEXT_SEVERITY(4);  // warning
    logging.printf("Firmware %s failed to start and was rolled back; running %s instead\n", failed_name,
                   version_number);
  } else {
    LOG_SET_NEXT_SEVERITY(4);  // warning
    logging.println("A firmware update failed to start and was rolled back to this version");
  }
  set_event(EVENT_OTA_ROLLBACK, 0);
#endif  // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
}

void ota_confirm_service(void) {
  /* Everything expensive is behind the flag, so this costs one read on the
     millions of passes where nothing is owed. */
  if (ota_confirm_take_pending()) {
    mark_ota_image_valid();
    /* A watchdog reset is worth a warning while the boot it caused is still
       proving itself. Past this point it has, so acknowledge it: the status
       goes back to green, and the event stays in the list with its count and
       time. Otherwise only clearing every event would turn the page green. */
    clear_event(EVENT_RESET_WDT);
  }
}

void mark_ota_image_valid(void) {
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
  /* Only one boot is ever pending, so this is a no-op on every ordinary start;
   * checking the state first keeps the log line meaningful instead of claiming
   * an update was confirmed on a board that has not been updated. */
  const esp_partition_t* running = esp_ota_get_running_partition();
  esp_ota_img_states_t state;
  if (running == NULL || esp_ota_get_state_partition(running, &state) != ESP_OK) {
    return;
  }
  if (state != ESP_OTA_IMG_PENDING_VERIFY) {
    return;
  }
  if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK) {
    LOG_SET_NEXT_SEVERITY(5);  // notice
    logging.printf("Firmware %s started cleanly and is now confirmed; rollback window closed\n", version_number);
  } else {
    /* Nothing to do about it here, but say so: the image stays pending, so the
     * next reset - however ordinary - will roll it back, and that is otherwise
     * a very confusing thing to have happen. */
    LOG_SET_NEXT_SEVERITY(4);  // warning
    logging.println("Could not confirm this firmware; the next restart will roll it back");
  }
#endif  // CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
}
