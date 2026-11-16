#!/usr/bin/env python3
"""Run one G1 cohort against build535 with named wall-case exceptions only."""
import argparse
import importlib.util
import json
import sys
import types
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parent
STAGED = N11.parents[1]
REPO = STAGED.parents[3]
NIGHT = STAGED / "night_20261115"
N8 = NIGHT / "N8"
A2 = N8 / "A2_A3_N12/section6_8"
N10 = NIGHT / "N10"
BUILD = N11 / "build535"
BASE = A2 / "validation/run_g1_candidate.py"
PARSER = N10 / "parse_g1_identity.py"
DIRECT_WALL_CASES = (
    "Requested-wall preparation uses construction ownership without claiming final quality",
    "Complete wall demand precedes a feasible optional collapse",
    "Known early wall debt does not wait for the complete entry census",
    "Idle surface census advances without draining ahead of an active round",
    "Surface preparation closes requested-wall coverage without certifying unrelated volume",
)
INDIRECT_WALL_CASES = (
    "DC-14 FIRST_ROW entry: two wall markers, two symmetry planes, fresh event then halving event",
)
WALL_EXCEPTIONS = frozenset((*DIRECT_WALL_CASES, *INDIRECT_WALL_CASES))


def sha(path):
    """Hash one exact provenance file."""
    import hashlib
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def load_candidate():
    """Load inherited G1 functions and bind every run path to N11/build535."""
    source = BASE.read_text()
    assert "build534" in source
    source = source.replace("build534", "build535")
    anchor = 'sys.argv = [str(HOUSE), "build534", case]'
    anchor = 'sys.argv = [str(HOUSE), "build535", case]'
    assert source.count(anchor) == 1, anchor
    module = types.ModuleType("n11_g1_base")
    module.__file__ = str(BASE)
    exec(compile(source, str(BASE), "exec"), module.__dict__)
    module.__file__ = str(Path(__file__).resolve())
    module.HERE, module.STAGED, module.REPO = HERE, STAGED, REPO
    module.NIGHT, module.N8, module.A2, module.N10 = NIGHT, N8, A2, N10
    module.BUILD, module.READY, module.PARSER_PATH = BUILD, HERE / "validation_run_ready.json", PARSER
    module.HANDLES = BUILD / "terminal_handles.json"
    module.MANIFEST = N10 / "G1_shared_reference_manifest.json"
    module.SELECTION = N10 / "G1_actual_selection.json"
    return module


def exception_inventory():
    """Describe the exact direct and indirect N11 wall cases permitted to differ."""
    return {"direct_NativeSurfacePreparationEngine3D": list(DIRECT_WALL_CASES),
            "indirect_NativeBL14Entry": list(INDIRECT_WALL_CASES),
            "allowance_scope": "exact Catch2 testcase names only; no whole-tag allowance"}


def ready_receipt(module, status):
    """Seal current build and G1 reference inputs before a cohort launch."""
    paths = [BASE, PARSER, module.MANIFEST, module.SELECTION,
             *(N10 / f"G1_reference_{cohort}.json" for cohort in ("serial", "mpi2", "mpi4")),
             *(N10 / f"G1_{cohort}_primary_terminal_receipt.json" for cohort in ("serial", "mpi2", "mpi4")),
             module.RUNTIME, BUILD / "build_status.json", BUILD / "source_pins.json",
             BUILD / "generated_build_graph_after.json", BUILD / "terminal_handles.json"]
    value = {"run_ready": True, "build_status_sha256": sha(BUILD / "build_status.json"),
             "source_pins_sha256": sha(BUILD / "source_pins.json"),
             "helper_sha256": {str(path.resolve()): sha(path) for path in paths},
             "validation_helper_sha256": {str(Path(__file__).resolve()): sha(__file__)},
             "test_binary_sha256": status["test_binary_sha256"],
             "wall_exception_inventory": exception_inventory()}
    target = module.READY
    if target.exists():
        assert json.loads(target.read_text()) == value, "N11 G1 ready receipt is frozen"
    else:
        target.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    return target


def install_named_exceptions(module, parser_module):
    """Patch only the inherited comparator call to allow the six listed names."""
    compare = parser_module.compare
    comparisons = []

    def compare_named(reference, candidate, _ignored):
        result = compare(reference, candidate, WALL_EXCEPTIONS)
        comparisons.append(result)
        return result

    parser_module.compare = compare_named
    save = module.save_reports

    def save_report(cohort, parsed, summary):
        result = comparisons[-1]
        summary["wall_exceptions"] = exception_inventory()
        summary["wall_exception_differences"] = [row for row in result["changes"]
                                                  if row["case"] in WALL_EXCEPTIONS]
        summary["non_wall_differences"] = result["unexpected_changes"]
        summary["reporter_totals_equal"] = result["reporter_totals_equal"]
        save(cohort, parsed, summary)

    module.save_reports = save_report


def verify_host(path, module):
    """Require fresh actual-host preflight and enough free memory for the job cap."""
    return module.verify_host_preflight(path)


def process_identity(pid):
    """Return a process PID, state, and stable host start tick if it exists."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "state": fields[0], "start_ticks": int(fields[19])}
    except (OSError, ValueError, IndexError):
        return None


def terminal_rows(owned):
    """Mark saved identities live only when PID and start tick still match."""
    rows = []
    for saved in owned.values():
        current = process_identity(saved["pid"])
        alive = bool(current and current["start_ticks"] == saved["start_ticks"]
                     and current["state"] not in ("Z", "X"))
        rows.append(dict(saved, alive=alive))
    return rows


def instrument_g1_case(module):
    """Record the test process tree and terminal identities in each cohort case."""
    original_prepare = module.prepare_house

    def prepare(*args):
        case, house = original_prepare(*args)
        owned = {}
        original_sample = house["sample_tree"]
        original_popen = house["subprocess"].Popen

        def sample(root_pid):
            rows = original_sample(root_pid)
            for row in rows:
                current = process_identity(row["pid"])
                if current:
                    owned[(current["pid"], current["start_ticks"])] = dict(current, name=row["name"])
            return rows

        def launch(*args, **kwargs):
            process = original_popen(*args, **kwargs)
            current = process_identity(process.pid)
            if current:
                owned[(current["pid"], current["start_ticks"])] = dict(current, name="test_driver")
            return process

        house["sample_tree"] = sample
        house["subprocess"].Popen = launch
        original_main = house["main"]

        def run(*args, **kwargs):
            try:
                return original_main(*args, **kwargs)
            finally:
                house["subprocess"].Popen = original_popen
                folders = sorted((module.HERE / "cases").glob(case + "_*"),
                                 key=lambda item: item.stat().st_mtime)
                if folders:
                    rows = terminal_rows(owned)
                    peak = max((json.loads(line).get("sampled_process_tree_rss_bytes", 0)
                                for line in (folders[-1] / "resources.jsonl").read_text().splitlines()),
                               default=0)
                    receipt = {"case": str(folders[-1]), "owned_processes": rows,
                               "all_owned_handles_terminal": bool(rows) and not any(r["alive"] for r in rows),
                               "sampled_job_peak_rss_bytes": peak, "job_rss_limit_bytes": 19_999_490_048,
                               "job_rss_within_limit": peak <= 19_999_490_048}
                    (folders[-1] / "n11_terminal_receipt.json").write_text(
                        json.dumps(receipt, indent=2) + "\n")
                    assert receipt["all_owned_handles_terminal"] and receipt["job_rss_within_limit"]
        house["main"] = run
        return case, house

    module.prepare_house = prepare


def run_cohort(cohort, host_path):
    """Run one serial, two-rank, or four-rank G1 candidate cohort."""
    module = load_candidate()
    aliases = load_aliases()
    aliases.verify_and_publish()
    build_status = json.loads((BUILD / "build_status.json").read_text())
    verify_host(host_path, module)
    ready_receipt(module, build_status)
    parser_module = module.load_module("n11_g1_identity", PARSER)
    manifest, reports, selection = module.verify_references(parser_module)
    status, pins, graph, handles = module.verify_build()
    runtime = module.check_runtime()
    module.expose_build_alias(status, pins)
    install_named_exceptions(module, parser_module)
    instrument_g1_case(module)
    module.run_cohort(cohort, parser_module, manifest, reports, selection,
                      status, pins, runtime, graph, handles, host_path)
    summary = json.loads((HERE / f"G1_candidate_{cohort}_summary.json").read_text())
    assert not summary["non_wall_differences"], summary["non_wall_differences"]
    return summary


def load_aliases():
    """Load the dynamic build535 alias verifier without publishing it."""
    alias_path = N11 / "build535_aliases.py"
    spec = importlib.util.spec_from_file_location("n11_g1_build_aliases", alias_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def self_check():
    """Check retargeted references, exact named exceptions, and no-launch behavior."""
    import tempfile
    module = load_candidate()
    result = module.self_check()
    assert result["passed"] and not result["launched"]
    assert module.HERE == HERE and module.BUILD == BUILD and module.READY.parent == HERE
    parser_module = module.load_module("n11_g1_identity_check", PARSER)
    install_named_exceptions(module, parser_module)
    assert len(DIRECT_WALL_CASES) == 5 and len(INDIRECT_WALL_CASES) == 1
    assert WALL_EXCEPTIONS == set((*DIRECT_WALL_CASES, *INDIRECT_WALL_CASES))
    aliases = load_aliases()
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        module.HERE, module.READY = root, root / "validation_run_ready.json"
        status = json.loads((BUILD / "build_status.json").read_text())
        ready_receipt(module, status)
        checked = module.verify_build()
        status, pins = checked[0], checked[1]
        module.expose_build_alias(status, pins)
        _, _, selection = module.verify_references(parser_module)
        module.verify_host_preflight = lambda _path: {"mocked": True}
        module.expose_build_alias = lambda *_args: None
        case, house = module.prepare_house(
            "serial", 1, 1, selection, status, pins,
            A2 / "validation/G1_serial_host_preflight.json")
        assert case == "N8_G1_candidate_serial" and house["ROOT"] == root
        assert house["BINARY"] == Path(status["test_binary"])
    module.HERE, module.READY = HERE, HERE / "validation_run_ready.json"
    assert aliases.verify_and_publish.__globals__["HERE"] == HERE
    return {"passed": True, "launched": False, "build": str(BUILD),
            "base_runner": str(BASE), "exceptions": exception_inventory()}


def main():
    """Dispatch one independently sealed G1 cohort or a read-only self-check."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cohort", choices=("serial", "mpi2", "mpi4"), nargs="?")
    parser.add_argument("--host-preflight", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
    else:
        if not args.cohort or args.host_preflight is None:
            parser.error("launch requires one cohort and --host-preflight")
        print(json.dumps(run_cohort(args.cohort, args.host_preflight), indent=2))


if __name__ == "__main__":
    main()
