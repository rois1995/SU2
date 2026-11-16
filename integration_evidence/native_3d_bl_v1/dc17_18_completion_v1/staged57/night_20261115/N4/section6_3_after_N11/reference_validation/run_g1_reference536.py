#!/usr/bin/env python3
"""Record fresh zero-exception G1 baseline cohorts against build536."""
import argparse
import hashlib
import importlib.util
import json
import math
import sys
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
BUILD = PHASE / "build536"
BASE = N8 / "A2_A3_N12/section6_8/validation/run_g1_candidate.py"
N11_G1 = N11 / "validation/run_n11_g1.py"
ALIAS_PATH = PHASE / "build536_aliases.py"
PARSER_PATH = N10 / "parse_g1_identity.py"
MANIFEST = N10 / "G1_shared_reference_manifest.json"
SELECTION = N10 / "G1_actual_selection.json"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
WRAPPER = Path(__file__).resolve()
CASE_COUNTS = {"serial": 375, "mpi2": 57, "mpi4": 57}


def sha(path):
    """Hash one immutable input or recorded output."""
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def load_module(name, path):
    """Load a frozen helper from its explicit path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def load_candidate():
    """Load the original G1 comparator with only the build label and paths retargeted."""
    source = BASE.read_text()
    assert "build534" in source
    source = source.replace("build534", "build536")
    module = types.ModuleType("n4_g1_candidate536")
    module.__file__ = str(WRAPPER)
    exec(compile(source, str(BASE), "exec"), module.__dict__)
    module.HERE, module.A2 = HERE, HERE
    module.N8, module.NIGHT, module.STAGED = N8, NIGHT, STAGED
    module.REPO, module.N10 = STAGED.parents[3], N10
    module.BUILD, module.READY = BUILD, HERE / "validation_run_ready.json"
    module.HANDLES, module.HOUSE = BUILD / "terminal_handles.json", STAGED / "run_cases.py"
    module.RUNTIME, module.MANIFEST, module.SELECTION = RUNTIME, MANIFEST, SELECTION
    module.PARSER_PATH = PARSER_PATH
    module.verify_host_preflight = verify_host_preflight
    return module


def verify_host_preflight(path):
    """Require a fresh actual-host check with the unchanged whole-job memory cap."""
    receipt = json.loads(Path(path).read_text())
    free = int(receipt["MemAvailable"].split()[-2]) * 1024
    age = time.time() - float(receipt["unix"])
    assert receipt.get("actual_host_proc") is True and receipt.get("sample_seconds", 0) >= 3
    assert 0 <= age <= 180 and receipt.get("active_OWNED_heavy_processes") == []
    assert free >= 19_999_490_048 and receipt.get("loadavg")
    return receipt


def load_n11_wrapper():
    """Bind the frozen N11 process monitor and G1 flow to the new phase roots."""
    wrapper = load_module("n4_frozen_n11_g1", N11_G1)
    wrapper.HERE, wrapper.N11, wrapper.STAGED, wrapper.REPO = HERE, N11, STAGED, STAGED.parents[3]
    wrapper.NIGHT, wrapper.N8, wrapper.N10, wrapper.BUILD = NIGHT, N8, N10, BUILD
    wrapper.BASE, wrapper.PARSER = BASE, PARSER_PATH
    wrapper.N11_G1_HELPER = N11_G1
    wrapper.load_candidate = load_candidate
    wrapper.load_aliases = lambda: load_module("n4_build536_aliases", ALIAS_PATH)
    wrapper.WALL_EXCEPTIONS = frozenset()
    wrapper.exception_inventory = lambda: {"allowed_cases": [], "scope": "zero wall-case exceptions"}
    return wrapper


def ready_receipt(module, status):
    """Seal build536, historical 528 references, frozen helpers, and this wrapper."""
    paths = [BASE, PARSER_PATH, MANIFEST, SELECTION, N10 / "run_g1_reference.py",
             *(N10 / f"G1_reference_{name}.json" for name in CASE_COUNTS),
             *(N10 / f"G1_{name}_primary_terminal_receipt.json" for name in CASE_COUNTS),
             N11_G1, ALIAS_PATH, RUNTIME, BUILD / "build_status.json", BUILD / "source_pins.json",
             BUILD / "generated_build_graph_after.json", BUILD / "terminal_handles.json", WRAPPER]
    helper_hashes = {str(path.resolve()): sha(path) for path in paths}
    value = {"run_ready": True, "build_status_sha256": sha(BUILD / "build_status.json"),
             "source_pins_sha256": sha(BUILD / "source_pins.json"),
             "helper_sha256": helper_hashes,
             "validation_helper_sha256": {str(WRAPPER): sha(WRAPPER)},
             "test_binary_sha256": status["test_binary_sha256"],
             "identity_bootstrap": {"manifest_sha256": sha(MANIFEST), "build": 528,
                                     "wall_case_exceptions": []}}
    target = module.READY
    if target.exists():
        assert json.loads(target.read_text()) == value, "fresh G1 ready receipt is frozen"
    else:
        target.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    return target


def old_assertion_totals(report):
    """Summarize the historical report's case and assertion outcomes."""
    cases = report["case_results"]
    failed = [row for row in cases if row.get("success") == "false"]
    assertions = [item for row in cases for item in row.get("assertions", [])]
    failed_assertions = [row for row in assertions if row.get("success") == "false"]
    return {"cases": len(cases), "failed_cases": len(failed),
            "assertions": len(assertions), "failed_assertions": len(failed_assertions)}


def run_cohort(cohort, host_preflight):
    """Run one fresh cohort and preserve its comparison even when historical assertions fail."""
    n11 = load_n11_wrapper()
    alias = load_module("n4_build536_aliases_runtime", ALIAS_PATH)
    alias_result = alias.verify_and_publish()
    candidate = load_candidate()
    status, pins, graph, handles = candidate.verify_build() if candidate.READY.exists() else (None, None, None, None)
    if status is None:
        status = json.loads((BUILD / "build_status.json").read_text())
        ready_receipt(candidate, status)
        status, pins, graph, handles = candidate.verify_build()
    n11.ready_receipt = ready_receipt
    summary_path = HERE / f"G1_candidate_{cohort}_summary.json"
    parsed_path = HERE / f"G1_candidate_{cohort}.json"
    runner_exit = None
    try:
        n11.run_cohort(cohort, host_preflight)
    except SystemExit as error:
        if not summary_path.is_file() or not parsed_path.is_file():
            raise
        runner_exit = int(error.code or 0)
    summary, parsed = json.loads(summary_path.read_text()), json.loads(parsed_path.read_text())
    reference = json.loads((N10 / f"G1_reference_{cohort}.json").read_text())
    folder = Path(summary["candidate_attempt"])
    receipt_path = folder / "n11_terminal_receipt.json"
    output = {"cohort": cohort, "ranks": summary["ranks"],
              "expected_records": CASE_COUNTS[cohort], "zero_wall_case_exceptions": True,
              "build536_status_sha256": sha(BUILD / "build_status.json"),
              "test_binary_sha256": status["test_binary_sha256"], "alias_result": alias_result,
              "reference_identity_build": 528, "reference_report_sha256": sha(N10 / f"G1_reference_{cohort}.json"),
              "old_assertion_numbers": old_assertion_totals(reference),
              "fresh_assertion_numbers": old_assertion_totals(parsed),
              "elapsed_seconds": summary["elapsed_wall_seconds"],
              "controller_returncode": summary["controller_returncode"],
              "reference_wrapper_exit": runner_exit,
              "identity_pass": summary["identity_pass"], "changed_case_count": summary["changed_case_count"],
              "first_difference": summary["first_difference"],
              "fresh_parsed_report": str(parsed_path), "fresh_parsed_report_sha256": sha(parsed_path),
              "fresh_summary": str(summary_path), "fresh_summary_sha256": sha(summary_path),
              "candidate_case": str(folder), "terminal_handles": str(receipt_path),
              "terminal_handles_sha256": sha(receipt_path), "candidate_run_sha256": summary["candidate_run_sha256"],
              "reference_receipt": summary["primary_run_ready"], "ready_sha256": sha(candidate.READY)}
    dest = HERE / f"G1_reference536_{cohort}.json"
    dest.write_text(json.dumps(output, indent=2, sort_keys=True) + "\n")
    return output


def self_check():
    """Execute alias, build/reference verification, and case setup through a stub main."""
    alias = load_module("n4_build536_aliases_selfcheck", ALIAS_PATH)
    alias_result = alias.self_check()
    published = alias.verify_and_publish()
    n11 = load_n11_wrapper()
    candidate = load_candidate()
    status, pins, graph, handles = candidate.verify_build() if candidate.READY.exists() else (None, None, None, None)
    if status is None:
        status = json.loads((BUILD / "build_status.json").read_text())
        ready_receipt(candidate, status)
        status, pins, graph, handles = candidate.verify_build()
    parser = candidate.load_module("n4_g1_parser_selfcheck", PARSER_PATH)
    manifest, reports, selection = candidate.verify_references(parser)
    assert manifest["build"] == 528 and set(selection["serial_filter"] for _ in [0])
    assert len(reports["serial"]["case_results"]) == 375
    assert all(len(reports[name]["case_results"]) == 57 * ranks
               for name, ranks in (("mpi2", 2), ("mpi4", 4)))
    candidate.verify_host_preflight = lambda _path: {"stub": True}
    timeout = math.ceil(manifest["cohorts"]["serial"]["seconds"] * 1.5)
    case, house = candidate.prepare_house("serial", 1, timeout, selection, status, pins,
        N8 / "A2_A3_N12/section6_8/validation/G1_serial_host_preflight.json")
    assert case == "N8_G1_candidate_serial" and house["ROOT"] == HERE
    assert Path(house["BINARY"]) == Path(status["test_binary"])
    assert house["CASES"][case] == (1, selection["serial_filter"])
    argv = [str(candidate.HOUSE), "build536", case]
    seen = {}
    def stub_main(runtime):
        seen.update(argv=argv, runtime=str(runtime), root=str(house["ROOT"]),
                    binary=str(house["BINARY"]), ranks=house["CASES"][case][0])
        return 0
    house["main"] = stub_main
    house["main"](RUNTIME)
    assert seen["ranks"] == 1 and seen["runtime"] == str(RUNTIME)
    return {"passed": True, "launched": False, "alias_self_check": alias_result,
            "published_aliases": published, "build_sha256": status["test_binary_sha256"],
            "serial_cases": len(reports["serial"]["case_results"]),
            "mpi2_cases": len(reports["mpi2"]["case_results"]),
            "mpi4_cases": len(reports["mpi4"]["case_results"]), "stub_main": seen}


def main():
    """Launch exactly one fresh cohort after receiving its current-host preflight."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cohort", choices=tuple(CASE_COUNTS), nargs="?")
    parser.add_argument("--host-preflight", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if not args.cohort or args.host_preflight is None:
        parser.error("cohort launch requires a fresh --host-preflight receipt")
    result = run_cohort(args.cohort, args.host_preflight)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
