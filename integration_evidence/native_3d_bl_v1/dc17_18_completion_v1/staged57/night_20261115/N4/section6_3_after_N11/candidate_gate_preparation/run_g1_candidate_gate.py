#!/usr/bin/env python3
"""Run one strict zero-exception G1 identity cohort against fresh build536 references."""
import argparse
import hashlib
import importlib.util
import json
import math
import re
import sys
import tempfile
import time
import types
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
N4 = PHASE.parent
STAGED = N4.parents[1]
NIGHT = STAGED / "night_20261115"
N8 = NIGHT / "N8"
N10 = NIGHT / "N10"
N11 = NIGHT / "N11"
BASE = N8 / "A2_A3_N12/section6_8/validation/run_g1_candidate.py"
N11_G1 = N11 / "validation/run_n11_g1.py"
PARSER_PATH = N10 / "parse_g1_identity.py"
SELECTION = N10 / "G1_actual_selection.json"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
SOURCE_CHECK = PHASE / "source_check.md"
DESIGN = PHASE / "inputs/CUT_PLAN_8_2.md"
CASE_COUNTS = {"serial": 375, "mpi2": 57, "mpi4": 57}


def sha(path):
    """Hash a file used by this validation helper."""
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def load_module(name, path):
    """Load an existing parser or monitor from its exact frozen path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def fresh_reference_bundle(reference_root, parser):
    """Validate and load the canonical completed fresh536 manifest and reports."""
    root = Path(reference_root)
    manifest = json.loads((root / "G1_shared_reference_manifest.json").read_text())
    assert manifest["state"] == "COMPLETE_FRESH_CUT_REFERENCE_C2_1"
    assert manifest["accepted_base"] == "f8c7b01085" and manifest["build"] == 536
    references = {}
    for cohort, ranks in (("serial", 1), ("mpi2", 2), ("mpi4", 4)):
        row = manifest["cohorts"][cohort]
        report_path, terminal_path = Path(row["report"]), Path(row["terminal_receipt"])
        assert row["ranks"] == ranks and row["cases_per_rank"] == CASE_COUNTS[cohort]
        assert row["case_records"] == ranks * CASE_COUNTS[cohort]
        assert sha(report_path) == row["report_sha256"]
        assert sha(terminal_path) == row["terminal_receipt_sha256"]
        terminal = json.loads(terminal_path.read_text())
        assert terminal.get("all_owned_handles_terminal") is True
        assert int(row["wrapper_seconds"]) == math.ceil(float(row["seconds"]) * 1.5)
        report = json.loads(report_path.read_text())
        assert report["ranks"] == ranks and len(report["case_results"]) == row["case_records"]
        references[cohort] = report
    selection = json.loads(SELECTION.read_text())
    assert selection["catalog"]["public_case_count"] == 102
    assert selection["serial_filter"] and selection["mpi_filter"]
    parser.self_check()
    return manifest, references, selection


def host_preflight(path):
    """Require a fresh host receipt and the unchanged 19073 MiB job ceiling."""
    row = json.loads(Path(path).read_text())
    free = int(row["MemAvailable"].split()[-2]) * 1024
    age = time.time() - float(row["unix"])
    assert row.get("actual_host_proc") is True and row.get("sample_seconds", 0) >= 3
    assert 0 <= age <= 180 and row.get("active_OWNED_heavy_processes") == []
    assert free >= 19_999_490_048 and row.get("loadavg")
    return row


def load_candidate(build_root, case_root, reference_root):
    """Load the original G1 runner retargeted to a real later build and fresh refs."""
    build_root, case_root = Path(build_root).resolve(), Path(case_root).resolve()
    label = build_root.name
    assert re.fullmatch(r"build[0-9]+", label), label
    source = BASE.read_text()
    assert "build534" in source
    source = source.replace("build534", label)
    old_report_hash = 'sha(N10 / f"G1_reference_{cohort}.json")'
    assert source.count(old_report_hash) == 1
    source = source.replace(old_report_hash, 'sha(Path(manifest["cohorts"][cohort]["report"]))', 1)
    module = types.ModuleType("cut_g1_candidate")
    module.__file__ = str(Path(__file__).resolve())
    exec(compile(source, str(BASE), "exec"), module.__dict__)
    module.HERE, module.A2 = case_root, case_root
    module.N8, module.NIGHT, module.STAGED = N8, NIGHT, STAGED
    module.REPO, module.N10 = STAGED.parents[3], N10
    module.BUILD = build_root
    module.READY = case_root / "validation_run_ready.json"
    module.HANDLES, module.HOUSE = build_root / "terminal_handles.json", STAGED / "run_cases.py"
    module.RUNTIME, module.SELECTION, module.PARSER_PATH = RUNTIME, SELECTION, PARSER_PATH
    module.MANIFEST = case_root / "G1_reference536_manifest.json"
    module.verify_host_preflight = host_preflight
    module.verify_references = lambda parser: fresh_reference_bundle(reference_root, parser)
    return module


def ready_receipt(module, status, reference_root):
    """Freeze build, source, runtime, fresh reference tables, and helper provenance."""
    reference_manifest = Path(reference_root) / "G1_shared_reference_manifest.json"
    fresh = json.loads(reference_manifest.read_text())
    reports = [Path(fresh["cohorts"][name]["report"]) for name in CASE_COUNTS]
    terminals = [Path(fresh["cohorts"][name]["terminal_receipt"]) for name in CASE_COUNTS]
    paths = [BASE, N11_G1, PARSER_PATH, SELECTION, RUNTIME, SOURCE_CHECK, DESIGN,
             module.MANIFEST,
             module.BUILD / "build_status.json", module.BUILD / "source_pins.json",
             module.BUILD / "generated_build_graph_after.json", module.BUILD / "terminal_handles.json",
             reference_manifest, *reports, *terminals, Path(__file__).resolve()]
    value = {"run_ready": True, "build_status_sha256": sha(module.BUILD / "build_status.json"),
             "source_pins_sha256": sha(module.BUILD / "source_pins.json"),
             "helper_sha256": {str(path.resolve()): sha(path) for path in paths},
             "validation_helper_sha256": {str(Path(__file__).resolve()): sha(__file__)},
             "test_binary_sha256": status["test_binary_sha256"],
             "reference_build": 536, "wall_case_exceptions": []}
    target = module.READY
    if target.exists():
        assert json.loads(target.read_text()) == value, "candidate G1 ready receipt is frozen"
    else:
        target.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    return target


def save_reference_manifest(module, manifest, reference_root):
    """Seal the complete fresh536 report/time set beneath the candidate output root."""
    source = Path(reference_root) / "G1_shared_reference_manifest.json"
    payload = json.loads(source.read_text())
    payload["source_manifest"] = str(source.resolve())
    payload["source_manifest_sha256"] = sha(source)
    path = module.HERE / "G1_reference536_manifest.json"
    if path.exists():
        assert json.loads(path.read_text()) == payload, "candidate reference manifest is frozen"
    else:
        path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")
    module.MANIFEST = path
    return path


def wrapper_for(build_root, case_root, reference_root):
    """Bind the frozen N11 process monitor to the new candidate G1 environment."""
    runner = load_module("cut_n11_g1_monitor", N11_G1)
    runner.HERE, runner.N11, runner.STAGED = Path(case_root), N11, STAGED
    runner.REPO, runner.NIGHT, runner.N8, runner.N10 = STAGED.parents[3], NIGHT, N8, N10
    runner.BUILD, runner.BASE, runner.PARSER = Path(build_root), BASE, PARSER_PATH
    runner.load_candidate = lambda: load_candidate(build_root, case_root, reference_root)
    runner.ready_receipt = lambda module, status: ready_receipt(module, status, reference_root)
    runner.load_aliases = lambda: types.SimpleNamespace(
        verify_and_publish=lambda: publish_candidate_build(build_root, case_root, reference_root))
    runner.WALL_EXCEPTIONS = frozenset()
    runner.exception_inventory = lambda: {"allowed_cases": [], "scope": "zero wall-case exceptions"}
    return runner


def publish_candidate_build(build_root, case_root, reference_root):
    """Write a ready receipt first, then verify and expose the supplied build."""
    module = load_candidate(build_root, case_root, reference_root)
    parser = module.load_module("cut_g1_alias_parser", PARSER_PATH)
    manifest, _, _ = module.verify_references(parser)
    save_reference_manifest(module, manifest, reference_root)
    status = json.loads((Path(build_root) / "build_status.json").read_text())
    ready_receipt(module, status, reference_root)
    status, pins, _, _ = module.verify_build()
    module.expose_build_alias(status, pins)
    return {"build": str(build_root), "binary": status["binary"],
            "test_binary": status["test_binary"], "test_binary_sha256": status["test_binary_sha256"]}


def run_cohort(cohort, build_root, case_root, reference_root, host_path):
    """Run one strict identity cohort and preserve evidence before returning a gate result."""
    Path(case_root).mkdir(parents=True, exist_ok=True)
    module = load_candidate(build_root, case_root, reference_root)
    manifest, references, selection = module.verify_references(
        module.load_module("cut_g1_parser", PARSER_PATH))
    save_reference_manifest(module, manifest, reference_root)
    status = json.loads((Path(build_root) / "build_status.json").read_text())
    host_preflight(host_path)
    ready_receipt(module, status, reference_root)
    runner = wrapper_for(build_root, case_root, reference_root)
    summary_path = Path(case_root) / f"G1_candidate_{cohort}_summary.json"
    parsed_path = Path(case_root) / f"G1_candidate_{cohort}.json"
    wrapper_exit = None
    try:
        runner.run_cohort(cohort, host_path)
    except SystemExit as error:
        if not summary_path.is_file() or not parsed_path.is_file():
            raise
        wrapper_exit = int(error.code or 0)
    summary = json.loads(summary_path.read_text())
    result = {"cohort": cohort, "ranks": summary["ranks"],
              "case_records": len(json.loads(parsed_path.read_text())["case_results"]),
              "reference_build": 536, "candidate_build": Path(build_root).name,
              "zero_wall_case_exceptions": summary.get("wall_exceptions", {}).get("allowed_cases") == [],
              "identity_pass": summary["identity_pass"],
              "reporter_totals_equal": summary["reporter_totals_equal"],
              "changed_case_count": summary["changed_case_count"],
              "first_difference": summary["first_difference"],
              "reference_seconds": manifest["cohorts"][cohort]["seconds"],
              "wrapper_timeout_seconds": summary["timeout_seconds"],
              "candidate_elapsed_seconds": summary["elapsed_wall_seconds"],
              "controller_returncode": summary["controller_returncode"],
              "reference_returncode": manifest["cohorts"][cohort]["process_returncode"],
              "candidate_summary": str(summary_path), "candidate_report": str(parsed_path),
              "candidate_case": summary["candidate_attempt"], "host_preflight": str(host_path),
              "terminal_handles": str(Path(summary["candidate_attempt"]) / "n11_terminal_receipt.json"),
              "zero_exception_scope": "strict full G1 table; no wall-case allowance"}
    gate = bool(result["identity_pass"] and result["reporter_totals_equal"] and
                result["zero_wall_case_exceptions"] and
                result["case_records"] == summary["expected_cases_per_rank"] * summary["ranks"])
    result["gate_passed"] = gate
    out = Path(case_root) / f"G1_cut_candidate_{cohort}.json"
    out.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    result["result_path"] = str(out)
    result["result_sha256"] = sha(out)
    result["wrapper_exit"] = wrapper_exit
    return result


def self_check():
    """Exercise strict synthetic comparison and real build/reference path binding, no launch."""
    parser = load_module("cut_g1_parser_selfcheck", PARSER_PATH)
    parser.self_check()
    ref_case = {"rank": "0", "name": "historical failure", "success": "false",
        "assertions": [{"success": "false", "type": "CHECK", "file": "fixture.cpp",
            "line": "12", "sections": [], "original": "value == expected",
            "expanded": "3.0 == 4.0", "xml_source": "synthetic.xml"}]}
    passed = json.loads(json.dumps(ref_case))
    passed["success"] = "true"
    passed["assertions"] = []
    comparison = parser.compare({"case_results": [ref_case], "reporter_totals": []},
                                {"case_results": [passed], "reporter_totals": []}, set())
    assert comparison["identity_pass"] is False
    reference_root = PHASE / "reference_validation"
    build_root = PHASE / "build536"
    with tempfile.TemporaryDirectory(prefix="g1_candidate_gate_check_") as temporary:
        case_root = Path(temporary) / "not_created_yet"
        assert not case_root.exists()
        module = load_candidate(build_root, case_root, reference_root)
        manifest, reports, selection = module.verify_references(parser)
        case_root.mkdir(parents=True)
        saved_manifest = save_reference_manifest(module, manifest, reference_root)
        status = json.loads((build_root / "build_status.json").read_text())
        alias = publish_candidate_build(build_root, case_root, reference_root)
        assert alias["test_binary"] == status["test_binary"]
        status, pins, graph, handles = module.verify_build()
        module.expose_build_alias(status, pins)
        module.verify_host_preflight = lambda _path: {"self_check": True}
        case, house = module.prepare_house(
            "serial", 1, math.ceil(manifest["cohorts"]["serial"]["seconds"] * 1.5),
            selection, status, pins, reference_root / "unused_preflight.json")
        assert case == "N8_G1_candidate_serial" and house["ROOT"] == case_root
        assert house["BINARY"] == Path(status["test_binary"]).resolve()
        assert house["CASES"][case] == (1, selection["serial_filter"])
        assert Path(status["test_binary"]).is_file()
        assert len(reports["serial"]["case_results"]) == 375
        assert saved_manifest.is_file() and graph["test_binary_sha256"] == status["test_binary_sha256"]
        assert handles["all_owned_handles_terminal"] is True
        assert pins
        timeouts = {name: math.ceil(row["seconds"] * 1.5)
                    for name, row in manifest["cohorts"].items()}
    monitor_setup = self_check_monitor_setup(build_root, reference_root)
    new_root_setup = self_check_new_case_root(build_root, reference_root)
    assert CASE_COUNTS == {"serial": 375, "mpi2": 57, "mpi4": 57}
    return {"passed": True, "launched": False, "reference": "fresh canonical build536 manifest and reports",
            "zero_exceptions": True, "failed_to_pass_is_difference": True,
            "case_counts_per_rank": CASE_COUNTS, "timeouts_seconds": timeouts,
            "bound_build": str(build_root), "bound_binary": status["test_binary"],
            "bound_case_root": "temporary namespace; no case launch",
            "monitor_setup": monitor_setup,
            "new_case_root_setup": new_root_setup,
            "reference_manifest_sha256": sha(reference_root / "G1_shared_reference_manifest.json")}


def self_check_monitor_setup(build_root, reference_root):
    """Exercise monitor alias, ready, references, instrumentation, and house setup without launch."""
    with tempfile.TemporaryDirectory(prefix="g1_candidate_monitor_check_") as temporary:
        case_root = Path(temporary) / "new_case_root"
        case_root.mkdir()
        module = load_candidate(build_root, case_root, reference_root)
        module.verify_host_preflight = lambda _path: {"self_check": True}
        runner = wrapper_for(build_root, case_root, reference_root)
        runner.load_candidate = lambda: module
        runner.verify_host = lambda _path, _module: {"self_check": True}
        entered = {"prepared": False, "instrumented": False}

        def stop_before_launch(*args):
            cohort, parser, manifest, _, selection, status, pins = args[:7]
            host_path = args[-1]
            ranks = {"serial": 1, "mpi2": 2, "mpi4": 4}[cohort]
            timeout = math.ceil(manifest["cohorts"][cohort]["seconds"] * 1.5)
            case, house = module.prepare_house(cohort, ranks, timeout, selection,
                                               status, pins, host_path)
            entered["prepared"] = case == "N8_G1_candidate_serial"
            entered["instrumented"] = house["main"].__name__ == "run"
            assert house["BINARY"] == Path(status["test_binary"]).resolve()
            summary = {"non_wall_differences": []}
            (case_root / f"G1_candidate_{cohort}_summary.json").write_text(
                json.dumps(summary) + "\n")

        module.run_cohort = stop_before_launch
        result = runner.run_cohort("serial", case_root / "unused_host_receipt.json")
        assert case_root.is_dir() and entered == {"prepared": True, "instrumented": True}
        assert result["non_wall_differences"] == []
        return {"passed": True, "launched": False, "case_root_created": True,
                "alias_ready_reference_instrument_setup": True,
                "candidate_summary_stubbed_before_test_driver": True}


def self_check_new_case_root(build_root, reference_root):
    """Verify top-level setup creates a fresh case root before saving its manifest."""
    global wrapper_for
    with tempfile.TemporaryDirectory(prefix="g1_candidate_newroot_check_") as temporary:
        root = Path(temporary) / "nonexistent_case_root"
        host = Path(temporary) / "host.json"
        host.write_text(json.dumps({"MemAvailable": "MemAvailable: 30000000 kB",
            "unix": time.time(), "actual_host_proc": True, "sample_seconds": 3,
            "active_OWNED_heavy_processes": [], "loadavg": "0.0 0.0 0.0"}))
        original = wrapper_for

        class SetupReached(RuntimeError):
            pass

        def stop_after_setup(*_args):
            assert root.is_dir()
            assert (root / "G1_reference536_manifest.json").is_file()
            assert (root / "validation_run_ready.json").is_file()
            raise SetupReached("stopped before monitor launch")

        wrapper_for = lambda *_args: types.SimpleNamespace(run_cohort=stop_after_setup)
        try:
            run_cohort("serial", build_root, root, reference_root, host)
        except SetupReached:
            pass
        finally:
            wrapper_for = original
        assert root.is_dir()
        return {"passed": True, "nonexistent_case_root_created_before_manifest": True,
                "monitor_launch": False}


def main():
    """Run the requested G1 cohort against the fresh build536 reference tables."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cohort", choices=tuple(CASE_COUNTS), nargs="?")
    parser.add_argument("--build-root", type=Path, default=PHASE / "build537")
    parser.add_argument("--case-root", type=Path, default=HERE)
    parser.add_argument("--reference-root", type=Path, default=PHASE / "reference_validation")
    parser.add_argument("--host-preflight", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if not args.cohort or args.host_preflight is None:
        parser.error("one cohort and its current-host --host-preflight are required")
    result = run_cohort(args.cohort, args.build_root, args.case_root,
                        args.reference_root, args.host_preflight)
    print(json.dumps(result, indent=2))
    if not result["gate_passed"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
