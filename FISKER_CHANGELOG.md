# Fisker Ocean development changelog

Fisker test releases use `MAJOR.MINOR.PATCH` versions:

- Increment `PATCH` for a correction, wording change, or small timing adjustment
  that does not change the test strategy.
- Increment `MINOR` for a new diagnostic, CAN test control, decoded signal, or
  supported hardware target.
- Increment `MAJOR` for an incompatible settings change, major replay redesign,
  or replacement of the exploratory contactor strategy with a confirmed one.

Release tags use `fisker-vMAJOR.MINOR.PATCH`. Binary and release-folder names
must contain the same version. A released binary is never overwritten; changes
always receive a new version.

## 2.63.1 - 2026-09-18

- Aligns the Fisker current-limit unit test with Battery Emulator's shared
  power-to-current flow: the battery integration publishes power limits and
  the shared main loop derives and applies current limits.
- Keeps the 50 A Fisker development safety ceiling while respecting lower
  operator-configured power limits.
- Corrects wake-frame testing so automatic startup DTC-clear traffic is not
  mistaken for an additional wake frame. Runtime DTC behaviour is unchanged.
- Makes uncached CI builds keep pioarduino's child environment on the same
  PlatformIO version as the parent, preventing the observed SCons import
  failure without changing firmware behaviour.

## 2.63.0 - 2026-09-18

- Creates the public integration branch from the field-tested v2.62.0 source.
- Transmits only the confirmed Fisker wake frames: `0x093` every 20 ms and
  `0x333` every 50 ms, with their rolling counters and calculated CRC bytes.
- Removes the 15 optional READY-mode candidate frames, their enable mask, and
  their More Battery Info controls. They remain available on the preserved
  `fisker-did-decoded-values` testing branch at tag `fisker-v2.62.0`.
- Retains decoded broadcast voltage, current and SOC, DID polling, DTC reading
  and clearing, on-demand BMS power cycling, operator power limits, and the
  hard 50 A charge/discharge safety ceiling.

## 2.60.0 - 2026-09-15

- Retains the v2.59 on-demand Stark BMS power-cycle command and shared Battery
  Emulator power-cycle safety sequence.
- Uses the standard main-page contactor controls for the Fisker wake loop and
  removes the duplicate contactor controls from More Battery Info.
- Keeps `0x093` and `0x333` as the default wake pair, with rolling counters and
  calculated CRC bytes.
- Corrects the default wake timing to the READY-log-derived periods: `0x093` at
  20 ms and `0x333` at 50 ms.
- Adds individually selectable optional READY-mode CAN-FD candidates, plus a
  master checkbox. Optional candidates remain disabled by default.
- Uses the measured candidate periods: 10 ms for `0x150`, `0x151`, `0x1B6`,
  `0x214`, `0x354`, `0x355`, `0x365`, and `0x366`; 20 ms for `0x236`, `0x260`,
  `0x311`, and `0x318`; and 100 ms for `0x358`, `0x507`, and `0x511`.
- Adds complete captured `0x214` READY and INACTIVE examples to More Battery
  Info and shows the most recently received BMS-origin `0x5A7` state signature.
- Keeps periodic DID polling and displays decoded values alongside raw UDS
  responses on More Battery Info.
- Makes manual DTC reads take priority over retries from periodic DID polling.
- Adds visible DTC scan progress, automatic page refresh while scanning, and an
  explicit failed/timed-out result.
- Tightens `0x59 0x02` DTC response validation and verifies parsing of the
  observed Fisker `C25583` response.
- Uses the standard Battery Emulator DTC status presentation and the upstream
  `fisker_ocean_dtc.json` description lookup. No Fisker-specific DTC colours
  were added.

Timing evidence note: protected-frame periods were reconstructed from rolling
counters in the READY capture because that logger dropped many frames. The
static, unprotected `0x365` and `0x366` periods have lower confidence than the
counter-protected frames and remain optional.

## 2.59.0 - 2026-09-14

- Added an on-demand BMS power-cycle command for Fisker development and recovery
  testing on Stark hardware.
- Used the shared Battery Emulator BMS reset state machine without enabling
  periodic BMS cycling.

## Earlier development history

Earlier source handovers used versions 2.1.0 through 2.2.1. Builds between
2.2.1 and 2.59.0 were experimental collaborator iterations and did not all have
a complete source release entry. Version 2.60.0 establishes the required
commit, tag, changelog, immutable binary, and checksum convention going forward.
