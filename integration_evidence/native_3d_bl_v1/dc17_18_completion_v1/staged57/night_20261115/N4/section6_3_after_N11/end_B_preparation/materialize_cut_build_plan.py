#!/usr/bin/env python3
"""Write only the minimal run_cut_checks receipt after build558 is sealed."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE.parent / "build558"


def sha(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def process_is_live(pid: int, start_ticks: int) -> bool:
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(")", 1)[1].split()
        return int(fields[19]) == start_ticks and fields[0] not in {"Z", "X"}
    except (OSError, ValueError, IndexError):
        return False


def make_plan() -> dict:
    status = json.loads((BUILD / "build_status.json").read_text())
    pins_path = BUILD / "source_pins.json"
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    terminal = json.loads((BUILD / "primary_terminal_receipt.json").read_text())
    graph = json.loads((BUILD / "generated_build_graph_after.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    pins = json.loads(pins_path.read_text())
    assert pins and all(sha(Path(path)) == digest for path, digest in pins.items())
    assert sha(Path(status["binary"])) == status["binary_sha256"]
    assert sha(Path(status["test_binary"])) == status["test_binary_sha256"]
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert terminal.get("actual_host_proc") is True and terminal.get("passed") is True
    assert terminal.get("all_owned_handles_terminal") is True and terminal.get("identities")
    assert all(not row.get("alive") for row in terminal["identities"])
    for row in handles.get("identities", []) + terminal["identities"]:
        assert row.get("alive") is not True
        assert not process_is_live(int(row["pid"]), int(row["start_ticks"]))
    assert graph.get("source_pins_sha256") == sha(pins_path)
    assert graph.get("build_status_sha256") == sha(BUILD / "build_status.json")
    return {"build_label": "build558", "source_pins_sha256": sha(pins_path)}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true", help="write build558/cut_build_plan.json once")
    args = parser.parse_args()
    plan = make_plan()
    target = BUILD / "cut_build_plan.json"
    if args.write:
        if target.exists():
            raise FileExistsError(target)
        target.write_text(json.dumps(plan, indent=2) + "\n")
    print(json.dumps({"passed": True, "written": args.write, "path": str(target), "plan": plan}, indent=2))


if __name__ == "__main__":
    main()
