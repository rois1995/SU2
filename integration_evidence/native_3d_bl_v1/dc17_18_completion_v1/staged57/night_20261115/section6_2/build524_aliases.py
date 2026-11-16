"""Expose immutable build524 receipts under case-runner aliases."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
BUILD = HERE / "build524"


def sha(path):
    """Hash one pinned build artifact."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def expose(name, value):
    """Write an alias only when absent or byte-equivalent."""
    path = HERE / name
    payload = json.dumps(value, indent=2) + "\n"
    if path.exists():
        assert path.read_text() == payload, f"conflicting build alias: {path}"
    else:
        path.write_text(payload)


def main():
    """Verify build524 pins and publish local aliases."""
    status_path = BUILD / "build_status.json"
    pins_path = BUILD / "source_pins.json"
    handles_path = BUILD / "terminal_handles.json"
    status = json.loads(status_path.read_text())
    pins = json.loads(pins_path.read_text())
    handles = json.loads(handles_path.read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0
    assert status["pins_verified"] and not status.get("changed")
    assert sha(status["binary"]) == status["binary_sha256"]
    assert sha(status["test_binary"]) == status["test_binary_sha256"]
    assert all(sha(path) == digest for path, digest in pins.items())
    assert handles.get("all_owned_handles_terminal") is True
    expose("build524_status.json", dict(status, completed_build=True))
    expose("build524_source_pins.json", pins)
    expose("build524_terminal_handles.json", handles)


if __name__ == "__main__":
    import sys
    if sys.argv[1:] == ["--self-check"]:
        assert BUILD == HERE / "build524"
        assert "build524_status.json" != "build_status.json"
        assert "build524_source_pins.json" != "source_pins.json"
        assert "build524_terminal_handles.json" != "terminal_handles.json"
        print(json.dumps({"passed": True, "launched": False}))
    else:
        main()
