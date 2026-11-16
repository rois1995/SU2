#!/usr/bin/env python3
"""Expose only a terminal, fully verified build529 under Section 6.6 aliases."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build529"


def sha(path):
    """Hash one build artifact with SHA-256."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def expose(name, value):
    """Create a deterministic alias once and reject any conflicting overwrite."""
    path = HERE / name
    payload = json.dumps(value, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload, f"conflicting build529 alias: {path}"
    else:
        path.write_text(payload)


def main():
    """Verify build status, both binaries, all source pins, and owned terminal handles."""
    status = json.loads((BUILD / "build_status.json").read_text())
    pins = json.loads((BUILD / "source_pins.json").read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    assert sha(status["binary"]) == status["binary_sha256"]
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    assert all(sha(path) == digest for path, digest in pins.items())
    assert handles.get("all_owned_handles_terminal") is True
    expose("build529_status.json", dict(status, completed_build=True))
    expose("build529_source_pins.json", pins)
    expose("build529_terminal_handles.json", handles)


if __name__ == "__main__":
    import sys
    if sys.argv[1:] == ["--self-check"]:
        assert BUILD == HERE / "build529"
        assert "build529" in ("build529_status.json", "build529_source_pins.json",
                               "build529_terminal_handles.json")[0]
        print(json.dumps({"passed": True, "launched": False}))
    else:
        main()
