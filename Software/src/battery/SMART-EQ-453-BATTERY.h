#ifndef SMART_EQ_453_BATTERY_H
#define SMART_EQ_453_BATTERY_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "CanBattery.h"
#include "SMART-EQ-453-HTML.h"

// ── Startup state machine ─────────────────────────────────────────────────────
enum class Smart453State : uint8_t {
  POWER_OFF = 0,
  LV_POWERED,
  WAKE_REQUESTED,
  WAIT_BMS_RESPONSE,
  BMS_RESPONDING,
  STATE_C3,
  STATE_C4,
  STATE_C5,
  STATE_C6,
  WAIT_RELAY_PERMIT,
  PRECHARGING,
  CONTACTORS_CLOSED,
  STATE_C7_READY,
  SHUTDOWN_REQUESTED,
  SHUTTING_DOWN,
  FAULT,
};

// ── Diagnostic scheduler ──────────────────────────────────────────────────────
enum class Smart453PollState : uint8_t {
  IDLE = 0,
  WAITING_RESPONSE,
};

// ── Safety: contactor control is disabled until explicitly enabled ─────────────
// Set SMART453_ALLOW_CONTACTOR_CONTROL to 1 only after:
//   - simulations pass
//   - fault-injection tests pass
//   - physical connector pinout is confirmed
//   - real-battery validation is performed
#ifndef SMART453_ALLOW_CONTACTOR_CONTROL
#define SMART453_ALLOW_CONTACTOR_CONTROL 0
#endif

class SmartEq453Battery : public CanBattery {
 public:
  SmartEq453Battery() {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "Smart EQ / Renault 453 17.6kWh";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

 private:
  SmartEq453HtmlRenderer renderer;

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  bool* allows_contactor_closing;

  // ── Pack limits ──────────────────────────────────────────────────────────
  static const int MAX_PACK_VOLTAGE_DV = 3700;   // 370.0 V
  static const int MIN_PACK_VOLTAGE_DV = 2700;   // 270.0 V
  static const int MAX_CELL_VOLTAGE_MV = 4200;
  static const int MIN_CELL_VOLTAGE_MV = 2700;
  static const int MAX_CELL_DEVIATION_MV = 200;
  static const int TOTAL_CAPACITY_WH   = 17600;
  static const int NUM_CELLS           = 96;
  static const int NUM_TEMPS           = 27;

  // ── State machine ────────────────────────────────────────────────────────
  Smart453State state = Smart453State::LV_POWERED;
  uint8_t vehicle_state_byte = 0x00;   // 0x350 byte 0

  // ── ISO-TP receive buffer ────────────────────────────────────────────────
  // Single-slot: one outstanding request at a time.
  uint8_t  isotp_buf[256];
  uint16_t isotp_rxlen    = 0;
  uint16_t isotp_exp_len  = 0;
  uint8_t  isotp_exp_sn   = 0;       // expected consecutive-frame sequence number
  uint8_t  isotp_pid      = 0xFF;    // PID we are currently receiving
  bool     isotp_busy     = false;
  unsigned long isotp_request_ts = 0; // millis() when last request was sent

  static const uint16_t ISOTP_TIMEOUT_MS = 250;

  // ── PID scheduler ────────────────────────────────────────────────────────
  uint8_t poll_phase = 0;  // 0 = session-start PIDs, 1 = steady-state
  uint8_t startup_pid_index = 0;

  unsigned long last_poll_fast_ms   = 0;
  unsigned long last_poll_medium_ms = 0;
  unsigned long last_poll_slow_ms   = 0;
  unsigned long last_poll_vslow_ms  = 0;

  static const uint16_t POLL_FAST_MS   =  500;   // 0x07, 0x01
  static const uint16_t POLL_MEDIUM_MS = 1000;   // 0x08, 0x29
  static const uint16_t POLL_SLOW_MS   = 5000;   // 0x41, 0x42, 0x04
  static const uint16_t POLL_VSLOW_MS  = 30000;  // 0x10, 0x11, 0x16, 0x25, 0x61, 0x0B

  // Ordered list of startup-only PIDs
  static const uint8_t STARTUP_PIDS[];
  static const uint8_t NUM_STARTUP_PIDS;

  // ── Raw decoded values ───────────────────────────────────────────────────
  uint16_t cell_voltage_raw[96] = {};    // raw / 1024.0 V
  uint16_t cell_resist_raw[96]  = {};    // raw (unit TBD)
  uint16_t temperature_raw[31]  = {};    // raw / 64.0 °C  (up to 31 channels)
  uint8_t  balancing_raw[96]    = {};
  uint16_t balancing_state_raw  = 0;

  uint16_t dc_link_voltage_raw   = 0;   // PID 0x07 P[7..8], / 64.0 V
  uint16_t pack_sum_voltage_raw  = 0;   // PID 0x07 P[9..10], / 64.0 V
  uint16_t pack_terminal_v_raw   = 0;   // PID 0x07 P[11..12], / 64.0 V
  int16_t  pack_current_raw      = 0;   // PID 0x07 P[13..14], signed / 32.0 A
  uint8_t  contactor_state       = 0;   // PID 0x07 P[15]: 0=open,1=pre,2=closed
  uint8_t  hv_plug_interlock     = 0;   // PID 0x07 P[16]
  uint8_t  service_disconnect    = 0;   // PID 0x07 P[17]
  uint16_t fusi_raw              = 0;   // PID 0x07 P[18..19]
  uint8_t  safety_mode           = 0;   // PID 0x07 P[22]
  uint8_t  relay_status          = 0;   // PID 0x07 P[23] bits [5:4]
  uint8_t  relay_permit          = 0;   // PID 0x07 P[23] bits [7:6]
  uint8_t  vehicle_mode_req      = 0;   // PID 0x07 P[24]
  uint8_t  bms_12v_supply_raw    = 0;   // PID 0x07 P[25], / 8.0 V
  uint16_t cell_min_raw          = 0;   // PID 0x07 P[3..4], / 1024.0 V
  uint16_t cell_max_raw          = 0;   // PID 0x07 P[5..6], / 1024.0 V

  int32_t  charge_limit_raw      = 0;   // PID 0x01 P[3..6], signed BE32 / 1024.0 A
  int32_t  discharge_limit_raw   = 0;   // PID 0x01 P[7..10]

  uint16_t soc_min_raw           = 0;   // PID 0x08 P[5..6]
  uint16_t soc_max_raw           = 0;   // PID 0x08 P[7..8]
  uint16_t capacity_init_raw     = 0;   // PID 0x08 P[13..14]
  uint16_t capacity_current_raw  = 0;   // PID 0x08 P[15..16]

  int16_t  isolation_kohm        = 0;   // PID 0x29 P[2..3]
  uint8_t  isolation_flags       = 0;   // PID 0x29 P[4]

  uint16_t capacity_measured_raw = 0;   // PID 0x0B P[2..3]

  // PID 0x61: retained as raw for version comparison
  uint8_t  pid61_raw[32]         = {};
  uint8_t  pid61_len             = 0;

  // PID 0x02: HV contactor cycle counter — safety critical
  uint32_t contactor_counter_initial = 0xFFFFFFFF;  // 0xFF = not yet read
  uint32_t contactor_counter_current = 0xFFFFFFFF;
  bool     contactor_counter_valid   = false;

  // Identification (stored as raw bytes)
  uint8_t  bms_id_raw[32]      = {};
  uint8_t  bms_id_len          = 0;
  uint8_t  bms_pn_raw[32]      = {};
  uint8_t  bms_pn_len          = 0;
  uint8_t  batt_serial_raw[32] = {};
  uint8_t  batt_serial_len     = 0;
  uint8_t  prod_date_raw[8]    = {};
  uint8_t  prod_date_len       = 0;

  // ── Frame 0x186 / 0x1F6 observations (passive, never transmitted) ────────
  bool     f186_ready          = false;    // 0x186 byte 0 == 0x19
  uint8_t  f1f6_byte1          = 0;        // 0x1F6 byte 1: 0x00=idle,0x30=precharge,0x20=ready

  // ── Broadcast frame values (confirmed from trace, scale candidates) ───────
  // These arrive at 10ms and are available immediately on power-up,
  // before any UDS polling completes. UDS values override them once fresh.
  //
  // 0x0C6 bytes 0-1: pack voltage, BE16 * 0.01 V  (327.37V confirmed from trace)
  // 0x0C6 bytes 4-5: DC link voltage, BE16 * 0.01 V (327.59V confirmed from trace)
  // 0x12E byte 0:    SOC candidate, raw / 2 = %  (94-95% confirmed from trace)
  // 0x090 byte 0:    DC link precharge %, 0xFF = capacitors discharged/not valid
  //                  (candidate — 16-channel multiplexed scan, all rise 9→99% during precharge)
  // 0x3B7 byte 0:    Temperature candidate, raw / 2 = °C
  //                  (74-75 observed → 37.0-37.5°C, slowly varying; confirm during validation)
  uint16_t bcast_voltage_raw    = 0;   // 0x0C6 bytes 0-1, /100 → dV
  uint16_t bcast_dc_link_raw    = 0;   // 0x0C6 bytes 4-5, /100 → dV
  uint8_t  bcast_soc_raw        = 0;   // 0x12E byte 0, *50 → pptt  (raw/2*100)
  bool     bcast_voltage_valid  = false;
  bool     bcast_soc_valid      = false;
  unsigned long last_bcast_ms   = 0;
  static const uint16_t BCAST_STALE_MS = 500;  // treat broadcast as stale after 500ms

  uint8_t  bcast_precharge_pct   = 0xFF;  // 0x090 byte 0: 0-100 / 0xFF=no data
  bool     bcast_precharge_valid = false;

  uint8_t  bcast_temp_raw        = 0xFF;  // 0x3B7 byte 0: raw; 0xFF=not received
  bool     bcast_temp_valid      = false;

  // ── Keepalive transmit ───────────────────────────────────────────────────
  unsigned long last_keepalive_ms = 0;
  static const uint16_t KEEPALIVE_MS = 100;

  // ── Timing milestones ────────────────────────────────────────────────────
  unsigned long last_pid07_ms = 0;   // freshness check

  // ── CAN frames ──────────────────────────────────────────────────────────
  CAN_frame frame_634_wake = {
      .FD = false, .ext_ID = false, .DLC = 4, .ID = 0x634,
      .data = {.u8 = {0x40, 0x00, 0x00, 0x00}}};

  CAN_frame frame_634_awake = {
      .FD = false, .ext_ID = false, .DLC = 4, .ID = 0x634,
      .data = {.u8 = {0x80, 0x00, 0x32, 0x00}}};

  CAN_frame frame_diag_request = {
      .FD = false, .ext_ID = false, .DLC = 8, .ID = 0x79B,
      .data = {.u8 = {0x02, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};

  CAN_frame frame_diag_request_22 = {
      .FD = false, .ext_ID = false, .DLC = 8, .ID = 0x79B,
      .data = {.u8 = {0x03, 0x22, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00}}};

  CAN_frame frame_flow_control = {
      .FD = false, .ext_ID = false, .DLC = 8, .ID = 0x79B,
      .data = {.u8 = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};

  // ── Private methods ──────────────────────────────────────────────────────
  void send_diag_request(uint8_t service, uint8_t pid);
  void send_diag_request_22(uint8_t pid_high, uint8_t pid_low);
  void process_isotp_complete(uint8_t pid, const uint8_t* data, uint16_t len);
  void decode_pid07(const uint8_t* p, uint16_t len);
  void decode_pid01(const uint8_t* p, uint16_t len);
  void decode_pid08(const uint8_t* p, uint16_t len);
  void decode_pid04(const uint8_t* p, uint16_t len);
  void decode_pid29(const uint8_t* p, uint16_t len);
  void decode_pid41_42(const uint8_t* p, uint16_t len, uint8_t block);
  void decode_pid10_11(const uint8_t* p, uint16_t len, uint8_t block);
  void decode_pid16(const uint8_t* p, uint16_t len);
  void decode_pid25(const uint8_t* p, uint16_t len);
  void decode_pid61(const uint8_t* p, uint16_t len);
  void decode_pid0B(const uint8_t* p, uint16_t len);
  void decode_pid02(const uint8_t* p, uint16_t len);
  void decode_pid80(const uint8_t* p, uint16_t len);
  void decode_pidEF(const uint8_t* p, uint16_t len);
  void decode_pidA0(const uint8_t* p, uint16_t len);
  void decode_pid_22_0304(const uint8_t* p, uint16_t len);
  void decode_bcast_0x0C6(const uint8_t* d, uint8_t dlc);
  void decode_bcast_0x12E(const uint8_t* d, uint8_t dlc);
  void decode_bcast_0x090(const uint8_t* d, uint8_t dlc);
  void decode_bcast_0x3B7(const uint8_t* d, uint8_t dlc);

  void schedule_next_poll(unsigned long now);
  bool poll_is_safe_to_send() const;
  void check_isotp_timeout(unsigned long now);
  void update_state_machine(unsigned long now);
  void check_contactor_counter_safety();

  static uint16_t be16(const uint8_t* b) { return ((uint16_t)b[0] << 8) | b[1]; }
  static int16_t  sbe16(const uint8_t* b) { return (int16_t)(((uint16_t)b[0] << 8) | b[1]); }
  static uint32_t be32(const uint8_t* b) {
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | b[3];
  }
  static int32_t sbe32(const uint8_t* b) { return (int32_t)be32(b); }
};

#endif
