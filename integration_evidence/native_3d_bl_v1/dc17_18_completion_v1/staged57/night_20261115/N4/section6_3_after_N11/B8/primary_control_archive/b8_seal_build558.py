#!/usr/bin/env python3
"""Seal terminal build receipts for B8 after an externally observed exit."""
from pathlib import Path
import hashlib
import json
import sys
import time

P = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
BUILD = P / "build558"
EXPECTED_PINS = 1781


def read(path):
    """Read one JSON receipt."""
    return json.loads(Path(path).read_text())


def sha(path):
    """Hash one sealed artifact."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def terminal(row):
    """Check PID and start-tick identity against /proc."""
    try:
        fields = (Path("/proc") / str(row["pid"]) / "stat").read_text().rsplit(")", 1)[1].split()
    except FileNotFoundError:
        return True
    return int(fields[19]) != int(row["start_ticks"]) or fields[0] in {"Z", "X"}


def main():
    """Verify build, binary, graph, and owned terminal handles before writing receipt."""
    if len(sys.argv) != 2:
        raise SystemExit("usage: seal_build.py <observed-exec-session-id>")
    status = read(BUILD / "build_status.json")
    assert status["state"] == "terminal" and status["exit_code"] == 0
    assert status["pins_verified"] and not status["changed"]
    pins = read(BUILD / "source_pins.json")
    assert len(pins) == EXPECTED_PINS, (len(pins), EXPECTED_PINS)
    assert all(sha(path) == digest for path, digest in pins.items())
    assert sha(status["binary"]) == status["binary_sha256"]
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    graph = read(BUILD / "generated_build_graph_after.json")
    assert graph["build_status_sha256"] == sha(BUILD / "build_status.json")
    assert graph["source_pins_sha256"] == sha(BUILD / "source_pins.json")
    assert all(sha(path) == digest for path, digest in graph["after"].items())
    launch = read(BUILD / "primary_launch_identity.json")
    handles = read(BUILD / "terminal_handles.json")
    assert handles["all_owned_handles_terminal"]
    rows = handles["identities"] + [dict(launch["controller"], name="controller")]
    for row in rows:
        assert terminal(row), row
    receipt = BUILD / "primary_terminal_receipt.json"
    assert not receipt.exists(), receipt
    value = {"actual_host_proc": True, "unix": time.time(), "exec_session": int(sys.argv[1]),
             "exec_exit_code": 0, "seconds": status["seconds"], "identities": rows,
             "all_owned_handles_terminal": True, "source_pins_verified": len(pins),
             "expected_source_pins": EXPECTED_PINS, "binary_sha256": status["binary_sha256"],
             "test_binary_sha256": status["test_binary_sha256"], "generated_graph_verified": True,
             "passed": True}
    receipt.write_text(json.dumps(value, indent=2) + "\n")
    print(json.dumps({"passed": True, "seconds": status["seconds"], "pins": len(pins),
                      "terminal_ids": len(rows), "CFD": status["binary_sha256"],
                      "test": status["test_binary_sha256"]}))


if __name__ == "__main__":
    main()
