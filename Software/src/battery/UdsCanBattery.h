#pragma once

#include "CanBattery.h"
#include "freertos/FreeRTOS.h"

#include <atomic>

// Extend this class to add UDS features to a battery integration.
//
// 1. Call `setup_uds(uint32_t uds_address, uint32_t uds_response_address)` in
//    your battery's setup() function to initialize UDS handling.
//     - uds_address (the CAN ID of the ECU to query, e.g. 0x7DF for generic
//       requests)
//     - uds_response_address (the CAN ID that UDS responses must come from, or
//       0 to auto-detect)
//
// 2. Call `set_pid_scan_list(const uint16_t* pids, uint16_t length)` to set the
//    list of PIDs to query, in scan order (e.g. {0xF18A, 0xF120, ...}). The
//    list is walked from start to end and then wraps around. Call it again at
//    any time to switch to a different list. PID scanning runs whenever no
//    sequence (below) is active. The scan uses two-byte UDS DIDs with
//    ReadDataByIdentifier (0x22) by default; call
//    `set_pid_scan_mode(PidScanMode::OneByteLocalId)` to query ECUs that only
//    speak KWP2000-style one-byte local identifiers (0x21) instead. The list
//    and handle_pid() interface stay the same in both modes.
//
// 3. Call `transmit_uds_can(unsigned long currentMillis)` in your battery's
//    transmit_can() function to send UDS requests periodically.
//
// 4. Call `handle_incoming_uds_can_frame(CAN_frame rx_frame)` in your battery's
//    `handle_incoming_can_frame(CAN_frame rx_frame)` function to process
//    incoming UDS responses. If it returns true, the frame was handled by the
//    UDS layer so you can ignore it.
//
// 5. Override `handle_pid(uint16_t pid, uint32_t value, const uint8_t* data,
//    uint16_t length)` to be passed successful PID query responses. The
//    arguments are:
//     - pid: the PID that the response is for
//     - value: the value of the PID (big-endian, truncated to four bytes if the
//       response is longer)
//     - data: the raw data bytes of the value
//     - length: the length of the value in bytes Only successful responses are
//       passed to handle_pid - failed requests (timeouts, negative responses)
//       are retried and then skipped without notifying the handler. Return 0 to
//       continue with the next PID in the scan list, or return a PID to request
//       it once out-of-sequence first (e.g. to sample a fast-changing PID more
//       often), after which the scan list resumes where it left off.
//
// SEQUENCES
//
// Multi-step UDS operations (resets, writes, routine control, security access,
// ...) are built from sequences of steps.
//   - Define an enum with a state for each step in your sequences.
//   - Start a sequence with `start_sequence(STARTING_STATE)`
//   - Override `on_uds_sequence_step(state, sid, data, len)` to handle any
//     response and send the next step.
//   - Optionally override `on_uds_sequence_timeout(state)` to handle a step
//     timing out (the superclass retries automatically).

class UdsCanBattery;

// Default HTML renderer for UDS batteries. Avoids needing a custom renderer
// class for simple UDS batteries that just show some basic information above
// the DTC section.

class UdsBatteryHtmlRenderer : public BatteryHtmlRenderer {
 public:
  explicit UdsBatteryHtmlRenderer(UdsCanBattery& battery) : battery(battery) {}

  String get_status_html() override;

  // Reads only per-instance data (the battery's own dtc pointer and info
  // HTML), so the advanced page may also show it for battery 2/3.
  bool renders_own_battery_data() override { return true; }

 private:
  UdsCanBattery& battery;
};

class UdsCanBattery : public CanBattery, public IsoTp {
 public:
  UdsCanBattery(CAN_Speed speed = CAN_Speed::CAN_SPEED_500KBPS) : CanBattery(speed), uds_renderer(*this) {}
  UdsCanBattery(CAN_Interface interface, CAN_Speed speed = CAN_Speed::CAN_SPEED_500KBPS)
      : CanBattery(interface, speed), uds_renderer(*this) {}

  // Sequence step states for our own functions. Internal states here should
  // have the UDS_STATE_INTERNAL bit set, subclass states should not.
  enum UdsState : uint16_t {
    UDS_STATE_IDLE = 0,
    UDS_STATE_INTERNAL = 0x8000,
    // Superclass-internal sequences.
    UDS_STATE_READ_DTC_START = UDS_STATE_INTERNAL | 0x01,
    UDS_STATE_READ_DTC,  // 0x19 0x02
    UDS_STATE_CLEAR_DTC_START = UDS_STATE_INTERNAL | 0x03,
    UDS_STATE_CLEAR_DTC,  // 0x14 FF FF FF
    UDS_STATE_SOFT_RESET_START = UDS_STATE_INTERNAL | 0x05,
    UDS_STATE_SOFT_RESET,  // 0x11 0x03
  };

  // Priority levels for UDS traffic, used by pause_uds() and
  // send_sequence_message(). Higher priority can bypass pauses, a pause at
  // level N blocks new sends with priority <= N.
  enum class UdsPriority : uint8_t {
    Sequence = 0,  // Sequence steps (the send_sequence_message() default)
    PidScan = 1,   // Regular PID scanning
    Custom = 2,    // User-initiated custom diagnostic messages
  };

  // Wire format used by the PID scan. Standard UDS ECUs are queried with
  // two-byte DIDs via ReadDataByIdentifier (0x22). Some ECUs (e.g. Renault
  // Zoe Gen1 / Kangoo) instead speak KWP2000-style one-byte local identifiers
  // via ReadDataByLocalIdentifier (0x21). Both modes share the same scan list,
  // handle_pid() interface and scan cycling - only the wire format differs.
  enum class PidScanMode : uint8_t {
    TwoByteDID = 0,      // UDS 0x22 ReadDataByIdentifier, two-byte DIDs (default)
    OneByteLocalId = 1,  // KWP2000 0x21 ReadDataByLocalIdentifier, one-byte local IDs
  };

  inline bool uds_is_busy() const {
    return seq_state != UDS_STATE_IDLE || pending_pid != 0 || pending_seq_state != UDS_STATE_IDLE ||
           seq_pause_ticks > 0 || isotp_is_busy();
  }

  virtual bool supports_read_DTC();
  virtual bool supports_reset_DTC();
  virtual void read_DTC();
  virtual void reset_DTC();

  // Advanced battery page (see UdsBatteryHtmlRenderer). The built-in renderer
  // shows the DTC section; override these hooks to customize it without
  // writing a whole renderer class:
  //  - get_uds_info_html(): HTML displayed above the DTC section (e.g. UDS
  //    address, VIN, raw PID payloads).
  //  - get_dtc_json_filename(): DTC description JSON file under
  //    BatteryHtmlRenderer::GITHUB_RAW_BASE_URL, or "" to only offer the
  //    local file picker.
  //  - get_dtc_standard_code_string(): false for raw 6-digit hex codes, true
  //    for SAE-format codes (e.g. P0C9500).
  virtual String get_uds_info_html() { return String(); }
  virtual const char* get_dtc_json_filename() { return ""; }
  virtual bool get_dtc_standard_code_string() { return true; }

  // The built-in generic renderer. Override to provide a fully custom page.
  virtual BatteryHtmlRenderer& get_status_renderer() override { return uds_renderer; }

  // Temporarily block new UDS sends for the given number of 100ms ticks. Blocks
  // transmits at or below the specified priority level, higher priority ones
  // can still send. In-flight transactions are unaffected — they may still
  // complete and time out; retries whose priority is blocked are given up
  // instead of re-sent. Call with 0 ticks to clear.
  void pause_uds(uint16_t ticks_100ms, UdsPriority block_upto = UdsPriority::PidScan);

  DATALAYER_BATTERY_DTC_TYPE* dtc = nullptr;

 protected:
  // Initializes the UDS layer. Must be called by subclasses in their setup() function. Has optional isFD overload
  void setup_uds(uint32_t uds_address, uint32_t uds_response_address, bool isFD = false);
  // Set (or change) the list of PIDs to scan, in order. The list is cycled
  // repeatedly - the scan restarts from the beginning of the new list.
  void set_pid_scan_list(const uint16_t* pid_list, uint16_t length);
  // Set (or change) the wire format used by the PID scan (default:
  // TwoByteDID).
  void set_pid_scan_mode(PidScanMode mode);

  // Queue a one-shot UDS soft reset (0x11 0x03). The request is not retried.
  bool request_uds_soft_reset();

  // Must be called by subclasses inside their `transmit_can` method.
  void transmit_uds_can(unsigned long currentMillis);
  // Must be called by subclasses inside their `handle_incoming_can_frame` method.
  bool handle_incoming_uds_can_frame(CAN_frame rx_frame);

  // Queues a new UDS sequence. Doesn't send anything yet, but during the next
  // UDS tick, `on_uds_sequence_step` will be called with the given state,
  // allowing the subclass to send the first step. Returns false if a sequence
  // is already queued.
  bool start_sequence(uint16_t state);

  // Send a single step of a UDS sequence. The state is handed back to
  // on_uds_sequence_step() along with the response. The step is retried
  // automatically on timeout, up to `max_retries`. Increasing `priority` allows
  // the send to override pauses (depending on level). `timeout_ticks` must be
  // non-zero. Returns false if unable to send.
  bool send_sequence_message(uint16_t state, SID sid, const uint8_t* data, uint16_t len, uint16_t timeout_ticks = 10,
                             uint8_t max_retries = 2, UdsPriority priority = UdsPriority::Sequence);

  // Override this to receive responses to your UDS sequence steps. The state
  // that was passed with the request is handed back here, along with the
  // response SID and payload. The subclass can call send_sequence_message()
  // again to send another message, or send nothing to end it.
  virtual void on_uds_sequence_step(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len) {}

  // Override this to be notified when a UDS sequence step has timed out and
  // exhausted its retry budget. The state that was passed with the request is
  // handed back here.
  virtual void on_uds_sequence_timeout(uint16_t state) {}

  // Override this to be passed successful PID query responses. A return value
  // of 0 causes the PID list to be scanned in order. Return a PID to read it
  // next, out-of-sequence (e.g. to sample a specific PID more often), after
  // which the scan list resumes where it left off.
  virtual uint16_t handle_pid(uint16_t pid, uint32_t value, const uint8_t* data, uint16_t length) { return 0; }

  // Called by the ISO-TP layer to emit CAN frames or notify of complete UDS responses.
  virtual void on_isotp_can_tx(uint32_t can_id, const uint8_t* can_data, uint8_t can_dlc) override;
  virtual void on_isotp_rx_complete(const uint8_t* data, int len, isotp_tatype tatype) override;

  // The address we'll send UDS requests to. Any ID over 0x7FF is counted as extended CAN and will be sent with the extended flag set.
  uint32_t uds_address = 0x7DF;
  // The address we require UDS responses to come from, or 0 to accept from any
  // address in the valid range.
  uint32_t uds_response_address = 0;
  // The address we are currently receiving a UDS response from.
  uint32_t uds_current_response_address = 0;

 private:
  // True if the current pause blocks new sends of the given priority.
  inline bool uds_paused_for(UdsPriority priority) const { return seq_pause_ticks > 0 && priority <= seq_pause_level; }

  bool transaction_tick();

  // Advance to the next entry in the PID scan list, wrapping around at the end.
  inline void advance_pid_scan() {
    if (++pid_scan_index >= pid_list_len) {
      pid_scan_index = 0;
    }
  }

  void uds_send(SID service_id, const uint8_t* data, uint16_t length, uint32_t timeout = 0);
  bool transmit_uds_pid_scan();
  bool on_uds_pid_scan_response(uint8_t sid, const uint8_t* data, uint16_t len);
  void on_uds_pid_scan_timeout();
  void on_uds_receive(const uint8_t* data, uint16_t len);
  void handle_dtc_response(const uint8_t* data, uint16_t len);
  // Dispatches responses for sequence states
  void handle_sequence(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len);
  // Dispatches responses for superclass-internal sequence states.
  void handle_internal_sequence(uint16_t state, uint8_t sid, const uint8_t* data, uint16_t len);

  // The request currently in flight as part of a sequence.
  struct {
    uint8_t sid = 0;
    uint8_t data[16] = {};
    uint16_t len = 0;
    uint32_t timeout_ticks = 0;
    uint8_t retries = 0;
    uint8_t max_retries = 0;
    UdsPriority priority = UdsPriority::Sequence;
  } seq_msg;

  uint16_t seq_state = UDS_STATE_IDLE;
  uint16_t pending_seq_state = UDS_STATE_IDLE;
  uint16_t pid_scan_index = 0;
  uint16_t pid_list_len = 0;
  uint16_t next_pid = 0;
  uint16_t pending_pid = 0;
  uint16_t seq_pause_ticks = 0;
  UdsPriority seq_pause_level = UdsPriority::PidScan;
  uint32_t previousUdsMillis100 = 0;
  uint8_t pid_scan_sid = static_cast<uint8_t>(SID::ReadDataByIdentifier);
  uint8_t pid_scan_id_bytes = 2;
  const uint16_t* pid_list = nullptr;
  uint8_t pid_retries = 0;
  int32_t uds_transaction_timeout = 0;
  bool send_messages_asFD = false;

  UdsBatteryHtmlRenderer uds_renderer;
};
