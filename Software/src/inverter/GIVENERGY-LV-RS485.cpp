#include "GIVENERGY-LV-RS485.h"

#include <algorithm>
#include <cstring>

#include "../communication/rs485/comm_rs485.h"
#include "../datalayer/datalayer.h"
#include "../devboard/hal/hal.h"
#include "../devboard/utils/events.h"
#include "../devboard/utils/logging.h"
#include "INVERTERS.h"

namespace {

// HR17/HR18 hash the battery's clock; any value ticking once a second will do.
constexpr uint32_t kClockSeed = 0x389D0000;

// How long len bytes take on the wire at 9600 8N1: 10 bits per byte, rounded up.
uint32_t tx_ms(size_t len) {
  return static_cast<uint32_t>((len * 10 * 1000 + 9599) / 9600);
}

constexpr char kSerial[] = "EM2026G001";
constexpr uint16_t kFirmware = 3022;     // the current BMS release; 3011 or higher selects HR26/HR27 on a G3
constexpr uint16_t kLimitCap_cA = 9000;  // 90 A: the new circuit has a 100 A DC MCB
// The inverter accepts cell voltages strictly between 2200 and 3700 mV and silently drops
// anything outside that (docs/05), so clamp one step inside those bounds, not onto them.
constexpr uint16_t kCellMin_mV = 2201;
constexpr uint16_t kCellMax_mV = 3699;
constexpr int kCells = 16;

// The G3 never takes a charge voltage from the battery: it charges at whatever current HR26
// allows until HR26 itself comes down. The real GivEnergy BMS cuts HR26 to 3.20 A near the top
// of charge; we mirror that so an LFP pack reaches its voltage knee at low current instead of
// letting the inverter's own ~58 V over-voltage trip do the job (it reads ~1.3 V above the pack
// at 60 A, so a hard cutoff at full current would trip it well before the pack is actually full).
constexpr uint16_t kTaperStart_mV = 3450;  // taper begins here: no reduction below this
constexpr uint16_t kTaperEnd_mV = 3550;    // taper reaches the trickle rate here
constexpr uint16_t kTrickle_cA = 300;      // 3 A trickle from kTaperEnd_mV to kCellStop_mV: lets
                                           // the BMS balance the top cells without stalling
constexpr uint16_t kCellStop_mV = 3600;    // charging stops here: belt and braces, since
                                           // Battery-Emulator's own cell over-voltage check
                                           // isn't effective for this battery module
// Below this, the charge taper's latch releases (see apply_taper_latch()): mirrors a real GivEnergy
// BMS holding 3.20 A through full; releases once the pack has clearly come off the top.
constexpr uint16_t kTrickleRelease_mV = 3400;
constexpr uint16_t kCellUnderStop_mV = 2900;  // LFP low-cell stop: backup to the battery's own
                                              // discharge limit already going to 0

// HR20 alarm bits that the inverter reads as "full" (bit 2) and "empty" (bit 3).
constexpr uint16_t kAlarmFull = 0x0004;
constexpr uint16_t kAlarmEmpty = 0x0008;

// Calibration guard. During a battery calibration the inverter's DSP holds both current limits at
// 8 A or more, whatever HR26 and HR27 say, and discharges past any reserve until the battery
// reports HR20 bit 3 ("empty"), then charges until it reports bit 2 ("full"). Nothing on the
// inverter times it out, and it survives a reboot. The requests don't change during one, so the
// guard infers it from the current against what this module advertised on the previous update:
//   A: HR26 below 8 A, and the charge more than 2 A over max(HR26, 1 A).
//   B: HR27 below 8 A, and the discharge more than 2 A over max(HR27, 2 A).
//   C: HR21 at 2% or less (the inverter's own reserve is at least 4%), and a discharge over 5 A.
//   D: HR20 bit 2 raised, and a charge over 2 A (outside a calibration bit 2 stops it outright).
// Outside a calibration the DSP's own floors let about 1 A of charge through at HR26 = 0 and 2 A
// of discharge at HR27 = 0; during one its floor is 8 A or more. So A and B measure from the
// inverter's floor, not from the advertised limit, and trip only well clear of it.
constexpr uint16_t kCalibrationFloor_cA = 800;  // the DSP's minimum limit during a calibration
constexpr int32_t kChargeFloor_cA = 100;        // what HR26 = 0 still lets through
constexpr int32_t kDischargeFloor_cA = 200;     // what HR27 = 0 still lets through
constexpr int32_t kLimitMargin_cA = 200;
// C could also fire if an inverter discharges below its reserve on its own during a grid outage
// (EPS). At 2% or less the pack is close to its own cut-off anyway, so the risk is low.
constexpr uint16_t kBelowFloorSoc_pct = 2;
constexpr int32_t kBelowFloorDischarge_cA = 500;
constexpr int32_t kAfterFullCharge_cA = 200;             // a calibration still lets ~180 W (3.4 A) through
constexpr uint32_t kChargeOverLimitWindowMs = 60000;     // A
constexpr uint32_t kDischargeOverLimitWindowMs = 60000;  // B
constexpr uint32_t kBelowFloorWindowMs = 60000;          // C
constexpr uint32_t kAfterFullWindowMs = 30000;           // D
// Ending a calibration: bits 2 and 3 together record "empty", then bit 2 alone records "full"
// (the inverter tests bit 3 first, so both at once never gets to "full"). The inverter moves the
// stage on the first DSP frame that carries a bit, a few per second; 10 s each leaves a margin.
constexpr uint32_t kEndingStepMs = 10000;
// Then normal values, watching A to D again with a shorter window. Anything tripping means the
// calibration didn't end (or it was something else), and the module locks out.
constexpr uint32_t kVerifyMs = 60000;
constexpr uint32_t kVerifyWindowMs = 20000;
// A trip this soon after a check that passed means the ending didn't take (a calibration stage
// the battery can't clear, or HR351 set on the inverter): lock out rather than end it again.
constexpr uint64_t kRepeatWindowMs = 3600000;

uint16_t clamp_cell(uint16_t mV) {
  return std::min(std::max(mV, kCellMin_mV), kCellMax_mV);
}

uint16_t limit_cA(uint16_t dA) {
  return static_cast<uint16_t>(std::min<uint32_t>(dA * 10u, kLimitCap_cA));
}

// How far HR26 should be pulled down as the highest cell nears full, independent of what the
// battery itself is asking for. snapshot_from_datalayer() takes the smaller of this and the
// battery-derived limit, so the taper can only tighten HR26, never loosen it.
uint16_t charge_taper_cA(uint16_t max_cell_mV) {
  if (max_cell_mV >= kCellStop_mV) {
    return 0;
  }
  if (max_cell_mV >= kTaperEnd_mV) {
    return kTrickle_cA;
  }
  if (max_cell_mV <= kTaperStart_mV) {
    return kLimitCap_cA;
  }
  return static_cast<uint16_t>(kLimitCap_cA - (max_cell_mV - kTaperStart_mV) * (kLimitCap_cA - kTrickle_cA) /
                                                  (kTaperEnd_mV - kTaperStart_mV));
}

uint16_t saturate16(uint32_t value) {
  return static_cast<uint16_t>(std::min<uint32_t>(value, 0xFFFF));
}

// The Growatt battery module computes Wh = Ah x measured pack voltage, so converting back to Ah
// (and hence cAh) needs that same measured voltage, not a fixed nominal one: cAh = Wh / (V) *
// 100 = Wh / (voltage_dV / 10) * 100 = Wh * 1000 / voltage_dV. Falls back to 512 (51.2 V nominal,
// 16 LFP cells) when the aggregate voltage isn't known yet.
uint32_t wh_to_cAh(uint32_t wh, uint16_t voltage_dV) {
  const uint16_t dV = voltage_dV != 0 ? voltage_dV : 512;
  return static_cast<uint32_t>(static_cast<uint64_t>(wh) * 1000 / dV);
}

// The pack's cell voltages, or, if any is missing, a spread from the
// aggregate min and max so the inverter still sees plausible cells.
void fill_cells(givenergy_lv::Snapshot& s) {
  const uint16_t* cells = datalayer.battery.status.cell_voltages_mV;
  const bool complete = std::all_of(cells, cells + kCells, [](uint16_t mV) { return mV != 0; });
  uint32_t sum = 0;
  for (int i = 0; i < kCells; i++) {
    uint16_t mV = cells[i];
    if (!complete) {
      mV = i == 0 ? s.cell_max_mV : i == 1 ? s.cell_min_mV : (s.cell_max_mV + s.cell_min_mV) / 2;
    }
    s.cells_mV[i] = clamp_cell(mV);
    sum += s.cells_mV[i];
  }
  s.cell_sum_mV = saturate16(sum);
}

}  // namespace

GivEnergyLvRs485Inverter::GivEnergyLvRs485Inverter(HardwareSerial& port)
    : port_(port), snapshot_(snapshot_from_datalayer()) {}

bool GivEnergyLvRs485Inverter::setup() {
  if (!rs485_begin(Name, port_, baud_rate(), SERIAL_8N1)) {
    logging.println("Failed to initialize RS485 pins!");
    return false;
  }
  return true;
}

void GivEnergyLvRs485Inverter::update_values() {
  // datalayer.battery.status.voltage_dV is 0 until the battery module has decoded a real pack
  // voltage; datalayer.aggregate keeps a 370 V placeholder meanwhile (see datalayer.h). Once the
  // battery has reported, stay ready even if a later reading is 0 - the FAULT path already zeroes
  // the limits, so there's nothing unsafe left to hide.
  const bool was_ready = ready_;
  if (datalayer.battery.status.voltage_dV != 0) {
    ready_ = true;
  }

  // snapshot_ still holds what the inverter has been obeying since the last update. On the update
  // that makes the module ready, nothing has been sent yet, so there's nothing to check against.
  if (was_ready) {
    update_guard(snapshot_, millis64());
  }
  snapshot_ = snapshot_from_datalayer();
  apply_guard(snapshot_);
  apply_taper_latch(snapshot_);

  if (incoming_message_counter_ > 0) {
    incoming_message_counter_--;
  }
  if (incoming_message_counter_ == 0) {
    set_event(EVENT_MODBUS_INVERTER_MISSING, 0);
  } else {
    clear_event(EVENT_MODBUS_INVERTER_MISSING);
  }
}

void GivEnergyLvRs485Inverter::receive() {
  const uint32_t now = millis();
  if (reply_len_ > 0) {
    if (now - request_ms_ < kTurnaroundMs) {
      return;
    }
    send_reply(now);
  }

  while (port_.available() > 0) {
    const int byte = port_.read();
    if (byte < 0) {
      break;
    }
    rx_[rx_len_++] = static_cast<uint8_t>(byte);
    if (rx_len_ < givenergy_lv::kRequestLen) {
      continue;
    }
    if (givenergy_lv::is_request(rx_)) {
      rx_len_ = 0;
      if (is_echo(now)) {
        continue;
      }
      handle_request(now);
      return;  // the reply goes out on a later call, after the turnaround
    }
    // Not a request (noise, or the tail of a frame): slide the window on a byte.
    std::memmove(rx_, rx_ + 1, givenergy_lv::kRequestLen - 1);
    rx_len_ = givenergy_lv::kRequestLen - 1;
  }
}

bool GivEnergyLvRs485Inverter::is_echo(uint32_t now_ms) const {
  return sent_write_echo_ && static_cast<int32_t>(echo_until_ms_ - now_ms) > 0 &&
         std::memcmp(rx_, sent_, givenergy_lv::kRequestLen) == 0;
}

void GivEnergyLvRs485Inverter::handle_request(uint32_t now_ms) {
  // A valid request means the inverter is alive on the bus even if we're not ready to answer it
  // yet: don't let a slow battery module get the inverter reported missing.
  incoming_message_counter_ = RS485_HEALTHY;
  if (!inverter_detected_) {
    inverter_detected_ = true;
    set_event(EVENT_MODBUS_INVERTER_DETECTED, 1);
  }
  if (rx_[1] == 6) {
    logging.printf("GivEnergy: inverter wrote %u to register %u of device %u\n",
                   static_cast<unsigned>(rx_[4] << 8 | rx_[5]), static_cast<unsigned>(rx_[2] << 8 | rx_[3]),
                   static_cast<unsigned>(rx_[0]));
  }
  if (!ready_) {
    return;  // no real snapshot yet: stay silent rather than answer with placeholder values
  }
  if (guard_ == Guard::Lockout) {
    return;  // silent for good: after ~30 s the inverter declares the battery lost
  }
  snapshot_.clock_hash = kClockSeed + now_ms / 1000;
  reply_len_ = givenergy_lv::build_reply(rx_, snapshot_, reply_);
  request_ms_ = now_ms;
}

void GivEnergyLvRs485Inverter::send_reply(uint32_t now_ms) {
  // No flush(): a 61-byte reply takes ~64 ms at 9600 baud, and blocking core_loop for that
  // long on every poll starves the CAN receive and trips EVENT_TASK_OVERRUN.
  port_.write(reply_, reply_len_);
  sent_write_echo_ = reply_len_ == givenergy_lv::kRequestLen && reply_[1] == 6;
  std::memcpy(sent_, reply_, givenergy_lv::kRequestLen);
  const uint32_t sent_end_ms = now_ms + tx_ms(reply_len_);
  echo_until_ms_ = sent_end_ms + kEchoWindowMs;
  reply_len_ = 0;
}

// The battery drivers only write the current when a frame arrives, and nothing zeroes it when the
// pack goes quiet. A missing pack also puts the emulator into FAULT, which zeroes HR26 and HR27
// and raises HR20 bits 2 and 3, so a frozen current would look like the inverter ignoring them.
// A configured pack 2 or 3 that never shows up keeps its MISSING warning raised, which holds the
// guard off for as long as it lasts. That can't happen with the single pack this module drives.
static bool battery_data_live() {
  const auto active = [](EVENTS_ENUM_TYPE e) {
    return get_event_pointer(e)->state != EVENT_STATE_INACTIVE;
  };
  return datalayer.battery.status.CAN_battery_still_alive != 0 && !active(EVENT_CAN_BATTERY_MISSING) &&
         !active(EVENT_CAN_BATTERY2_MISSING) && !active(EVENT_CAN_BATTERY3_MISSING);
}

void GivEnergyLvRs485Inverter::update_guard(const givenergy_lv::Snapshot& advertised, uint64_t now_ms) {
  const uint64_t elapsed_ms = now_ms - guard_since_ms_;
  const bool live = battery_data_live();
  if (!live) {
    std::fill(trigger_active_, trigger_active_ + kTriggers, false);
  }
  switch (guard_) {
    case Guard::Normal: {
      const uint8_t trigger = live ? tripped_trigger(advertised, now_ms, false) : 0;
      if (trigger == 0) {
        break;
      }
      set_event(EVENT_INVERTER_LIMITS_IGNORED, trigger);
      if (verified_ && now_ms - verified_ms_ < kRepeatWindowMs) {
        logging.printf("GivEnergy: inverter ignored the battery's limits again (trigger %c); locking out\n",
                       'A' + trigger - 1);
        lock_out(trigger, now_ms);
        break;
      }
      logging.printf("GivEnergy: inverter ignored the battery's limits (trigger %c); ending a calibration\n",
                     'A' + trigger - 1);
      enter_guard(Guard::EndingBoth, now_ms);
      break;
    }
    case Guard::EndingBoth:
      if (elapsed_ms >= kEndingStepMs) {
        enter_guard(Guard::EndingFull, now_ms);
      }
      break;
    case Guard::EndingFull:
      if (elapsed_ms >= kEndingStepMs) {
        enter_guard(Guard::Verify, now_ms);
      }
      break;
    case Guard::Verify: {
      if (!live) {
        // Nothing can be measured: hold the check open, and give it a full 60 s of live data
        // from when the battery comes back.
        guard_since_ms_ = now_ms;
        break;
      }
      const uint8_t trigger = tripped_trigger(advertised, now_ms, true);
      if (trigger != 0) {
        logging.printf("GivEnergy: inverter still ignores the battery's limits (trigger %c); locking out\n",
                       'A' + trigger - 1);
        lock_out(trigger, now_ms);
        break;
      }
      // A condition that started late in the check gets its full window before the all-clear.
      const bool counting = std::any_of(trigger_active_, trigger_active_ + kTriggers, [](bool b) { return b; });
      if (elapsed_ms >= kVerifyMs && !counting) {
        logging.println("GivEnergy: the inverter obeys the battery's limits again");
        clear_event(EVENT_INVERTER_LIMITS_IGNORED);
        verified_ = true;
        verified_ms_ = now_ms;
        enter_guard(Guard::Normal, now_ms);
      }
      break;
    }
    case Guard::Lockout:
      // Latched until the ESP32 restarts. "Clear events" on the web page mustn't hide it while the
      // module stays silent, so raise it again (only when cleared, so occurences stays put).
      if (get_event_pointer(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT)->state == EVENT_STATE_INACTIVE) {
        set_event_latched(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT, lockout_trigger_);
      }
      break;
  }
}

void GivEnergyLvRs485Inverter::lock_out(uint8_t trigger, uint64_t now_ms) {
  lockout_trigger_ = trigger;
  set_event_latched(EVENT_INVERTER_LIMITS_IGNORED_LOCKOUT, trigger);
  reply_len_ = 0;  // not even a reply already queued
  enter_guard(Guard::Lockout, now_ms);
}

uint8_t GivEnergyLvRs485Inverter::tripped_trigger(const givenergy_lv::Snapshot& advertised, uint64_t now_ms,
                                                  bool verifying) {
  // The aggregate current, as HR23 reports it: the sum of every pack, positive = charging.
  const int32_t current_cA = datalayer.aggregate.current_dA * 10;
  const int32_t charge_cA = std::max<int32_t>(current_cA, 0);
  const int32_t discharge_cA = std::max<int32_t>(-current_cA, 0);
  const bool holds[kTriggers] = {
      advertised.charge_limit_cA < kCalibrationFloor_cA &&
          charge_cA > std::max<int32_t>(advertised.charge_limit_cA, kChargeFloor_cA) + kLimitMargin_cA,
      advertised.discharge_limit_cA < kCalibrationFloor_cA &&
          discharge_cA > std::max<int32_t>(advertised.discharge_limit_cA, kDischargeFloor_cA) + kLimitMargin_cA,
      advertised.soc_pct <= kBelowFloorSoc_pct && discharge_cA > kBelowFloorDischarge_cA,
      (advertised.alarms & kAlarmFull) != 0 && charge_cA > kAfterFullCharge_cA,
  };
  const uint32_t windows_ms[kTriggers] = {kChargeOverLimitWindowMs, kDischargeOverLimitWindowMs, kBelowFloorWindowMs,
                                          kAfterFullWindowMs};
  uint8_t tripped = 0;
  for (int i = 0; i < kTriggers; i++) {
    if (!holds[i]) {
      trigger_active_[i] = false;
      continue;
    }
    if (!trigger_active_[i]) {
      trigger_active_[i] = true;
      trigger_since_ms_[i] = now_ms;
    }
    const uint32_t window_ms = verifying ? kVerifyWindowMs : windows_ms[i];
    if (tripped == 0 && now_ms - trigger_since_ms_[i] >= window_ms) {
      tripped = static_cast<uint8_t>(i + 1);
    }
  }
  return tripped;
}

void GivEnergyLvRs485Inverter::enter_guard(Guard state, uint64_t now_ms) {
  guard_ = state;
  guard_since_ms_ = now_ms;
  std::fill(trigger_active_, trigger_active_ + kTriggers, false);
}

void GivEnergyLvRs485Inverter::apply_guard(givenergy_lv::Snapshot& s) const {
  switch (guard_) {
    case Guard::EndingBoth:
      s.charge_limit_cA = 0;
      s.discharge_limit_cA = 0;
      s.alarms |= kAlarmFull | kAlarmEmpty;
      break;
    case Guard::EndingFull:
      s.charge_limit_cA = 0;
      s.discharge_limit_cA = 0;
      s.alarms = (s.alarms & ~kAlarmEmpty) | kAlarmFull;
      break;
    case Guard::Lockout:
      // Nothing goes out any more, but the ERROR event's FAULT does this anyway, except while a
      // user-forced recovery charge holds the emulator ACTIVE.
      s.limit_cA = 0;
      s.charge_limit_cA = 0;
      s.discharge_limit_cA = 0;
      s.alarms |= kAlarmFull | kAlarmEmpty;
      break;
    case Guard::Normal:
    case Guard::Verify:
      break;
  }
}

// A replay of a real GivEnergy battery's charge showed its BMS cut HR26 to 3.20 A at the LFP
// knee and then held it there for 3 h 43 min through 100%, before releasing it again. charge_taper_cA() alone doesn't do that: it follows the highest cell live, so as the
// cells relax once the current drops, HR26 springs back up and invites the inverter to push
// current into a full pack again. This latches the taper at kTrickle_cA once it gets there, and
// holds it until the highest cell falls below kTrickleRelease_mV. Only evaluated while the
// battery data is live (reused from the calibration guard); while it's stale, the latch just
// keeps whatever state it was last in.
void GivEnergyLvRs485Inverter::apply_taper_latch(givenergy_lv::Snapshot& s) {
  if (battery_data_live()) {
    const uint16_t max_cell_mV = datalayer.aggregate.cell_max_voltage_mV;
    if (!taper_latched_ && max_cell_mV >= kTaperEnd_mV) {
      taper_latched_ = true;
    } else if (taper_latched_ && max_cell_mV < kTrickleRelease_mV) {
      taper_latched_ = false;
    }
  }
  if (taper_latched_) {
    s.charge_limit_cA = std::min(s.charge_limit_cA, kTrickle_cA);
  }
}

givenergy_lv::Snapshot GivEnergyLvRs485Inverter::snapshot_from_datalayer() {
  const DATALAYER_AGGREGATE_TYPE& agg = datalayer.aggregate;
  givenergy_lv::Snapshot s{};
  std::strcpy(s.serial, kSerial);
  s.firmware = kFirmware;

  const uint32_t full_cAh = wh_to_cAh(agg.reported_total_capacity_Wh, agg.voltage_dV);
  s.capacity_Ah = saturate16(full_cAh / 100);
  s.status = 0x00CC;  // bits 2, 3, 6 and 7, always set on a real battery in normal running
  if (agg.current_dA <= 0) {
    s.status |= 0x01;
  }
  if (agg.current_dA != 0) {
    s.status |= 0x02;
  }
  s.soc_pct = agg.reported_soc / 100;
  s.voltage_cV = agg.voltage_dV * 10;
  s.current_cA = static_cast<int16_t>(std::min(std::max(agg.current_dA * 10, -32000), 32000));
  s.temperature_C = agg.temperature_max_dC / 10;
  // HR25 only matters to a G3 below firmware 3011 (which uses it instead of HR26/HR27); the real
  // battery still holds it at 90 A while it cuts HR26/HR27, so only fault zeroes it too.
  if (datalayer.system.status.system_status != FAULT) {
    s.limit_cA = kLimitCap_cA;
    s.charge_limit_cA = std::min(limit_cA(agg.max_charge_current_dA), charge_taper_cA(agg.cell_max_voltage_mV));
    s.discharge_limit_cA = limit_cA(agg.max_discharge_current_dA);
  }

  const int16_t t_max = agg.temperature_max_dC;
  const int16_t t_min = agg.temperature_min_dC;
  const int16_t sensors[5] = {t_max, t_min, t_min, t_max, static_cast<int16_t>((t_max + t_min) / 2)};
  std::copy(sensors, sensors + 5, s.sensor_temps_dC);

  const uint8_t cells = datalayer.battery.info.number_of_cells;
  s.cell_count = (cells >= 1 && cells <= kCells) ? cells : kCells;
  s.pack_voltage_mV = saturate16(agg.voltage_dV * 100u);
  s.current_mA = agg.current_dA * 100;
  s.full_capacity_cAh = full_cAh;
  s.design_capacity_cAh = s.full_capacity_cAh;
  s.remaining_cAh = wh_to_cAh(agg.reported_remaining_capacity_Wh, agg.voltage_dV);
  s.block2_soc_pct = s.soc_pct;
  s.block2_word28 = 0x0E10;

  s.cell_max_mV = clamp_cell(agg.cell_max_voltage_mV);
  s.cell_min_mV = clamp_cell(agg.cell_min_voltage_mV);
  fill_cells(s);
  s.temp_max_dC = t_max;
  s.temp_min_dC = t_min;

  // HR20 alarm bits: emulating the DSP firmware showed bit 2 (over-voltage) drops its charge
  // bound to 0 outside a calibration, and to ~180 W during one, lower than the 8 A floor a zero
  // HR26 leaves; bit 3 (under-voltage) cuts discharge to 10%. Neither faults or latches. A real
  // GivEnergy battery raised bit 2 for 271 s straight after its last top-up to 57.5 V at the top of charge.
  // The battery side leads: whenever it allows no charge (its BMS disabled charging or stopped
  // sending limits, or the safety layer applied its voltage limit, the user's ceiling or 100% SoC),
  // HR26 is 0 and bit 2 goes with it; likewise a zero discharge limit raises bit 3. This module's
  // own cell thresholds (the taper's stop and kCellUnderStop_mV) and the user's charge-voltage
  // ceiling stay as backstops in case the battery side doesn't stop first.
  const bool fault = datalayer.system.status.system_status == FAULT;
  const bool user_ceiling_reached = datalayer.battery_settings.user_set_voltage_limits_active &&
                                    agg.voltage_dV >= datalayer.battery_settings.max_user_set_charge_voltage_dV;
  const bool battery_stops_charge = agg.max_charge_current_dA == 0;
  const bool battery_stops_discharge = agg.max_discharge_current_dA == 0;
  if (fault || battery_stops_charge || agg.cell_max_voltage_mV >= kCellStop_mV || user_ceiling_reached) {
    s.alarms |= 0x0004;
  }
  if (fault || battery_stops_discharge || agg.cell_min_voltage_mV <= kCellUnderStop_mV) {
    s.alarms |= 0x0008;
  }
  return s;
}
