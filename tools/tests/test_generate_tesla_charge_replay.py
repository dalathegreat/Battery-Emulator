import sys
import unittest
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from generate_tesla_charge_replay import (  # noqa: E402
    CHARGE_054,
    INITIAL_053,
    STARTING_053,
    STEADY_053,
    generate_replay,
)


class TeslaChargeReplayTests(unittest.TestCase):
    def setUp(self):
        self.frames = generate_replay()

    def test_contains_only_missing_family_and_one_command(self):
        self.assertEqual({frame.can_id for frame in self.frames}, {0x053, 0x054, 0x055})
        commands = [frame for frame in self.frames if frame.can_id == 0x054]
        self.assertEqual(len(commands), 1)
        self.assertEqual(commands[0].payload if hasattr(commands[0], "payload") else commands[0].data, CHARGE_054)

    def test_053_startup_sequence_reaches_steady_before_command(self):
        frames_053 = [frame for frame in self.frames if frame.can_id == 0x053]
        self.assertEqual(frames_053[0].data, INITIAL_053)
        self.assertIn(STARTING_053, [frame.data for frame in frames_053])
        self.assertIn(STEADY_053, [frame.data for frame in frames_053])
        first_steady = next(frame for frame in frames_053 if frame.data == STEADY_053)
        command = next(frame for frame in self.frames if frame.can_id == 0x054)
        self.assertGreater(command.timestamp_s - first_steady.timestamp_s, 0.5)

    def test_055_counter_and_checksum_patterns(self):
        frames_055 = [frame for frame in self.frames if frame.can_id == 0x055]
        self.assertEqual([frame.data[0] for frame in frames_055[:5]], [0, 1, 2, 3, 0])
        self.assertEqual([frame.data[6] for frame in frames_055[:5]], [1, 1, 1, 1, 2])
        for frame in frames_055:
            self.assertEqual(frame.data[7], (0x55 + sum(frame.data[:7])) & 0xFF)

    def test_replay_is_finite_and_under_one_thousand_frames(self):
        self.assertLess(len(self.frames), 1000)
        self.assertLessEqual(self.frames[-1].timestamp_s, 5.5)


if __name__ == "__main__":
    unittest.main()
