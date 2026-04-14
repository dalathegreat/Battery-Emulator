#ifndef CCS_WHITE_BEET_BATTERY_H
#define CCS_WHITE_BEET_BATTERY_H

#include <Arduino.h>
#include <SPI.h>
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "Battery.h"

// Whitebeet framing / protocol. Everything here mirrors FreeV2G/Whitebeet.py
// and FreeV2G/FramingInterface.py so field names, sub-IDs and payload layouts
// match the Python reference one-to-one.

namespace whitebeet {

// Framing markers (FramingAPIDef.py).
constexpr uint8_t START_OF_FRAME = 0xC0;
constexpr uint8_t END_OF_FRAME = 0xC1;

// Module IDs.
constexpr uint8_t MOD_SYSTEM = 0x10;
constexpr uint8_t MOD_V2G = 0x27;
constexpr uint8_t MOD_SLAC = 0x28;
constexpr uint8_t MOD_CP = 0x29;
constexpr uint8_t MOD_ERROR = 0xFF;  // protocol-level error reply

// System sub-IDs.
constexpr uint8_t SUB_SYS_GET_FW_VERSION = 0x41;

// Control-pilot sub-IDs.
constexpr uint8_t SUB_CP_SET_MODE = 0x40;
constexpr uint8_t SUB_CP_GET_MODE = 0x41;
constexpr uint8_t SUB_CP_START = 0x42;
constexpr uint8_t SUB_CP_STOP = 0x43;
constexpr uint8_t SUB_CP_SET_DC = 0x44;
constexpr uint8_t SUB_CP_GET_STATE = 0x48;

// SLAC sub-IDs.
constexpr uint8_t SUB_SLAC_START = 0x42;
constexpr uint8_t SUB_SLAC_STOP = 0x43;
constexpr uint8_t SUB_SLAC_START_MATCH = 0x44;
constexpr uint8_t SUB_SLAC_SUCCESS = 0x80;
constexpr uint8_t SUB_SLAC_FAILED = 0x81;

// V2G sub-IDs.
constexpr uint8_t SUB_V2G_SET_MODE = 0x40;
constexpr uint8_t SUB_V2G_GET_MODE = 0x41;
constexpr uint8_t SUB_V2G_EVSE_SET_CFG = 0x60;
constexpr uint8_t SUB_V2G_EVSE_SET_DC = 0x62;
constexpr uint8_t SUB_V2G_EVSE_UPDATE_DC = 0x63;
constexpr uint8_t SUB_V2G_EVSE_START_LISTEN = 0x6A;
constexpr uint8_t SUB_V2G_EVSE_SET_AUTH = 0x6B;
constexpr uint8_t SUB_V2G_EVSE_SET_SCHEDULES = 0x6C;
constexpr uint8_t SUB_V2G_EVSE_SET_CABLE_CHECK_OK = 0x6D;
constexpr uint8_t SUB_V2G_EVSE_START_CHARGING = 0x6E;
constexpr uint8_t SUB_V2G_EVSE_STOP_CHARGING = 0x6F;
constexpr uint8_t SUB_V2G_EVSE_STOP_LISTEN = 0x70;

// V2G notifications (EVSE receives).
constexpr uint8_t NOTIF_SESSION_STARTED = 0x80;
constexpr uint8_t NOTIF_PAYMENT_SELECTED = 0x81;
constexpr uint8_t NOTIF_REQUEST_AUTHORIZATION = 0x82;
constexpr uint8_t NOTIF_ENERGY_TRANSFER_MODE = 0x83;
constexpr uint8_t NOTIF_REQUEST_SCHEDULES = 0x84;
constexpr uint8_t NOTIF_DC_PARAMS_CHANGED = 0x85;
constexpr uint8_t NOTIF_AC_PARAMS_CHANGED = 0x86;
constexpr uint8_t NOTIF_REQUEST_CABLE_CHECK = 0x87;
constexpr uint8_t NOTIF_PRECHARGE_STARTED = 0x88;
constexpr uint8_t NOTIF_REQUEST_START_CHARGING = 0x89;
constexpr uint8_t NOTIF_REQUEST_STOP_CHARGING = 0x8A;
constexpr uint8_t NOTIF_WELDING_DETECTION_STARTED = 0x8B;
constexpr uint8_t NOTIF_SESSION_STOPPED = 0x8C;
constexpr uint8_t NOTIF_SESSION_ERROR = 0x8E;

// Payload size limits. MAX_FRAME covers header(6) + payload + csum(1) + end(1),
// plus some slack for the SPI size prefix.
constexpr size_t MAX_PAYLOAD = 512;
constexpr size_t MAX_FRAME = MAX_PAYLOAD + 16;

struct Frame {
  uint8_t mod_id;
  uint8_t sub_id;
  uint8_t req_id;
  uint16_t payload_len;
  uint8_t payload[MAX_PAYLOAD];
};

// Tiny FIFO for received frames so notifications sitting in the queue don't
// get dropped while we wait for a command reply with a specific req_id.
constexpr size_t FRAME_RING_SIZE = 8;

class WhitebeetSpi {
 public:
  WhitebeetSpi();
  bool begin(gpio_num_t sck, gpio_num_t mosi, gpio_num_t miso, gpio_num_t cs, gpio_num_t rx_ready,
             gpio_num_t tx_pending);

  // Queue a raw framed packet for the next SPI exchange. Blocks (briefly) if
  // previous TX hasn't been drained yet.
  bool send(const uint8_t* data, size_t len);

  // Drive one SPI exchange cycle: honor rx_ready, exchange size-header, then
  // exchange the data frame. Caller invokes this on every poll tick. Received
  // bytes go to rx_buf_; decoded frames land in the caller-supplied queue via
  // drainReceived().
  void tick();

  // Pop one complete frame's worth of bytes from the RX buffer into out, if any.
  // Returns true and sets *out_len if a frame was popped.
  bool drainReceived(uint8_t* out, size_t* out_len, size_t out_cap);

 private:
  SPIClass spi_{HSPI};
  gpio_num_t cs_ = GPIO_NUM_NC;
  gpio_num_t rx_ready_ = GPIO_NUM_NC;
  gpio_num_t tx_pending_ = GPIO_NUM_NC;

  uint8_t tx_buf_[MAX_FRAME];
  size_t tx_len_ = 0;
  bool tx_pending_flag_ = false;

  // Holds the tail of the previous exchange — a complete framed packet if we
  // parsed a START/END pair. Drained by drainReceived().
  uint8_t rx_frame_[MAX_FRAME];
  size_t rx_frame_len_ = 0;
  bool rx_frame_ready_ = false;

  void transfer(uint8_t* buf, size_t len);
};

class WhitebeetFraming {
 public:
  explicit WhitebeetFraming(WhitebeetSpi* spi) : spi_(spi) {}

  // Build the framed packet (start/end markers, length, checksum) and queue
  // it for transmit. Returns the request ID assigned to the frame.
  uint8_t sendCommand(uint8_t mod_id, uint8_t sub_id, const uint8_t* payload, size_t payload_len);

  // Call every tick. Runs the SPI layer and parses incoming bytes into frames
  // which land in the ring buffer.
  void tick();

  // Pop a frame matching (mod, sub, req) from the ring; wildcard with 0xFF.
  // When mod is MOD_ERROR the matched frame is the protocol error reply, and
  // any sub/req filter is ignored.
  bool poll(Frame* out, uint8_t match_mod, uint8_t match_sub, uint8_t match_req);

  // Pop any frame matching (mod, one_of(subs)). Wildcard mod with 0xFF.
  bool pollAny(Frame* out, uint8_t match_mod, const uint8_t* subs, size_t n_subs);

 private:
  WhitebeetSpi* spi_;
  uint8_t next_req_id_ = 0;

  Frame ring_[FRAME_RING_SIZE];
  uint8_t ring_head_ = 0;
  uint8_t ring_count_ = 0;

  static uint8_t fletcher8(const uint8_t* data, size_t len);
  void pushFrame(const Frame& f);
  bool parseAndStore(const uint8_t* raw, size_t len);
};

class Whitebeet {
 public:
  explicit Whitebeet(WhitebeetFraming* framing) : framing_(framing) {}

  // Run every tick to drive the SPI layer.
  void tick() { framing_->tick(); }

  // Synchronous helpers: send command, wait for reply with matching req_id.
  // Return true on ACK (return-code byte 0x00), false on timeout or NACK. Reply
  // payload (not including the return-code byte) is copied into out_payload if
  // non-null.
  bool systemGetVersion(char* out_version, size_t out_cap);

  bool cpSetMode(uint8_t mode);
  bool cpStart();
  bool cpStop();
  bool cpSetDutyCycle(uint8_t percent);
  bool cpGetState(uint8_t* state);

  bool slacStart(uint8_t mode);
  bool slacStop();
  bool slacStartMatching();
  // Non-blocking check for SLAC completion. Returns:
  //   0 = not yet,  1 = success,  2 = failed.
  uint8_t slacCheckMatched();

  bool v2gSetMode(uint8_t mode);
  bool v2gGetMode(uint8_t* mode);

  struct EvseConfig {
    const char* evse_id_din = "+49*123*456*789";
    const char* evse_id_iso = "DE*A23*E45B*78C";
    uint8_t protocols[2] = {0, 1};
    uint8_t n_protocols = 2;
    uint8_t payment_methods[2] = {0};
    uint8_t n_payment_methods = 1;
    uint8_t energy_transfer_modes[6] = {0, 1, 2, 3, 4, 5};
    uint8_t n_energy_transfer_modes = 6;
    bool certificate_installation_support = false;
    bool certificate_update_support = false;
  };
  bool v2gEvseSetConfiguration(const EvseConfig& cfg);

  struct DcParams {
    uint8_t isolation_level = 0;
    uint32_t min_voltage = 200;
    uint32_t min_current = 0;
    uint32_t max_voltage = 500;
    uint32_t max_current = 125;
    uint32_t max_power = 50000;
    uint32_t peak_current_ripple = 1;
    uint8_t status = 0;
  };
  bool v2gEvseSetDcChargingParameters(const DcParams& p);

  struct DcUpdate {
    uint8_t isolation_level = 0;
    uint32_t present_voltage = 0;
    uint32_t present_current = 0;
    uint32_t max_voltage = 500;
    uint32_t max_current = 125;
    uint32_t max_power = 50000;
    uint8_t status = 0;
  };
  bool v2gEvseUpdateDcChargingParameters(const DcUpdate& p);

  bool v2gEvseStartListen();
  bool v2gEvseStopListen();
  bool v2gEvseSetAuthorizationStatus(bool authorized);
  // Sends an empty-schedule response (code=0, 0 tuples, no energy, no tariffs).
  bool v2gEvseSetSchedulesEmpty();
  bool v2gEvseSetCableCheckFinished(bool ok);
  bool v2gEvseStartCharging();
  bool v2gEvseStopCharging();

  // Non-blocking: pull the next EVSE notification (sub 0x80-0x91). Returns
  // false if none pending.
  bool pollEvseNotification(Frame* out);

  // Parsers for selected notification payloads. Safe on short payloads.
  struct ParsedSessionStarted {
    uint8_t protocol;
    uint8_t session_id[8];
    uint8_t evcc_id_len;
    uint8_t evcc_id[32];
  };
  bool parseSessionStarted(const Frame& f, ParsedSessionStarted* out);

  struct ParsedEnergyMode {
    bool has_departure_time;
    uint32_t departure_time;
    bool has_energy_request;
    uint32_t energy_request;
    uint32_t max_voltage;
    bool has_min_current;
    uint32_t min_current;
    uint32_t max_current;
    bool has_max_power;
    uint32_t max_power;
    uint8_t selected_energy_transfer_mode;
  };
  bool parseEnergyMode(const Frame& f, ParsedEnergyMode* out);

  struct ParsedDcChanged {
    uint32_t max_voltage;
    uint32_t max_current;
    bool has_max_power;
    uint32_t max_power;
    bool ready;
    uint8_t error_code;
    uint8_t soc;
    uint32_t target_voltage;
    uint32_t target_current;
  };
  bool parseDcChanged(const Frame& f, ParsedDcChanged* out);

  struct ParsedSessionStopped {
    uint8_t closure_type;
  };
  bool parseSessionStopped(const Frame& f, ParsedSessionStopped* out);

  struct ParsedSessionError {
    uint8_t code;
  };
  bool parseSessionError(const Frame& f, ParsedSessionError* out);

  struct ParsedStopCharging {
    uint32_t timeout_ms;
    bool renegotiation;
  };
  bool parseStopCharging(const Frame& f, ParsedStopCharging* out);

 private:
  WhitebeetFraming* framing_;

  // Send (mod,sub,payload), wait for matching req reply, return ACK status.
  // If out_frame is non-null, the full reply frame is copied there.
  bool sendAck(uint8_t mod, uint8_t sub, const uint8_t* payload, size_t payload_len, uint32_t timeout_ms,
               Frame* out_frame);

  // Helpers that mirror FreeV2G's _valueToExponential and payloadReaderReadExponential.
  static void encodeExp(uint8_t* out, uint32_t value);
  static uint32_t decodeExp(const uint8_t* in);
};

class CcsBattery : public ::Battery {
 public:
  CcsBattery();

  virtual void setup(void) override;
  virtual void update_values() override;
  virtual const char* interface_name() override { return "SPI (Whitebeet)"; }

  static constexpr const char* Name = "CCS Whitebeet (ISO15118 EVSE)";

 private:
  enum State : uint8_t {
    BOOT,
    INIT_SERVICES,
    WAIT_EV,
    START_SLAC_MATCH,
    WAIT_SLAC_MATCH,
    START_V2G,
    LISTENING,
    CHARGE_ACTIVE,
    STOPPING,
    FAULT,
  };

  WhitebeetSpi spi_;
  WhitebeetFraming framing_;
  Whitebeet wb_;

  State state_ = BOOT;
  unsigned long state_entered_ms_ = 0;
  unsigned long last_tick_ms_ = 0;
  unsigned long last_update_dc_ms_ = 0;
  unsigned long last_cp_poll_ms_ = 0;

  char fw_version_[32] = {0};
  uint8_t ev_reported_soc_ = 0;
  uint32_t ev_target_voltage_ = 0;
  uint32_t ev_target_current_ = 0;
  uint32_t ev_max_voltage_ = 500;
  uint32_t ev_max_current_ = 125;
  uint32_t ev_max_power_ = 50000;
  uint8_t last_session_error_ = 0;

  void enter(State s);
  void handleNotification(const Frame& f);
  Whitebeet::DcUpdate buildDcUpdate() const;
  void forceStop(uint8_t reason);
};

}  // namespace whitebeet

// Exported to battery factory.
using WhitebeetCcsBattery = whitebeet::CcsBattery;

#endif
