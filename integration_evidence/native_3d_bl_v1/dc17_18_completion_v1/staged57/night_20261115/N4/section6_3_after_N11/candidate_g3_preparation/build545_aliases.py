#!/usr/bin/env python3
"""Verify frozen build545 artifacts and publish aliases only in this candidate folder."""
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
BUILD = PHASE / "build545"


def sha(path):
    """Return SHA256 for one pinned file."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def expose(name, payload):
    """Write idempotent build aliases within this new helper folder."""
    target = HERE / name
    body = json.dumps(payload, indent=2) + "\n"
    if target.exists():
        assert target.read_text() == body, f"refusing to replace frozen alias {target}"
    else:
        target.write_text(body)


def verify_build():
    """Verify build status, binaries, source pins, generated graph, and terminal handles."""
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


def publish():
    """Verify build545 and expose only folder-local run_cases aliases."""
    verified = verify_build()
    status, pins, handles = verified["status"], verified["pins"], verified["handles"]
    staged = PHASE.parents[2]
    sys.path.insert(0, str(staged))
    import run_cases as checks
    checks.ROOT = HERE
    checks.REPO = staged.parents[3]
    checks.BINARY = Path(status["test_binary"])
    expose("build545_status.json", dict(status, completed_build=True))
    expose("build545_source_pins.json", pins)
    expose("build545_terminal_handles.json", handles)
    return verified


def main():
    """Publish aliases after an independent verification pass."""
    result = publish()
    status = result["status"]
    print(json.dumps({"passed": True, "build": str(BUILD),
                      "binary_sha256": status["binary_sha256"],
                      "test_binary_sha256": status["test_binary_sha256"],
                      "terminal_owned_handles": len(result["handles"]["identities"])}, indent=2))


if __name__ == "__main__":
    main()
