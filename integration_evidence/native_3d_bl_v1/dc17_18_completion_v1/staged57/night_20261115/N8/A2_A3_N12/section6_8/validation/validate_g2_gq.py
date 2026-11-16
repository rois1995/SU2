#!/usr/bin/env python3
"""Validate completed N8 G2/GQ prefix outputs against accepted N1 references."""
import argparse
import hashlib
import importlib.util
import json
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
A2 = HERE.parent
STAGED = A2.parents[3]
NIGHT = A2.parent.parent.parent
REPO = A2.parents[7]
N1 = NIGHT / "section6_1"
N1_FULL = N1 / "full_workflow"
N1_COLLAPSE = N1 / "first_collapse"
BUILD = A2 / "build534"
COMPARATOR = NIGHT / "N4/compare_integer_rows.py"
COMPARATOR68 = HERE.parent / "drafts/compare_integer_rows_68.py"
EXPECTED_COUNTS = (480, 473, 346, 263, 469, 464)
EXPECTED_WORK = 90_809_869
spec = importlib.util.spec_from_file_location("n8_integer_row_comparator", COMPARATOR)
if spec is None or spec.loader is None:
    raise RuntimeError(f"Cannot import the existing integer comparator: {COMPARATOR}")
comparator = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = comparator
spec.loader.exec_module(comparator)


def sha(path):
    """Hash one evidence or build artifact without loading it into memory."""
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def read_json(path):
    return json.loads(Path(path).read_text())


def accepted_n1():
    """Require the complete accepted N1 workflow and its selected reference identity."""
    status = read_json(N1_FULL / "status.json")
    workflow = read_json(N1_FULL / "full_workflow_receipt.json")
    identity = read_json(N1_FULL / "section6_1_reference_identity.json")
    first = read_json(N1_FULL / "final_source_first_collapse_gate.json")
    provenance = read_json(N1_FULL / "provenance.json")
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("inputs_unchanged") and status.get("source_and_binary_pins_unchanged")
    assert status.get("all_owned_handles_terminal")
    assert workflow.get("workflow_accepted") is True
    assert identity.get("identity_passed") is True
    assert first.get("passed") and first.get("upper_work_sum") == EXPECTED_WORK
    counts = tuple(int(row["accepted"]) for row in first["per_rank"])
    assert counts == EXPECTED_COUNTS
    return {"status": status, "workflow": workflow, "identity": identity,
            "first_collapse": first, "provenance": provenance}


def verify_gq_population(reference):
    """Require six streams and the first three quality windows in the accepted N1 logs."""
    streams = comparator.streams(N1_FULL)
    assert len(streams) == 6, f"expected six accepted N1 rank streams, found {len(streams)}"
    ranks = set()
    for rows in streams.values():
        windows = [row for row in rows if row.marker == "NATIVE3D_QUALITY" and
                   row.fields.get("stage") == "window"]
        if len(windows) >= 3:
            ranks.add(int(windows[2].fields["rank"]))
    assert ranks == set(range(6)), f"accepted N1 GQ reference lacks third windows: {sorted(ranks)}"
    compared = comparator.compare_streams(streams, streams, "GQ")
    assert compared.get("passed") is True
    return {"streams": len(streams), "third_window_ranks": sorted(ranks),
            "self_comparison": compared, "full_provenance": reference["provenance"]}


def check_n5_old_gq_scope():
    """Record why the older N5 GQ output is not the current accepted reference."""
    old = NIGHT / "N5/GQ_candidate"
    old_cfg = old / "run.cfg"
    current_cfg = N1_FULL / "run.cfg"
    old_lines = old_cfg.read_text().splitlines()
    current_lines = current_cfg.read_text().splitlines()
    assert current_lines == old_lines + ["", "ADAP_NATIVE_JOB_MEMORY_LIMIT= 19073"]
    return {"n5_gq_candidate_config_sha256": sha(old_cfg),
            "accepted_n1_config_sha256": sha(current_cfg),
            "difference": "N1 config adds ADAP_NATIVE_JOB_MEMORY_LIMIT=19073; require N1 config for GQ candidate",
            "n5_gq_is_current_reference": False}


def build534():
    """Verify terminal build status, source pins, generated graph, and handles."""
    status_path = BUILD / "build_status.json"
    pins_path = BUILD / "source_pins.json"
    handles_path = BUILD / "terminal_handles.json"
    graph_path = BUILD / "generated_build_graph_after.json"
    status = read_json(status_path)
    pins = read_json(pins_path)
    handles = read_json(handles_path)
    graph = read_json(graph_path)
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    assert handles.get("all_owned_handles_terminal") is True
    assert not any(row.get("alive") for row in handles.get("identities", []))
    assert graph.get("build_status_sha256") == sha(status_path)
    assert graph.get("source_pins_sha256") == sha(pins_path)
    assert graph.get("after") and all(Path(path).is_file() and sha(path) == digest
                                        for path, digest in graph["after"].items())
    binary = Path(status["binary"]).resolve(strict=True)
    assert sha(binary) == status["binary_sha256"]
    pin_map = pins.get("source_pins", pins)
    generated = {str((REPO / "integration_evidence/build-integrated-v3-p5-20261114" / name).resolve())
                 for name in ("build.ninja", "compile_commands.json")}
    assert not generated.intersection(pin_map), "generated graph files are not source pins"
    for path, expected in pin_map.items():
        assert sha(Path(path).resolve(strict=True)) == expected, f"source pin changed: {path}"
    return {"status": status, "source_pins_sha256": sha(pins_path), "binary": str(binary)}


def require_terminal_case(case, gate):
    """Reject live, failed, or out-of-scope case directories before reading their output."""
    case = case.resolve(strict=True)
    assert HERE in case.parents, "candidate case must be below N8/A2_A3_N12/validation"
    status = read_json(case / "status.json")
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("gate_passed") is True and status.get("gate") == gate
    assert status.get("inputs_unchanged") and status.get("source_and_binary_pins_unchanged")
    assert status.get("all_owned_handles_terminal")
    return case, status


def check_candidate_identity(case, build, n1, gate):
    """Bind candidate logs to the frozen build534 and accepted N1 execution inputs."""
    provenance = read_json(case / "provenance.json")
    assert provenance.get("binary_sha256") == build["status"]["binary_sha256"]
    assert provenance.get("source_pins_sha256") == build["source_pins_sha256"]
    assert provenance.get("config_sha256") == sha(N1_FULL / "run.cfg")
    candidate_inputs = provenance.get("input_hashes", {})
    reference_inputs = n1["provenance"].get("input_hashes", {})
    for name in ("m6.su2", "m6.su2.native_ref", "baseline_euler_seed.dat"):
        left = {Path(path).name: digest for path, digest in reference_inputs.items() if Path(path).name == name}
        right = {Path(path).name: digest for path, digest in candidate_inputs.items() if Path(path).name == name}
        assert len(left) == len(right) == 1 and left[name] == right[name], f"{gate} input mismatch: {name}"
    return provenance


def compare_candidate(case, gate, reference):
    """Use the 6.8 comparator for strict old rows plus the census-call addition."""
    result = subprocess.run([sys.executable, str(COMPARATOR68), str(reference), str(case),
                             "--gate", gate], capture_output=True, text=True, check=False)
    parsed = json.loads(result.stdout)
    receipt = {"gate": gate, "passed": result.returncode == 0 and parsed.get("passed") is True,
               "reference": str(reference), "candidate": str(case),
               "comparator_sha256": sha(COMPARATOR68), "comparison": parsed,
               "stdout": result.stdout, "stderr": result.stderr}
    (case / f"{gate.lower()}_accepted_n1_integer_comparison.json").write_text(
        json.dumps(receipt, indent=2) + "\n")
    assert receipt["passed"], receipt
    return receipt


def verify_g2_receipt(case):
    """Enforce exact six-rank accepted-work/count anchors before integer comparison."""
    gate = read_json(case / "G2_gate_receipt.json")
    collapse = gate.get("P6c_collapse_gate", {})
    assert gate.get("passed") is True and collapse.get("passed") is True
    assert collapse.get("upper_work_sum") == EXPECTED_WORK
    rows = collapse.get("per_rank", [])
    assert tuple(int(row["accepted"]) for row in rows) == EXPECTED_COUNTS
    assert len(rows) == 6 and collapse.get("zero_upper_refinements") is True
    return gate


def verify_gq_receipt(case):
    """Require the third quality-window row from every rank plus the passing G2 prefix."""
    gate = read_json(case / "GQ_gate_receipt.json")
    counts = {int(rank): int(value) for rank, value in gate.get("quality_window_rows", {}).items()}
    assert gate.get("passed") is True and gate.get("anchor") == "third NATIVE3D_QUALITY stage=window row per rank"
    assert set(counts) == set(range(6)) and all(counts[rank] >= 3 for rank in range(6))
    assert gate.get("G2_gate_receipt", {}).get("passed") is True
    return gate


def self_check():
    """Run only comparator/parser self-checks and validate accepted reference provenance."""
    reference = accepted_n1()
    n1_gq = verify_gq_population(reference)
    old_scope = check_n5_old_gq_scope()
    result = subprocess.run([sys.executable, str(COMPARATOR68), "--self-check"],
                            capture_output=True, text=True, check=False)
    assert result.returncode == 0, result.stdout + result.stderr
    return {"passed": True, "launched": False, "built": False,
            "G2_reference": str(N1_COLLAPSE), "GQ_reference": str(N1_FULL),
            "N1_identity_state": reference["identity"].get("state"),
            "N1_binary_sha256": reference["provenance"].get("binary_sha256"),
            "N1_source_pins_sha256": reference["provenance"].get("source_pins_sha256"),
            "GQ_population": n1_gq, "prior_N5_GQ_scope": old_scope,
            "comparator_self_check": json.loads(result.stdout)}


def validate(gate, case_path):
    """Validate one already terminal G2 or GQ prefix case; never launches work."""
    reference = accepted_n1()
    build = build534()
    case, _ = require_terminal_case(Path(case_path), gate)
    check_candidate_identity(case, build, reference, gate)
    if gate == "G2":
        verify_g2_receipt(case)
        receipt = compare_candidate(case, gate, N1_COLLAPSE)
    else:
        verify_gq_receipt(case)
        receipt = compare_candidate(case, gate, N1_FULL)
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--g2-case", type=Path)
    parser.add_argument("--gq-case", type=Path)
    args = parser.parse_args()
    if args.self_check:
        assert not args.g2_case and not args.gq_case
        print(json.dumps(self_check(), indent=2))
        return
    assert bool(args.g2_case) != bool(args.gq_case), "provide exactly one completed G2 or GQ case"
    gate, case = ("G2", args.g2_case) if args.g2_case else ("GQ", args.gq_case)
    print(json.dumps(validate(gate, case), indent=2))


if __name__ == "__main__":
    main()
