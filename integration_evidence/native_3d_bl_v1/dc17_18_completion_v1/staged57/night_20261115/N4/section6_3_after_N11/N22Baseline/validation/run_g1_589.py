#!/usr/bin/env python3
"""Run one N14 G1 cohort against the sealed fresh536 table."""
from __future__ import annotations
import argparse
import importlib.util
import json
import sys
import types
from pathlib import Path

P = Path(__file__).resolve().parents[2]
HERE = P / "N22Baseline/validation"
NIGHT = P.parent.parent
N14 = NIGHT / "N14/section6_12"
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
B3_VALIDATION = P / "B3/validation"
FRESH = B3_VALIDATION / "references"
N10 = NIGHT / "N10"
BUILD = P / "build589"
BASE = NIGHT / "N11/validation/run_n11_g1.py"


def load_module(name, path):
    """Load a Python helper by its frozen source path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def alias_reference_files():
    """Create read-only name aliases for fresh536 reports consumed by N11's runner."""
    target = HERE / "reference_aliases"
    target.mkdir(exist_ok=True)
    sources = {"G1_shared_reference_manifest.json": FRESH / "G1_shared_reference_manifest.json",
               "G1_actual_selection.json": N10 / "G1_actual_selection.json"}
    sources.update({f"G1_reference_{cohort}.json": FRESH / f"G1_candidate_{cohort}.json"
                    for cohort in ("serial", "mpi2", "mpi4")})
    sources["run_g1_reference.py"] = N14 / "validation/run_g1_552.py"
    manifest=json.loads((FRESH / "G1_shared_reference_manifest.json").read_text())
    sources.update({f"G1_{cohort}_primary_terminal_receipt.json":Path(row["primary_terminal_receipt"])
                    for cohort,row in manifest["cohorts"].items()})
    for name, source in sources.items():
        assert source.is_file(), source
        link = target / name
        if link.is_symlink():
            assert link.resolve(strict=True) == source.resolve(strict=True), link
        else:
            assert not link.exists(), f"unexpected preexisting alias: {link}"
            link.symlink_to(source.resolve(strict=True))
    return target


def configure_runner():
    """Retarget the N11 cohort engine while replacing only its reference bindings."""
    runner = load_module("n14_n11_g1_runner", BASE)
    refs = alias_reference_files()
    runner.HERE = HERE
    runner.N11 = N14
    runner.STAGED = STAGED
    runner.REPO = REPO
    runner.NIGHT = NIGHT
    runner.N8 = NIGHT / "N8"
    runner.A2 = runner.N8 / "A2_A3_N12/section6_8"
    runner.N10 = refs
    runner.BUILD = BUILD
    runner.BASE = runner.A2 / "validation/run_g1_candidate.py"
    runner.PARSER = N10 / "parse_g1_identity.py"
    runner.DIRECT_WALL_CASES = ()
    runner.INDIRECT_WALL_CASES = ()
    runner.WALL_EXCEPTIONS = frozenset()
    runner.load_aliases = lambda: load_module("b3_build589_aliases", P / "build589_aliases.py")

    def load_candidate():
        source = runner.BASE.read_text()
        assert "build534" in source
        source = source.replace("build534", "build589")
        anchor = 'sys.argv = [str(HOUSE), "build589", case]'
        assert source.count(anchor) == 1, anchor
        candidate = types.ModuleType("n14_g1_build589_candidate")
        candidate.__file__ = str(runner.BASE)
        exec(compile(source, str(runner.BASE), "exec"), candidate.__dict__)
        candidate.__file__ = str(Path(__file__).resolve())
        candidate.HERE, candidate.STAGED, candidate.REPO = HERE, STAGED, REPO
        candidate.NIGHT, candidate.N8, candidate.A2, candidate.N10 = NIGHT, runner.N8, runner.A2, refs
        candidate.BUILD = BUILD
        candidate.READY = HERE / "validation_run_ready.json"
        candidate.PARSER_PATH = runner.PARSER
        candidate.HANDLES = BUILD / "terminal_handles.json"
        candidate.MANIFEST = refs / "G1_shared_reference_manifest.json"
        candidate.SELECTION = refs / "G1_actual_selection.json"
        return candidate

    runner.load_candidate = load_candidate
    return runner


def verify_fresh536(parser):
    """Load the exact fresh536 cohort tables and frozen rank selections."""
    refs = alias_reference_files()
    manifest = json.loads((refs / "G1_shared_reference_manifest.json").read_text())
    assert manifest.get("state") in ("COMPLETE_FRESH_CUT_REFERENCE_C2_1", "N14_ACCEPTED_REFERENCE_SET")
    assert manifest.get("build") in (536, 552)
    selection = json.loads((refs / "G1_actual_selection.json").read_text())
    reports = {cohort: json.loads((refs / f"G1_reference_{cohort}.json").read_text())
               for cohort in ("serial", "mpi2", "mpi4")}
    expected = {"serial": 375, "mpi2": 114, "mpi4": 228}
    for cohort, report in reports.items():
        assert len(report.get("case_results", [])) == expected[cohort]
        assert manifest["cohorts"][cohort]["case_records"] == expected[cohort]
        assert parser.compare(report, report, set())["identity_pass"]
    return manifest, reports, selection


def ready_receipt(runner, module, status, cohort):
    """Freeze this cohort's adopted552 reference and complete helper provenance."""
    paths = [BASE, runner.BASE, runner.PARSER, FRESH / "G1_shared_reference_manifest.json",
             N10 / "G1_actual_selection.json", *(
                 FRESH / f"G1_candidate_{name}.json" for name in ("serial", "mpi2", "mpi4")),
             *(
                 Path(json.loads((FRESH / "G1_shared_reference_manifest.json").read_text())["cohorts"][name]["terminal_receipt"])
                 for name in ("serial", "mpi2", "mpi4")),
             runner.N10 / "run_g1_reference.py",
             *(runner.N10 / f"G1_{name}_primary_terminal_receipt.json" for name in ("serial", "mpi2", "mpi4")),
             module.RUNTIME, P / "build589_aliases.py", BUILD / "build_status.json",
             BUILD / "source_pins.json", BUILD / "generated_build_graph_after.json",
             BUILD / "terminal_handles.json", HERE / f"G1_{cohort}_host_preflight.json", Path(__file__)]
    value = {"run_ready": True, "build_status_sha256": runner.sha(BUILD / "build_status.json"),
             "source_pins_sha256": runner.sha(BUILD / "source_pins.json"),
             "helper_sha256": {str(path.resolve()): runner.sha(path) for path in paths},
             "validation_helper_sha256": {str(Path(__file__).resolve()): runner.sha(__file__)},
             "test_binary_sha256": status["test_binary_sha256"],
             "wall_exception_inventory": runner.exception_inventory(),
             "reference_case_table": str(FRESH / f"G1_candidate_{cohort}.json")}
    target = module.READY
    if target.exists():
        assert json.loads(target.read_text()) == value, "B3 G1 ready receipt is frozen"
    else:
        target.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    return target


def run_cohort(cohort):
    """Execute and summarize one fixed serial, MPI2, or MPI4 selection."""
    runner = configure_runner()
    module = runner.load_candidate()
    aliases = runner.load_aliases()
    aliases.verify_and_publish()
    status = json.loads((BUILD / "build_status.json").read_text())
    module.verify_host_preflight(HERE / f"G1_{cohort}_host_preflight.json")
    module.READY = HERE / f"validation_run_ready_{cohort}.json"
    ready_receipt(runner, module, status, cohort)
    parser_module = module.load_module("n14_g1_identity", runner.PARSER)
    manifest, reports, selection = verify_fresh536(parser_module)
    build_status, pins, graph, handles = module.verify_build()
    runtime = module.check_runtime()
    module.expose_build_alias(build_status, pins)
    runner.install_named_exceptions(module, parser_module)
    runner.instrument_g1_case(module)
    module.run_cohort(cohort, parser_module, manifest, reports, selection,
                      build_status, pins, runtime, graph, handles,
                      HERE / f"G1_{cohort}_host_preflight.json")
    result = json.loads((HERE / f"G1_candidate_{cohort}_summary.json").read_text())
    result["reference_report"] = str(FRESH / f"G1_candidate_{cohort}.json")
    result["reference_report_sha256"] = runner.sha(result["reference_report"])
    result["reference_manifest"] = str(FRESH / "G1_shared_reference_manifest.json")
    result["wallstage_changes_are_reported_by_exact_case_and_assertion"] = True
    (HERE / f"G1_candidate_{cohort}_summary.json").write_text(json.dumps(result, indent=2) + "\n")
    assert not result.get("non_wall_differences"), result.get("non_wall_differences")
    return result


def self_check():
    """Check paths, immutable reference aliases, and the bounded named exception set."""
    runner = configure_runner()
    module = runner.load_candidate()
    parser_module = module.load_module("n14_g1_identity_selfcheck", runner.PARSER)
    parser_module.self_check()
    manifest, reports, selection = verify_fresh536(parser_module)
    assert runner.BUILD == BUILD and runner.HERE == HERE
    assert len(runner.WALL_EXCEPTIONS) == 0
    return {"passed": True, "launched": False, "build": str(BUILD),
            "adopted552_reference": str(FRESH), "cohort_records":
            {name: len(report["case_results"]) for name, report in reports.items()},
            "wall_case_allowlist": sorted(runner.WALL_EXCEPTIONS)}


def main():
    """Run a selected cohort or the no-launch wiring check."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cohort", choices=("serial", "mpi2", "mpi4"))
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
    elif args.cohort:
        print(json.dumps(run_cohort(args.cohort), indent=2))
    else:
        parser.error("provide --cohort or --self-check")


if __name__ == "__main__":
    main()
