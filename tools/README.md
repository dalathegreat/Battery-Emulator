# CAN trace tools

`pcan_trace_analyzer.py` extracts timing and payload behavior from PCAN-View
TRC 1.x/2.x files and Battery-Emulator web CAN logger exports.

Run the Ext. Module-focused analysis:

```sh
python3 tools/pcan_trace_analyzer.py TRACE_C_EXT_MODULE_CHARGE.trc
```

The default IDs are `0x053`, `0x054`, `0x055`, `0x056`, `0x118`, `0x221`,
`0x3A1`, and `0x3C2`. Override them or emit JSON for further processing:

```sh
python3 tools/pcan_trace_analyzer.py capture.txt --ids 053,055,056 --json
```

The text report includes frame counts, first/last timestamps, average/median
periods, a gap-resistant nominal average, min/max jitter, payload variants,
changed-byte positions, likely byte or nibble rolling counters, and simple
additive checksum candidates. Counter and checksum findings are candidates for
reverse engineering, not protocol declarations; validate them across more than
one capture before implementing transmit logic.

The tooling tests are included in the project's CTest suite and require Python 3.
To run only the tooling tests directly:

```sh
python3 -m unittest discover -s tools/tests -v
```

## Tesla charge-mode replay

Generate the finite replay used for the first live charge-mode test:

```sh
python3 tools/generate_tesla_charge_replay.py tesla-charge-mode-test.txt
```

The generated 5.5-second log contains the measured `0x053` startup sequence,
`0x055` counters/checksum, and one `0x054` charge command. It intentionally
omits `0x056` for a bus that already has an active `0x056` producer. Select
**CAN Native** and leave **Loop** unchecked in Battery Emulator CAN Replay.

This replay was useful as a bounded diagnostic, but it did not put the live
battery into charge mode and triggered a task-overrun event. The successful
Ext. Module trace shows that the actual enable transition also requires the
charge-specific `0x118` profile. Use the integrated firmware implementation,
not a looping web replay, for the live charge-mode test.
