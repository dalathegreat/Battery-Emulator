export interface FirmwareInfo {
  hardware: string;
  firmware: string;
}

export interface CanlogStatusResponse {
  can_logging_active: boolean;
  can_usb_logging: boolean;
  can_sd_logging: boolean;
  loop_playback: boolean;
  replay_running: boolean;
  imported_log_chars: number;
  can_replay_interface: number;
}
