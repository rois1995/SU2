#!/usr/bin/env python3
"""Run N11 G3 once per action, with the 6.9 physical and ratio gates."""
import argparse
import importlib.util
import json
import os
import tempfile
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
BASE = A2 / "validation/validate_n8.py"
CASE_GLOB = "section6_2_bl_*"
PHYSICAL = HERE / "validate_n11.py"
PAIR = N11 / "section6_8_preparation/drafts/check_n11_g3_pair.py"


def load_module(name, path):
    """Load one existing N11 or section6_8 helper from its exact path."""
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def load_n8_validator():
    """Load inherited safeguard functions and bind all N11/accepted paths."""
    module = types.ModuleType("n11_n8_safeguard")
    module.__file__ = str(BASE)
    exec(compile(BASE.read_text(), str(BASE), "exec"), module.__dict__)
    module.HERE, module.STAGED, module.REPO = HERE, STAGED, REPO
    module.A2, module.N8, module.NIGHT = N11, N8, NIGHT
    module.SECTION1 = NIGHT / "section6_1"
    module.SECTION2 = NIGHT / "section6_2"
    module.BUILD = N11 / "build535"
    module.IDENTITY_PATH = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
    module.WALL_REFERENCE_LOG = NIGHT / "N9/section6_6/cases/N9_66_bl_20261116T135717_78861/run.log"
    module.prepare_build = lambda: load_module("n11_physical_helper", PHYSICAL).publish_build_aliases()
    original_adapt = module.adapted_safeguard_source

    def adapted(base, mode):
        source = original_adapt(base, mode)
        source = source.replace("build534", "build535")
        source = source.replace("STAGED=HERE.parents[4]", "STAGED=HERE.parents[2]")
        marker = "   gate_source=gate_source.replace('20GB serial peak gate','19073 MiB serial peak gate')\n"
        assert source.count(marker) == 1
        removals = (
            "    gates['at_least_336_splits']=row.get('splits',-1)>=336\n",
            "    gates['first_336_edits_identical']=len(reference_edits)>=336 and len(actual_edits)>=336 and reference_edits[:336]==actual_edits[:336]\n",
        )
        injected = "".join("   gate_source=gate_source.replace(" + repr(item) + ",'',1)\n"
                           for item in removals)
        source = source.replace(marker, marker + injected, 1)
        source = source.replace("from build535_aliases import main as expose_build",
                                "from build535_aliases import verify_and_publish as expose_build", 1)
        pin_anchor = "extra={str(OLD.resolve()):sha(OLD),str(Path(__file__).resolve()):sha(__file__)}"
        assert source.count(pin_anchor) == 1
        source = source.replace(pin_anchor,
            "extra={str(HOST_PREFLIGHT.resolve()):sha(HOST_PREFLIGHT),str(OLD.resolve()):sha(OLD),"
            "str(Path(__file__).resolve()):sha(__file__)}", 1)
        case_pin = "extra_pins={str(OLD.resolve()):sha(OLD),str(Path(__file__).resolve()):sha(__file__),str((HERE/'build535_aliases.py').resolve()):sha(HERE/'build535_aliases.py')}"
        assert source.count(case_pin) == 1
        source = source.replace(case_pin,
            "extra_pins={str(HOST_PREFLIGHT.resolve()):sha(HOST_PREFLIGHT),"
            "str(OLD.resolve()):sha(OLD),str(Path(__file__).resolve()):sha(__file__),"
            "str((R/'night_20261115/N11/build535_aliases.py').resolve()):"
            "sha(R/'night_20261115/N11/build535_aliases.py'),"
            "str((HERE/'validate_n11.py').resolve()):sha(HERE/'validate_n11.py'),"
            "str((R/'night_20261115/N11/section6_8_preparation/drafts/check_n11_g3_pair.py').resolve()):"
            "sha(R/'night_20261115/N11/section6_8_preparation/drafts/check_n11_g3_pair.py')}", 1)
        setup_anchor = "checks.save=setup;filters='[NativeActualSavedSurfaceStage3D]'"
        assert source.count(setup_anchor) == 1
        source = source.replace(setup_anchor, setup_anchor + ";n11_monitor(checks,owned)", 1)
        handle_anchor = "   assert len(bl_case)==1,bl_case\n"
        assert source.count(handle_anchor) == 1
        source = source.replace(handle_anchor,
            handle_anchor + "   n11_terminal_receipt(bl_case[0],owned,HOST_PREFLIGHT,monitor_state)\n", 1)
        compile(source, str(HERE / "run_remaining_checks.py"), "exec")
        return source

    module.adapted_safeguard_source = adapted
    module.match_bl_wrapper = match_bl_wrapper
    return module


def adapted_g3_source(runner):
    """Generate the corrected build535 G3 source with only 6.9 gate removals."""
    base = runner.load("n11_s2_source", runner.SECTION2 / "run_remaining_checks.py")
    source = runner.adapted_safeguard_source(base, "bl")
    assert source.count("from build535_aliases import verify_and_publish as expose_build") == 1
    source = runner.bl_wrapper_source(source)
    source = source.replace("gate.self_check();", "gate.self_check();match_bl_wrapper(gate);", 1)
    assert "gate.self_check();match_bl_wrapper(gate);raise SystemExit(gate.main(bl_case[0]))" in source
    return source


def host_preflight(path):
    """Require current-host ownership, load, and free RAM before G3 launch."""
    receipt = json.loads(Path(path).read_text())
    available = int(receipt["MemAvailable"].split()[-2]) * 1024
    assert receipt.get("actual_host_proc") is True and receipt.get("sample_seconds", 0) >= 3
    assert receipt.get("active_OWNED_heavy_processes") == []
    assert available >= 19_999_490_048 and receipt.get("loadavg")
    return receipt


def record_pid(owned, row):
    """Remember one observed run-case PID and its non-reusable start tick."""
    current = process_identity(row["pid"])
    if current:
        owned[(current["pid"], current["start_ticks"])] = dict(current, name=row.get("name", "process"))


def install_process_monitor(checks, owned):
    """Wrap the existing process sampler and Popen to retain terminal identities."""
    original_sample = checks.sample_tree
    original_popen = checks.subprocess.Popen

    def sample(root):
        rows = original_sample(root)
        for row in rows:
            record_pid(owned, row)
        return rows

    def popen(*args, **kwargs):
        process = original_popen(*args, **kwargs)
        record_pid(owned, {"pid": process.pid, "name": "test_driver"})
        return process

    checks.sample_tree = sample
    checks.subprocess.Popen = popen
    return original_sample, original_popen


def process_identity(pid):
    """Return one PID's current state and Linux start tick."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "state": fields[0], "start_ticks": int(fields[19])}
    except (OSError, ValueError, IndexError):
        return None


def terminal_receipt(case, owned, host_path, state):
    """Write and enforce actual terminal process identities and sampled job RSS."""
    rows = []
    for saved in owned.values():
        current = process_identity(saved["pid"])
        alive = bool(current and current["start_ticks"] == saved["start_ticks"]
                     and current["state"] not in ("Z", "X"))
        rows.append(dict(saved, alive=alive))
    peak = max((json.loads(line).get("sampled_process_tree_rss_bytes", 0)
                for line in (case / "resources.jsonl").read_text().splitlines()), default=0)
    result = {"case": str(case), "owned_processes": rows,
              "all_owned_handles_terminal": bool(rows) and not any(row["alive"] for row in rows),
              "sampled_job_peak_rss_bytes": peak, "job_rss_limit_bytes": 19_999_490_048,
              "job_rss_within_limit": peak <= 19_999_490_048,
              "host_preflight": str(host_path), "host_preflight_sha256": sha(host_path)}
    (case / "n11_terminal_receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    checks = state.get("checks")
    if checks:
        checks.sample_tree, checks.subprocess.Popen = state["original_sample"], state["original_popen"]
    assert result["all_owned_handles_terminal"] and result["job_rss_within_limit"]


def probe_gate(_gate, cases):
    """Finish the no-launch path after the inherited gate's own self-check."""
    assert len(cases) == 1 and Path(cases[0]).is_dir()


def probe_main(checks, cases, folder):
    """Exercise fixture provenance setup using a temporary case, with no solver."""
    folder.mkdir(parents=True)
    checks.save(folder / "pins.json", {})
    cases.clear()
    cases.append(folder)
    raise SystemExit(0)


def execute_g3_source(probe=False, host_path=None):
    """Execute the retargeted G3 runner or its no-launch setup probe."""
    original_env = dict(os.environ)
    original_path = list(sys.path)
    original_argv = list(sys.argv)
    sys.path.insert(0, str(N11))
    sys.path.insert(0, str(HERE))
    sys.argv = [str(Path(__file__).resolve()), "bl"]
    runner = load_n8_validator()
    base = runner.load("n11_s2_source", runner.SECTION2 / "run_remaining_checks.py")
    source = adapted_g3_source(runner)
    owned, monitor_state = {}, {}
    namespace = {"__name__": "__main__", "__file__": str(Path(__file__).resolve()),
                 "OLD": base.OLD, "HERE": HERE, "STAGED": STAGED, "REPO": REPO,
                 "HOST_PREFLIGHT": Path(host_path), "owned": owned,
                 "monitor_state": monitor_state,
                 "n11_terminal_receipt": terminal_receipt,
                 "set_bl_wrapper_timeout": runner.bl_wrapper_timeout,
                 "match_bl_wrapper": match_bl_wrapper,
                 "n11_probe_gate": probe_gate}
    def monitor(checks, process_rows):
        sample, popen = install_process_monitor(checks, process_rows)
        monitor_state.update(checks=checks, original_sample=sample, original_popen=popen)

    namespace["n11_monitor"] = monitor
    code = 0
    try:
        with tempfile.TemporaryDirectory() as temp:
            folder = Path(temp) / "case_probe"
            if probe:
                namespace["n11_probe_main"] = lambda checks, cases: probe_main(checks, cases, folder)
                source = source.replace("try:checks.main(R/'runtime_20261113_474/loaded_runtime_pins.json')",
                                        "try:n11_probe_main(checks,bl_case)", 1)
                source = source.replace("gate.self_check();match_bl_wrapper(gate);raise SystemExit(gate.main(bl_case[0]))",
                                        "gate.self_check();n11_probe_gate(gate,bl_case)", 1)
                source = source.replace("   n11_terminal_receipt(bl_case[0],owned,HOST_PREFLIGHT,monitor_state)\n", "", 1)
            try:
                exec(compile(source, str(Path(__file__).resolve()), "exec"), namespace)
            except SystemExit as error:
                code = int(error.code or 0)
    finally:
        if monitor_state.get("checks"):
            checks = monitor_state["checks"]
            checks.sample_tree = monitor_state["original_sample"]
            checks.subprocess.Popen = monitor_state["original_popen"]
        os.environ.clear()
        os.environ.update(original_env)
        sys.path[:] = original_path
        sys.argv[:] = original_argv
    return code, namespace


def match_bl_wrapper(gate):
    """Keep old physical conditions except withdrawn split predicates; require 900s."""
    original = gate.prior.conditions

    def conditions(status, command, row, identical):
        gates = original(status, command, row, identical)
        gates.pop("at_least_300_splits", None)
        gates["serial_bounded_command"] = command.get("ranks") == 1 and command.get("timeout_seconds") == 900
        return gates

    gate.prior.conditions = conditions


def precheck_first():
    """Require first-run physical validity and strict ratio before second launch."""
    receipt_path = HERE / "n11_g3_first_gate.json"
    assert receipt_path.is_file(), "run and pass first G3 gate before second launch"
    receipt = json.loads(receipt_path.read_text())
    assert receipt.get("passed") is True, "first G3 physical/purpose gate failed; stop under R5"
    return Path(receipt["case"])


def launch_one(label, host_path):
    """Launch one G3 case, validate physical rows, then apply its ratio gate."""
    output = HERE / f"n11_g3_{label}_gate.json"
    assert not output.exists(), f"refusing repeat of G3 {label} action"
    before = set((HERE / "cases").glob(CASE_GLOB))
    host_preflight(host_path)
    load_module("n11_g3_physical_prelaunch", PHYSICAL).publish_build_aliases()
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    code, namespace = execute_g3_source(probe=False, host_path=host_path)
    cases = set((HERE / "cases").glob(CASE_GLOB)) - before
    case = Path(namespace.get("bl_case", [None])[0]) if namespace.get("bl_case") else None
    if case is None:
        assert len(cases) == 1, f"expected one new G3 case, observed {len(cases)}"
        case = cases.pop()
    assert case.is_dir(), f"G3 run case was not preserved: {case}"
    if code != 0:
        failure = {"passed": False, "case": str(case), "controller_returncode": code,
                   "physical_and_purpose_gates_evaluated": False}
        output.write_text(json.dumps(failure, indent=2) + "\n")
        raise SystemExit(1)
    physical = load_module("n11_g3_physical", PHYSICAL).physical_case(case)
    summary = None
    if physical.get("passed") is True:
        pair = load_module("n11_g3_pair", PAIR)
        summary = pair.run_summary(case, pair.load_identity())
    passed = physical.get("passed") is True and summary["ratio_strictly_below_0_70"]
    receipt = {"passed": passed, "case": str(case), "controller_returncode": code,
               "physical": physical,
               "purpose": ({key: value for key, value in summary.items() if key != "_edits"}
                           if summary is not None else {"evaluated": False,
                                                        "reason": "physical gate failed"}),
               "split_count_is_report_only": True,
               "gate_order": "physical validity, then geometry-origin rejection ratio <0.70"}
    output.write_text(json.dumps(receipt, indent=2) + "\n")
    if not passed:
        raise SystemExit(1)
    return receipt


def compare_pair(second):
    """Compare accepted edit identity after both separate first-stage gates pass."""
    left = precheck_first()
    second_receipt = json.loads((HERE / "n11_g3_second_gate.json").read_text())
    right = Path(second_receipt["case"]).resolve(strict=True)
    assert right == Path(second).resolve(strict=True) and second_receipt.get("passed") is True
    helper = load_module("n11_g3_pair_compare", PAIR)
    result = helper.compare_pair(left, right)
    target = HERE / "n11_g3_pair_comparison.json"
    target.write_text(json.dumps(result, indent=2) + "\n")
    return result


def self_check():
    """Probe build alias/import/pin setup with a stub before test-driver launch."""
    runner = load_n8_validator()
    source = adapted_g3_source(runner)
    assert "build535_aliases import verify_and_publish as expose_build" in source
    assert "STAGED=HERE.parents[2]" in source and "at_least_336_splits" in source
    assert "gate_source=gate_source.replace(" in source and "set_bl_wrapper_timeout(checks)" in source
    assert runner.HERE == HERE and runner.BUILD == N11 / "build535"
    preflight = A2 / "validation/G1_serial_host_preflight.json"
    host_preflight(preflight)
    load_module("n11_g3_physical_precheck", PHYSICAL).publish_build_aliases()
    code, namespace = execute_g3_source(probe=True, host_path=preflight)
    assert code == 0 and len(namespace.get("bl_case", [])) == 1
    return {"passed": True, "launched": False, "build": str(runner.BUILD),
            "prelaunch_setup_probe": True,
            "g3_controller": str(NIGHT / "section6_2/run_remaining_checks.py"),
            "purpose_helper": str(PAIR), "physical_helper": str(PHYSICAL)}


def main():
    """Launch only the requested first/second G3 action or compare a passed pair."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "first", "second", "pair"))
    parser.add_argument("second_case", nargs="?", type=Path)
    parser.add_argument("--host-preflight", type=Path)
    args = parser.parse_args()
    if args.action == "self-check":
        result = self_check()
    elif args.action == "first":
        if args.host_preflight is None:
            parser.error("first run requires --host-preflight")
        result = launch_one("first", args.host_preflight)
    elif args.action == "second":
        precheck_first()
        if args.host_preflight is None:
            parser.error("second run requires --host-preflight")
        result = launch_one("second", args.host_preflight)
    else:
        if args.second_case is None:
            parser.error("pair requires the second G3 case directory")
        result = compare_pair(args.second_case)
    print(json.dumps(result, indent=2, default=str))
    if result.get("passed") is False:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
