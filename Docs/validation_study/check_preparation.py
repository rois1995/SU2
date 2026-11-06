#!/usr/bin/env python3
"""Check this unexecuted study design. Does not import or launch a flow solver."""
import csv
import hashlib
import json
import math
from pathlib import Path

root = Path(__file__).resolve().parent
study = json.loads((root / "study_preparation.json").read_text())
assert study["schema"] == "SU2_STRESS_STUDY_PREPARATION_1"
assert study["status"] == "DESIGN_PREPARED_NOT_RUN"
assert not study["solver_ready"] and study["calibration_lock"] is None
assert study["physical_runs"] == 0 and not study["cluster_access"]
sources = {s["id"]: s for s in json.loads((root / "sources.json").read_text())}
groups = {}
references = 0
for case in study["cases"]:
    assert groups.setdefault(case["group"], case["split"]) == case["split"]
    source = sources[case["id"]]
    path = root / case["reference"]
    assert hashlib.sha256(path.read_bytes()).hexdigest() == source["reference_sha256"]
    assert source["source_file_sha256"] is None
    with path.open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    assert rows and len({r["Output"] for r in rows}) == len(rows)
    for row in rows:
        lo, hi, scale = (float(row[k]) for k in ("Lower", "Upper", "Scale"))
        assert all(math.isfinite(x) for x in (lo, hi, scale))
        assert 0 < lo == hi < .01 and scale == .005
    references += len(rows)
assert set(groups.values()) == {"CALIBRATION", "VALIDATION"}
parameters = json.loads((root / study["parameters"]).read_text())
assert parameters["status"] == "UNSELECTED_NOT_CALIBRATED"
assert parameters["signs"] == [1, -1]
with (root / "run_matrix.csv").open(newline="") as stream:
    runs = list(csv.DictReader(stream))
assert len(runs) == len({r["Run"] for r in runs}) == 58
assert all(r["Status"] == "DESIGN_PREPARED_NOT_RUN" for r in runs)
assert {r["Case"] for r in runs} == {c["id"] for c in study["cases"]}
assert all(r["Epsilon"] == "FROZEN_FROM_CALIBRATION" for r in runs
           if r["Split"] == "VALIDATION" and r["Mode"] == "PRESCRIBED")
with (root / "results_template.csv").open(newline="") as stream:
    assert list(csv.DictReader(stream)) == []
print(f"PASS: {len(study['cases'])} cases, {references} nominal references, {len(runs)} unexecuted definitions; no calibrated lock or solver execution.")
