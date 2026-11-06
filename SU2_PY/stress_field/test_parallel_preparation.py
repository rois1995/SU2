from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from campaign import config_options
from prepare_parallel_checks import prepare


class ParallelPreparationTests(unittest.TestCase):
    def test_bounded_local_plan_and_checkpoint_dependencies(self):
        with tempfile.TemporaryDirectory() as temporary, patch("subprocess.run",side_effect=AssertionError("unexpected process")):
            root=Path(temporary)/"prepared"
            plan=prepare(root,"/missing/SU2_CFD")
            self.assertEqual(len(plan["jobs"]),20)
            self.assertEqual(len(plan["comparisons"]),10)
            seen=set()
            for job in plan["jobs"]:
                self.assertEqual(job["command"][1:3],["--host","localhost"])
                self.assertIn(job["ranks"],(1,2))
                self.assertLessEqual(job["timeout_seconds"],45)
                self.assertTrue(set(job["depends_on"])<=seen)
                config=config_options((root/job["cwd"]/"resolved.cfg").read_text())
                self.assertLessEqual(int(config["ITER"]),4)
                self.assertEqual(len(job["checkpoint_copies"]),2 if job["depends_on"] else 0)
                seen.add(job["id"])
            self.assertEqual(plan["CFD_runs"],0)
            with self.assertRaises(FileExistsError):prepare(root,"/missing/SU2_CFD")


if __name__=="__main__":unittest.main()
