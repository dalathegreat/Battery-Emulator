#!/usr/bin/env python3

from pathlib import Path
import shutil
import sys
import re


ROOT = Path(__file__).resolve().parent

SAFETY = ROOT / "Software/src/devboard/safety/safety.cpp"
UUGP_CPP = ROOT / "Software/src/charger/UUGP-CHARGER.cpp"
UUGP_H = ROOT / "Software/src/charger/UUGP-CHARGER.h"


def fail(message):
    print()
    print("ERROR:")
    print(message)
    print()
    sys.exit(1)


def backup(path):
    backup_path = path.with_suffix(path.suffix + ".before_uugp_safety_fix")
    if not backup_path.exists():
        shutil.copy2(path, backup_path)
        print(f"Backup: {backup_path}")


def replace_once(text, old, new, description):
    if text.count(old) != 1:
        fail(
            f"Expected exactly one occurrence for:\n"
            f"{description}\n"
            f"Found: {text.count(old)}"
        )
    return text.replace(old, new, 1)


def main():
    print("=== UUGP safety fix ===")
    print()

    for path in (SAFETY, UUGP_CPP, UUGP_H):
        if not path.exists():
            fail(f"Could not find {path}")

    # ============================================================
    # safety.cpp
    # ============================================================

    safety = SAFETY.read_text(encoding="utf-8")

    safety_marker = """  // Additional Double-Battery safeties are checked here
"""

    safety_insert = """  // UUGP communicates over RS485, so the CAN charger watchdog above
  // does not apply. If UUGP communication is not verified, force the
  // battery-side power limits to zero.
  //
  // This is intentionally done before the final current-limit conversion
  // later in this function, which will also zero the corresponding current
  // limits.
  if (charger && charger->type() == ChargerType::UUGP &&
      !datalayer.charger.uugp_communication_ok) {
    datalayer.battery.status.max_charge_power_W = 0;
    datalayer.battery.status.max_discharge_power_W = 0;
  }

"""

    if safety_insert.strip() not in safety:
        safety = replace_once(
            safety,
            safety_marker,
            safety_insert + safety_marker,
            "UUGP communication safety check in safety.cpp",
        )
        backup(SAFETY)
        SAFETY.write_text(safety, encoding="utf-8")
        print("safety.cpp: added fail-safe power limiting.")
    else:
        print("safety.cpp: communication safety check already present.")

    # ============================================================
    # UUGP-CHARGER.h
    # ============================================================

    header = UUGP_H.read_text(encoding="utf-8")

    old_members = """  uint16_t transaction_id = 0;
  uint16_t expected_transaction_id = 0;
  uint16_t expected_register = 0;
  uint16_t expected_count = 0;
"""

    new_members = """  uint16_t transaction_id = 0;
  uint16_t expected_transaction_id = 0;
  uint8_t expected_function = 0;
  uint16_t last_ack_transaction_id = 0;
  bool initialization_waiting_for_ack = false;
  uint16_t expected_register = 0;
  uint16_t expected_count = 0;
"""

    if "uint8_t expected_function = 0;" not in header:
        header = replace_once(
            header,
            old_members,
            new_members,
            "UUGP transaction/ack state",
        )
        backup(UUGP_H)
        UUGP_H.write_text(header, encoding="utf-8")
        print("UUGP-CHARGER.h: added transaction/initialization state.")
    else:
        print("UUGP-CHARGER.h: transaction/initialization state already present.")

    # ============================================================
    # UUGP-CHARGER.cpp
    # ============================================================

    cpp = UUGP_CPP.read_text(encoding="utf-8")

    # ------------------------------------------------------------
    # 1. Store expected function for every request.
    # ------------------------------------------------------------

    old_send = """const uint16_t transaction = next_transaction();
  expected_transaction_id = transaction;

  const uint16_t length = static_cast<uint16_t>(2 + payload_length);
"""

    new_send = """const uint16_t transaction = next_transaction();
  expected_transaction_id = transaction;
  expected_function = function;

  const uint16_t length = static_cast<uint16_t>(2 + payload_length);
"""


    if "expected_function = function;" not in cpp:
        cpp = replace_once(
            cpp,
            old_send,
            new_send,
            "record expected UUGP function",
        )
        print("UUGP-CHARGER.cpp: requests now record expected function.")
    else:
        print("UUGP-CHARGER.cpp: expected function already recorded.")

    # ------------------------------------------------------------
    # 2. Initial power limit must be ZERO.
    # ------------------------------------------------------------

    old_initial_limit = """        case 6:
          write_single(REG_POWER_LIMIT, 10000);
          break;
"""


    new_initial_limit = """        case 6:
          // Never enable power during initialization. The charger must remain
          // stopped until all initialization commands have been acknowledged.
          write_single(REG_POWER_LIMIT, 0);
          break;
"""


    if old_initial_limit in cpp:
        cpp = replace_once(
            cpp,
            old_initial_limit,
            new_initial_limit,
            "safe UUGP initialization power limit",
        )
        print("UUGP-CHARGER.cpp: initialization power limit changed to 0.")
    elif "write_single(REG_POWER_LIMIT, 0);" in cpp:
        print("UUGP-CHARGER.cpp: initialization power limit already zero.")
    else:
        fail("Could not find UUGP initialization power-limit command.")

    # ------------------------------------------------------------
    # 3. Replace initialization state machine.
    # ------------------------------------------------------------

    init_pattern = re.compile(
        r"  void UUGPCharger::initialize\(\) \{.*?"
        r"\n  \}\n\n  void UUGPCharger::update_power_limit\(\)",
        re.DOTALL,
    )

    init_match = init_pattern.search(cpp)

    if not init_match:
        fail("Could not locate UUGPCharger::initialize().")

    old_init = init_match.group(0)

    new_init = """  void UUGPCharger::initialize() {
    if (!ensure_serial()) {
      return;
    }

    /*
     * Every initialization write must be acknowledged before moving to
     * the next step. If the response never arrives, the same step is
     * retried after SETTING_INTERVAL_MS.
     */
    if (initialization_waiting_for_ack) {
      if (last_ack_transaction_id == expected_transaction_id) {
        initialization_waiting_for_ack = false;

        ++initialization_step;

        if (initialization_step > 12) {
          initialization_complete = true;
        }

        last_setting_ms = millis();
        return;
      }
      // No valid acknowledgement yet. Re-send the same step below.
    }

    switch (initialization_step) {
      case 0:
      case 1:
      case 2:
      case 3:
      case 4:
      case 5:
        initialize_system_time();
        break;

      case 6:
      case 7:
      case 8:
        initialize_current_limiting();
        break;

      case 9:
      case 10:
      case 11:
        initialize_pcs_information();
        break;

      case 12:
        initialize_start_mode();
        break;

      default:
        initialization_complete = true;
        return;
    }

    initialization_waiting_for_ack = true;
    last_setting_ms = millis();
  }

  void UUGPCharger::update_power_limit()"""

    cpp = cpp[:init_match.start()] + new_init + cpp[init_match.end():]
    print("UUGP-CHARGER.cpp: initialization now requires acknowledgements.")

    # ------------------------------------------------------------
    # 4. Replace update_power_limit().
    # ------------------------------------------------------------

    power_pattern = re.compile(
        r"void UUGPCharger::update_power_limit\(\) \{.*?"
        r"\n  \}\n\n  void UUGPCharger::poll_status\(\)",
        re.DOTALL,
    ) 

    power_match = power_pattern.search(cpp)

    if not power_match:
        fail("Could not locate UUGPCharger::update_power_limit().")

    new_power = """  void UUGPCharger::update_power_limit() {
    /*
     * If communication has not been verified, never send a non-zero
     * power limit. The UUGP may otherwise retain a previously accepted
     * power setting.
     */
    if (!datalayer.charger.uugp_communication_ok) {
      write_single(REG_POWER_LIMIT, 0);
      return;
    }

    const uint16_t bms_limit = bms_power_limit_W();
    uint16_t power_limit = bms_limit;

    /*
     * User configuration can reduce the BMS limit, but can never
     * increase it. The BMS/safety layer remains authoritative.
     */
    if (uugp_allow_discharge_to_home_grid &&
        uugp_power_limit_W < power_limit) {
      power_limit = uugp_power_limit_W;
    }

    if (power_limit > 22000) {
      power_limit = 22000;
    }

    write_single(REG_POWER_LIMIT, power_limit);
  }

  void UUGPCharger::poll_status()"""

    cpp = cpp[:power_match.start()] + new_power + cpp[power_match.end():]
    print("UUGP-CHARGER.cpp: power limit now fails safe and respects BMS limit.")

    # ------------------------------------------------------------
    # 5. Replace process_response().
    # ------------------------------------------------------------

    response_pattern = re.compile(
        r"void UUGPCharger::process_response\(const uint8_t\* frame, size_t length\) \{.*?"
        r"\n  \}\n\n  void UUGPCharger::receive\(\)",
        re.DOTALL,
    )

    response_match = response_pattern.search(cpp)

    if not response_match:
        fail("Could not locate UUGPCharger::process_response().")

    new_response = """  void UUGPCharger::process_response(const uint8_t* frame, size_t length) {
    if (length < 9) {
      return;
    }

    if (frame[2] != 0 || frame[3] != 0 || frame[6] != UNIT_ID) {
      return;
    }

    const uint16_t response_transaction =
        (static_cast<uint16_t>(frame[0]) << 8) |
        frame[1];

    const uint8_t function = frame[7];

    /*
     * Only accept a response to the request currently outstanding.
     * This is especially important for writes: a random/stale write
     * response must never make communication_ok true.
     */
    if (response_transaction != expected_transaction_id) {
      return;
    }

    /*
     * Exception responses are never considered successful.
     */
    if (function & 0x80) {
      return;
    }

    if (function != expected_function) {
      return;
    }

    if (function == FC_WRITE_SINGLE || function == FC_WRITE_MULTIPLE) {
      /*
       * Both write responses contain:
       * transaction + protocol + length + unit + function + 4 bytes data
       * => 12 bytes total.
       */
      if (length != 12) {
        return;
      }

      last_response_ms = millis();
      last_ack_transaction_id = response_transaction;
      datalayer.charger.uugp_communication_ok = true;
      return;
    }

    if (function != FC_READ_INPUT && function != FC_READ_HOLDING) {
      return;
    }

    const uint8_t byte_count = frame[8];

    if ((byte_count & 1) != 0 ||
        byte_count > 100 ||
        9 + byte_count != length) {
      return;
    }

    const uint16_t count = byte_count / 2;

    if (count != expected_count) {
      return;
    }

    uint16_t values[50];

    for (uint16_t i = 0; i < count; ++i) {
      values[i] =
          (static_cast<uint16_t>(frame[9 + i * 2]) << 8) |
          frame[10 + i * 2];
    }

    if (function == FC_READ_INPUT) {
      process_input_registers(expected_register, values, count);
    } else {
      process_holding_registers(expected_register, values, count);
    }

    last_response_ms = millis();
    datalayer.charger.uugp_communication_ok = true;
  }

  void UUGPCharger::receive()"""

    cpp = cpp[:response_match.start()] + new_response + cpp[response_match.end():]
    print("UUGP-CHARGER.cpp: responses now require matching transaction/function.")

    # ------------------------------------------------------------
    # Write all changes.
    # ------------------------------------------------------------

    backup(UUGP_CPP)
    UUGP_CPP.write_text(cpp, encoding="utf-8")

    print()
    print("=== UUGP safety fix complete ===")
    print()
    print("Important behavior:")
    print("  - Initialization starts with 0 W.")
    print("  - Initialization does not advance without a valid ACK.")
    print("  - Lost RS485 communication forces battery power limits to 0 W.")
    print("  - UUGP user limit cannot exceed the BMS limit.")
    print("  - Stale/wrong UUGP responses cannot mark communication healthy.")
    print()
    print("Review with:")
    print("  git diff")
    print()
    print("Then run the full build and tests before committing.")


if __name__ == "__main__":
    main()