"""Verify the isolated CFD disk restart for P6c's accepted second mesh."""
from pathlib import Path
import csv
import hashlib
import io
import json
import math
import re
import sys
import time
import numpy as np

HERE = Path(__file__).resolve().parent
STAGED = HERE.parents[1]
SOURCE = HERE / "full_workflow"
D = HERE / "disk_restart_event2"
EVENTS = SOURCE / "full_workflow_corrected_marker_receipt.json"
RUNTIME_MANIFEST = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
sys.path.insert(0, str(STAGED.parents[3] / "TestCases/adaptation/capability"))
import capcheck

sha = lambda p: hashlib.file_digest(Path(p).open("rb"), "sha256").hexdigest()
helpers = {str(p): sha(p) for p in (Path(__file__), Path(capcheck.__file__))}
workflow = json.loads(EVENTS.read_text())
assert workflow["workflow_accepted"] is True and len(workflow["events"]) == 2
event = workflow["events"][1]
counts = event["actual_mesh_counts"]
assert set(counts) == {"points", "cells"} and all(isinstance(v, int) and v > 0 for v in counts.values())
assert event["certification_six_rank_sums"]["cells"] == counts["cells"]
status = json.loads((D / "status.json").read_text())
assert status["state"] == "terminal" and status["exit_code"] == 0
assert status["pins_verified"] and not status["changed"] and status["runtime_receipt_written"]
terminal = json.loads((D / "terminal_handles.json").read_text())
assert terminal["all_owned_handles_terminal"] and not terminal["live_session_members"]
assert terminal["actual_host_handles"] and all(x["terminal"] for x in terminal["actual_host_handles"])
post_terminal=json.loads((D/"post_terminal_handles.json").read_text())
assert post_terminal["actual_host_proc"] and post_terminal["all_owned_handles_terminal"]
assert post_terminal["controller_identity"]==terminal["controller_identity"]
runtime = json.loads((D / "loaded_runtime_startup.json").read_text())
assert runtime["all_six_executing_inodes_and_loaded_elf_pinned"]
assert len(runtime["ranks"]) == 6 and {x["rank"] for x in runtime["ranks"]} == set(range(6))
known_runtime = json.loads(RUNTIME_MANIFEST.read_text())
build = json.loads((HERE / "build_status.json").read_text())
binary_path = str(Path(build["binary"]).resolve())
assert sha(binary_path) == build["binary_sha256"]
for rank in runtime["ranks"]:
    assert rank["executing_inode_verified"] and rank["unmatched"] == []
    assert rank["elf_count"] == len(rank["mapped_elf_files"])
    assert rank["mapped_elf_files"]
    for path, digest in rank["mapped_elf_files"].items():
        expected = build["binary_sha256"] if path == binary_path else known_runtime.get(path)
        assert expected is not None and digest == expected and sha(path) == digest

pins = json.loads((D / "pins.json").read_text())
assert all(sha(path) == digest for path, digest in pins.items())
config = (D / "run.cfg").read_text()
for line in ("ADAP_LOOP= NO", "RESTART_SOL= YES", "READ_BINARY_RESTART= YES",
             "MESH_FILENAME= accepted498.su2", "SOLUTION_FILENAME= accepted498_input",
             "RESTART_FILENAME= continued498_restart", "CONV_FILENAME= continued498_history",
             "ITER= 1"):
    assert config.splitlines().count(line) == 1
for source_name, local_name in (
        ("adapted_mesh_adap_00002.su2", "accepted498.su2"),
        ("adapted_mesh_adap_00002.su2.native_ref", "accepted498.su2.native_ref"),
        ("restart_flow_adap_00002.dat", "accepted498_input.dat")):
    assert sha(D / local_name) == sha(SOURCE / source_name) == event["outputs"][source_name]

log = (D / "run.log").read_text()
assert "Read flow solution from: accepted498_input." in log
assert "continued498_restart.dat" in log and "NATIVE3D_DETAIL stage=refine" not in log
flow_match = re.search(r"\|\s*0\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|\s*([-+.0-9eE]+)\|", log)
assert flow_match
flow = dict(zip(("rms_rho", "CL", "CD"), map(float, flow_match.groups())))
assert all(math.isfinite(x) for x in flow.values())
rows = [{k.strip(): v.strip() for k, v in row.items()}
        for row in csv.DictReader(io.StringIO((D / "continued498_history.csv").read_text()), skipinitialspace=True)]
assert rows and all(math.isfinite(float(v)) for row in rows for v in row.values())

mesh = capcheck.read_su2(D / "accepted498.su2")
before, old, _ = capcheck.read_restart(D / "accepted498_input.dat")
after, fields, precision = capcheck.read_restart(D / "continued498_restart.dat")
assert len(mesh.P) == counts["points"] and len(mesh.E) == counts["cells"]
assert mesh.P.shape == before.shape == after.shape
coordinate_error = max(float(np.max(np.abs(p - mesh.P))) for p in (before, after))
assert coordinate_error <= 1e-12 and all(np.isfinite(v).all() for v in fields.values())
rho = fields["Density"]
internal = fields["Energy"] - sum(fields["Momentum_" + s] ** 2 for s in "xyz") / (2 * rho)
assert np.all(rho > 0) and np.all(internal > 0)
changes = {k: float(np.max(np.abs(fields[k] - old[k]))) for k in
           ("Density", "Momentum_x", "Momentum_y", "Momentum_z", "Energy")}
assert max(changes.values()) > 0
assert sha(D / "continued498_restart.dat") != sha(D / "accepted498_input.dat")
assert all(sha(path)==digest for path,digest in pins.items())
assert helpers == {path:sha(path) for path in helpers}
receipt = {
    "time": time.time(), "disk_restart_verified": True,
    "adapted_points": len(mesh.P), "adapted_cells": len(mesh.E), "ranks": 6,
    "standalone_qualified_cfd": True, "seconds": status["seconds"],
    "source_checkpoint": str(SOURCE), "source_event": 2,
    "source_event_receipt_sha256": sha(EVENTS),
    "coordinate_maximum_error": coordinate_error, "all_output_fields_finite": True,
    "minimum_density": float(rho.min()), "minimum_internal_energy_density": float(internal.min()),
    "precision": precision, "continued_cfd": flow, "continued_history": rows,
    "conservative_field_changes": changes, "all_inputs_and_pins_unchanged": True,
    "pin_count": len(pins), "validation_helpers": helpers,
    "artifact_sha256": {name: sha(D / name) for name in (
        "accepted498.su2", "accepted498.su2.native_ref", "accepted498_input.dat",
        "continued498_restart.dat", "run.cfg", "run.log", "continued498_history.csv",
        "status.json", "terminal_handles.json", "post_terminal_handles.json", "loaded_runtime_startup.json")},
    "scope": "Actual isolated steady CFD disk restart on P6c accepted second adapted mesh and matching continued-CFD state. One further CFD iteration; not convergence qualification, adaptation resume, or full-goal completion."
}
(D / "disk_restart_validation.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({"disk_restart_verified": True, "source_event": 2,
    "adapted_points": len(mesh.P), "adapted_cells": len(mesh.E),
    "coordinate_maximum_error": coordinate_error, "continued_cfd": flow,
    "one_iteration_only": True, "full_goal_complete": False}))
