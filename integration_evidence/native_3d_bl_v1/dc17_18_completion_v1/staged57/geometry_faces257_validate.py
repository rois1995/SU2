"""Run one face-only diagnostic with existing source/runtime/process accounting."""
from pathlib import Path
import json
import os
import sys

import run_cases
from prepare_boundary202 import sha256

ROOT = Path(__file__).resolve().parent
folder = ROOT / "m6_runs/euler_build256_canonical_geometry_debt_mach_only_real_cfd50k/geometry_faces257_inputs_v1"
preparation = json.loads((folder / "preparation.json").read_text())
assert preparation["input_hashes_before"] == preparation["input_hashes_after"]
pins = dict(preparation["input_hashes_after"])
pins.update({str(folder / name): item["sha256"] for name, item in preparation["outputs"].items()})
for path in (Path(__file__).resolve(), folder / "preparation.json"):
    pins[str(path)] = sha256(path)
assert all(sha256(Path(p)) == digest for p, digest in pins.items())
run_cases.CASES["geometry_faces257"] = (1, "[NativeGeometryResidualFaces3D]")
os.environ["SU2_NATIVE_FROZEN_CONFIG"] = str(folder / "run.cfg")
old = set((ROOT / "cases").glob("geometry_faces257_*"))
sys.argv = [str(Path(run_cases.__file__).resolve()), "build257", "geometry_faces257"]
try:
    run_cases.main()
finally:
    changed = [p for p, digest in pins.items() if sha256(Path(p)) != digest]
    new = set((ROOT / "cases").glob("geometry_faces257_*")) - old
    assert len(new) == 1
    case = new.pop()
    (case / "face_input_pins.json").write_text(json.dumps(pins, indent=2) + "\n")
    (case / "face_input_provenance.json").write_text(json.dumps({"all_inputs_unchanged": not changed, "changed": changed,
        "scope": "Exact face/binding inputs and immutable authority,not a complete current mesh/cavity."}, indent=2) + "\n")
    assert not changed, changed
