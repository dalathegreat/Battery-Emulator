#!/usr/bin/env python3

from pathlib import Path
import shutil
import sys


ROOT = Path(__file__).resolve().parent

SAFETY = ROOT / "Software/src/devboard/safety/safety.cpp"
UUGP_CPP = ROOT / "Software/src/charger/UUGP-CHARGER.cpp"


def backup(path: Path):
    backup_path = path.with_suffix(path.suffix + ".before_uugp_compile_fix")
    if not backup_path.exists():
        shutil.copy2(path, backup_path)
        print(f"Backup: {backup_path}")
    else:
        print(f"Backup already exists: {backup_path}")


def fail(message):
    print()
    print("ERROR:")
    print(message)
    print()
    print("No further changes were made by this script.")
    sys.exit(1)


def main():
    print("=== UUGP compile fix ===")
    print()

    for path in (SAFETY, UUGP_CPP):
        if not path.exists():
            fail(f"Could not find {path}")

    # ------------------------------------------------------------
    # 1. safety.cpp
    # ------------------------------------------------------------

    safety = SAFETY.read_text(encoding="utf-8")

    old_safety = """  if (charger) {
    // Assuming chargers are all CAN here.
    // Check that the charger has been seen and is still sending CAN messages.
    // If we go 60s without messages we raise a warning
    check_can_component_alive(datalayer.charger.CAN_charger_still_alive, charger_detected, EVENT_CAN_CHARGER_DETECTED,
                              EVENT_CAN_CHARGER_MISSING, charger->interface());
  }
"""

    new_safety = """  if (charger && charger->type() != ChargerType::UUGP) {
    // CAN chargers only. UUGP communicates over RS485.
    // If we go 60s without CAN messages we raise a warning.
    check_can_component_alive(
        datalayer.charger.CAN_charger_still_alive,
        charger_detected,
        EVENT_CAN_CHARGER_DETECTED,
        EVENT_CAN_CHARGER_MISSING,
        static_cast<CanCharger*>(charger)->interface());
  }
"""

    if "static_cast<CanCharger*>(charger)->interface()" in safety:
        print("safety.cpp: compile fix already present.")
    else:
        if old_safety not in safety:
            fail(
                "The expected charger block was not found in safety.cpp.\n"
                "The file may have changed since this script was prepared."
            )

        backup(SAFETY)
        safety = safety.replace(old_safety, new_safety, 1)
        SAFETY.write_text(safety, encoding="utf-8")
        print("safety.cpp: fixed Charger*/CanCharger interface issue.")

    # ------------------------------------------------------------
    # 2. UUGP-CHARGER.cpp
    # ------------------------------------------------------------

    uugp = UUGP_CPP.read_text(encoding="utf-8")

    broken = """  switch (initialization_step) {
      case 6:
    write_single(REG_POWER_LIMIT, 10000);
    break;
    case 7: {
      uint16_t soc = uugp_discharge_cutoff_soc;
      if (soc < 10 || soc > 90) {
        soc = 80;
      }
      write_single(REG_DISCHARGE_CUTOFF_SOC, soc);
      break;
    }
    case 8:
      write_single(REG_CONTROL_MODE, 0);
      break;
  }
  void UUGPCharger::initialize_pcs_information() {
"""

    # Also support the same code if the unusual whitespace characters
    # have already been normalized.
    broken_normalized = """  switch (initialization_step) {
    case 6:
    write_single(REG_POWER_LIMIT, 10000);
    break;
    case 7: {
      uint16_t soc = uugp_discharge_cutoff_soc;
      if (soc < 10 || soc > 90) {
        soc = 80;
      }
      write_single(REG_DISCHARGE_CUTOFF_SOC, soc);
      break;
    }
    case 8:
      write_single(REG_CONTROL_MODE, 0);
      break;
  }
  void UUGPCharger::initialize_pcs_information() {
"""

    fixed = """  switch (initialization_step) {
    case 6:
      write_single(REG_POWER_LIMIT, 10000);
      break;
    case 7: {
      uint16_t soc = uugp_discharge_cutoff_soc;
      if (soc < 10 || soc > 90) {
        soc = 80;
      }
      write_single(REG_DISCHARGE_CUTOFF_SOC, soc);
      break;
    }
    case 8:
      write_single(REG_CONTROL_MODE, 0);
      break;
  }
}

void UUGPCharger::initialize_pcs_information() {
"""

    if "void UUGPCharger::initialize_pcs_information()" not in uugp:
        fail("initialize_pcs_information() was not found in UUGP-CHARGER.cpp.")

    # Detect whether the function is already properly closed.
    marker = "void UUGPCharger::initialize_current_limiting()"
    start = uugp.find(marker)
    next_func = uugp.find("void UUGPCharger::initialize_pcs_information()", start)

    if start == -1 or next_func == -1:
        fail("Could not safely locate initialize_current_limiting().")

    section = uugp[start:next_func]

    if section.rstrip().endswith("}\n") or section.rstrip().endswith("}"):
        print("UUGP-CHARGER.cpp: initialize_current_limiting() already closed.")
    else:
        if broken in uugp:
            replacement = broken
        elif broken_normalized in uugp:
            replacement = broken_normalized
        else:
            fail(
                "initialize_current_limiting() does not match the expected "
                "source layout. No change made to UUGP-CHARGER.cpp."
            )

        backup(UUGP_CPP)
        uugp = uugp.replace(replacement, fixed, 1)
        UUGP_CPP.write_text(uugp, encoding="utf-8")
        print("UUGP-CHARGER.cpp: added missing closing brace.")

    print()
    print("=== Compile fix complete ===")
    print()
    print("Review with:")
    print("  git diff -- Software/src/devboard/safety/safety.cpp")
    print("  git diff -- Software/src/charger/UUGP-CHARGER.cpp")
    print()
    print("Then build before committing.")


if __name__ == "__main__":
    main()