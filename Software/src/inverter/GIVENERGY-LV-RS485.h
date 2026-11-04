#ifndef GIVENERGY_LV_RS485_H
#define GIVENERGY_LV_RS485_H

#include "GIVENERGY-LV-FRAMES.h"
#include "Rs485InverterProtocol.h"

// Answers a GivEnergy LV inverter (tested against a G3 5 kW hybrid) as its
// battery: device 1 is the pack, devices 2 to 5 are empty slots.
class GivEnergyLvRs485Inverter : public Rs485InverterProtocol {
 public:
  explicit GivEnergyLvRs485Inverter(HardwareSerial& port = Serial2);
  const char* name() override { return Name; }
  bool setup() override;
  void receive() override;
  void update_values() override;
  static constexpr const char* Name = "GivEnergy LV battery via RS485";

  // Wire values for the current datalayer. Public for the unit tests.
  static givenergy_lv::Snapshot snapshot_from_datalayer();

  // What the calibration guard is doing. Public for the unit tests.
  enum class Guard : uint8_t { Normal, EndingBoth, EndingFull, Verify, Lockout };
  Guard guard() const { return guard_; }
  const givenergy_lv::Snapshot& snapshot() const { return snapshot_; }

 private:
  int baud_rate() override { return 9600; }
  bool is_echo(uint32_t now_ms) const;
  void handle_request(uint32_t now_ms);
  void send_reply(uint32_t now_ms);
  void update_guard(const givenergy_lv::Snapshot& advertised, uint64_t now_ms);
  uint8_t tripped_trigger(const givenergy_lv::Snapshot& advertised, uint64_t now_ms, bool verifying);
  void enter_guard(Guard state, uint64_t now_ms);
  void lock_out(uint8_t trigger, uint64_t now_ms);
  void apply_guard(givenergy_lv::Snapshot& s) const;
  void apply_taper_latch(givenergy_lv::Snapshot& s);

  // Wait before replying. Modbus RTU only needs 3.5 character times (3.6 ms at 9600), but a G3
  // isn't ready to receive that soon after its own request: an emulator on a G3 failed at every
  // delay up to 25 ms (tried in 5 ms steps) and worked reliably from 30 ms. A real GivEnergy
  // battery replies 72 to 96 ms after the end of each request (median 93 ms), so wait inside that range,
  // well clear of the limit. Even a 61-byte HR
  // reply then ends well inside the G3's 240 ms poll cycle.
  static constexpr uint32_t kTurnaroundMs = 75;
  // A frame identical to the write echo just sent, this soon after, is the transceiver hearing itself.
  static constexpr uint32_t kEchoWindowMs = 50;
  // As PYLON-LV-RS485: 12 updates of 1 s (about 12 s) without a request raise the missing event.
  static constexpr uint8_t RS485_HEALTHY = 12;

  HardwareSerial& port_;
  givenergy_lv::Snapshot snapshot_;
  uint8_t rx_[givenergy_lv::kRequestLen] = {};
  size_t rx_len_ = 0;
  uint8_t reply_[givenergy_lv::kMaxReplyLen] = {};
  size_t reply_len_ = 0;
  uint32_t request_ms_ = 0;
  uint8_t sent_[givenergy_lv::kRequestLen] = {};
  bool sent_write_echo_ = false;
  // Deadline past which a frame identical to our last write echo is a genuine retry, not the
  // transceiver hearing its own transmission: the end of that transmission plus kEchoWindowMs.
  uint32_t echo_until_ms_ = 0;
  uint8_t incoming_message_counter_ = RS485_HEALTHY;
  bool inverter_detected_ = false;
  // Set once update_values() has seen a real pack voltage from the battery module. Until then,
  // receive() parses and discards requests without replying, so we never tell the inverter a
  // DataLayer default (370 V) is the pack voltage. Sticky: once true, stays true.
  bool ready_ = false;
  // The calibration guard (see update_guard()). Triggers A to D each remember when their
  // condition started, and whether it still holds.
  static constexpr int kTriggers = 4;
  Guard guard_ = Guard::Normal;
  uint64_t guard_since_ms_ = 0;
  uint64_t trigger_since_ms_[kTriggers] = {};
  bool trigger_active_[kTriggers] = {};
  bool verified_ = false;  // a check has passed; verified_ms_ is when the last one did
  uint64_t verified_ms_ = 0;
  uint8_t lockout_trigger_ = 0;
  // Set once the charge taper reaches the trickle and cleared once the cells come off the top
  // (see apply_taper_latch()): mirrors a real GivEnergy BMS holding HR26 at 3.20 A through the
  // top of charge rather than letting it spring back up as the cells relax. Per instance: a
  // restart clears it, which is fine: if the pack is still full, charging pushes the highest cell
  // back to kTaperEnd_mV within minutes and the latch re-engages.
  bool taper_latched_ = false;
};

#endif
