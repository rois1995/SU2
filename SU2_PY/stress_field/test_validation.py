import json
from pathlib import Path
import tempfile
import unittest
from validation import freeze, assess


class ValidationTests(unittest.TestCase):
    def test_split_integrity_missing_cases_and_parameters(self):
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory)
            (root/"params.json").write_text('{"epsilon":0.01}')
            (root/"ref.csv").write_text('Output,Lower,Upper,Scale\nDrag,1,1.1,2\n')
            cases=[{"id":"a","group":"A","split":"CALIBRATION","reference":"ref.csv"},
                   {"id":"b","group":"B","split":"VALIDATION","reference":"ref.csv"}]
            study={"schema":"SU2_STRESS_STUDY_1","family":"synthetic only","parameters":"params.json","cases":cases}
            (root/"study.json").write_text(json.dumps(study))
            freeze(root/"study.json",root/"lock.json")
            (root/"results.csv").write_text('Case,Output,Baseline,Minimum,Maximum,Requested,Completed,CostSeconds\na,Drag,0.8,0.7,0.9,5,4,2\n')
            result=assess(root/"lock.json",root/"results.csv",root/"report.json")
            self.assertEqual(result["outputs"][0]["status"],"INCOMPLETE_SCENARIO_RANGE")
            self.assertAlmostEqual(result["outputs"][0]["miss_distance"],.1)
            self.assertEqual(result["outputs"][1]["status"],"MISSING")
            with (root/"results.csv").open("a") as stream:stream.write('b,Drag,0.8,,,5,0,3\n')
            failed=assess(root/"lock.json",root/"results.csv",root/"failed.json")
            self.assertEqual(failed["outputs"][1]["status"],"ALL_SCENARIOS_FAILED")
            self.assertEqual(failed["outputs"][1]["cost_seconds"],3)
            (root/"params.json").write_text('{"epsilon":0.02}')
            with self.assertRaisesRegex(ValueError,"parameters changed"):
                assess(root/"lock.json",root/"results.csv",root/"other.json")
            cases[1]["group"]="A";(root/"study.json").write_text(json.dumps(study))
            with self.assertRaisesRegex(ValueError,"leaks"):
                freeze(root/"study.json",root/"bad.json")

if __name__=="__main__":unittest.main()
