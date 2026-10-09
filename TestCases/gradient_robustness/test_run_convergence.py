"""Check that contention gating measures assigned CPUs and fails conservatively."""
import unittest
from unittest.mock import patch

from run_convergence import affinity_cpu_activity


class CpuActivityTests(unittest.TestCase):
    def measure(self, before, after):
        with patch("run_convergence.os.sched_getaffinity", return_value={0, 1}), \
                patch("run_convergence.Path.read_text", side_effect=[before, after]), \
                patch("run_convergence.time.sleep"):
            return affinity_cpu_activity()

    def test_unassigned_load_and_guest_time_are_excluded(self):
        before = "cpu0 0 0 0 0 0 0 0 0 0 0\ncpu1 0 0 0 0 0 0 0 0 0 0\ncpu2 0 0 0 0\n"
        after = "cpu0 20 0 0 60 20 0 0 0 20 0\ncpu1 30 0 0 70 0 0 0 0 30 0\ncpu2 100 0 0 0\n"
        self.assertAlmostEqual(self.measure(before, after), 0.25)

    def test_assigned_cpus_saturated(self):
        self.assertEqual(self.measure("cpu0 0 0 0 0 0 0 0 0\ncpu1 0 0 0 0 0 0 0 0\n",
                                      "cpu0 100 0 0 0 0 0 0 0\ncpu1 100 0 0 0 0 0 0 0\n"), 1.0)

    def test_missing_malformed_and_unchanged_counters_wait(self):
        good = "cpu0 1 0 0 99 0 0 0 0\ncpu1 1 0 0 99 0 0 0 0\n"
        for after in (good, "cpu0 1 0 0 99\n", "cpu0 invalid\ncpu1 invalid\n"):
            self.assertEqual(self.measure(good, after), 1.0)

    def test_unreadable_counters_wait(self):
        with patch("run_convergence.Path.read_text", side_effect=OSError):
            self.assertEqual(affinity_cpu_activity(), 1.0)


if __name__ == "__main__":
    unittest.main()
