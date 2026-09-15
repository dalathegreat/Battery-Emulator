# Tesla charge-mode CAN trace analysis

This analysis is based on the supplied August 10, 2026 capture set. Raw trace
files are intentionally not stored in the repository.

## Measured Ext. Module timing

`TRACE_G_054_CHARGE_COMMAND.trc` is the cleanest capture containing a single
operator command with uninterrupted Ext. Module traffic around it. The later
`TRACE_A_EXT_MODULE_CHARGE.trc` is the reference for an actual successful charge
transition.

| CAN ID | Frames | Raw average | Median | Min–max | Behavior |
| --- | ---: | ---: | ---: | ---: | --- |
| `0x053` | 2,723 | 20.044 ms | 20.000 ms | 19.8–20.3 ms | periodic |
| `0x055` | 5,446 | 10.022 ms | 10.000 ms | 9.7–10.3 ms | periodic |
| `0x056` | 551 | 99.044 ms | 99.000 ms | 98.2–99.8 ms | periodic |
| `0x054` | 1 | — | — | — | manual command |
| `0x118` | 5,446 | 10.022 ms | 10.000 ms | 9.7–10.3 ms | periodic |
| `0x221` | 1,090 | 50.110 ms | 50.100 ms | 48.8–51.4 ms | periodic |
| `0x3A1` | 1,090 | 50.110 ms | 50.100 ms | 48.5–51.7 ms | periodic |
| `0x3C2` | 1,090 | 50.110 ms | 50.100 ms | 48.1–52.1 ms | periodic |

Longer traces contain capture pauses and missing frames, which inflate the raw
average. Their medians and gap-resistant nominal averages agree with the table.

## Payload behavior

### `0x053`

The message has no rolling counter. Ext. Module startup uses three observed states,
all at the 20 ms period:

1. `54 30 84 C3 8F 28 46 0D` for roughly 100–140 ms.
2. `D4 30 84 C3 8F 28 46 0D` for roughly 2.8–3.0 seconds.
3. `D4 30 84 CB 8F 28 46 0D` continuously afterward and during successful charge.

The first state was visible in `first 500 ms after 0x054.trc` and
`TRACE_E_FULL_PROCESS.trc`; `TRACE_G` began during state 2. The state changes
preceded the recorded `0x054` command, so the command did not reset or advance
this sequence.

### `0x055`

The 10 ms payload is:

```text
[counter2, state, 00, 00, 00, 00, counter16, checksum]
```

- Byte 0 counts `0, 1, 2, 3` modulo 4 on every frame.
- Byte 1 is normally `0`; it is `1` on the byte-0 `2` phase after `0x053`
  changes from `54 ...` to `D4 ...`.
- Bytes 2–5 are zero in all supplied captures.
- Byte 6 counts `0..15` modulo 16, incrementing after each complete four-frame
  byte-0 cycle (about every 40 ms).
- Byte 7 is an additive checksum:
  `(0x55 + sum(bytes 0..6)) & 0xFF`.

The two counters continue through a recorded `0x054` command; the command does
not restart them.

### `0x056`

The 99 ms payload is:

```text
00 00 00 00 00 00 counter16 checksum
```

- Byte 6 counts `0..15` modulo 16 on every frame.
- Byte 7 is `(0x56 + sum(bytes 0..6)) & 0xFF`, which simplifies to
  `(0x56 + byte[6]) & 0xFF` for the observed payload.
- Its counter also continues through `0x054`; it is independent of `0x055`.

### `0x054`

`TRACE_G_054_CHARGE_COMMAND.trc` contains one manual frame at 13,293.8 ms:

```text
01 00 02 E8 03 30 87 00
```

Other traces show the same payload only when the operator sends it. It is not a
periodic keepalive. Repeated appearances in long process captures are separated
by many seconds and represent repeated manual attempts. This is an
operator-to-Ext. Module command; the Tesla battery does not act on it directly.

### Successful `0x118` charge transition

The decisive change in `TRACE_A_EXT_MODULE_CHARGE.trc` is Ext. Module's periodic
`0x118` profile, not the `0x054` operator command:

- During charge startup, byte 1 has high nibble `0x8`, byte 2 is `0x2D`, byte
  5 remains `0x08`, and byte 7 is `0x00`.
- At 17,653.2 ms, byte 2 becomes `0xE9` and byte 5 changes from `0x08` to
  `0x48` while byte 7 remains `0x00`.
- About 1.1 seconds later, the BMS `0x212` status changes to charging.
- At 39,260.8 ms, byte 5 returns from `0x48` to `0x08`; the BMS subsequently
  exits charging.

The unsuccessful `TRACE_G` attempt kept byte 5 at `0x08` and used byte 7
`0x80`. Replaying only `0x053`, `0x055`, and `0x054` therefore cannot reproduce
the successful transition.

The other successful steady profiles observed in `TRACE_A` are:

- `0x221`: alternating mux values `0x20` and `0x21` at 50 ms.
- `0x3C2`: alternating `10 55 55 55 00 00 5D 19` and
  `01 55 15 15 00 00 55 09` at 50 ms.
- `0x3A1`: alternating payload families beginning `03 00 98 6E BE 00` and
  `88 42 0B C8 00 10`, with rolling counter/checksum fields.

## Baseline and conflict comparison

- Battery Emulator baseline: `0x053` and `0x055` are absent.
- Ext. Module successful traffic: `0x053` at 20 ms, `0x055` at 10 ms, and `0x056`
  at about 99 ms.
- Full-system failure: overlapping `0x118`, `0x221`, `0x3A1`, and `0x3C2`
  producers create short inter-frame spacings and extra payload variants. This
  supports the protocol-conflict hypothesis rather than an electrical fault.
- In `TRACE_E_FULL_PROCESS.trc`, `0x053`/`0x055` begin at about 74.3 seconds,
  after the Battery Emulator is removed, and execute the same Ext. Module startup
  sequence before the later charge command.

## Live replay result

The first finite web-replay experiment injected the measured `0x053`/`0x055`
startup and one `0x054` while Battery Emulator remained active. It did not
enter charge: the BMS stayed in support/no-power state and advertised zero
maximum charge current. The device logged a task-overrun event. This confirms
both that `0x054` is insufficient without Ext. Module translating it and that the
web replay path is not appropriate for permanent 10 ms charge traffic.

## Live integrated-firmware result

The first integrated-firmware test reached `ABOUT_TO_CHARGE`, but remained at
zero charge current. The PCS reported `Vcfront Mia`, `Dcdc12 Vsupport Faulted`,
and `Dcdc Lv Rationality`; the low-voltage bus remained near 12 V. A live CAN
capture identified the implementation error: the generic additive checksum
generator had been applied to `0x3A1`, even though that frame uses a different
counter/checksum sequence. The resulting `0x3A1` payloads did not occur in the
successful Ext. Module trace.

After replacing that generator with the exact measured 16-frame `0x3A1`
counter/checksum cycle and aligning mux 0 to even counters and mux 1 to odd
counters, the August 10 live test reached the Tesla CAN charge state:

- `BMS_uiChargeStatus`: `CHARGING`
- `BMS_hvState`: `UP_FOR_CHARGE`
- raw BMS maximum charge current: 250 A
- PCS 12 V support: active at 14.22 V, supplying about 33-40 A to the AGM bus
- PCS DCDC support/rationality faults: cleared
- charge-port `0x21D`: `2E 18 49 0C AC 00 60 01`, decoded as EVSE request
  asserted and AC charge state `ENABLED`

The verified live `0x3A1` pair was `88 42 0B C8 00 10 A2 5A` followed by
`03 00 98 6E BE 00 B0 82`, exactly matching the successful Ext. Module trace.

This did **not** establish physical AC charging. The charge-port light remained
off and the EVSE showed no power delivery. The roughly 2.1 A / 756 W seen at
the pack is consistent with the PCS DCDC converter supplying the heavily
loaded 12 V AGM bank, not with AC entering through the onboard charger. The
BMS status and charge-port `ENABLED` state are therefore necessary protocol
milestones, but are not proof of OBC energy flow.

The successful Ext. Module capture also contains static frame `0x052` at about
100.2 ms:

```text
85 9B E4 27 65 28 30 00
```

It is absent from the Battery Emulator baseline and from the first integrated
firmware. The current public Model 3/Y DBC does not decode it. Because an
unsuccessful trace also contains `0x052`, it is not sufficient by itself. The
second integrated firmware adds this exact static frame only while charge mode
is active.

The second August 10 live test verified `0x052` on the bus at exactly 100 ms.
Starting charge mode again reached the same BMS/PCS CAN state but did not begin
physical charging until the operator pressed the button on the connected EVSE.
After that EVSE-side trigger, the pack stabilized at approximately 2.4-2.5 A
and 864-900 W inward at 363 V. At the same time the PCS DCDC was supplying the
AGM bank with approximately 24.5-25.4 A at 14.1-14.2 V (about 350 W). The
positive inward pack power therefore cannot be explained by the DCDC load and
confirms AC energy flow through the onboard charger. The operator also
confirmed that the physical charge-port light turned green.

The live charge-port status remained `2D 18 21 0C 80 00 60 01`, with AC charge
state `ENABLED` but Tesla SWCAN/digital communication and the decoded EVSE
request bit inactive. This shows that this EVSE can initiate analog-pilot
charging with its local button even when the Tesla digital handshake is not
established. The required operator sequence for this setup is therefore:

1. Connect and power the EVSE.
2. Start Battery Emulator charge mode.
3. Press the EVSE button to initiate power delivery.

The remaining CP lost-communication alerts are not the immediate blocker: the
successful Ext. Module trace contains the same missing GTW, VCSEC, VCFRONT, and UI
status bits while physical charging is underway.

## Firmware injector specification

The integrated implementation keeps one producer per overlapping CAN ID and
switches the existing Tesla transmit profile while charge mode is active:

1. Start `0x053` at 20 ms using the observed three-state startup sequence.
2. Start `0x055` at 10 ms with both counters and its additive checksum.
3. Continue or inject `0x056` at 99 ms only if there is no existing producer;
   two producers with unsynchronized counters should not coexist.
4. Inject the measured static `0x052` payload at 100 ms while charge mode is
   active.
5. Replace, rather than duplicate, the normal `0x118`, `0x221`, `0x3A1`, and
   `0x3C2` producers with the successful Ext. Module profiles.
6. After the 3.14-second startup stage, set `0x118` byte 2 to `0xE9` and byte 5
   to `0x48`; preserve rolling counters and recompute each checksum.
7. On stop, clear `UI_chargeEnableRequest` while keeping the charge profile and
   `0x118` byte 5 at `0x48` (`DI_proximity`) alive. The successful Ext. Module
   trace changed the charge-port proximity from latched to unlatched at
   36,655.8 ms, reported both latch controls disengaged by 37,255.1 ms, and
   reported the connector disconnected at 38,852.8 ms. Ext. Module did not return
   byte 5 to `0x08` until 39,260.8 ms, after removal. This establishes that the
   Ext. Module release sequence preserves the authorization through removal; it
   does not establish that `DI_proximity` is sufficient by itself. A live test
   with the previously installed firmware kept `0x48` asserted and detected the
   handle button, but the latch remained blocking. A short replay of Ext. Module's
   exact `0x333 04 30 84 07 02` alongside the installed firmware also did not
   release it because the installed `0x333 84 30 20 07 02` producer remained on
   the bus. The next firmware test therefore removes that conflicting producer
   and preserves both states continuously. After fresh
   `0x264` measurements confirm no more than 0.5 A and 100 W for one second,
   pulse `UI_openChargePortDoorRequest` for 400 ms to ask the charge port to
   release the connector, then restore the normal Battery Emulator `0x118`
   drive profile and stop `0x052`. If zero current cannot be confirmed within
   15 seconds, stop the charge profile without issuing the release request.

A subsequent live test sent Ext. Module's exact `0x333 04 30 84 07 02` at its
measured 100 ms cadence for five seconds while the handle button was held and
AC power was absent. The latch remained blocking, ruling out `0x333` payload
or cadence as sufficient authorization. The next isolated state difference is
`0x334 UI_closureConfirmed`: the working Ext. Module trace advertises decoded
value 1 while Battery Emulator advertised 0. Battery Emulator now advertises
value 1 while preserving its existing powertrain-control fields and valid
counter/checksum sequence.

The `UI_closureConfirmed` firmware was then tested live. On Stop Charge Mode,
Battery Emulator correctly cleared charge enable (`0x333` byte 0 `04` to
`00`), confirmed zero current from fresh PCS `0x264` frames, and issued the
release request (`0x333` byte 0 `01`). The charge port remained latched. A new
independent successful Ext. Module capture exposed the missing authorization:
Ext. Module continuously sends `0x339 VCSEC_authentication` payload
`41 44 F8 00 00 03 80 00` at about 100 ms. It decodes as
`PASSIVE_BLE_UNLOCKED` with `VCSEC_chargePortLockStatus = UNLOCKED`. Battery
Emulator did not produce `0x339`, consistent with
the observed charge-port VCSEC MIA alert. The charge profile now sends this
exact captured frame while charge mode is active, including the guarded
zero-current wait and release pulse, and stops it when charge mode finishes.

The `0x339` firmware was then verified live. While charge mode was active, a
physical two-second charge-handle button press immediately moved the latch;
the same action had remained latched before `0x339` was present. This confirms
that the captured VCSEC frame supplies the missing unlock authorization. The
normal `0x339` authorization combined with the guarded `0x333` release pulse
was also observed to unlatch the connector on Stop Charge Mode. A later
experimental build replaced that pulse with
`0x339 VCSEC_lockRequestType = 6`, decoded as
`PASSIVE_BLE_EXTERIOR_CHARGEHANDLEBUTTON_UNLOCK`; live testing showed no latch
movement from either that build or an isolated locked-to-unlocked `0x339`
replay. The failed substitution was removed and the previously observed
normal-`0x339` plus `0x333` release sequence was restored. The unplugged
charge-port ECU also selected its native white LED state, confirming that
ready/plug/charge colors should remain state-driven rather than directly
synthesized by the emulator.

A repeat test of the restored sequence did not move the latch. That test also
exposed a state-machine weakness: Battery Emulator treated the 400 ms request
window as a successful release without checking either charge-port feedback
frame, then immediately withdrew the charge profile and VCSEC authorization.
The successful Ext. Module trace instead kept both alive through latch movement
and physical removal. Stop now repeats the zero-current-guarded request for up
to five seconds, watches `0x21D CP_proximity` and both `0x25D` latch-control
states for actual movement, and preserves the charge/unlock profile for a
short unplug window after movement is reported. A timeout without latch
feedback is logged as a failed release rather than success.

That feedback-confirmed build was tested with EVSE AC absent. It preserved the
charge profile and `0x339` authorization for the complete five-second request
window, but `0x21D` and `0x25D` remained in their latched/blocking states. This
rules out early authorization withdrawal as the cause. Newer Tesla signal
metadata distinguishes `UI_chargePortLatchRequest` from bit 0's
`UI_openChargePortDoorRequest`, confirming that the existing bit-0 pulse was
aimed at the door rather than the connector latch. The exact latch bit position
is not published in the available metadata. The next guarded test therefore
pulses byte-0 bit 7 only after fresh zero-current confirmation. That bit is not
an arbitrary new bus value: legacy Battery Emulator firmware transmitted it in
the long-used `0x333` payload `84 30 84 07 02`. It remains experimental until
the charge-port ECU reports actual latch movement.

The guarded byte-0 bit-7 build was then tested live with EVSE AC absent. The
charge port remained latched, ruling out that legacy bit as the connector-latch
request. A broader comparison found that the earlier `UI_closureConfirmed`
test had copied only one field from Ext. Module's `0x334`, while the working trace
continuously advertised different full payloads for `0x102`, `0x103`, `0x334`,
and `0x3B3`. The next candidate removes the failed bit-7 pulse and selects the
exact captured charge-session payloads for those four frames while charge mode
and its guarded release window are active.

That broader charge-session build was also tested live with EVSE AC absent and
did not move the latch. A record-order comparison against the independent
successful Ext. Module unlatch trace then exposed a concrete remaining mismatch:
Ext. Module held `0x118` byte 7 at `0x80` before and throughout latch movement,
whereas Battery Emulator used the charging capture's `0x00` value. This also
accounts exactly for the corresponding `0x118` checksum-byte difference. To
preserve the already verified AC-charge profile, Battery Emulator now switches
byte 7 to `0x80` only after Stop Charge Mode is requested, regenerates the
checksum, and retains that release profile through the zero-current dwell and
feedback-confirmed release window.

That `0x118`-only candidate was tested live and did not move the latch. The
emulator's exported post-test CAN log confirmed that it returned to its normal
profile after the five-second release timeout. Rechecking the complete
successful trace showed that the latch did not first report movement until
56,946.1 ms. Ext. Module also kept `0x333` at the exact payload
`04 30 84 07 02` at its normal approximately 500 ms cadence throughout that
transition. Battery Emulator instead cleared byte-0 bit 2, repeated the
modified frame at 20 ms, and timed out after five seconds. The next diagnostic
candidate therefore preserves Ext. Module's exact `0x333` payload and native
500 ms cadence for up to 65 seconds. It enters that phase only after fresh
`0x264` measurements show no more than 5 V, 0.5 A, and 100 W for one second,
and immediately aborts if those measurements become stale or the line becomes
live again. Once latch feedback is observed, the full profile remains alive
for a 30-second unplug window, matching the successful trace's sustained
authorization more closely.

The 65-second candidate was then tested live. It preserved the charge profile
until its timeout but the latch feedback remained blocking. The BMS contactor
state was `CLOSED` during both this attempt and Ext. Module's successful movement;
the contactors opened only when Battery Emulator exited charge mode at the
timeout, so an open pack contactor was not the missing prerequisite. The live
CAN capture exposed a more direct problem: Battery Emulator actually sent
`0x333 04 30 20 07 02`, not the intended Ext. Module payload
`04 30 84 07 02`, because the configured charge-limit updater rewrote the
termination field after start. The same runtime comparison showed additional
differences in Ext. Module's adjacent controller-origin transmit group. The next
candidate therefore writes the complete `0x333` payload immediately before
each 500 ms transmission and selects the trace-captured release profiles for
`0x207`, `0x241`, `0x247`, `0x284`, `0x293`, `0x2E8`, `0x313`, `0x500`, and
`0x55A`. Counter/checksum frames retain valid rolling counters and regenerated
checksums. These profiles are active only inside the voltage/current/power-
guarded release window.

## Corrected latch/cover interpretation from the 2026-08-12 traces

Two new PCAN captures correct an important attribution error in the earlier
release analysis:

- `EXT_MODULE_LATCH_RELEASE_2026-08-12.trc` shows that changing `0x333` byte 0
  from `0x04` (charge enabled) to `0x00` (charge stopped) does **not** itself
  move the inlet latch. The observed release sequences followed a physical
  press of the charge-handle button. The earlier apparent automatic release on
  Stop therefore mixed that physical input into the CAN response and must not
  be treated as proof of a software-only release command.
- `OPEN_CHARGE_PORT_COVER.trc` was recorded after the connector was removed
  and contains repeatable cover-open/cover-close actuator cycles. During the
  open-cover request, `0x333` byte 0 alternates between `0x05` and `0x04`:
  bit 2 keeps charge mode enabled while bit 0 is the charge-port-door open
  request. This is a door/cover command, not a connector-latch-release command.
- `0x21D`, `0x25D`, `0x41D`, and `0x43D` report the inlet's physical input and
  actuator state. Their transitions differ substantially between a connector
  inserted/handle-button release and an unplugged cover movement, so a single
  `0x21D 0C -> 08` rule is not a complete state model.
- The alternating `0x441` heartbeat is present continuously in both captures
  without an action-correlated payload change. It is supporting VCSEC/body
  traffic, not evidence of a release command by itself.

Consequently, Prepare to Unplug must leave the charge-session CAN profile alive
for the real handle button to be recognized. It must not claim or wait 65
seconds for an automatic latch release that Ext. Module itself does not reproduce.
Starting Charge Mode may separately pulse `0x333` bit 0 for approximately 200
ms to open the hatch; this is a cover command and must not be presented as an
inserted-connector unlock control.

The long trace further narrows the required user workflow. A handle-button
release at about 116.658 seconds begins while `0x333` is still
`04 30 84 07 02`: `0x21D` changes from a proximity value of 3 to 2, the PCS
removes line current, and `0x25D` then advances through its latch movement
states (`...2A...`, `...9A...`, `...A3...`, `...A4...`). Ext. Module Stop at
176.909 seconds changes `0x333` to `00 30 84 07 02` and `0x339` from `F8` to
`FC`, but does not release the latch. A later handle event near 187.760 seconds
while stopped changes the input/status frames without producing the A3/A4
latch movement. After charge mode is re-enabled at 218.198 seconds, a later
handle press again produces the complete release sequence around 230.161
seconds. The reliable connector-removal order is therefore:

1. Leave the Ext. Module charge profile enabled.
2. Press the physical handle button; the PCS removes current before the latch
   moves.
3. Wait for latch-release feedback and unplug the connector.
4. Only then exit the charge profile by handing directly to normal inverter
   operation while continuing to request closed battery contactors.

For Battery Emulator, the safest useful change is to replace the current
automatic-release claim with a feedback-driven "prepare to unplug" workflow:
keep the charge profile alive, watch the handle/proximity and latch feedback,
and hand directly to normal inverter operation after the connector is removed.
The physical handle-button transition automatically arms Prepare to Unplug;
the operator does not need to click a web control before removing the plug.
The web control remains only as an optional manual fallback.
Live testing also showed that the brief proximity-2 handle transition may be
missed even though the subsequent cyclic frames continuously report proximity
1 (removed) and latch state 4 (disengaged). The state machine may infer that
missed transition only if it previously observed proximity 3 (inserted) during
the same charge session. This recovers the real unplug sequence without
mistaking the initially empty port for a completed unplug when the hatch opens.
Prepare to Unplug is unavailable until proximity 3 has confirmed an inserted
connector during the current charge session. The firmware also rejects the
command at the battery interface, because selecting the release profile with
an empty port can extend the locking pin into the connector opening.
There is no automatic success timeout. The handoff additionally requires
normal inverter permission and either a fresh zero charge-line measurement or
the absence of fresh charge-line frames for a complete two-second freshness
window after the ordered handle/latch/unplug sequence. Live testing showed
that the PCS may stop transmitting `0x264` after physical unplug rather than
send a final all-zero sample. This stale-frame condition is used only as the
internal post-unplug handoff gate; the MQTT charge-line measurements remain
invalid and are not converted to zero. The handoff does not intentionally
traverse the Tesla contactor-opening states. Safety faults, equipment stop,
and withdrawn inverter permission retain authority over the contactors. This
matches the reproducible Ext. Module behavior without inventing a software-only
latch command that the captures do not show.


This is trace-derived, unit tested, and live tested with independently
confirmed inward pack power. BMS status or DCDC current alone must still not be
used as proof of charging; the decisive live evidence was sustained positive
pack power after the EVSE-side start command.
