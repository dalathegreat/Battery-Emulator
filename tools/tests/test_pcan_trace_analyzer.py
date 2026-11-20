import sys
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from pcan_trace_analyzer import analyze_id, parse_can_id, parse_trace  # noqa: E402


PCAN_V1 = """\
;$FILEVERSION=1.1
     1)         4.9  Rx         0055  8  00 00 00 00 00 00 09 5E
     2)        14.9  Rx         0055  8  01 00 00 00 00 00 09 5F
     3)        24.9  Rx         0055  8  02 01 00 00 00 00 09 61
     4)        34.9  Rx         0055  8  03 00 00 00 00 00 09 61
     5)        44.9  Rx         0055  8  00 00 00 00 00 00 0A 5F
"""

PCAN_V2 = """\
;$FILEVERSION=2.0
;$COLUMNS=N,O,T,B,I,d,R,L,D
     1)       100.0  Rx   1  0053  -  -  8  D4 30 84 CB 8F 28 46 0D
     2)       120.0  Rx   1  0053  -  -  8  D4 30 84 CB 8F 28 46 0D
"""

WEB_LOG = """\
(828.899) RX0 56 [8] 00 00 00 00 00 00 02 58
(828.999) RX0 56 [8] 00 00 00 00 00 00 03 59
"""


class TraceParserTests(unittest.TestCase):
    def test_parses_pcan_v1_and_detects_modulo_four_counter(self):
        frames = parse_trace(PCAN_V1.splitlines())
        self.assertEqual(len(frames), 5)
        analysis = analyze_id(frames, 0x055)
        self.assertEqual(analysis.frame_count, 5)
        self.assertAlmostEqual(analysis.average_interval_ms, 10.0)
        self.assertEqual(analysis.behavior, "periodic (~10.000 ms)")
        self.assertTrue(
            any(candidate.field == "byte[0]" and candidate.modulus == 4 for candidate in analysis.counter_candidates)
        )
        self.assertTrue(
            any("CAN_ID_low_byte" in candidate.algorithm for candidate in analysis.checksum_candidates)
        )

    def test_parses_pcan_v2_bus_and_flag_columns(self):
        frames = parse_trace(PCAN_V2.splitlines())
        self.assertEqual([frame.can_id for frame in frames], [0x053, 0x053])
        self.assertEqual([frame.channel for frame in frames], ["1", "1"])
        self.assertEqual(frames[0].data, (0xD4, 0x30, 0x84, 0xCB, 0x8F, 0x28, 0x46, 0x0D))

    def test_converts_web_logger_seconds_to_milliseconds(self):
        frames = parse_trace(WEB_LOG.splitlines())
        self.assertEqual(len(frames), 2)
        self.assertAlmostEqual(frames[0].timestamp_ms, 828899.0)
        self.assertAlmostEqual(analyze_id(frames, 0x056).average_interval_ms, 100.0)
        self.assertEqual(frames[0].channel, "0")

    def test_one_shot_command_is_classified_manual_like(self):
        frames = parse_trace(["(1.250) TX1 054 [8] 01 00 02 E8 03 30 87 00"])
        analysis = analyze_id(frames, 0x054)
        self.assertEqual(analysis.behavior, "one-shot/manual-like")
        self.assertEqual(analysis.payloads[0]["data"], "01 00 02 E8 03 30 87 00")

    def test_single_non_command_frame_does_not_claim_manual_behavior(self):
        frames = parse_trace(["(1.250) RX0 056 [8] 00 00 00 00 00 00 02 58"])
        analysis = analyze_id(frames, 0x056)
        self.assertEqual(analysis.behavior, "single frame (period unknown)")

    def test_detects_counter_that_increments_every_four_frames(self):
        lines = []
        timestamp = 0
        for slow_counter in range(16):
            for fast_counter in range(4):
                checksum = (0x55 + fast_counter + slow_counter) & 0xFF
                lines.append(
                    f"({timestamp / 1000:.3f}) RX0 055 [8] "
                    f"{fast_counter:02X} 00 00 00 00 00 {slow_counter:02X} {checksum:02X}"
                )
                timestamp += 10
        analysis = analyze_id(parse_trace(lines), 0x055)
        candidate = next(
            candidate for candidate in analysis.counter_candidates if candidate.field == "byte[6]"
        )
        self.assertEqual(candidate.modulus, 16)
        self.assertEqual(candidate.increment_every_frames, 4)

    def test_can_id_parser_treats_unprefixed_values_as_hex(self):
        self.assertEqual(parse_can_id("221"), 0x221)
        self.assertEqual(parse_can_id("0x3A1"), 0x3A1)


if __name__ == "__main__":
    unittest.main()
