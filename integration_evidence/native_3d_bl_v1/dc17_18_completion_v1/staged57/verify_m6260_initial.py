"""Check the live attempt's completed original-grid CFD/target against the fixed baseline."""
from pathlib import Path
import hashlib
import json
import sys
import numpy as np

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parents[3] / "TestCases/adaptation/capability"))
import capcheck

run = ROOT / "m6_runs/euler_build260_paired_geometry_priority_mach_only_real_cfd50k"
inputs = [run / "restart_flow_adap_00000.dat",
          ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/restart_flow_adap_00000.dat",
          ROOT / "metric_target254_conditioned_v1/L1/restart_flow.dat", Path(__file__).resolve(),
          Path(capcheck.__file__).resolve()]
sha = lambda p: hashlib.file_digest(p.open("rb"), "sha256").hexdigest()
before = {str(p): sha(p) for p in inputs}
actual = capcheck.read_restart(inputs[0])
previous = capcheck.read_restart(inputs[1])
qualified = capcheck.read_restart(inputs[2])
points, fields, _ = actual
assert len(points) == 61330 and np.array_equal(points, previous[0]) and np.array_equal(points, qualified[0])
assert len(fields) == 22 and set(fields) == set(previous[1]) and set(fields) <= set(qualified[1])
assert sorted(k for k in fields if k.startswith("SENSOR_")) == ["SENSOR_MACH"]
matches = {k: bool(np.array_equal(v, previous[1][k]) and np.array_equal(v, qualified[1][k])) for k, v in fields.items()}
assert all(matches.values())
after = {str(p): sha(p) for p in inputs}
assert before == after
out = {"positions_bit_identical": True, "all_22_fields_bit_identical_to256_and_qualified_target": True,
       "common_fields_bit_identical": matches, "sensor_fields": ["SENSOR_MACH"],
       "input_hashes_before": before, "input_hashes_after": after,
       "scope": "Completed INITIAL original-grid CFD/target in the actual frozen260 attempt;same saved seed and corrected Mach-only L1/50k tensor. Not adapted CFD, mesh acceptance or transfer proof."}
(run / "initial_cfd_target_equivalence.json").write_text(json.dumps(out, indent=2) + "\n")
print(json.dumps({k: out[k] for k in ("positions_bit_identical", "all_22_fields_bit_identical_to256_and_qualified_target", "sensor_fields")}))
