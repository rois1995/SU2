#!/usr/bin/env python3
"""Verify build534's terminal pins and expose its binary to reused house checks."""
import hashlib
import json
import sys
from pathlib import Path
HERE = Path(__file__).resolve().parent
A2 = HERE.parent
BUILD = A2 / "build534"

def expose(name, value):
    """Publish build aliases idempotently for existing house helpers."""
    path = HERE / name
    payload = json.dumps(value, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload
    else:
        path.write_text(payload)

def sha(path):
    """Hash a file for exact provenance checks."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()

def verify_build():
    """Verify terminal binaries, source pins, generated graph and actual-host handles."""
    status = json.loads((BUILD / "build_status.json").read_text())
    pins = json.loads((BUILD / "source_pins.json").read_text())
    graph = json.loads((BUILD / "generated_build_graph_after.json").read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    assert sha(status["binary"]) == status["binary_sha256"]
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    assert all(sha(name) == digest for name, digest in pins.items())
    assert graph["build_status_sha256"] == sha(BUILD / "build_status.json")
    assert graph["source_pins_sha256"] == sha(BUILD / "source_pins.json")
    assert graph["binary_sha256"] == status["binary_sha256"]
    assert graph["test_binary_sha256"] == status["test_binary_sha256"]
    assert graph.get("after") and all(Path(name).is_file() and sha(name) == digest
                                       for name, digest in graph["after"].items())
    identities = handles.get("identities", [])
    assert handles.get("actual_host_proc") is True and handles.get("all_owned_handles_terminal") is True
    assert identities and all(isinstance(row.get("pid"), int) and
                              isinstance(row.get("start_ticks"), int) and not row.get("alive")
                              for row in identities)
    return {"status": status, "pins": pins, "graph": graph, "handles": handles}


def main():
    """Verify build534 and publish idempotent aliases for reused house checks."""
    verified = verify_build()
    status, pins, handles = verified["status"], verified["pins"], verified["handles"]
    STAGED = A2.parents[3]
    sys.path.insert(0, str(STAGED))
    import run_cases as checks
    checks.ROOT = HERE
    checks.REPO = STAGED.parents[3]
    checks.BINARY = Path(status["test_binary"])
    expose("build534_status.json", dict(status, completed_build=True))
    expose("build534_source_pins.json", pins)
    expose("build534_terminal_handles.json", handles)
    return {"binary": str(status["binary"]), "binary_sha256": status["binary_sha256"],
            "test_binary": str(checks.BINARY), "test_binary_sha256": status["test_binary_sha256"],
            "pins_sha256": sha(BUILD / "source_pins.json"),
            "graph_sha256": sha(BUILD / "generated_build_graph_after.json"),
            "terminal_owned_handles": len(handles["identities"])}

if __name__ == "__main__":
    print(json.dumps(main(), indent=2))
