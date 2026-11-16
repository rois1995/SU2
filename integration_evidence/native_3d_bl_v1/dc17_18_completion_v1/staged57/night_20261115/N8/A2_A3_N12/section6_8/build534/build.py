#!/usr/bin/env python3
"""Build the connected A1/A2/A3/N12/item4 candidate without pinning generated Ninja files."""
import hashlib
import argparse
import json
import os
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[8]
STAGED = ROOT.parents[4]
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
TOOLS = REPO / "integration_evidence/build-integrated-v2/tools"
GRAPH = (BUILD / "build.ninja", BUILD / "compile_commands.json")
sys.path.insert(0, str(STAGED))
from run_cases import sample_tree


def sha(path):
    """Hash a single file with SHA256."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def save(name, value):
    """Write one build534 receipt without replacing prior evidence."""
    path = ROOT / name
    assert not path.exists(), f"refusing to overwrite build receipt: {path}"
    path.write_text(json.dumps(value, indent=2) + "\n")


def identity(pid):
    """Capture a process PID and start tick for terminal ownership checks."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return {"pid": pid, "state": fields[0], "start_ticks": int(fields[19])}
    except (OSError, ValueError, IndexError):
        return None


def graph_snapshot():
    """Record generated build graph files separately from source pins."""
    return {str(path.resolve()): (sha(path) if path.is_file() else None) for path in GRAPH}


def source_pins():
    """Verify declared inputs and ensure generated graph products are excluded."""
    path = ROOT / "source_pins.json"
    pins = json.loads(path.read_text())
    graph_paths = {str(item.resolve()) for item in GRAPH}
    assert not graph_paths.intersection(pins), "generated Ninja graph products are not source pins"
    assert pins and all(sha(item) == digest for item, digest in pins.items())
    return path, pins


def build_environment():
    """Set the known Ninja PATH, compile scratch, and single-threaded libraries."""
    env = os.environ.copy()
    env["PATH"] = str(TOOLS) + os.pathsep + env.get("PATH", "")
    env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1",
               TMPDIR=str(REPO / "integration_evidence/build-integrated-v2/compile-tmp"))
    assert (TOOLS / "ninja").is_file() and Path(env["TMPDIR"]).is_dir()
    return env


def verify_terminal(owned):
    """Return saved process identities with their current terminal status."""
    rows = []
    for saved in owned.values():
        current = identity(saved["pid"])
        alive = bool(current and current["start_ticks"] == saved["start_ticks"]
                      and current["state"] not in ("Z", "X"))
        rows.append(dict(saved, alive=alive))
    return rows


def run_build():
    """Build the frozen source once and seal binary plus generated-graph provenance."""
    pins_path, pins = source_pins()
    assert not (ROOT / "build_status.json").exists()
    before = graph_snapshot()
    env = build_environment()
    argv = ["nice", "-n", "19", str(TOOLS / "ninja"), "-C", str(BUILD), "-j4",
            "UnitTests/test_driver", "SU2_CFD/src/SU2_CFD"]
    start = time.time()
    save("build_plan.json", {"argv": argv, "source_pins_sha256": sha(pins_path),
                             "generated_graph_before": before,
                             "environment": {key: env[key] for key in (
                                 "PATH", "OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS", "TMPDIR")},
                             "started_unix": start, "nice": 19, "workers": 4})
    owned = {}
    with (ROOT / "build.log").open("x") as log, (ROOT / "build_resources.jsonl").open("x") as resources:
        proc = subprocess.Popen(argv, cwd=REPO, env=env, stdout=log,
                                stderr=subprocess.STDOUT, start_new_session=True)
        (ROOT / "build_status.json").write_text(json.dumps({
            "state": "running", "controller": identity(os.getpid()),
            "identity": identity(proc.pid)}, indent=2) + "\n")
        while proc.poll() is None:
            rows = sample_tree(proc.pid)
            for row in rows:
                found = identity(row["pid"])
                if found:
                    owned[(found["pid"], found["start_ticks"])] = dict(found, name=row["name"])
            resources.write(json.dumps({"unix": time.time(), "loadavg": Path("/proc/loadavg").read_text(),
                                        "meminfo": Path("/proc/meminfo").read_text(), "processes": rows,
                                        "sampled_tree_rss_bytes": sum(row["rss_bytes"] for row in rows)}) + "\n")
            resources.flush()
            time.sleep(10)
        code = proc.wait()
    terminal_rows = verify_terminal(owned)
    owned_terminal = not any(row["alive"] for row in terminal_rows)
    if not owned_terminal:
        code = 1
    changed = [path for path, digest in pins.items() if sha(path) != digest]
    status = {"state": "terminal", "exit_code": code, "seconds": time.time() - start,
              "pins_verified": not changed, "changed": changed,
              "binary": str(BUILD / "SU2_CFD/src/SU2_CFD"),
              "test_binary": str(BUILD / "UnitTests/test_driver")}
    if code == 0:
        status["binary_sha256"] = sha(status["binary"])
        status["test_binary_sha256"] = sha(status["test_binary"])
    (ROOT / "build_status.json").write_text(json.dumps(status, indent=2) + "\n")
    save("terminal_handles.json", {"actual_host_proc": True, "verified_unix": time.time(),
                                    "identities": terminal_rows,
                                    "all_owned_handles_terminal": owned_terminal})
    after = graph_snapshot()
    assert all(value is not None for value in after.values()), "generated build graph output is missing"
    save("generated_build_graph_after.json", {
        "schema": "n8_a2_a3_n12_generated_build_graph_v1", "build_status_sha256": sha(ROOT / "build_status.json"),
        "source_pins_sha256": sha(pins_path), "before": before, "after": after,
        "binary_sha256": status.get("binary_sha256"),
        "test_binary_sha256": status.get("test_binary_sha256")})
    raise SystemExit(code or bool(changed))


def self_check():
    """Check build paths and generated-product exclusions without invoking Ninja."""
    assert ROOT.name == "build534" and REPO.name == "SU2_NativeIntegrated"
    assert BUILD.is_dir() and (TOOLS / "ninja").is_file()
    assert (REPO / "integration_evidence/build-integrated-v2/compile-tmp").is_dir()
    if (ROOT / "source_pins.json").is_file():
        _, pins = source_pins()
        pin_count = len(pins)
    else:
        pin_count = None
    return {"passed": True, "launched": False, "build": str(BUILD),
            "ninja_on_path": str(TOOLS / "ninja"), "generated_graph_products": [str(p) for p in GRAPH],
            "source_pin_count": pin_count, "build_workers": 4, "nice": 19}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
    else:
        run_build()
