#!/usr/bin/env python3
"""Expose immutable reference-build526 or candidate-build527 receipts locally."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build527"
CFD_SHA256 = "31c43b6e3bc1398535d8ebcc3e05476fd5c6a7172e8721d91fbd7d94845eed72"
TEST_SHA256 = "7245c5ef405dd83c5be785cf372b2064986cf3001ab22d1ba2aa7cd3d04cfd99"


def sha(path):
    """Hash one build artifact."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def expose(name, value):
    """Write an alias once, or require its existing bytes to match."""
    path = HERE / name
    payload = json.dumps(value, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload, f"conflicting build alias: {path}"
    else:
        path.write_text(payload)


def main():
    """Verify build527 terminal evidence before exposing per-case aliases."""
    status = json.loads((BUILD / "build_status.json").read_text())
    pins = json.loads((BUILD / "source_pins.json").read_text())
    handles = json.loads((BUILD / "terminal_handles.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0
    assert status["pins_verified"] and not status.get("changed")
    assert status["binary_sha256"] == CFD_SHA256
    assert status["test_binary_sha256"] == TEST_SHA256
    assert sha(status["binary"]) == status["binary_sha256"]
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    assert all(sha(path) == digest for path, digest in pins.items())
    assert handles.get("all_owned_handles_terminal") is True
    expose("build527_status.json", dict(status, completed_build=True))
    expose("build527_source_pins.json", pins)
    expose("build527_terminal_handles.json", handles)


if __name__ == "__main__":
    import sys
    if sys.argv[1:] == ["--self-check"]:
        assert BUILD == HERE / "build527"
        assert all((HERE / name).name != name.replace("527", "") for name in (
            "build527_status.json", "build527_source_pins.json", "build527_terminal_handles.json"))
        print(json.dumps({"passed": True, "launched": False}))
    else:
        main()
