#ifndef JK_ACTIVE_BALANCER_H
#define JK_ACTIVE_BALANCER_H

#include "../datalayer/datalayer.h"
#include "../datalayer/datalayer_extended.h"
#include "../lib/CoulombSOC/CoulombSOC.h"
#include "../system_settings.h"
#include "CanBattery.h"
#include "JK-ACTIVE-BALANCER-HTML.h"

/* JK Active Balancer: up to 15 balancers polled over CAN at 250 kbps (request ID 0x01..0x0F, reply
   data types 0x01-0x04) plus a CAB500 CAN current sensor (ID 0x3C2). SOC is coulomb counted by
   CoulombSOC. Pack layout, protection limits, SOC window, power ramps and the firmware polarity are
   user settings (user_selected_jk_*) read once in setup(): changes need a reboot.
   Firmware version selects the polarity of BIT4 (cell count) / BIT5 (wire resistance) in the 0x02 status byte:
     1 = all balancers V11.55 : bits INVERTED  (1 = OK, 0 = ALARM)   - verified on bench 2026-10-01
     2 = all balancers V11.56 : bits as documented (1 = ALARM, 0 = OK)
     3 = mixed: per balancer from user_selected_jk_fw_mask (bit 15 = balancer 1 ... bit 1 = balancer 15, 1 = V11.55)
   An inconsistent layout sets config_error: the battery is then not polled, contactors stay open and the
   "More battery info" page shows the reason. */
enum JKConfigError : uint8_t {
  JK_CFG_OK = 0,
  JK_CFG_BRIDGE_AND_LOW_VOLTAGE = 1,
  JK_CFG_CELLS_NOT_MULTIPLE = 2,
  JK_CFG_TOO_MANY_BALANCERS = 3,
  JK_CFG_BAD_CELL_COUNT = 4,
  JK_CFG_BAD_FW_VERSION = 5,
  JK_CFG_BAD_VOLTAGE_ORDER = 6,
  JK_CFG_BAD_SOC_WINDOW = 7,
};

class JKActiveBalancer : public CanBattery {
 public:
  // Use this constructor for the second battery.
  JKActiveBalancer(DATALAYER_BATTERY_TYPE* datalayer_ptr, DATALAYER_INFO_JK_ACTIVE_BALANCER* extended,
                   bool* contactor_allowed_ptr, CAN_Interface targetCan)
      : CanBattery(targetCan, CAN_Speed::CAN_SPEED_250KBPS), renderer(datalayer_ptr, extended) {
    datalayer_battery = datalayer_ptr;
    datalayer_jk = extended;
    allows_contactor_closing = contactor_allowed_ptr;
    *allows_contactor_closing = false;  // Pessimistic: wait until the balancers answer
  }

  // Use the default constructor to create the first or single battery.
  JKActiveBalancer()
      : CanBattery(CAN_Speed::CAN_SPEED_250KBPS), renderer(&datalayer.battery, &datalayer_extended.jk_active_balancer) {
    datalayer_battery = &datalayer.battery;
    datalayer_jk = &datalayer_extended.jk_active_balancer;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    *allows_contactor_closing = false;
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);
  static constexpr const char* Name = "JK Active Balancer (CAB500)";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }
  bool supports_real_BMS_status() { return true; }
  bool soc_plausible();
  // Reset the coulomb-counter cycle counter (SOH returns to 100 %). Applied in update_values().
  bool supports_reset_cycles() { return true; }
  void reset_cycles() { cycles_reset_request = true; }
  // CAB500 CAN speed reconfiguration (UDS over the battery bus), runs as a state machine in transmit_can()
  bool supports_cab500_speed_change() { return true; }
  void request_cab500_speed_change(uint16_t kbps) { cab500_speed_request_kbps = kbps; }

  static constexpr int MAX_BALANCERS = 15;           // CAN address is ID & 0x0F -> 1..15
  static constexpr int MAX_CELLS_PER_BALANCER = 24;  // Hardware maximum

 private:
  static constexpr unsigned long POLL_INTERVAL_MS = 200;
  static constexpr unsigned long POLL_TIMEOUT_MS = 1000;
  // Balancer #1 is polled for this long after boot; with no answer the battery stays Disconnected until
  // reboot. The balancers take about 5 s to boot, so power them before or together with the emulator.
  static constexpr unsigned long BOOT_POLL_WINDOW_MS = 10000;
  static constexpr int MAX_FAULT_COUNT = 10;  // 1 fault per silent second
  static constexpr unsigned long SOC_UPDATE_MS = 50;
  static constexpr float DEGRADATION_PER_CYCLE = 0.0001f;  // 0.01 % capacity per full cycle
  // Reported instead of 0 W in fault / deviation states: on 0 W a Deye starts charging or discharging at once
  static constexpr uint32_t FAULT_POWER_W = 50;

  // CAB500 CAN current sensor: -2 variant, standard frame, DLC 8, every 10 ms
  uint32_t cab500_can_id = 0x3C2;  // frame ID of the fitted CAB500 variant, from the setting (0x3C0..0x3C5)
  static constexpr unsigned long CAB500_BOOT_GRACE_MS = 30000;     // No fault counting for the first 30 s
  static constexpr unsigned long CAB500_ERROR_PERSIST_MS = 10000;  // ERROR_INDICATION must persist 10 s to latch

  // CAB500 UDS diagnostics (speed reconfiguration): request ID 0x68D, reply ID 0x68E, DID 0xF012 = CAN speed
  static constexpr uint32_t CAB500_UDS_TX_ID = 0x68D;
  static constexpr uint32_t CAB500_UDS_RX_ID = 0x68E;
  static constexpr unsigned long CAB500_DETECT_LISTEN_MS = 2000;  // per candidate speed
  static constexpr unsigned long CAB500_UDS_TIMEOUT_MS = 1000;
  static constexpr unsigned long CAB500_WRITE_SETTLE_MS = 1000;  // datasheet: >= 1 s after the write
  static constexpr unsigned long CAB500_RESET_REPLY_MS = 300;    // the reset answer may be lost
  static constexpr unsigned long CAB500_REBOOT_MS = 1500;
  void cab500_speed_machine(unsigned long currentMillis);
  void cab500_speed_enter(uint8_t state, unsigned long now);
  void cab500_speed_finish(uint8_t state, unsigned long now);
  void cab500_send_uds(uint8_t dlc, uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3, uint8_t b4, uint8_t b5);
  static CAN_Speed kbps_to_can_speed(uint16_t kbps);

  static uint32_t discharge_power_limit_W(uint32_t max_power_W, float soc_pct, float bottom_soc_pct,
                                          float ramp_width_pct, uint32_t min_power_W);
  static uint32_t charge_power_limit_W(uint32_t max_power_W, float soc_pct, float max_soc_pct, float ramp_width_pct,
                                       uint32_t min_power_W);
  static uint32_t remaining_Wh_by_soc(float soc_pct, uint32_t capacity_Wh);
  float rated_Ah() const;
  void apply_cycle_degradation(uint32_t cycles);
  void save_cycles(uint32_t cycles);
  const char* cycles_key() const { return battery_index == 1 ? "JK_CYCLES" : "JK_CYCLES2"; }

  JkActiveBalancerHtmlRenderer renderer;
  DATALAYER_BATTERY_TYPE* datalayer_battery;
  DATALAYER_INFO_JK_ACTIVE_BALANCER* datalayer_jk;
  bool* allows_contactor_closing;

  // --- CoulombSOC integration ---
  CoulombSOCConfig soc_cfg;
  CoulombSOC soc;
  bool soc_inited = false;
  bool cycles_reset_request = false;
  bool battery_full = false;   // SOC reached the SOC max setting
  bool battery_empty = false;  // SOC reached the SOC min setting
  uint16_t BMS_cell_max_mV = 0;
  uint16_t BMS_cell_min_mV = 0;
  float BMS_current_A = 0.0f;  // Positive = charging (after the reverse setting)
  float Battery_Ah = 100.0f;
  float Battery_Calc_Ah = 100.0f;  // Battery_Ah after cycle degradation
  uint32_t Calculated_capacity_Wh = 0;

  bool battery_can_alive = false;
  bool cells_ever_complete = false;  // every configured cell has reported a voltage at least once since boot
  unsigned long previousMillis1s = 0;
  unsigned long previousMillis50 = 0;

  int number_of_balancers = 0;
  int max_balancers_active = 8;
  bool reverse_current_active = false;

  // Pack layout from the settings, copied at boot in setup()
  int JK_NUMBER_OF_CELLS = 96;
  int JK_CELLS_BALANCER = 24;
  int JK_BRIDGE_CELLS = 23;  // Usable cells per balancer in bridge mode (1 tap sacrificed for the bridge)
  int JK_SERIES_CELLS = 96;  // Electrical series count: one pack in low-voltage mode, all cells otherwise
  bool JK_BRIDGE_MODE = false;
  bool JK_BALANCER_LOW_VOLTAGE = false;  // Parallel packs, one balancer per pack
  uint8_t JK_FW_VERSION = 2;
  uint16_t JK_FW_MASK = 0xF000;
  uint8_t config_error = JK_CFG_OK;

  // Protection limits from the settings, copied at boot
  float MAX_PACK_VOLTAGE_DV = 96 * 41.9f;  // deciVolt, series cells x mV per cell / 100
  float MIN_PACK_VOLTAGE_DV = 96 * 29.0f;
  uint16_t MAX_CELL_DEVIATION_MV = 150;
  uint16_t MAX_CELL_VOLTAGE_MV = 4220;
  uint16_t MIN_CELL_VOLTAGE_MV = 2700;

  // Power limits / ramps from the settings, copied at boot
  uint32_t max_charge_W_active = 20000;
  uint32_t max_discharge_W_active = 20000;
  float dis_ramp_pct_active = 3.0f;
  float dis_ramp_bottom_pct_active = 7.0f;  // SOC where discharge power reaches zero
  uint16_t dis_cutoff_hyst_dV_active = 30;  // discharge resumes this far above the target discharge voltage
  uint32_t dis_min_power_W_active = 0;
  float chg_ramp_pct_active = 5.0f;
  uint32_t chg_min_power_W_active = 4000;

  int current_balancer_to_poll = 0;
  unsigned long last_poll_millis = 0;
  unsigned long last_request_millis[MAX_BALANCERS] = {0};
  unsigned long last_response_millis[MAX_BALANCERS] = {0};
  int fault_counters[MAX_BALANCERS] = {0};
  // Balancer hardware alarms latch until reboot (a flapping cell tap must not bring the pack back on its own).
  // The CAB500 hardware error latches the same way.
  bool cell_count_alarm_latched[MAX_BALANCERS] = {false};
  bool wire_alarm_latched[MAX_BALANCERS] = {false};
  // Communication faults latch until reboot as well (user decision): a bus that dropped out for 10 s is
  // not trusted again without a power cycle
  bool balancer_comms_latched[MAX_BALANCERS] = {false};
  // Events are raised once per fault (set_event counts every call as an occurrence)
  bool cab500_comm_event_raised = false;
  bool cab500_hw_event_raised = false;
  bool cells_lost_event_raised = false;
  bool cab500_comm_warn_active = false;
  bool balancer_comm_warn_active = false;
  bool deviation_warn_active = false;
  // Width of the linear power derate above the deviation setting; at setting + this the limits are FAULT_POWER_W
  static constexpr uint16_t DEVIATION_DERATE_MV = 50;
  // Readings beyond the sensor's range are start-up artefacts (a CAB500 that is powering up sends zeros,
  // which decode to -2147483 A through the 0x80000000 offset) and must never reach the coulomb counter
  static constexpr int32_t CAB500_MAX_PLAUSIBLE_mA = 600000;  // CAB500 = 500 A sensor, overcurrent flag ~520 A
  // An alarm bit must be present in this many consecutive status frames before it latches, so a
  // garbage status byte from a balancer that is still booting cannot lock the pack until reboot
  static constexpr uint8_t ALARM_DEBOUNCE_FRAMES = 3;
  uint8_t cell_count_alarm_streak[MAX_BALANCERS] = {0};
  uint8_t wire_alarm_streak[MAX_BALANCERS] = {0};

  // --- CAB500 state ---
  float ct_cab500_current_sum_A = 0.0f;  // Accumulator for averaging between SOC ticks
  uint16_t ct_cab500_sample_count = 0;
  unsigned long ct_cab500_boot_millis = 0;
  unsigned long ct_cab500_last_frame_millis = 0;
  unsigned long ct_cab500_error_start_millis = 0;
  bool ct_cab500_error_active = false;    // ERROR_INDICATION currently set
  bool ct_cab500_hardware_error = false;  // Latched until reboot
  uint8_t ct_cab500_error_code = 0;       // Latched raw error byte (0x41/0x42/0x44/0x46)
  int ct_cab500_fault_counter = 0;        // 1 per silent second, increment-only like the balancers
  bool ct_cab500_comms_fault = false;

  // --- CAB500 speed reconfiguration state ---
  volatile uint16_t cab500_speed_request_kbps = 0;  // set by the web task, consumed in transmit_can()
  uint16_t cab500_speed_target_kbps = 0;
  uint16_t cab500_speed_found_kbps = 0;
  uint8_t cab500_speed_state = 0;  // JKCab500SpeedState
  uint8_t cab500_detect_index = 0;
  uint32_t cab500_frames_seen = 0;  // 0x3C2 frames received, counted in every state
  uint32_t cab500_frames_mark = 0;  // cab500_frames_seen when the current state was entered
  unsigned long cab500_state_since = 0;
  uint8_t cab500_uds_reply_sid = 0;    // service id of the last 0x68E reply, 0x7F = negative response
  uint16_t cab500_uds_reply_kbps = 0;  // speed read back by ReadDataByIdentifier 0xF012

  CAN_frame JK_REQUEST_FRAME = {.FD = false, .ext_ID = false, .DLC = 1, .ID = 0x01, .data = {0xFF}};

  struct BalancerData {
    uint16_t cell_voltages_mV[MAX_CELLS_PER_BALANCER];
    int16_t temperature_dC;
    uint16_t total_voltage_10mV;
    uint16_t max_balance_current_mA;
    uint16_t balance_current_mA;  // live balancing current from the 0x02 frame
    uint16_t max_delta_mV;        // highest minus lowest cell as the balancer sees it (0x02 frame)
    uint8_t identified_cells;     // cells the balancer actually detects (0x01 frame)
    uint8_t set_cells;            // cell count configured in the balancer (0x03 frame)
    bool balance_switch_on;
    bool is_charge_balancing;
    bool is_discharge_balancing;
    bool alarm_cell_count_mismatch;
    bool alarm_wire_resistance;
    bool fw_inverted;         // true = V11.55 (BIT4/BIT5: 1 = OK), set in setup() from JK_FW_VERSION
    uint8_t raw_status_byte;  // last 0x02 status byte as received, for diagnostics
  };
  BalancerData balancers[MAX_BALANCERS];
};

#endif
