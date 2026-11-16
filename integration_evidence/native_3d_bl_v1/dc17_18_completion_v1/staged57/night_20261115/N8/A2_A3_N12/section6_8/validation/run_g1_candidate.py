#!/usr/bin/env python3
"""Run one N8 G1 candidate cohort and compare it with the shared accepted reference."""
import argparse
import hashlib
import importlib.util
import json
import math
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
A2 = HERE.parent
N8 = A2.parent.parent
STAGED = N8.parents[1]
NIGHT = N8.parent
REPO = STAGED.parents[3]
N10 = NIGHT / "N10"
BUILD = A2 / "build534"
READY = HERE / "validation_run_ready.json"
HANDLES = BUILD / "terminal_handles.json"
HOUSE = STAGED / "run_cases.py"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
MANIFEST = N10 / "G1_shared_reference_manifest.json"
SELECTION = N10 / "G1_actual_selection.json"
PARSER_PATH = N10 / "parse_g1_identity.py"


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def candidate_source(timeout_seconds):
    source = HOUSE.read_text()
    replacements = {
        'timeout_seconds = 2400 if case == "m6_stage" else 600':
            f'timeout_seconds = {timeout_seconds}',
        'argv += [str(BINARY), filter_expr, "--durations", "yes"]':
            'argv += [str(BINARY), filter_expr, "--durations", "yes", "--reporter", "xml"]',
        '["mpirun", "--bind-to", "none", "-n", str(ranks)]':
            '["mpirun", "--bind-to", "none", "-n", str(ranks), "--output-filename", str(attempt / "rank_output")]',
    }
    for before, after in replacements.items():
        assert source.count(before) == 1, before
        source = source.replace(before, after, 1)
    compile(source, str(HOUSE), "exec")
    return source


def verify_references(parser):
    manifest = json.loads(MANIFEST.read_text())
    assert manifest["state"] == "COMPLETE_SHARED_REFERENCE_under_6_6"
    assert manifest["accepted_base"] == "429c6c1a60"
    reports = {}
    for cohort in ("serial", "mpi2", "mpi4"):
        row = manifest["cohorts"][cohort]
        report = N10 / f"G1_reference_{cohort}.json"
        receipt = N10 / f"G1_{cohort}_primary_terminal_receipt.json"
        assert str(report).endswith(row["report"].split("/night_20261115/N10/")[-1])
        assert sha(report) == row["report_sha256"]
        assert sha(receipt) == row["terminal_receipt_sha256"]
        parsed = json.loads(report.read_text())
        assert len(parsed["case_results"]) == row["case_records"]
        assert parsed["ranks"] == row["ranks"]
        reports[cohort] = parsed
    selection = json.loads(SELECTION.read_text())
    assert selection["catalog"]["public_case_count"] == 102
    return manifest, reports, selection


def verify_build():
    status_path = BUILD / "build_status.json"
    pins_path = BUILD / "source_pins.json"
    graph_path = BUILD / "generated_build_graph_after.json"
    handles_path = HANDLES
    status = json.loads(status_path.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True
    pins = json.loads(pins_path.read_text())
    changed = [name for name, digest in pins.items() if sha(name) != digest]
    assert not changed, f"changed build sources: {changed[:5]}"
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    graph = json.loads(graph_path.read_text())
    assert graph["build_status_sha256"] == sha(status_path)
    assert graph["source_pins_sha256"] == sha(pins_path)
    assert all(Path(name).is_file() and sha(name) == digest for name, digest in graph["after"].items())
    assert graph["test_binary_sha256"] == status["test_binary_sha256"]
    handles = json.loads(handles_path.read_text())
    assert handles.get("all_owned_handles_terminal") is True
    assert not any(row.get("alive") for row in handles.get("identities", []))
    ready = json.loads(READY.read_text())
    assert ready.get("run_ready") is True
    assert ready.get("build_status_sha256") == sha(status_path)
    assert ready.get("source_pins_sha256") == sha(pins_path)
    for name, digest in ready.get("helper_sha256", {}).items():
        assert sha(name) == digest, f"changed ready helper: {name}"
    validation_pin = ready.get("validation_helper_sha256", {}).get(str(Path(__file__).resolve()))
    assert validation_pin == sha(__file__), "candidate runner is not separately sealed"
    return status, pins, graph, handles


def expose_build_alias(status, pins):
    alias_status = HERE / "build534_status.json"
    alias_pins = HERE / "build534_source_pins.json"
    status_payload = dict(status, completed_build=True)
    pins_payload = json.dumps(pins, indent=2) + "\n"
    if alias_status.exists():
        assert json.loads(alias_status.read_text()) == status_payload
    else:
        alias_status.write_text(json.dumps(status_payload, indent=2) + "\n")
    if alias_pins.exists():
        assert alias_pins.read_text() == pins_payload
    else:
        alias_pins.write_text(pins_payload)


def check_runtime():
    runtime = json.loads(RUNTIME.read_text())
    changed = [name for name, digest in runtime.items() if sha(name) != digest]
    assert not changed, f"changed runtime pins: {changed[:5]}"
    return runtime


def compact_first_difference(comparison, parser):
    changes = comparison["unexpected_changes"]
    if not changes:
        return None
    row = changes[0]
    old = row.get("reference") or {}
    new = row.get("candidate") or {}
    old_failed = parser.comparable(old).get("failed_assertions", []) if old else []
    new_failed = parser.comparable(new).get("failed_assertions", []) if new else []
    for index in range(max(len(old_failed), len(new_failed))):
        before = old_failed[index] if index < len(old_failed) else None
        after = new_failed[index] if index < len(new_failed) else None
        if before != after:
            return {"rank": row["rank"], "case": row["case"],
                    "reference": before, "candidate": after}
    return {"rank": row["rank"], "case": row["case"],
            "reference_success": old.get("success"), "candidate_success": new.get("success")}


def save_reports(cohort, parsed, summary):
    report_path = HERE / f"G1_candidate_{cohort}.json"
    report_path.write_text(json.dumps(parsed, indent=2, sort_keys=True) + "\n")
    summary["candidate_parsed_report_sha256"] = sha(report_path)
    summary_path = HERE / f"G1_candidate_{cohort}_summary.json"
    summary_path.write_text(json.dumps(summary, indent=2, sort_keys=True) + "\n")
    print(json.dumps(summary, sort_keys=True), flush=True)


def verify_host_preflight(path):
    receipt = json.loads(Path(path).read_text())
    assert receipt.get("sample_seconds", 0) >= 3
    assert receipt.get("active_OWNED_heavy_processes") == []
    assert receipt.get("MemAvailable")
    assert receipt.get("loadavg")
    return receipt


def prepare_house(cohort, ranks, timeout, selection, status, pins, host_preflight_path):
    case = f"N8_G1_candidate_{cohort}"
    case_dir = HERE / "cases"
    assert not list(case_dir.glob(case + "_*")), f"refusing accidental repeat of {cohort}"
    expose_build_alias(status, pins)
    verify_host_preflight(host_preflight_path)
    for key in list(os.environ):
        if key.startswith("SU2_NATIVE_"):
            del os.environ[key]
    os.environ.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
                      OMPI_MCA_osc="pt2pt", TMPDIR=str(REPO / "integration_evidence/build-integrated-v2/compile-tmp"))
    source = candidate_source(timeout)
    namespace = {"__name__": "N8_G1_candidate_house", "__file__": str(HOUSE)}
    exec(compile(source, str(HOUSE), "exec"), namespace)
    namespace.update(ROOT=HERE, BINARY=Path(status["test_binary"]))
    provenance = [Path(__file__).resolve(), PARSER_PATH, N10 / "run_g1_reference.py",
                  MANIFEST, SELECTION, READY, HANDLES, BUILD / "build_status.json",
                  BUILD / "source_pins.json", BUILD / "generated_build_graph_after.json",
                  RUNTIME, Path(host_preflight_path), N10 / f"G1_reference_{cohort}.json",
                  N10 / f"G1_{cohort}_primary_terminal_receipt.json"]
    source_save = namespace["save"]
    def save_with_provenance(path, value):
        if Path(path).name == "pins.json":
            for pinned in provenance:
                value[str(pinned.resolve())] = sha(pinned)
        source_save(path, value)
    namespace["save"] = save_with_provenance
    filter_key = "serial_filter" if ranks == 1 else "mpi_filter"
    namespace["CASES"][case] = (ranks, selection[filter_key])
    return case, namespace


def run_cohort(cohort, parser, manifest, reference_reports, selection, status, pins,
               runtime, graph, handles, host_preflight_path):
    ranks = {"serial": 1, "mpi2": 2, "mpi4": 4}[cohort]
    expected = 375 if ranks == 1 else 57
    timeout = math.ceil(manifest["cohorts"][cohort]["seconds"] * 1.5)
    case, namespace = prepare_house(cohort, ranks, timeout, selection, status, pins,
                                   host_preflight_path)
    case_dir = HERE / "cases"
    started = time.time()
    process_code = 0
    try:
        sys.argv = [str(HOUSE), "build534", case]
        namespace["main"](RUNTIME)
    except SystemExit as error:
        process_code = int(error.code or 0)
    attempts = sorted(case_dir.glob(case + "_*"), key=lambda path: path.stat().st_mtime)
    assert len(attempts) == 1, f"expected one candidate attempt; found {len(attempts)}"
    folder = attempts[0]
    parsed = parser.parse_run(folder, ranks, expected)
    ref = reference_reports[cohort]
    comparison = parser.compare(ref, parsed, set())
    expected_code = manifest["cohorts"][cohort]["process_returncode"]
    code_equal = process_code == expected_code
    summary = {
        "cohort": cohort, "ranks": ranks, "expected_cases_per_rank": expected,
        "candidate_case": case, "timeout_seconds": timeout,
        "reference_seconds": manifest["cohorts"][cohort]["seconds"],
        "elapsed_wall_seconds": time.time() - started, "controller_returncode": process_code,
        "candidate_attempt": str(folder), "candidate_status": str(folder / "status.json"),
        "candidate_resource_receipt": str(folder / "resources.jsonl"),
        "candidate_pins": str(folder / "pins.json"),
        "candidate_command": str(folder / "command.json"),
        "candidate_run_sha256": sha(folder / "run.log"),
        "candidate_parsed_report_sha256": None,
        "identity_pass": comparison["identity_pass"],
        "reporter_totals_equal": comparison["reporter_totals_equal"],
        "reference_returncode": expected_code, "returncode_equal": code_equal,
        "changed_case_count": len(comparison["changes"]),
        "first_difference": compact_first_difference(comparison, parser),
        "wall_exceptions": [],
        "host_preflight": str(host_preflight_path),
        "host_preflight_sha256": sha(host_preflight_path),
        "primary_run_ready": str(READY), "primary_run_ready_sha256": sha(READY),
        "terminal_handles": str(HANDLES), "terminal_handles_sha256": sha(HANDLES),
        "build_status": str(BUILD / "build_status.json"),
        "build_status_sha256": sha(BUILD / "build_status.json"),
        "source_pins": str(BUILD / "source_pins.json"),
        "source_pins_sha256": sha(BUILD / "source_pins.json"),
        "generated_graph_receipt": str(BUILD / "generated_build_graph_after.json"),
        "generated_graph_sha256": sha(BUILD / "generated_build_graph_after.json"),
        "binary_sha256": status["test_binary_sha256"],
        "runtime_pins": str(RUNTIME), "runtime_pins_sha256": sha(RUNTIME),
        "reference_manifest": str(MANIFEST), "reference_manifest_sha256": sha(MANIFEST),
        "selection_sha256": sha(SELECTION),
        "reference_report_sha256": sha(N10 / f"G1_reference_{cohort}.json"),
    }
    save_reports(cohort, parsed, summary)
    if not summary["identity_pass"]:
        raise SystemExit(1)


def self_check():
    parser = load_module("g1_identity_candidate_self", PARSER_PATH)
    parser.self_check()
    example = {"rank": "0", "name": "synthetic failure", "success": "false",
               "assertions": [{"success": "false", "type": "CHECK", "file": "src.cpp",
                   "line": "17", "sections": ["row"], "original": "value == target",
                   "expanded": "1.25 == 1.25", "xml_source": "synthetic.xml"}]}
    changed = json.loads(json.dumps(example))
    changed["assertions"][0]["expanded"] = "1.25 == 1.5"
    diff = parser.compare({"case_results": [example], "reporter_totals": []},
                          {"case_results": [changed], "reporter_totals": []}, set())
    first = compact_first_difference(diff, parser)
    assert first["reference"]["file"] == "src.cpp" and first["reference"]["line"] == "17"
    assert first["reference"]["expanded"] == "1.25 == 1.25"
    assert first["candidate"]["expanded"] == "1.25 == 1.5"
    manifest, reports, selection = verify_references(parser)
    for cohort, ranks in (("serial", 1), ("mpi2", 2), ("mpi4", 4)):
        timeout = math.ceil(manifest["cohorts"][cohort]["seconds"] * 1.5)
        source = candidate_source(timeout)
        assert f"timeout_seconds = {timeout}" in source
        assert '"--reporter", "xml"' in source
        assert '"--output-filename", str(attempt / "rank_output")' in source
        report = reports[cohort]
        assert report["ranks"] == ranks
        assert len(report["case_results"]) == (375 if ranks == 1 else ranks * 57)
        assert selection["serial_filter" if ranks == 1 else "mpi_filter"]
        assert manifest["cohorts"][cohort]["process_returncode"] == report["exit_code"]
    return {"passed": True, "launched": False, "cohorts": ["serial", "mpi2", "mpi4"],
            "reference_records": {key: len(value["case_results"]) for key, value in reports.items()}}


def main():
    parser_args = argparse.ArgumentParser(description=__doc__)
    parser_args.add_argument("cohort", choices=("serial", "mpi2", "mpi4"), nargs="?")
    parser_args.add_argument("--self-check", action="store_true")
    parser_args.add_argument("--host-preflight", type=Path)
    args = parser_args.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), sort_keys=True))
        return
    if not args.cohort:
        raise SystemExit("select exactly one cohort per invocation")
    if not args.host_preflight:
        raise SystemExit("execution requires the fresh actual-host preflight receipt")
    parser = load_module("g1_identity_candidate", PARSER_PATH)
    manifest, reports, selection = verify_references(parser)
    status, pins, graph, handles = verify_build()
    runtime = check_runtime()
    expose_build_alias(status, pins)
    run_cohort(args.cohort, parser, manifest, reports, selection, status, pins,
               runtime, graph, handles, args.host_preflight)


if __name__ == "__main__":
    main()
