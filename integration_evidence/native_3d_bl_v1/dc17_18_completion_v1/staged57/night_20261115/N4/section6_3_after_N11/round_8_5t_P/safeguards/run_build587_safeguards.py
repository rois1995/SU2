#!/usr/bin/env python3
"""Prepare accepted Section 6.8 safeguards for terminal N11/build587."""
import argparse
import hashlib
import importlib.util
import json
import os
import platform
import subprocess
import sys
import tempfile
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parents[1]
NIGHT = N11.parents[1]
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
SECTION2 = NIGHT / "section6_2"
SECTION68 = NIGHT / "N8/A2_A3_N12/section6_8"
FULL_VALIDATION = SECTION68 / "validation"
FULL_VALIDATOR = FULL_VALIDATION / "validate_n8.py"
CHECKS_SOURCE = SECTION2 / "run_remaining_checks.py"
BUILD = N11 / "build587"
ALIAS_SOURCE = N11 / "build587_aliases.py"
MODES = ("merit481", "saved481", "saved_current")


def sha(path):
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load pinned helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def publish_build_aliases():
    """Use the exact N11 alias verifier, redirecting only its alias output root."""
    source = load("n11_build587_aliases_qualified", ALIAS_SOURCE)
    helpers = source.load_alias_helper()
    helpers.update(HERE=HERE, BUILD=BUILD, STAGED=STAGED, REPO=REPO)
    verified = helpers["verify_build"]()
    status, pins, handles = verified["status"], verified["pins"], verified["handles"]
    sys.path.insert(0, str(STAGED))
    try:
        import run_cases as checks
        checks.ROOT, checks.REPO = HERE, REPO
        checks.BINARY = Path(status["test_binary"])
    finally:
        sys.path.pop(0)
    helpers["expose"]("build587_status.json", dict(status, completed_build=True))
    helpers["expose"]("build587_source_pins.json", pins)
    helpers["expose"]("build587_terminal_handles.json", handles)
    return {"binary": status["binary"], "test_binary": status["test_binary"],
            "build_sha256": status["binary_sha256"], "owned_handles_terminal": True}


def alias_import_stub():
    """Provide the existing runner's import name from the exact N11 alias source."""
    alias = load("n11_build587_aliases_for_safeguards", ALIAS_SOURCE)
    alias.main = publish_build_aliases
    sys.modules["build587_aliases"] = alias
    return alias


def section62_source():
    """Retarget only accepted safeguard paths and preserve the qualified 492-v3 input."""
    source = CHECKS_SOURCE.read_text()
    assert "build525" in source and "build534" not in source
    source = source.replace("build525", "build587")
    source = source.replace("STAGED = HERE.parents[1]", f"STAGED = Path({str(STAGED)!r})")
    geo_pin = "value[str(Path(__file__).resolve())] = sha(__file__)"
    saved_pin = "pinned_inputs = {str(Path(__file__).resolve()): sha(__file__),"
    assert source.count(geo_pin) == source.count(saved_pin) == 1
    source = source.replace(geo_pin, geo_pin +
        f"\n            value[str(Path({str(FULL_VALIDATOR)!r}).resolve())] = sha(Path({str(FULL_VALIDATOR)!r}))" +
        f"\n            value[str(Path({str(__file__)!r}).resolve())] = sha(Path({str(__file__)!r}))")
    source = source.replace(saved_pin, saved_pin +
        f"\n                     str(Path({str(FULL_VALIDATOR)!r}).resolve()): sha(Path({str(FULL_VALIDATOR)!r}))," +
        f"\n                     str(Path({str(__file__)!r}).resolve()): sha(Path({str(__file__)!r})),")
    source = source.replace("(HERE/'build587_aliases.py')", f"(Path({str(ALIAS_SOURCE)!r}))")
    source = source.replace('alias = HERE / "build587_aliases.py"',
                            f'alias = Path({str(ALIAS_SOURCE)!r})')
    source = source.replace('str((HERE / "build587_aliases.py").resolve()):\n'
                            '                         sha(HERE / "build587_aliases.py")',
                            f'str(Path({str(ALIAS_SOURCE)!r}).resolve()):\n'
                            f'                         sha(Path({str(ALIAS_SOURCE)!r}))')
    binding = 'if mode == "saved_current":\n        producer_receipt ='
    assert source.count(binding) == 1
    source = source.replace(binding,
        'if mode in ("saved481", "saved_current"):\n        producer_receipt =', 1)
    return source


def two_dimensional_source(mode):
    validator = load("n11_accepted_validate_n8", FULL_VALIDATOR)
    source = validator.adapted_safeguard_source(
        load("n11_accepted_section62_checks", CHECKS_SOURCE), mode)
    source = source.replace("STAGED=HERE.parents[4]", f"STAGED=Path({str(STAGED)!r})")
    source = source.replace("build534", "build587")
    source = source.replace("HERE/'build587_aliases.py'", f"Path({str(ALIAS_SOURCE)!r})")
    compile(source, str(Path(__file__).resolve()), "exec")
    return source


def build_action(mode):
    assert mode in MODES, mode
    if mode in ("merit481", "saved481", "saved_current"):
        source = section62_source()
        namespace = {"__name__": "n11_build587_safeguards", "__file__": str(__file__)}
        return source, namespace, mode
    source = two_dimensional_source(mode)
    namespace = {"__name__": "__main__", "__file__": str(Path(__file__).resolve()),
                 "OLD": STAGED / "p7p8_r3_20261115/run_remaining_checks.py"}
    return source, namespace, mode


def resource_snapshot():
    loadavg = Path("/proc/loadavg").read_text().strip()
    meminfo = Path("/proc/meminfo").read_text().splitlines()
    available = next(int(line.split()[1]) for line in meminfo if line.startswith("MemAvailable:"))
    return {"hostname": platform.node(), "boot_id": Path("/proc/sys/kernel/random/boot_id").read_text().strip(),
            "procfs_pid1": Path("/proc/1/stat").is_file(), "loadavg": loadavg,
            "mem_available_kib": available, "mem_available_mib": available // 1024}


def process_identity(pid):
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "state": fields[0], "ppid": int(fields[1]),
                "start_ticks": int(fields[19]),
                "name": (Path("/proc") / str(pid) / "comm").read_text().strip()}
    except (OSError, ValueError, IndexError):
        return None


def own_run_preflight():
    """Reject overlap only with runs owned by this wrapper, not unrelated user apps."""
    lock = HERE / "active_run.json"
    if lock.exists():
        data = json.loads(lock.read_text())
        current = process_identity(data.get("pid", -1))
        if current and current.get("start_ticks") == data.get("start_ticks"):
            raise RuntimeError(f"this launcher owns an active safeguard run: {data}")
        raise RuntimeError(f"stale owned-run lock needs review: {data}")
    active = []
    for path in (HERE / "cases").glob("*/safeguard_process_receipt.json"):
        receipt = json.loads(path.read_text())
        if receipt.get("state") == "running":
            root = receipt.get("root", {})
            current = process_identity(root.get("pid", -1))
            if current and current.get("start_ticks") == root.get("start_ticks"):
                active.append({"receipt": str(path), "root": root})
    assert not active, f"owned safeguard child remains active: {active}"
    return {"active_owned_runs": active}


def process_tree(root_pid):
    rows = {}
    for entry in Path("/proc").iterdir():
        if entry.name.isdigit():
            row = process_identity(int(entry.name))
            if row:
                rows[row["pid"]] = row
    owned, pending = set(), [root_pid]
    while pending:
        pid = pending.pop()
        if pid in owned:
            continue
        owned.add(pid)
        pending.extend(row["pid"] for row in rows.values() if row["ppid"] == pid)
    result = []
    for pid in owned:
        row = rows.get(pid)
        if not row:
            continue
        memory = {}
        try:
            for line in (Path("/proc") / str(pid) / "status").read_text().splitlines():
                if ":" in line:
                    key, value = line.split(":", 1)
                    if key in ("VmRSS", "VmHWM"):
                        memory[key] = int(value.split()[0]) * 1024
        except OSError:
            pass
        result.append({**row, **memory})
    return result


def install_run_monitor(binary):
    """Wrap the existing run_cases child launch and record PID/start/RSS until exit."""
    original = subprocess.Popen
    binary = str(Path(binary).resolve())

    class MonitoredProcess:
        def __init__(self, process, receipt, started):
            self._process, self._receipt, self._started = process, receipt, started
            root = process_identity(process.pid)
            assert root, "launched child PID has no /proc identity"
            self._root, self._seen, self._peak_tree, self._peak_hwm = root, {}, 0, {}
            self._record(False)

        def __getattr__(self, name):
            return getattr(self._process, name)

        def _record(self, final):
            rows = process_tree(self._process.pid)
            self._peak_tree = max(self._peak_tree, sum(row.get("VmRSS", 0) for row in rows))
            for row in rows:
                key = f"{row['pid']}:{row['start_ticks']}"
                self._seen[key] = {**self._seen.get(key, {}), **row}
                self._peak_hwm[key] = max(self._peak_hwm.get(key, 0), row.get("VmHWM", 0), row.get("VmRSS", 0))
            result = {"state": "running", "host": resource_snapshot(), "root": self._root,
                      "observed_processes": rows, "peak_sampled_tree_rss_bytes": self._peak_tree,
                      "peak_hwm_by_pid_start_ticks": self._peak_hwm}
            if final:
                live = []
                for item in self._seen.values():
                    current = process_identity(item["pid"])
                    alive = bool(current and current["start_ticks"] == item["start_ticks"] and
                                 current["state"] not in ("Z", "X"))
                    item["alive_at_terminal"] = alive
                    if alive:
                        live.append({key: item[key] for key in ("pid", "start_ticks", "name")})
                result.update(state="terminal", exit_code=self._process.returncode,
                              seconds=time.monotonic() - self._started,
                              actual_host_proc=Path("/proc/1/stat").is_file(),
                              identities=list(self._seen.values()), live_owned_descendants=live)
            path = self._receipt
            temporary = path.with_suffix(path.suffix + ".tmp")
            temporary.write_text(json.dumps(result, indent=2) + "\n")
            temporary.replace(path)
            return result

        def poll(self):
            code = self._process.poll()
            if code is None:
                self._record(False)
            if code is not None:
                result = self._record(True)
                assert not result["live_owned_descendants"], result["live_owned_descendants"]
            return code

        def wait(self, *args, **kwargs):
            code = self._process.wait(*args, **kwargs)
            self.poll()
            return code

    def monitored_popen(*args, **kwargs):
        argv = args[0] if args else kwargs.get("args")
        process = original(*args, **kwargs)
        arguments = [str(item) for item in argv] if isinstance(argv, (list, tuple)) else [str(argv)]
        if binary in arguments and arguments[0] != "ldd":
            cwd = Path(kwargs.get("cwd", HERE)).resolve()
            return MonitoredProcess(process, cwd / "safeguard_process_receipt.json", time.monotonic())
        return process

    subprocess.Popen = monitored_popen
    return original


def host_preflight():
    snapshot = resource_snapshot()
    assert snapshot["procfs_pid1"] and snapshot["boot_id"], "host /proc identity unavailable"
    assert snapshot["mem_available_mib"] >= 19073, \
        f"available RAM below 19073 MiB: {snapshot['mem_available_mib']} MiB"
    return snapshot


class PreparationReached(Exception):
    """Sentinel raised by the self-check's run_cases.main stub before Popen."""


def prepare_until_stub(mode):
    """Run actual derived helper setup and stop exactly at its run_cases.main boundary."""
    alias_import_stub()
    prior_run_cases = sys.modules.get("run_cases")
    checks = load(f"n11_safeguard_run_cases_{mode}", STAGED / "run_cases.py")
    sys.modules["run_cases"] = checks
    captured = {}

    def main_stub(runtime_manifest=None):
        captured.update(runtime_manifest=runtime_manifest, root=checks.ROOT,
                        repo=checks.REPO, binary=Path(checks.BINARY).resolve(),
                        argv=list(sys.argv))
        raise PreparationReached(mode)

    checks.main = main_stub
    original_mkdtemp = tempfile.mkdtemp
    old_environment = dict(os.environ)
    tempfile.mkdtemp = lambda *args, **kwargs: str(HERE / "stubbed-scratch")
    try:
        publish_build_aliases()
        if mode in ("merit481", "saved481", "saved_current"):
            source = section62_source()
            namespace = {"__name__": f"n11_safeguard_preflight_{mode}", "__file__": str(Path(__file__).resolve())}
            exec(compile(source, str(CHECKS_SOURCE), "exec"), namespace)
            if mode == "merit481":
                namespace["run_geometry"]()
            else:
                namespace["run_saved"](mode)
        else:
            source, namespace, _ = build_action(mode)
            namespace["__name__"] = "__main__"
            sys.argv = [str(Path(__file__).resolve()), mode]
            exec(compile(source, str(Path(__file__).resolve()), "exec"), namespace)
    except PreparationReached as result:
        assert str(result) == mode
    finally:
        tempfile.mkdtemp = original_mkdtemp
        os.environ.clear()
        os.environ.update(old_environment)
        if prior_run_cases is None:
            sys.modules.pop("run_cases", None)
        else:
            sys.modules["run_cases"] = prior_run_cases
    assert captured, f"{mode} setup did not reach the stubbed run_cases.main"
    assert captured["root"] == HERE and captured["repo"] == REPO
    expected_binary = Path(json.loads((BUILD / "build_status.json").read_text())["test_binary"]).resolve()
    assert captured["binary"] == expected_binary
    assert Path(captured["runtime_manifest"]).resolve() == (STAGED / "runtime_20261113_474/loaded_runtime_pins.json").resolve()
    if mode.startswith("2d"):
        assert captured["argv"][-1] == f"section6_2_{mode}"
        assert checks.CASES[f"section6_2_{mode}"][0] == int(mode[-1])
    return {"mode": mode, "stopped_at": "stubbed run_cases.main before Popen",
            "root": str(captured["root"]), "repo": str(captured["repo"]),
            "binary": str(captured["binary"]), "runtime_manifest": str(captured["runtime_manifest"])}


def terminal_proof():
    status = json.loads((BUILD / "build_status.json").read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and status.get("changed") == []
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert handles.get("identities") and all(not row.get("alive") and
        isinstance(row.get("pid"), int) and isinstance(row.get("start_ticks"), int)
        for row in handles["identities"])
    return {"build_status_sha256": sha(BUILD / "build_status.json"),
            "terminal_handles_sha256": sha(BUILD / "terminal_handles.json"),
            "build_sha256": status["binary_sha256"],
            "terminal_pid_count": len(handles["identities"])}


def execute_action(mode):
    proof = terminal_proof()
    resources = host_preflight()
    own_run_preflight()
    lock_path = HERE / "active_run.json"
    root = process_identity(os.getpid())
    assert root
    descriptor = os.open(lock_path, os.O_CREAT | os.O_EXCL | os.O_WRONLY, 0o600)
    with os.fdopen(descriptor, "w") as stream:
        json.dump({"pid": root["pid"], "start_ticks": root["start_ticks"], "mode": mode}, stream)
        stream.write("\n")
    original_popen = install_run_monitor(json.loads((BUILD / "build_status.json").read_text())["test_binary"])
    code = 0
    try:
        alias_import_stub()
        if mode in ("merit481", "saved481", "saved_current"):
            source = section62_source()
            namespace = {"__name__": "n11_build587_safeguards", "__file__": str(__file__)}
            exec(compile(source, str(CHECKS_SOURCE), "exec"), namespace)
            publish_build_aliases()
            if mode == "merit481":
                namespace["run_geometry"]()
            else:
                namespace["run_saved"](mode)
        else:
            source, namespace, _ = build_action(mode)
            namespace.update({"HERE": HERE, "STAGED": STAGED, "REPO": REPO,
                              "set_bl_wrapper_timeout": lambda _checks: None,
                              "match_bl_wrapper": lambda _gate: None})
            os.environ["OMPI_MCA_osc"] = "pt2pt"
            publish_build_aliases()
            sys.argv = [str(Path(__file__).resolve()), mode]
            exec(compile(source, str(Path(__file__).resolve()), "exec"), namespace)
    except SystemExit as exc:
        code = int(exc.code or 0)
        if code:
            raise
    finally:
        subprocess.Popen = original_popen
        if lock_path.exists():
            lock_path.unlink()
    receipts = sorted((HERE / "cases").glob("*/safeguard_process_receipt.json"),
                      key=lambda p: p.stat().st_mtime_ns)
    assert receipts, "run_cases did not produce monitored process receipt"
    receipt = json.loads(receipts[-1].read_text())
    assert receipt.get("state") == "terminal" and receipt.get("actual_host_proc") is True
    assert receipt.get("exit_code") == 0 and not receipt.get("live_owned_descendants")
    assert receipt.get("root", {}).get("start_ticks") and receipt.get("identities")
    assert receipt.get("peak_sampled_tree_rss_bytes", 0) <= 19_999_490_048
    return {"mode": mode, "build_terminal_proof": proof, "resources_at_start": resources,
            "run_terminal_receipt": str(receipts[-1]), "run_terminal_receipt_sha256": sha(receipts[-1]),
            "process_tree_peak_rss_bytes": receipt["peak_sampled_tree_rss_bytes"],
            "terminal_process_count": len(receipt["identities"])}


def self_check():
    assert FULL_VALIDATOR.is_file() and CHECKS_SOURCE.is_file() and ALIAS_SOURCE.is_file()
    status = json.loads((BUILD / "build_status.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    proof = terminal_proof()
    # Exercise the real terminal pin verification and alias writer; outputs stay in this new directory.
    aliases = publish_build_aliases()
    assert aliases["build_sha256"] == status["binary_sha256"]
    source = section62_source()
    compile(source, str(CHECKS_SOURCE), "exec")
    section62_ns = {"__name__": "n11_safeguard_preflight", "__file__": str(__file__)}
    exec(compile(source, str(CHECKS_SOURCE), "exec"), section62_ns)
    assert section62_ns["HERE"] == HERE and section62_ns["STAGED"] == STAGED
    assert section62_ns["REPO"] == REPO
    assert "build587" in source and "build525" not in source
    assert "build481_status" not in source and 'mode in ("saved481", "saved_current")' in source
    assert str(FULL_VALIDATOR) in source and str(Path(__file__).resolve()) in source
    alias = alias_import_stub()
    assert Path(alias.__file__).resolve() == ALIAS_SOURCE.resolve()
    validator = load("n11_accepted_validate_n8_selfcheck", FULL_VALIDATOR)
    assert Path(validator.__file__).resolve() == FULL_VALIDATOR.resolve()
    prepared = {}
    for mode in MODES:
        src, namespace, _ = build_action(mode)
        compile(src, str(Path(__file__).resolve()), "exec")
        if mode in ("merit481", "saved481", "saved_current"):
            assert str(ALIAS_SOURCE) in src
        else:
            assert str(Path(__file__).resolve()) in src or namespace["__file__"] == str(Path(__file__).resolve())
        prepared[mode] = {"source_sha256": hashlib.sha256(src.encode()).hexdigest(),
                          "namespace_name": namespace["__name__"]}
    prepared_calls = [prepare_until_stub(mode) for mode in MODES]
    host = host_preflight()
    return {"passed": True, "launched": False, "build": str(BUILD),
            "terminal_proof": proof, "resources_observed": host,
            "accepted_validator_sha256": sha(FULL_VALIDATOR),
            "section62_runner_sha256": sha(CHECKS_SOURCE),
            "build587_alias_source": str(ALIAS_SOURCE), "modes": prepared,
            "preparations_stopped_at_stub": prepared_calls, "alias_setup": "qualified N11 build587_aliases.py",
            "target_binding": "accepted qualified checkpoint492/v3 for saved481 and saved_current"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("mode", choices=("self-check", *MODES))
    args = parser.parse_args()
    if args.mode == "self-check":
        print(json.dumps(self_check(), indent=2))
    else:
        print(json.dumps(execute_action(args.mode), indent=2))


if __name__ == "__main__":
    main()
