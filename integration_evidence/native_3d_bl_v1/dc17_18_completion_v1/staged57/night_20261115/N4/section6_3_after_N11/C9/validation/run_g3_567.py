#!/usr/bin/env python3
"""Prepare or explicitly launch one end-cut G3 case using sealed build567."""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parents[1]
NIGHT = PHASE.parents[1]
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
BUILD = PHASE / "build567"
HOUSE = STAGED / "run_cases.py"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
N14 = NIGHT / "N14/section6_12"
REFERENCE = N14 / "validation/cases/n14_g3_first_20261117T104157_3163858"
OLD_BUILD = N14 / "build552"
PHYSICAL = NIGHT / "N11/validation/validate_n11.py"
IDENTITY = NIGHT / "N9/section6_6/drafts/check_g3_identity_66.py"
FILTER = "[NativeActualSavedSurfaceStage3D]"
CASE = "C9_G3"
MAX_JOB_RSS = 19_999_490_048
ALIASES = ("build567_status.json", "build567_source_pins.json", "build567_terminal_handles.json")


class SetupReached(Exception):
    """The no-launch harness reached test_driver Popen after saving run pins."""


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def process_identity(pid: int) -> dict | None:
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "state": fields[0], "start_ticks": int(fields[19])}
    except (OSError, ValueError, IndexError):
        return None


def verify_build() -> dict:
    """Require a sealed build with verified source, binary, graph and handles."""
    status = json.loads((BUILD / "build_status.json").read_text())
    pins = json.loads((BUILD / "source_pins.json").read_text())
    graph = json.loads((BUILD / "generated_build_graph_after.json").read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    primary_terminal = json.loads((BUILD / "primary_terminal_receipt.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    assert pins and all(sha(Path(path)) == digest for path, digest in pins.items())
    assert sha(Path(status["binary"])) == status["binary_sha256"]
    assert sha(Path(status["test_binary"])) == status["test_binary_sha256"]
    assert graph.get("source_pins_sha256") == sha(BUILD / "source_pins.json")
    assert graph.get("build_status_sha256") == sha(BUILD / "build_status.json")
    assert graph.get("after") and all(sha(Path(path)) == digest for path, digest in graph["after"].items())
    identities = handles.get("identities", [])
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert identities and all(row.get("alive") is False and isinstance(row.get("pid"), int)
                              and isinstance(row.get("start_ticks"), int) for row in identities)
    assert all(process_identity(row["pid"]) is None or
               process_identity(row["pid"])["start_ticks"] != row["start_ticks"] or
               process_identity(row["pid"])["state"] in {"Z", "X"} for row in identities)
    assert primary_terminal.get("actual_host_proc") is True
    assert primary_terminal.get("all_owned_handles_terminal") is True
    assert primary_terminal.get("passed") is True
    primary_ids = primary_terminal.get("identities", [])
    assert primary_ids and all(row.get("alive") is not True for row in primary_ids)
    for row in primary_ids:
        current = process_identity(int(row["pid"]))
        assert not current or current["start_ticks"] != int(row["start_ticks"]) or current["state"] in {"Z", "X"}
    return {"status": status, "pins": pins, "handles": handles,
            "primary_terminal": primary_terminal}


def publish_local_aliases(workspace: Path, verified: dict) -> None:
    """Materialize build aliases only inside the selected run workspace."""
    status = dict(verified["status"], completed_build=True)
    rows = (status, verified["pins"], verified["handles"])
    for name, value in zip(ALIASES, rows):
        target = workspace / name
        if target.exists():
            assert json.loads(target.read_text()) == value, target
        else:
            target.write_text(json.dumps(value, indent=2) + "\n")


def old_input_pins() -> dict[str, str]:
    """Carry the N14 G3 frozen fixture inputs while dropping build552 artifacts."""
    old = json.loads((REFERENCE / "pins.json").read_text())
    old_source = json.loads((OLD_BUILD / "source_pins.json").read_text())
    old_status = json.loads((OLD_BUILD / "build_status.json").read_text())
    excluded = set(old_source) | {old_status["binary"], old_status["test_binary"]}
    result = {path: digest for path, digest in old.items() if path not in excluded}
    assert result and all(sha(Path(path)) == digest for path, digest in result.items())
    return result


def host_preflight(path: Path) -> dict:
    """Validate a fresh actual-host receipt; no preflight is fabricated here."""
    receipt = json.loads(path.read_text())
    available = int(receipt["MemAvailable"].split()[-2]) * 1024
    assert receipt.get("actual_host_proc") is True
    assert receipt.get("sample_seconds", 0) >= 3
    assert receipt.get("active_OWNED_heavy_processes") == []
    assert available >= MAX_JOB_RSS and receipt.get("loadavg")
    return receipt


def apply_reference_environment() -> None:
    """Replay N14's saved SU2_NATIVE environment, preserving its frozen-config override."""
    command = json.loads((REFERENCE / "command.json").read_text())
    for key, value in command.get("environment", {}).items():
        if key.startswith("SU2_NATIVE_") and key != "SU2_NATIVE_FROZEN_CONFIG":
            os.environ[key] = value
    os.environ["OMPI_MCA_osc"] = "pt2pt"


def load_house(workspace: Path, status: dict):
    """Load the inherited run_cases and bind only its root, binary and G3 case."""
    namespace = {"__name__": "endb_g3_run_cases", "__file__": str(HOUSE)}
    source = HOUSE.read_text()
    timeout = "timeout_seconds = 2400 if case == \"m6_stage\" else 600"
    assert source.count(timeout) == 1
    source = source.replace(timeout, "timeout_seconds = 2400 if case == \"m6_stage\" else 900", 1)
    exec(compile(source, str(HOUSE), "exec"), namespace)
    namespace.update(ROOT=workspace, REPO=REPO, BINARY=Path(status["test_binary"]))
    namespace["CASES"][CASE] = (1, FILTER)
    return namespace


def configure_house(house, provenance: list[Path], *, frozen_config: bytes | None = None):
    """Pin new adapter inputs and preserve N14's exact frozen configuration."""
    original_save = house["save"]
    inputs = old_input_pins()

    def save(path, value):
        if Path(path).name == "pins.json":
            for item in provenance:
                value[str(item.resolve())] = house["sha"](item)
            assert all(house["sha"](Path(item)) == digest for item, digest in inputs.items())
            value.update(inputs)
            if frozen_config is None:
                config_bytes = (REFERENCE / "run.cfg").read_bytes()
            else:
                config_bytes = frozen_config
            config = Path(path).parent / "run.cfg"
            config.write_bytes(config_bytes)
            value[str(config.resolve())] = house["sha"](config)
            os.environ["SU2_NATIVE_FROZEN_CONFIG"] = str(config.resolve())
        original_save(path, value)

    house["save"] = save


def hook_processes(house, owned: dict[tuple[int, int], dict], capture_launch=None):
    """Preserve actual PID/start-tick identity and optionally intercept Popen."""
    original_sample = house["sample_tree"]

    def sample(root_pid):
        rows = original_sample(root_pid)
        for row in rows:
            current = process_identity(row["pid"])
            if current:
                owned[(current["pid"], current["start_ticks"])] = dict(current, name=row["name"])
        return rows

    house["sample_tree"] = sample
    subprocess = house["subprocess"]
    original_popen = subprocess.Popen

    def popen(argv, *args, **kwargs):
        values = [str(value) for value in argv]
        binary = str(house["BINARY"])
        if values and values[0] != "ldd" and binary in values:
            pid = kwargs.get("cwd")
            attempt = Path(pid)
            assert (attempt / "pins.json").is_file() and (attempt / "command.json").is_file()
            if capture_launch is not None:
                capture_launch(values, attempt)
                raise SetupReached("intentional stop before test_driver Popen")
            process = original_popen(argv, *args, **kwargs)
            identity = process_identity(process.pid)
            if identity:
                owned[(identity["pid"], identity["start_ticks"])] = dict(identity, name="run_cases-controller")
            return process
        return original_popen(argv, *args, **kwargs)

    subprocess.Popen = popen
    return original_popen


def finalize_handles(owned: dict[tuple[int, int], dict]) -> list[dict]:
    terminal = []
    for saved in owned.values():
        current = process_identity(saved["pid"])
        alive = bool(current and current["start_ticks"] == saved["start_ticks"]
                     and current["state"] not in ("Z", "X"))
        terminal.append(dict(saved, alive=alive))
    return terminal


def runtime_workspace(workspace: Path, verified: dict):
    workspace.mkdir(parents=True, exist_ok=True)
    publish_local_aliases(workspace, verified)
    return workspace


def run_or_stub(*, host_path: Path | None, stub: bool) -> dict:
    """Run the G3 case or stop at its exact Popen boundary in temporary storage."""
    verified = verify_build()
    runtime_receipt = None if stub else host_preflight(host_path)
    if stub:
        if (HERE / "g3_prelaunch_stub_receipt.json").exists():
            raise FileExistsError(HERE / "g3_prelaunch_stub_receipt.json")
        temp = tempfile.TemporaryDirectory(prefix="endb_g3_setup_")
        workspace = Path(temp.name)
        publish_local_aliases(workspace, verified)
    else:
        if any((HERE / "cases").glob(f"{CASE}_*")):
            raise FileExistsError("G3 case output already exists; refusing repeat")
        workspace = HERE
        runtime_workspace(workspace, verified)

    house = load_house(workspace, verified["status"])
    case_key = CASE
    synthetic = None
    if stub:
        case_key = "endb_g3_setup_stub"
        house["CASES"][case_key] = (1, FILTER)
        synthetic = workspace / "setup_stub_host.json"
        synthetic.write_text(json.dumps({"setup_stub_only": True, "not_launch_evidence": True}) + "\n")
    provenance = [Path(__file__).resolve(), HERE / "validate_end_b.py", PHYSICAL, IDENTITY, HOUSE,
                            RUNTIME, BUILD / "build_status.json", BUILD / "source_pins.json",
                            BUILD / "terminal_handles.json", BUILD / "primary_terminal_receipt.json",
                            BUILD / "generated_build_graph_after.json",
                            REFERENCE / "run.cfg", REFERENCE / "pins.json",
                            REFERENCE / "command.json",
                            workspace / "build567_terminal_handles.json"]
    if stub:
        provenance.append(synthetic)
    elif host_path is not None:
        provenance.append(host_path)
        launch_identity = HERE / "g3_primary_launch_identity.json"
        launch_wrapper = HERE / "launch_g3.py"
        assert launch_identity.is_file() and launch_wrapper.is_file()
        provenance.extend((launch_identity, launch_wrapper))
    configure_house(house, provenance)
    owned: dict[tuple[int, int], dict] = {}
    restore = hook_processes(house, owned, capture_launch=(lambda argv, case: seen.append({
        "argv": argv, "case": str(case), "pins_sha256": sha(case / "pins.json"),
        "command_sha256": sha(case / "command.json")})) if stub else None)
    old_argv = sys.argv[:]
    old_env = {key: value for key, value in os.environ.items()
               if key.startswith("SU2_NATIVE_") or key == "OMPI_MCA_osc"}
    seen = []
    code = 0
    started = time.time()
    try:
        sys.argv = [str(HOUSE), "build567", case_key]
        apply_reference_environment()
        try:
            house["main"](RUNTIME)
        except SetupReached:
            pass
        except SystemExit as error:
            code = int(error.code or 0)
    finally:
        house["subprocess"].Popen = restore
        sys.argv = old_argv
        for key in tuple(os.environ):
            if key.startswith("SU2_NATIVE_") or key == "OMPI_MCA_osc":
                os.environ.pop(key, None)
        os.environ.update(old_env)

    if stub:
        try:
            assert len(seen) == 1, f"expected exactly one intercepted G3 Popen, got {len(seen)}"
            launch = seen[0]
            command = json.loads((Path(launch["case"]) / "command.json").read_text())
            assert command["build"] == "build567" and command["case"] == case_key
            assert command["ranks"] == 1 and command["timeout_seconds"] == 900
            assert command["argv"][-3] == FILTER
            receipt = {"passed": True, "launched": False, "built": False,
                       "setup_boundary": "after pins.json and command.json, before test_driver Popen",
                       "case": launch["case"], "pins_sha256": launch["pins_sha256"],
                       "command_sha256": launch["command_sha256"], "argv": command["argv"],
                       "synthetic_setup_only": True, "host_preflight_used": False,
                       "build_binary_sha256": verified["status"]["test_binary_sha256"]}
            output = HERE / "g3_prelaunch_stub_receipt.json"
            if output.exists():
                raise FileExistsError(output)
            output.write_text(json.dumps(receipt, indent=2) + "\n")
            return receipt
        finally:
            temp.cleanup()

    cases = sorted((HERE / "cases").glob(f"{case_key}_*"), key=lambda p: p.stat().st_mtime)
    assert len(cases) == 1, f"expected one fresh case, found {len(cases)}"
    case = cases[0]
    status = json.loads((case / "status.json").read_text())
    terminal = finalize_handles(owned)
    resource_rows = [json.loads(line) for line in (case / "resources.jsonl").read_text().splitlines() if line]
    sample_peak = max((int(row.get("sampled_process_tree_rss_bytes", 0)) for row in resource_rows), default=0)
    receipt = {"case": str(case), "case_exit_code": status.get("exit_code"),
               "actual_host_proc": True, "host_preflight_sha256": sha(host_path),
               "elapsed_seconds": time.time() - started, "filter": FILTER, "ranks": 1,
               "nice": 19, "sampled_process_tree_peak_rss_bytes": sample_peak,
               "sampled_process_tree_cap_bytes": MAX_JOB_RSS,
               "sampled_process_tree_within_cap": sample_peak <= MAX_JOB_RSS,
               "identities": terminal,
               "all_owned_handles_terminal": bool(terminal) and not any(row["alive"] for row in terminal),
               "pins_sha256": sha(case / "pins.json"), "run_log_sha256": sha(case / "run.log"),
               "status": status}
    receipt["passed"] = (receipt["case_exit_code"] == 0 and receipt["all_owned_handles_terminal"]
                         and receipt["sampled_process_tree_within_cap"])
    terminal_path = case / "g3_terminal_receipt.json"
    assert not terminal_path.exists()
    terminal_path.write_text(json.dumps(receipt, indent=2) + "\n")
    assert receipt["case_exit_code"] == 0 and receipt["all_owned_handles_terminal"]
    assert receipt["sampled_process_tree_within_cap"]

    physical = load_module("endb_g3_physical", PHYSICAL).physical_case(case)
    assert physical.get("passed") is True
    gate = load_module("endb_g3_gate", HERE / "validate_end_b.py").g3_result(case, terminal_path)
    out = case / "g3_end_b_gate.json"
    assert not out.exists()
    out.write_text(json.dumps(gate, indent=2) + "\n")
    return {"passed": gate["passed"], "launched": True, "built": False,
            "case": str(case), "gate": str(out), "terminal_receipt": str(terminal_path)}


def self_check() -> dict:
    """Check the source bindings and N14 reference without build or process launch."""
    assert HOUSE.is_file() and RUNTIME.is_file() and REFERENCE.is_dir()
    assert all((REFERENCE / name).is_file() for name in ("run.cfg", "pins.json", "command.json"))
    assert PHYSICAL.is_file() and IDENTITY.is_file()
    filtered = old_input_pins()
    assert BUILD == PHASE / "build567" and FILTER == "[NativeActualSavedSurfaceStage3D]"
    return {"passed": True, "launched": False, "built": False,
            "case": CASE, "filter": FILTER, "ranks": 1,
            "reference": str(REFERENCE), "input_pin_count": len(filtered),
            "build_root": str(BUILD), "requires_sealed_build567": True}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument("--self-check", action="store_true")
    actions.add_argument("--prelaunch-stub", action="store_true")
    actions.add_argument("--launch", action="store_true")
    parser.add_argument("--host-preflight", type=Path)
    args = parser.parse_args()
    if args.self_check:
        result = self_check()
    elif args.prelaunch_stub:
        result = run_or_stub(host_path=None, stub=True)
    else:
        if args.host_preflight is None:
            parser.error("--launch requires a fresh --host-preflight receipt")
        result = run_or_stub(host_path=args.host_preflight, stub=False)
    print(json.dumps(result, indent=2), flush=True)
    if result.get("passed") is not True:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
