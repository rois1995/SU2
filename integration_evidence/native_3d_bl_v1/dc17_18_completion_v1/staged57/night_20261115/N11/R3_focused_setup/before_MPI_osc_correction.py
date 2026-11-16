#!/usr/bin/env python3
"""Launch the exact focused B2 wall-law unit tags once against build535."""
import argparse
import importlib.util
import json
import os
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parent
STAGED = N11.parents[1]
REPO = STAGED.parents[3]
BUILD = N11 / "build535"
HOUSE = STAGED / "run_cases.py"
RUNTIME = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"
FILTER = "[NativeWallLawB2],[NativeWallGeometryLaw3D],[NativeWallDiagnostics68]"
CASE = "n11_b2_focused_wall"
MAX_JOB_RSS = 19_999_490_048


def load_house():
    """Load the existing case runner into a namespace bound to N11/build535."""
    namespace = {"__name__": "n11_focused_house", "__file__": str(HOUSE)}
    exec(compile(HOUSE.read_text(), str(HOUSE), "exec"), namespace)
    namespace.update(ROOT=HERE, REPO=REPO, BINARY=REPO / "integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver")
    namespace["CASES"][CASE] = (1, FILTER)
    return namespace


def host_preflight(path):
    """Require the existing actual-host preflight shape and unchanged RAM cap."""
    receipt = json.loads(Path(path).read_text())
    free = int(receipt["MemAvailable"].split()[-2]) * 1024
    assert receipt.get("actual_host_proc") is True and receipt.get("sample_seconds", 0) >= 3
    assert receipt.get("active_OWNED_heavy_processes") == []
    assert free >= MAX_JOB_RSS and receipt.get("loadavg")
    return receipt


def identity(pid):
    """Capture one PID's process state and stable Linux start tick."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": int(pid), "state": fields[0], "start_ticks": int(fields[19])}
    except (OSError, ValueError, IndexError):
        return None


def monitored_sampler(house, owned):
    """Wrap inherited process-tree samples to retain exact PID identities."""
    original = house["sample_tree"]

    def sample(root_pid):
        rows = original(root_pid)
        for row in rows:
            current = identity(row["pid"])
            if current:
                owned[(current["pid"], current["start_ticks"])] = dict(current, name=row["name"])
        return rows

    house["sample_tree"] = sample


def monitored_popen(house, owned):
    """Capture the launched test PID before a short test can evade sampling."""
    subprocess = house["subprocess"]
    original = subprocess.Popen

    def launch(*args, **kwargs):
        process = original(*args, **kwargs)
        current = identity(process.pid)
        if current:
            owned[(current["pid"], current["start_ticks"])] = dict(current, name="test_driver")
        return process

    subprocess.Popen = launch
    return original


def add_case_provenance(house, provenance):
    """Pin this wrapper and the actual build/runtime receipts in run_cases output."""
    original = house["save"]

    def save(path, value):
        if Path(path).name == "pins.json":
            for item in provenance:
                value[str(item.resolve())] = house["sha"](item)
        original(path, value)

    house["save"] = save


def run_once(host_receipt):
    """Run exactly one serial focused case and seal its own terminal identities."""
    aliases = json.loads((HERE / "build535_status.json").read_text())
    assert aliases.get("completed_build") is True
    house = load_house()
    assert Path(house["BINARY"]).is_file()
    provenance = [Path(__file__).resolve(), HOUSE, RUNTIME,
                  BUILD / "build_status.json", BUILD / "source_pins.json",
                  BUILD / "terminal_handles.json", Path(host_receipt)]
    add_case_provenance(house, provenance)
    owned = {}
    monitored_sampler(house, owned)
    restore_popen = monitored_popen(house, owned)
    before = set((HERE / "cases").glob(f"{CASE}_*"))
    sys.argv = [str(HOUSE), "build535", CASE]
    start = time.time()
    code = 0
    try:
        try:
            house["main"](RUNTIME)
        except SystemExit as error:
            code = int(error.code or 0)
    finally:
        house["subprocess"].Popen = restore_popen
    attempts = set((HERE / "cases").glob(f"{CASE}_*")) - before
    assert len(attempts) == 1, f"expected one fresh case, got {len(attempts)}"
    folder = attempts.pop()
    terminal = []
    for saved in owned.values():
        current = identity(saved["pid"])
        alive = bool(current and current["start_ticks"] == saved["start_ticks"]
                     and current["state"] not in ("Z", "X"))
        terminal.append(dict(saved, alive=alive))
    peak = max((json.loads(line).get("sampled_process_tree_rss_bytes", 0)
                for line in (folder / "resources.jsonl").read_text().splitlines()), default=0)
    receipt = {"case": str(folder), "exit_code": code, "elapsed_seconds": time.time() - start,
               "filter": FILTER, "ranks": 1, "nice": 19, "job_peak_sampled_rss_bytes": peak,
               "job_rss_limit_bytes": MAX_JOB_RSS, "job_rss_within_limit": peak <= MAX_JOB_RSS,
               "owned_processes": terminal,
               "all_owned_handles_terminal": bool(terminal) and not any(r["alive"] for r in terminal),
               "pins_sha256": house["sha"](folder / "pins.json"),
               "run_log_sha256": house["sha"](folder / "run.log")}
    (folder / "n11_terminal_receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
    assert code == 0 and receipt["all_owned_handles_terminal"] and receipt["job_rss_within_limit"]
    return receipt


def main():
    """Verify terminal build aliases before one user-authorized test launch."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--host-preflight", type=Path)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        assert int("39112836 kB".split()[-2]) == int("MemAvailable: 39112836 kB".split()[-2])
        house = load_house()
        assert house["CASES"][CASE] == (1, FILTER)
        assert house["ROOT"] == HERE and house["BINARY"] == REPO / "integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver"
        print(json.dumps({"passed": True, "launched": False, "case": CASE,
                          "filter": FILTER, "build": str(BUILD)}))
        return
    if args.host_preflight is None:
        parser.error("launch requires --host-preflight")
    host_preflight(args.host_preflight)
    spec = importlib.util.spec_from_file_location("n11_build535_aliases",
                                                  N11 / "build535_aliases.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.verify_and_publish()
    print(json.dumps(run_once(args.host_preflight), indent=2))


if __name__ == "__main__":
    main()
