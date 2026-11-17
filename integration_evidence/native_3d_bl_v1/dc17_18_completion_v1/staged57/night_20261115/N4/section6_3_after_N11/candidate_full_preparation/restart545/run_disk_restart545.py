#!/usr/bin/env python3
"""Retarget the accepted isolated restart runner to the Section 6.8 full workflow."""
import argparse
import importlib.util
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
VALIDATION = HERE.parent
SECTION6_8 = VALIDATION.parent
SECTION1 = HERE.parents[3] / "section6_1"
STAGED = HERE.parents[4]
BASE = STAGED / "night_20261115/section6_1/run_disk_restart522.py"
CFD_SHA = "4ca1b712c3eb985d7d9f6132bec067508f3e7943365939a12d97005ab25276da"
TEST_SHA = "3e265eb16b5f8cc796e548a971f50bf9f7bf3268d12953ff58a3ba18af1e7a1d"


def add_restart_helper_pins(source):
    """Pin both Section 6.8 wrappers and the two inherited 522 wrappers."""
    old = 'source_pins.update(extra_source_pins)\nassert all(sha(p) == digest for p, digest in source_pins.items())'
    new = ('source_pins.update(extra_source_pins)\n'
           'for helper in (HERE / "run_disk_restart545.py", HERE / "validate_disk_restart545.py", '
           'STAGED / "night_20261115/section6_1/run_disk_restart522.py", '
           'STAGED / "night_20261115/section6_1/validate_disk_restart522.py", '
           'STAGED / "night_20261115/N8/A2_A3_N12/section6_8/validation/restart534/run_disk_restart534.py", '
           'STAGED / "night_20261115/N8/A2_A3_N12/section6_8/validation/restart534/validate_disk_restart534.py"):\n'
           '    key = str(helper.resolve())\n'
           '    actual = sha(helper)\n'
           '    assert key not in source_pins or source_pins[key] == actual, key\n'
           '    source_pins[key] = actual\n'
           'assert all(sha(p) == digest for p, digest in source_pins.items())')
    assert source.count(old) == 1, "expected exactly one source pin merge anchor"
    return source.replace(old, new, 1)


def retarget_owned_cpu_observation(source):
    """Keep all-process CPU observations while gating only the owned CFD session."""
    edits = (
        ('processes[identity] = {"comm": name, "ticks": int(fields[11]) + int(fields[12])}',
         'processes[identity] = {"comm": name, "ticks": int(fields[11]) + int(fields[12]), "session_owned": int(fields[3]) == session}'),
        ('row["ticks"] += delta\n            row["comm"] = processes[identity]["comm"]',
         'row["ticks"] += delta\n            row["comm"] = processes[identity]["comm"]\n            row["session_owned"] = processes[identity]["session_owned"]'),
        ('busy = busy_cpu_rows(cpu_window_deltas, cpu_window_elapsed)',
         'all_busy = busy_cpu_rows(cpu_window_deltas, cpu_window_elapsed)\n'
         '            busy = [row for row in all_busy if cpu_window_deltas[(row["pid"], row["start_ticks"])]["session_owned"]]'),
        ('"busy_count": len(busy), "scope": "all /proc processes keyed by PID/start_ticks"',
         '"busy_count": len(busy), "busy_processes_observed": all_busy, "scope": "all-process observation; gate counts owned CFD session"'),
        ('cpu_monitor = {"passed": cpu_covered, "window_count": len(windows),',
         'cpu_monitor = {"passed": cpu_covered, "window_count": len(windows),\n'
         '        "gate_scope": "owned CFD session; user processes are observations only",'),
    )
    for old, new in edits:
        assert source.count(old) == 1, f"expected exactly one resource anchor: {old}"
        source = source.replace(old, new, 1)
    return source


def load_base():
    """Load the reviewed runner retargeter as code, without running its main."""
    spec = importlib.util.spec_from_file_location("section61_restart_runner", BASE)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.HERE = HERE
    module.STAGED = STAGED
    module.CFD_SHA = CFD_SHA
    module.TEST_SHA = TEST_SHA
    module.MAX_JOB_RSS = 19999490048
    module.read_source_receipts = read_source_receipts
    module.source_receipts_pass = source_receipts_pass
    original = module.retargeted_source

    def retargeted_source():
        """Apply exact, single-anchor path and evidence changes to the inherited runner."""
        source = original()
        edits = (
            ('sys.path.insert(0, str(Path(__file__).resolve().parents[2]))',
             'sys.path.insert(0, str(Path(__file__).resolve().parents[5]))'),
            ('STAGED = HERE.parents[1]', 'STAGED = Path(__file__).resolve().parents[5]'),
            ('BUILD = HERE / "build522/build_status.json"',
             'BUILD = Path(__file__).resolve().parents[2] / "build545/build_status.json"'),
            ('SOURCE_PINS = HERE / "build522/source_pins.json"',
             'SOURCE_PINS = Path(__file__).resolve().parents[2] / "build545/source_pins.json"'),
            ('EXTRA_SOURCE_PINS = HERE / "unchanged_build499_source_pins.json"',
             'EXTRA_SOURCE_PINS = Path(__file__).resolve().parents[1] / "unchanged_build499_source_pins.json"'),
            ('identity = json.loads((SOURCE / "section6_1_reference_identity.json").read_text())',
             'identity = json.loads((SOURCE / "cut_g5_payload_identity.json").read_text())'),
            ('SOURCE / "section6_1_reference_identity.json", SOURCE / "p7p8_event1_gate.json"',
             'SOURCE / "cut_g5_payload_identity.json", SOURCE / "p7p8_event1_gate.json"'),
            ('assert identity["identity_passed"] is True',
             'assert identity["passed"] is True and identity["workflow_gates_passed"] is True and identity["strict_full_identity"]["passed"] is True'),
            ('HERE / "validate_disk_restart522.py"', 'HERE / "validate_disk_restart545.py"'),
        )
        for old, new in edits:
            assert source.count(old) == 1, f"expected exactly one runner anchor: {old}"
            source = source.replace(old, new, 1)
        return retarget_owned_cpu_observation(add_restart_helper_pins(source))

    module.retargeted_source = retargeted_source

    def supervise(source, source_case, case):
        """Supervise the named 545 entry point and retain terminal process evidence."""
        argv = [sys.executable, str(HERE / "run_disk_restart545.py"), "--internal",
                "--source-case", str(source_case), "--case-dir", str(case)]
        child = subprocess.Popen(argv, cwd=HERE, start_new_session=True)
        identity = module.wait_identity(child.pid)
        wrapper = {"pid": child.pid, "start_ticks": identity["start_ticks"],
                   "started_unix": time.time(), "argv": argv}
        (case.parent / f"{case.name}_controller_launch.json").write_text(json.dumps(wrapper, indent=2) + "\n")
        wrapper["owned_cpu_ids"] = sorted(os.sched_getaffinity(0))
        code = child.wait()
        receipt = module.post_terminal_receipt(case, wrapper)
        assert receipt["all_owned_handles_terminal"], receipt
        raise SystemExit(code)

    module.supervise = supervise
    return module


REFERENCE_HASHES = {
    "adapted_mesh_adap_00001.su2": "2aea6a341b0de7c3175aef7c9671c9f42a17a767c5acc35478a8d89bef87eca7",
    "adapted_mesh_adap_00001.su2.native_ref": "9bddc9a6317e82cf732adb13ab157c690c46ee7fd820eebf62bf672553e129b9",
    "restart_flow_adap_00001.dat": "b77362d32483541b97c9ead50bc52ff33e90a0593db3c5d363977a61331c4091",
    "adapted_mesh_adap_00002.su2": "63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36",
    "adapted_mesh_adap_00002.su2.native_ref": "36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a",
    "restart_flow_adap_00002.dat": "44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94",
}


def digest(path):
    """Hash an identity or output without reading it all into memory."""
    import hashlib
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify_n8_identity(case):
    """Validate the real N8 receipt and independently hash its six output payloads."""
    import json
    case = Path(case).resolve(strict=True)
    identity = json.loads((case / "cut_g5_payload_identity.json").read_text())
    reference = SECTION1 / "accepted_reference_payloads.json"
    payloads = json.loads(reference.read_text())
    assert identity.get("reference") == str(reference)
    assert identity.get("reference_sha256") == digest(reference)
    assert payloads.get("hashes") == REFERENCE_HASHES
    assert identity.get("hashes") == REFERENCE_HASHES
    assert identity.get("passed") is True and identity.get("workflow_gates_passed") is True and identity.get("strict_full_identity", {}).get("passed") is True
    assert identity.get("mismatches") == {}
    assert {name: digest(case / name) for name in REFERENCE_HASHES} == REFERENCE_HASHES
    return identity


def source_receipts_pass(status, workflow, identity, event_gates, timer_gates):
    """Preserve the six-event and timer receipt gate under N8's identity schema."""
    terminal = (status.get("state") == "terminal" and status.get("exit_code") == 0 and
                status.get("all_owned_handles_terminal") is True)
    provenance = (status.get("inputs_unchanged") is True and
                  status.get("source_and_binary_pins_unchanged") is True)
    accepted = (status.get("full_workflow_accepted") is True and
                workflow.get("workflow_accepted") is True)
    checked = (workflow.get("P7_P8_events_checked") == [1, 2] and
               workflow.get("F3b_timer_events_checked") == [1, 2])
    identity_ok = (identity.get("passed") is True and
                   identity.get("workflow_gates_passed") is True and
                   identity.get("strict_full_identity", {}).get("passed") is True)
    events = all(len(rows) == 2 and all(row.get("event") == event and
                  row.get("passed") is True for event, row in zip((1, 2), rows))
                 for rows in (event_gates, timer_gates))
    return bool(terminal and provenance and accepted and checked and identity_ok and events)

def read_source_receipts(case):
    """Require source acceptance and independently verify the six actual full-run files."""
    case = Path(case).resolve(strict=True)
    status = json.loads((case / "status.json").read_text())
    workflow = json.loads((case / "full_workflow_receipt.json").read_text())
    identity = verify_n8_identity(case)
    events = [json.loads((case / f"p7p8_event{event}_gate.json").read_text()) for event in (1, 2)]
    timers = [json.loads((case / f"f3b_event{event}_timer_gate.json").read_text()) for event in (1, 2)]
    assert source_receipts_pass(status, workflow, identity, events, timers), "source full workflow lacks accepted event/timer receipts"
    return status, workflow, identity, events, timers


def main():
    """Run the existing controller only after the N8 full-workflow receipts pass."""
    sys.path.insert(0, str(HERE))
    module = load_base()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--internal", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        source = module.retargeted_source()
        compile(source, str(BASE), "exec")
        module.self_check_resource_helpers()
        required_paths = (HERE, VALIDATION, SECTION6_8, SECTION1, STAGED, BASE,
                          SECTION6_8 / "build545/build_status.json",
                          SECTION6_8 / "build545/source_pins.json",
                          VALIDATION / "unchanged_build499_source_pins.json",
                          SECTION1 / "accepted_reference_payloads.json",
                          STAGED / "runtime_20261113_474/loaded_runtime_pins.json",
                          STAGED / "night_20261115/section6_1/validate_disk_restart522.py",
                          HERE / "validate_disk_restart545.py")
        assert all(path.exists() for path in required_paths), required_paths
        assert "STAGED = Path(__file__).resolve().parents[5]" in source
        assert "build545/build_status.json" in source and "cut_g5_payload_identity.json" in source
        good = {"state": "terminal", "exit_code": 0, "all_owned_handles_terminal": True,
                "inputs_unchanged": True, "source_and_binary_pins_unchanged": True,
                "full_workflow_accepted": True}
        workflow = {"workflow_accepted": True, "P7_P8_events_checked": [1, 2],
                    "F3b_timer_events_checked": [1, 2]}
        identity = {"passed": True, "workflow_gates_passed": True, "strict_full_identity": {"passed": True}}
        gates = [{"event": i, "passed": True} for i in (1, 2)]
        assert source_receipts_pass(good, workflow, identity, gates, gates)
        assert not source_receipts_pass(good, workflow, {"passed": True, "workflow_gates_passed": True}, gates, gates)
        assert not source_receipts_pass(good, workflow,
                                       {"passed": True, "workflow_gates_passed": True,
                                        "strict_full_identity": {"passed": False}}, gates, gates)
        assert not source_receipts_pass(dict(good, exit_code=1), workflow, identity, gates, gates)
        assert '"busy_processes_observed": all_busy' in source
        assert '"gate_scope": "owned CFD session; user processes are observations only"' in source
        assert "night_20261115/section6_1/run_disk_restart522.py" in source
        assert "night_20261115/section6_1/validate_disk_restart522.py" in source
        setup = {"source": str(VALIDATION / "full_workflow"),
                 "build": str(SECTION6_8 / "build545/build_status.json"),
                 "source_pins": str(SECTION6_8 / "build545/source_pins.json"),
                 "extra_source_pins": str(VALIDATION / "unchanged_build499_source_pins.json")}
        assert all(Path(value).exists() for value in setup.values()), setup
        receipt = {"passed": True, "launched": False, "build_sha": CFD_SHA,
                   "resource_helper_self_check": "passed",
                   "helper_sha256": {path.name: digest(path) for path in
                                     (Path(__file__), HERE / "validate_disk_restart545.py", BASE,
                                      STAGED / "night_20261115/section6_1/validate_disk_restart522.py")},
                   "setup_paths": {key: {"path": value, "exists": Path(value).exists()}
                                   for key, value in setup.items()}}
        (HERE / "restart545_prelaunch_selfcheck.json").write_text(json.dumps(receipt, indent=2) + "\n")
        print(json.dumps(receipt, indent=2))
        return
    assert args.source_case is not None and args.source_case.is_dir()
    source_case = args.source_case.resolve(strict=True)
    read_source_receipts(source_case)
    module.apply_owned_cpu_budget()
    case = args.case_dir.resolve() if args.case_dir else HERE / f"disk_restart_{source_case.name}_event2"
    assert HERE in case.parents and not case.exists()
    source = module.retargeted_source()
    if args.internal:
        os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
        os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
        exec(compile(source, str(BASE), "exec"),
             {"__name__": "__main__", "__file__": str(HERE / "run_disk_restart545.py")})
        return
    module.supervise(source, source_case, case)


if __name__ == "__main__":
    main()
