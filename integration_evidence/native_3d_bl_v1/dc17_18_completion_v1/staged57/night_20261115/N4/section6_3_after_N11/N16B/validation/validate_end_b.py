#!/usr/bin/env python3
"""Unapplied end-phase-B G3 result check; G2/GQ remain with the B8 receipt path."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parents[1]
NIGHT = PHASE.parents[1]
N14 = NIGHT / "N14/section6_12"
N14_VALIDATION = N14 / "validation"
G3_REFERENCE = N14_VALIDATION / "cases/n14_g3_first_20261117T104157_3163858"
G3_GATE = N14_VALIDATION / "n14_g3_first_gate.json"
G3_PAIR = N14_VALIDATION / "n14_g3_pair_comparison.json"
G2_REFERENCE = N14_VALIDATION / "cases/N14_G2_552"
GQ_REFERENCE = N14_VALIDATION / "cases/N14_GQ_552"
IDENTITY_HELPER = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
G4_HELPER = HERE / "run_g4_574.py"
G3_RUNNER = HERE / "run_g3_574.py"
G4_STUB = HERE / "prelaunch_g4_574.py"
PLAN_HELPER = HERE / "materialize_cut_build_plan.py"
UNIT_GATES = PHASE.parent / "run_unit_gates.py"
BUILD = PHASE / "build574"
G3_MINIMUM = 336


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def check_n14_references() -> dict:
    """Verify the exact sealed reference artifacts without changing them."""
    first = json.loads(G3_GATE.read_text())
    pair = json.loads(G3_PAIR.read_text())
    assert first["passed"] is True
    assert Path(first["case"]).resolve() == G3_REFERENCE.resolve(strict=True)
    assert first["physical"]["passed"] is True
    assert first["purpose"]["accepted_splits"] >= 689
    assert pair["passed"] is True and pair["first_mismatch_index"] is None
    assert pair["compared_edits"] >= G3_MINIMUM
    g3_status = json.loads((G3_REFERENCE / "status.json").read_text())
    assert g3_status.get("state") == "terminal" and g3_status.get("exit_code") == 0
    assert g3_status.get("pins_verified") is True and not g3_status.get("changed")

    prefixes = {}
    for gate, path in (("G2", G2_REFERENCE), ("GQ", GQ_REFERENCE)):
        status = json.loads((path / "status.json").read_text())
        assert status.get("state") == "terminal"
        assert status.get("stop_reason") == f"{gate}_prefix_passed"
        assert status.get("gate") == gate
        assert status.get("all_owned_handles_terminal") is True
        assert status.get("source_and_binary_pins_unchanged") is True
        prefixes[gate] = {"status": status, "status_sha256": sha(path / "status.json")}

    return {
        "g3_reference": str(G3_REFERENCE),
        "g3_gate_sha256": sha(G3_GATE),
        "g3_pair_sha256": sha(G3_PAIR),
        "g3_first_accepted": first["purpose"]["accepted_splits"],
        "g3_pair_common_edits": pair["compared_edits"],
        "g2_gq": prefixes,
    }


def g3_result(case: Path, terminal_receipt_path: Path) -> dict:
    """Apply 6.12 end-cut G3 physical and identity gates to an existing run."""
    case = case.resolve(strict=True)
    status = json.loads((case / "status.json").read_text())
    physical = json.loads((case / "cheap_bl_receipt.json").read_text())
    terminal = json.loads(terminal_receipt_path.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    assert terminal.get("actual_host_proc") is True
    assert terminal.get("case_exit_code") == 0 and terminal.get("passed") is True
    assert terminal.get("status", {}).get("state") == "terminal"
    assert terminal["status"].get("exit_code") == 0
    assert terminal.get("all_owned_handles_terminal") is True
    handles = terminal.get("identities", [])
    assert handles, "terminal receipt must preserve observed process identities"
    for handle in handles:
        assert isinstance(handle.get("pid"), int) and isinstance(handle.get("start_ticks"), int)
        assert handle.get("alive") is False
    assert physical.get("passed") is True
    reference_physical = json.loads(G3_GATE.read_text())["physical"]["gates"]
    actual_gates = physical.get("gates", {})
    # 6.12 keeps every physical gate. The old P6c-prefix gate is deliberately absent.
    assert set(actual_gates) == set(reference_physical), (set(actual_gates), set(reference_physical))
    failed = {name: value for name, value in actual_gates.items() if value is not True}
    assert not failed, failed

    identity = load("end_b_g3_identity", IDENTITY_HELPER)
    ref_edits, ref_observers, ref_stages, _, _ = identity.records(G3_REFERENCE)
    new_edits, new_observers, new_stages, _, _ = identity.records(case)
    count, row = identity.accepted_count(case, new_stages)
    assert len(new_edits) == count
    assert count >= G3_MINIMUM, count
    common = min(len(ref_edits), len(new_edits))
    mismatch = next((i for i in range(common) if ref_edits[i] != new_edits[i]), None)
    ref_work = identity.observer_adjusted_work(ref_stages, ref_observers)
    new_work = identity.observer_adjusted_work(new_stages, new_observers)
    assert mismatch is None, mismatch
    assert ref_work["observer_adjusted_entry_work"] == new_work["observer_adjusted_entry_work"]
    return {
        "passed": True,
        "case": str(case),
        "reference": str(G3_REFERENCE),
        "physical_gates": actual_gates,
        "terminal_receipt_sha256": sha(terminal_receipt_path),
        "accepted_splits": count,
        "minimum_splits": G3_MINIMUM,
        "common_edit_count": common,
        "first_mismatch_index": mismatch,
        "observer_adjusted_entry_work": {
            "reference": ref_work["observer_adjusted_entry_work"],
            "candidate": new_work["observer_adjusted_entry_work"],
        },
        "report_only_split_rows": {
            "reference": identity.accepted_count(G3_REFERENCE, ref_stages)[0],
            "candidate": count,
            "candidate_saved_stage": row,
        },
        "excluded_legacy_gate": "P6c first 336 edits",
    }


def self_check() -> dict:
    """Check the adapter wiring and sealed references without running any work."""
    refs = check_n14_references()
    assert BUILD == PHASE / "build574"
    assert G4_HELPER.is_file() and UNIT_GATES.is_file()
    source = UNIT_GATES.read_text()
    assert '"g4": ("2d1", "2d2", "2d4")' in source
    g4_source = G4_HELPER.read_text()
    compile(g4_source, str(G4_HELPER), "exec")
    assert 'BUILD_LABEL = "build574"' in g4_source
    assert "start_ticks" in g4_source and "all_observed_identities_terminal" in g4_source
    g3_source = G3_RUNNER.read_text()
    compile(g3_source, str(G3_RUNNER), "exec")
    assert "--prelaunch-stub" in g3_source and "--launch" in g3_source
    for helper in (G4_STUB, PLAN_HELPER):
        compile(helper.read_text(), str(helper), "exec")
    return {
        "passed": True,
        "launched": False,
        "built": False,
        "g3_identity_reference": refs["g3_reference"],
        "g3_reference_splits": refs["g3_first_accepted"],
        "g3_pair_common_edits": refs["g3_pair_common_edits"],
        "g3_minimum_splits": G3_MINIMUM,
        "g3_p6c_prefix_gate": False,
        "g4_ranks": [1, 2, 4],
        "g2_gq_references": {gate: data["status"]["stop_reason"] for gate, data in refs["g2_gq"].items()},
        "g2_gq_candidate_validation_owner": "existing B8 receipts/direct comparison",
        "g4_runner": str(G4_HELPER),
        "g3_runner": str(G3_RUNNER),
        "g4_prelaunch_stub": str(G4_STUB),
        "cut_build_plan_helper": str(PLAN_HELPER),
        "build574_status_present": (BUILD / "build_status.json").is_file(),
        "build574": "validate terminal status, pinned binary hashes, source pins and handle receipt before use",
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "g3"))
    parser.add_argument("--case", type=Path)
    parser.add_argument("--terminal-receipt", type=Path)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.action == "self-check":
        assert args.case is None and args.out is None and args.terminal_receipt is None
        result = self_check()
    else:
        if args.case is None or args.out is None:
            parser.error("result checks require --case and --out")
        if args.terminal_receipt is None:
            parser.error("G3 requires --terminal-receipt with PID/start-tick terminal evidence")
        if args.out.exists():
            parser.error("refusing to overwrite prior G3 result")
        result = g3_result(args.case, args.terminal_receipt)
        args.out.parent.mkdir(parents=True, exist_ok=True)
        args.out.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if result.get("passed") is not True:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
