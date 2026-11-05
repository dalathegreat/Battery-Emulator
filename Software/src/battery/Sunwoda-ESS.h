#ifndef SUNWODA_ESS_BATTERY_H
#define SUNWODA_ESS_BATTERY_H

#include "CanBattery.h"
#include "Sunwoda-ESS-HTML.h"

/*
Sunwoda BCMU (Battery Cluster Management Unit) home ESS battery, as used behind the
"FerroAMP" branded native Windows app.

CAN IDs follow the pattern 0x0C50FF00 + <address>, where <address> is the decimal
number embedded in the vendor's variable names (e.g. gMainInfo_80 -> 0x0C50FF00 + 80
= 0x0C50FF50). This mapping was reverse engineered from the vendor's BCMU_APP CAN
variable map together with a real CAN capture (see tools/pcanOut.txt), so treat
anything not explicitly listed here as unconfirmed.

CAN speed is not confirmed from the vendor documentation. 500kbit/s is assumed as a
starting point (most common default); change CAN_Speed below if the battery does not
appear on the bus.
*/
/*
Addressing. A CAN ID is four bytes:
  priority | source node | destination node | object
The BCMU's node address is configurable (param 1004 + the front ADDR switch, see param 1032), so
two packs can share one bus as 0x50 and 0x51. It is therefore a per-instance value and the ID
helpers are member functions - which is why frames are dispatched on the OBJECT byte below rather
than on the whole ID, since switch labels must be compile-time constants.
  0x04 = write, 0x06 = write ack, 0x0C = broadcast, 0x10 = read request, 0x12 = read reply.
*/
static constexpr uint8_t NODE_ADDR_BCMU_DEFAULT = 0x50;
static constexpr uint8_t NODE_ADDR_SELF = 0x64;  // the address the vendor's own tool uses
static constexpr uint8_t NODE_ADDR_BSMU = 0x6E;  // the master the BCMU reports to

// Node addresses the second and third BCMU are expected to answer to in a multi-pack system. Each
// pack needs its own: set that unit's config param 1032 to 0x000F and turn on its front ADDR switch
// (switch 1 gives 0x51) to move it from 0x50, or set the address in its config where the switch does
// not work. The emulator holds three batteries at most.
static constexpr uint8_t SUNWODA_SECOND_BCMU_ADDRESS = 0x51;
static constexpr uint8_t SUNWODA_THIRD_BCMU_ADDRESS = 0x52;

class SunwodaBattery : public CanBattery {
 public:
  // V4.04 runs the host bus at 250 kbit/s (stock), V1.16 at 500. Start at 250 and let
  // autodetect_can_speed() find the other if needed.

  // First or only battery: the default node address, and the primary datalayer.
  SunwodaBattery() : CanBattery(CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = &datalayer.battery;
    allows_contactor_closing = &datalayer.system.status.battery_allows_contactor_closing;
    bcmu_addr = NODE_ADDR_BCMU_DEFAULT;
  }

  // Second battery. `address` must match what that BCMU answers to - set its param 1032 to 0x000F
  // and turn on ADDR switch 1 to move it from 0x50 to 0x51, or the two will collide.
  SunwodaBattery(DATALAYER_BATTERY_TYPE* datalayer_ptr, bool* contactor_closing, CAN_Interface targetCan,
                 uint8_t address)
      : CanBattery(targetCan, CAN_Speed::CAN_SPEED_250KBPS) {
    datalayer_battery = datalayer_ptr;
    allows_contactor_closing = contactor_closing;
    bcmu_addr = address;
  }

  // A dormant instance listens for nothing and sends nothing. Used for the second BCMU while it is not
  // yet known whether there is one: it is created before the bus starts, then woken by
  // finalize_battery_detection() if its node address is heard.
  void set_dormant(bool d) { dormant = d; }

  // True once a frame from this node address (0x50..0x5F) has been heard on any Sunwoda instance's bus.
  static bool node_heard(uint8_t address) {
    return address >= NODE_ADDR_BCMU_DEFAULT && address < NODE_ADDR_BCMU_DEFAULT + 16 &&
           (nodes_heard & (1u << (address - NODE_ADDR_BCMU_DEFAULT)));
  }

  virtual void setup(void);
  virtual void handle_incoming_can_frame(CAN_frame rx_frame);
  virtual void update_values();
  virtual void transmit_can(unsigned long currentMillis);

  static constexpr const char* Name = "Sunwoda ESS Battery (BCMU)";

  BatteryHtmlRenderer& get_status_renderer() { return renderer; }

  // Manual contactor control buttons on the advanced battery page - see ID_CONTACTOR_CONTROL below.
  // Closing is only ever a deliberate button press; opening also happens automatically on any
  // alarm or fault (check_auto_open()).
  bool supports_vendor_command(const char* identifier) override {
    return find_vendor_command(identifier) != nullptr || strcmp(identifier, CMD_OPEN_ALL) == 0;
  }
  void request_vendor_command(const char* identifier) override;
  static constexpr const char* CMD_OPEN_ALL = "swOpenAll";

  // Manually-triggered RTC time sync, time supplied by the browser - see ID_TIME_SYNC below.
  bool supports_time_sync() override { return true; }
  void request_time_sync(uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute,
                         uint16_t ms_in_minute) override;

 private:
  SunwodaExtendedData extended_data;
  SunwodaHtmlRenderer renderer = SunwodaHtmlRenderer(&extended_data);

  DATALAYER_BATTERY_TYPE* datalayer_battery;
  bool* allows_contactor_closing;
  uint8_t bcmu_addr;
  bool dormant = false;
  static inline volatile uint16_t nodes_heard = 0;  // bit n: node 0x50 + n has been heard

  bool is_primary() const { return datalayer_battery == &datalayer.battery; }

  // CAN IDs for this instance's BCMU. See the addressing comment at the top of this file.
  uint32_t request_id(uint8_t object) const {
    return 0x10000000UL | ((uint32_t)NODE_ADDR_SELF << 16) | ((uint32_t)bcmu_addr << 8) | object;
  }
  uint32_t write_id(uint8_t object) const {
    return 0x04000000UL | ((uint32_t)NODE_ADDR_SELF << 16) | ((uint32_t)bcmu_addr << 8) | object;
  }

  /*
  ADDRESSING - confirmed 2026-09-24 from a capture of the vendor's own tool talking to a real BCMU.

  The 29-bit ID is NOT "a base plus the object address". It is four bytes:

      priority | source node | destination node | object

  A BCMU defaults to node 0x50, so every 0x0C50FFxx frame is that BCMU BROADCASTING
  (src 0x50, dst 0xFF) its own telemetry. Those are fine to receive: handle_incoming_can_frame()
  matches the source byte against this instance's bcmu_addr and then dispatches on the OBJ_*
  object number in the low byte.

  They are useless for commanding it. Transmitting 0x0C50FF46 means "I am the BCMU, broadcasting
  object 0x46 to everyone" - which is why the BCMU never once replied to us and never acted on a
  single command. To address the BCMU you send to dst 0x50 with your own address as src:

      10 64 50 00   20 00 04                 read object 0x00, 4 subindexes starting at 0
      12 50 64 00   63 00 B2 07 02 1A 10 33  BCMU replies: 0x07B2=1970, day 26/month 2, 16:51
      12 50 64 00   A1 03 BC 87              ...continued: subindex 3, millisecond 0x87BC
      10 64 50 36   20 00 01                 read object 0x36 (contactor self-test status)
      12 50 64 36   E1 00 00 00              BCMU replies: value 0

  Mux bit5 (0x20) marks an addressed request/response transaction; broadcasts have it clear.
  Bit6 = FIRST frame, bit7 = LAST frame, low nibble = subindex count (see the block further down).
  A read request is `20 <start_subindex> <count>`.

  NODE_ADDR_SELF is deliberately 0x64 - the address the vendor's own tool uses. Impersonating it is
  the one address we KNOW the BCMU answers, so it removes a variable while the ID format is still
  being proven. The vendor tool must be disconnected while we use it, or the two will collide.
  Once this is confirmed working, try moving to a free address (0x65 is a device on the BCMU's
  internal channel 2, 0x50 is the BCMU itself, so something like 0x66 is the natural candidate).
  */
  static const uint8_t MUX_READ_REQUEST = 0x20;  // bit5 only: request, carries no data words
  static const uint8_t MUX_TXN = 0x20;           // addressed request/response (always set)
  static const uint8_t MUX_FIRST = 0x40;
  static const uint8_t MUX_LAST = 0x80;
  static const uint8_t MAX_WORDS_PER_FRAME = 3;  // 1 mux + 1 subindex + 3 u16 = DLC 8

  /*
  WRITE PROTOCOL - captured from the vendor's BMU-configuration tool and verified on hardware:

      04 64 50 <obj>   C1 <subindex> <lo> <hi>    write one subindex (priority 0x04)
      06 50 64 <obj>   C1 <subindex> <lo> <hi>    BCMU ack (priority 0x06)

  The ack echoes the value the object holds AFTER the write, so an ack carrying the old value
  means the write was refused (e.g. the read-only time object).
  */

  /*
  RTC time sync, decoded from BCMU V1.16 firmware and verified on hardware 2026-09-25 (the BCMU's
  clock went from 1970 to the sent time, and the EEPROM snapshot updated a minute later).

  The time object (gOD0_Time) itself is read-only: a priority-0x04 write is acked with the OLD
  value. Time is instead set by a broadcast at priority 0x00 - the dispatcher at 0x7F1001 switches
  on the priority byte, and case 0x00 (0x7F1197) calls the time-sync receiver 0x7F3180. It only
  requires that the frame arrives on the host bus and dst is 0xFF or the BCMU's address; the object
  byte is not checked (0x00 is used for clarity). The payload is raw, with no mux/subindex:
      [year lo][year hi][month][day][hour][minute][ms-in-minute lo][ms-in-minute hi]
  */
  static const uint32_t ID_TIME_SYNC = ((uint32_t)NODE_ADDR_SELF << 16) | (0xFFUL << 8) | 0x00;

  // Object numbers, not CAN IDs: incoming frames are dispatched on the low byte of the ID after
  // the source address has been matched against this instance's bcmu_addr.
  static const uint8_t OBJ_SYSTEM_STATUS = 0x32;   // gStateInfo_50
  static const uint8_t OBJ_SWITCH_STATUS = 0x33;   // gIoSwhInfo_51 (contactors)
  static const uint8_t OBJ_SYSTEM_CONTROL = 70;    // gCtrlInfo_70, written to the BCMU
  static const uint8_t OBJ_ALARM_INFO = 0x34;      // gAlarmInfo_52
  static const uint8_t OBJ_FAULT_INFO = 0x35;      // gFaultInfo_53
  static const uint8_t OBJ_CLUSTER_INFO = 0x36;    // gClusterInfo_54 (contactor self-test)
  static const uint8_t OBJ_CELL_BALANCE_0 = 0x3C;  // gCellBalInfo_60 (cells 1-252)
  static const uint8_t OBJ_MAIN_INFO = 0x50;       // gMainInfo_80
  static const uint8_t OBJ_VOLT_CHARA = 0x51;      // gVoltChara_81
  static const uint8_t OBJ_TEMP_CHARA = 0x52;      // gTempChara_82
  static const uint8_t OBJ_CELL_VOLTAGE_0 = 0x55;  // gCellVolt_85 (cells 1-252, mV)
  static const uint8_t OBJ_CURRENT_LIMIT = 0x5A;   // gCurrLimit_90

  // Individual temperature sensor readings, gTempArray_87. See the comment on
  // SunwodaExtendedData::temperatures_dC in Sunwoda-ESS-HTML.h for how this was identified -
  // not confirmed against vendor documentation, only a real CAN capture.
  static const uint8_t OBJ_TEMP_ARRAY_0 = 0x57;

  // Software/hardware version info. Not part of the gXxxInfo_NN / 0x0C50FF00+address family
  // above - the vendor's variable map (tools/SoftwareAddresses.txt) lists these as "Software
  // version number" and "Hardware version number" (both U16, display format factor 10), but
  // gives no explicit CAN ID. These two IDs were instead identified from a real CAN capture
  // (tools/pcanOut.txt): they are the only frames that broadcast a small, constant value once
  // per second, which fits a version number far better than any other observed field. Treat as
  // a best-effort inference pending confirmation against real hardware.
  // These two are addressed to the BSMU (0x0C <bcmu> 6E <obj>) rather than broadcast to 0xFF.
  static const uint8_t OBJ_SOFTWARE_VERSION = 0x07;
  static const uint8_t OBJ_HARDWARE_VERSION = 0x1D;

  /*
  MANUAL CONTACTOR CONTROL - verified on hardware 2026-09-25 (HWv2 / ES01 running BCMU V1.16).

  Contactor control is object 71 (gSwitchCtrl_71). One U16 per contactor, per the vendor sheet:
      71/0 main (positive)   71/1 precharge   71/2 "intermediate" = the NEGATIVE contactor   71/3 fan
  Codes: 0x00F0 normal engage, 0x000F normal disengage (0xFFF0/0xFF0F "forced" are not used here).

  Preconditions, from the V1.16 write handlers (object 70 at 0x7F94A5, object 71 at 0x7F951A):
    - Config parameter 1008 "Enable communication with BSMU" (EEPROM 0x13F3F0, RAM 0x0F8758) must be
      1, otherwise every object-70/71 write is ACKed and then silently dropped. It cannot be set over
      CAN (the config object refuses writes) - it has to be set once in EEPROM over BDM.
    - Work mode must be 8 "Debugging" (70/1 = 8). The mode switch is only accepted from Normal mode
      and outside Initialization; resending it while already in Debugging is harmless.
    - Not while the BCMU's own contactor sequence is busy (RAM 0x2E08).
  So every close is sent as 70/1 = 8 followed by the 71 write (request_vendor_command()).
  Contactor feedback arrives on ID_SWITCH_STATUS (0x0C50FF33) about once per second.
  */
  static const uint8_t OBJ_CONTACTOR_CONTROL = 71;  // gSwitchCtrl_71
  static const uint16_t CONTACTOR_ENGAGE = 0x00F0;
  static const uint16_t CONTACTOR_DISENGAGE = 0x000F;

  struct VendorCommandDef {
    const char* identifier;  // matches the button/route identifier in advanced_battery_html.cpp
    uint8_t object;          // object number; the CAN ID is built from bcmu_addr at send time
    uint8_t subindex;
    uint16_t value;
  };
  static const VendorCommandDef VENDOR_COMMANDS[];
  static const uint8_t VENDOR_COMMAND_COUNT;
  static const VendorCommandDef CMD_DEBUG_MODE;  // 70/1 = 8, sent before every close
  static const VendorCommandDef CMD_MAIN_OPEN, CMD_PRECHARGE_OPEN, CMD_NEGATIVE_OPEN;
  static const VendorCommandDef* find_vendor_command(const char* identifier);

  // Pending writes, oldest first; transmit_can() sends one per call. Overflow drops the newest.
  static const uint8_t VENDOR_COMMAND_QUEUE_SIZE = 8;
  const VendorCommandDef* vendor_command_queue[VENDOR_COMMAND_QUEUE_SIZE] = {};
  uint8_t vendor_command_queue_count = 0;
  bool enqueue_vendor_command(const VendorCommandDef* cmd);

  /*
  CAN SPEED AUTO-DETECT. BCMU V1.16 runs the host bus at 500 kbit/s, V4.04 at 250 kbit/s (verified
  2026-09-28 on both boards; the frames are otherwise identical). We start at 500k and, whenever
  nothing has been heard from the BCMU (source node 0x50) for CAN_SPEED_PROBE_MS, switch to the other
  speed and keep alternating until it is heard. A node at the wrong speed error-flags every frame,
  so nothing is transmitted on purpose until the BCMU has been heard at the current speed.
  Only safe when the BCMU has an interface to itself (e.g. the MCP2515/2518 add-on). While it shares
  the native bus with the inverter, probing drags that port's speed with it and cuts the inverter off
  as well, so leave this false and set the BCMU's speed to match the bus instead.
  */
  static const bool CAN_SPEED_AUTODETECT = false;
  static const unsigned long CAN_SPEED_PROBE_MS = 3000;
  static const unsigned long BCMU_LINK_TIMEOUT_MS = 2000;
  unsigned long last_bcmu_rx_ms = 0;
  unsigned long last_speed_switch_ms = 0;
  bool can_at_250k = false;
  void autodetect_can_speed(unsigned long currentMillis);
  bool bcmu_link_up(unsigned long currentMillis) const {
    return last_bcmu_rx_ms != 0 && currentMillis - last_bcmu_rx_ms < BCMU_LINK_TIMEOUT_MS;
  }

  // Drops any pending writes (including closes) and queues main, precharge and negative open.
  void open_all_contactors(bool hold_open = false);

  // Opens every contactor while any alarm or fault is active and any contactor reports closed,
  // but only after a contactor was closed with the manual buttons (manual_control). When the BCMU
  // self-starts, its own protection decides; standing alarms (equalization, undervoltage) would
  // otherwise open a normally running pack every second.
  // Repeated at most once per AUTO_OPEN_INTERVAL_MS until the feedback shows all open.
  void check_auto_open();
  bool manual_control = false;
  static const unsigned long AUTO_OPEN_INTERVAL_MS = 1000;
  unsigned long last_auto_open_ms = 0;

  /*
  Multiplex byte format, confirmed 2026-09-24 by decoding the BCMU's own broadcasts against the
  subindex counts in its firmware TX schedule table (flash 0x7FD311):

      data[0] = mux:  bit6 (0x40) = FIRST frame of the transfer
                      bit7 (0x80) = LAST  frame of the transfer
                      low nibble  = number of subindexes carried in this frame
      data[1] = starting subindex
      data[2..] = one little-endian u16 per subindex

  Worked examples straight off the bus - object 0x36 has 1 subindex and is sent as a single
  "C1 00 ..", object 0x34 has 4 and is sent as "43 00 .." (first, 3 items) then "81 03 .."
  (last, 1 item), object 0x5B has 7 and is sent as "43 00" / "03 03" / "81 06".

  So a complete write of one subindex is mux 0xC1 - FIRST and LAST set. We previously sent 0x41,
  which announces the first frame of a multi-frame transfer and promises a continuation that
  never arrived, so the BCMU never committed any of it. That is why no control command ever took
  effect, and why the BCMU never once answered a read request in an 88 s capture.
  */
  // Time broadcast built by request_time_sync(), sent once by transmit_can(). See ID_TIME_SYNC.
  CAN_frame SUNWODA_TIME_SYNC = {.FD = false, .ext_ID = true, .DLC = 8, .ID = ID_TIME_SYNC, .data = {0}};
  bool time_sync_requested = false;

  // Decoded values, applied to the datalayer in update_values()
  float packVoltage = 0.0f;
  float soc = 0.0f;
  float soh = 0.0f;

  uint16_t minCellVoltage = 3200;
  uint16_t maxCellVoltage = 3200;
  uint16_t minCellNumber = 0;
  uint16_t maxCellNumber = 0;

  int16_t minTemperature = 0;
  int16_t packCurrent_dA = 0;  // datalayer convention: positive = charging
  // gCurrLimit_90 subindices: charge current, discharge current (0.1 A), charge power,
  // discharge power (0.1 kW). The BCMU reports all zero whenever it is not running.
  uint16_t bmsLimits[4] = {0, 0, 0, 0};
  // Charge-only limit offered while contactors are held closed manually (Debugging mode).
  static const uint16_t RECOVERY_CHARGE_CURRENT_DA = 50;        // 5.0 A
  static const uint16_t RECOVERY_CHARGE_MAX_CELL_MV = 3500;     // stop offering it at this (LFP)
  static const uint16_t RECOVERY_CHARGE_RESUME_CELL_MV = 3350;  // ...and only resume below this
  bool recovery_charge_blocked = false;

  // Charge taper on the highest cell reading (see apply_charge_taper()).
  void apply_charge_taper(unsigned long now);
  static const uint16_t TAPER_TARGET_MV = 3500;             // 50 mV under BCMU 3550 mV cut, 100 mV under 3600 mV trip
  static const uint16_t TAPER_HARD_MARGIN_MV = 50;          // above target + this: halve the current
  static const uint16_t TAPER_STEP_MARGIN_MV = 20;          // only step up while this far below target
  static const uint16_t TAPER_START_DA = 30;                // 3.0 A at the start of every charge
  static const uint16_t TAPER_STEP_DA = 5;                  // +0.5 A ...
  static const unsigned long TAPER_STEP_MS = 10000;         // ... every 10 s
  static const uint16_t TAPER_CUT_DA = 10;                  // -1.0 A ...
  static const unsigned long TAPER_CUT_INTERVAL_MS = 3000;  // ... at most every 3 s
  static const uint16_t TAPER_MAX_DA = 2000;                // never above 200 A (BCMU limit applies anyway)
  static const int16_t TAPER_CHARGING_DA = 5;               // pack counts as charging above 0.5 A
  uint16_t taper_cap_dA = TAPER_START_DA;
  unsigned long last_taper_step_ms = 0;
  unsigned long last_taper_cut_ms = 0;
  int16_t maxTemperature = 0;

  uint8_t actual_cell_count = 0;

  // Not confirmed from the vendor spec, chosen in line with similar LFP home ESS batteries
  // (e.g. Growatt HV Ark uses 150mV). Tune once real pack behavior is known.
  static const uint16_t MAX_CELL_DEVIATION_MV = 200;

  static uint16_t u16(const uint8_t* d) { return (uint16_t)d[0] | ((uint16_t)d[1] << 8); }
};

#endif
