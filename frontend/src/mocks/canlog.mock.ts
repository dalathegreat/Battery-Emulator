import type { CanlogStatusResponse } from "../types/system.types.js";

export function mockCanlogStatus(): CanlogStatusResponse {
  return {
    can_logging_active: false,
    can_usb_logging: false,
    can_sd_logging: false,
    loop_playback: false,
    replay_running: false,
    imported_log_chars: 0,
    can_replay_interface: 0,
  };
}
