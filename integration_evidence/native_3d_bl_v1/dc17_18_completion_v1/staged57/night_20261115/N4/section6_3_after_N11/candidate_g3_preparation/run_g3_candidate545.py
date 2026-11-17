#!/usr/bin/env python3
"""Run candidate build545 through the frozen C492 physical checks and 6.6 identity rule."""
import argparse
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
NIGHT = PHASE.parents[1]
REFERENCE_RUNNER = PHASE / "reference_g3_preparation/run_g3_reference536.py"
REFERENCE_ALIAS = PHASE / "reference_g3_preparation/build536_aliases.py"
REFERENCE = PHASE / "reference_g3_preparation/cases/n4_g3_reference_bl_20261116T233109_1495225"
IDENTITY = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
VALIDATOR = NIGHT / "N8/A2_A3_N12/section6_8/validation/validate_n8.py"
BASE_CHECKS = NIGHT / "section6_2/run_remaining_checks.py"
COMPARATOR = PHASE.parent / "compare_integer_rows.py"
G3_SUMMARY = PHASE / "reference_g3_preparation/G3_summary.json"
G3_TERMINAL = PHASE / "reference_g3_preparation/G3_primary_terminal_receipt.json"
REFERENCE_INPUTS = (
    REFERENCE / "run.log", REFERENCE / "nose_target_observation.jsonl",
    REFERENCE / "actual_saved_surface_stage_rank0.json", REFERENCE / "cheap_bl_receipt.json",
    REFERENCE / "status.json", G3_SUMMARY, G3_TERMINAL,
)
ALIAS = HERE / "build545_aliases.py"
SCRIPT = Path(__file__).resolve()


def sha(path):
    """Hash a file bound by the candidate run."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load(name, path):
    """Load an existing helper without changing its file or source pins."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def source_for_candidate():
    """Retarget only root, build, alias, case, and accepted G3 reference constants."""
    source = REFERENCE_RUNNER.read_text()
    substitutions = (
        ('HERE = Path(__file__).resolve().parent', f'HERE = Path({str(HERE)!r})'),
        ('BUILD = PHASE / "build536"', 'BUILD = PHASE / "build545"'),
        ('REFERENCE = SECTION1 / "cases/section6_1_bl_20261116T082025_3350201"',
         f'REFERENCE = Path({str(REFERENCE)!r})'),
        ('ALIAS = HERE / "build536_aliases.py"', 'ALIAS = HERE / "build545_aliases.py"'),
        ('CASE_PREFIX = "n4_g3_reference_bl"', 'CASE_PREFIX = "n4_g3_candidate_bl"'),
    )
    for before, after in substitutions:
        assert source.count(before) == 1, before
        source = source.replace(before, after, 1)
    old = '''    return [PHASE / "source_check.md", PHASE / "inputs/CUT_PLAN_8_2.md"] + [
            REFERENCE / name for name in ("run.log", "nose_target_observation.jsonl",
            "actual_saved_surface_stage_rank0.json", "cheap_bl_receipt.json", "status.json")] + [
            P6C / "nose_target_observation.jsonl", WALL_BASELINE]'''
    new = '''    return [PHASE / "source_check.md", PHASE / "inputs/CUT_PLAN_8_2.md",
            REFERENCE_RUNNER, REFERENCE_ALIAS, IDENTITY, VALIDATOR, BASE_CHECKS, COMPARATOR,
            *REFERENCE_INPUTS, P6C / "nose_target_observation.jsonl", WALL_BASELINE]'''
    assert source.count(old) == 1
    source = source.replace(old, new, 1)
    before = "case='n4_g3_reference_'+mode"
    assert source.count(before) == 1
    source = source.replace(before, "case='n4_g3_candidate_'+mode", 1)
    before = 'and "n4_g3_reference_" in source'
    assert source.count(before) == 1
    source = source.replace(before, 'and "n4_g3_candidate_" in source', 1)
    source = source.replace("build536", "build545")
    source = source.replace("g3_reference536.json", "g3_candidate545.json")
    source = source.replace("fresh_build545_identical_cut_g3_reference_v1",
                            "fresh_build545_identical_cut_g3_candidate_v1")
    compile(source, str(SCRIPT), "exec")
    return source


def load_candidate_runtime():
    """Configure the accepted reference runner against build545 and its own output folder."""
    source = source_for_candidate()
    namespace = {"__name__": "candidate_g3_build545", "__file__": str(SCRIPT),
                 "REFERENCE_RUNNER": REFERENCE_RUNNER, "REFERENCE_ALIAS": REFERENCE_ALIAS,
                 "IDENTITY": IDENTITY, "VALIDATOR": VALIDATOR, "BASE_CHECKS": BASE_CHECKS,
                 "COMPARATOR": COMPARATOR, "REFERENCE_INPUTS": REFERENCE_INPUTS}
    exec(compile(source, str(SCRIPT), "exec"), namespace)
    assert namespace["BUILD"] == PHASE / "build545"
    assert namespace["REFERENCE"] == REFERENCE
    assert namespace["CASE_PREFIX"] == "n4_g3_candidate_bl"
    return namespace


def self_check():
    """Verify candidate setup reaches the existing solver boundary only through its stub."""
    namespace = load_candidate_runtime()
    identity = load("candidate_g3_identity_check", IDENTITY)
    result = identity.self_check()
    assert result["launched"] is False
    runner_result = namespace["self_check"]()
    assert runner_result["passed"] and runner_result["launched"] is False
    assert runner_result["stub_reached_solver_boundary"] is True
    return {"passed": True, "launched": False, "candidate_build": "build545",
            "fresh_reference": str(REFERENCE), "identity_selfcheck": result,
            "stubbed_candidate_setup": runner_result}


def main():
    """Expose self-check only; primary invokes the reviewed launch path."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "launch"))
    args = parser.parse_args()
    result = self_check() if args.action == "self-check" else load_candidate_runtime()["launch"]()
    print(json.dumps(result, indent=2))
    if args.action == "launch" and result.get("passed") is not True:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
