#!/usr/bin/env python3
"""Expose build590 aliases only after the root seals its terminal build receipts."""
import json
import sys
from pathlib import Path

_FILE = Path(__file__).resolve()
P = _FILE.parent if _FILE.parent.name == "section6_3_after_N11" else _FILE.parents[2]
HERE = P / "N22M/validation"
STAGED = P.parents[2]
REPO = STAGED.parents[3]
BUILD = P / "build590"
SOURCE = STAGED / "night_20261115/N8/A2_A3_N12/section6_8/validation/build534_aliases.py"


def load_alias_helper():
    """Load the known build534 verifier with build590 paths and output root."""
    assert SOURCE.is_file(), SOURCE
    namespace = {"__name__": "_b8_build590_alias_delegate", "__file__": str(SOURCE)}
    exec(compile(SOURCE.read_text(), str(SOURCE), "exec"), namespace)
    namespace.update(HERE=HERE, BUILD=BUILD)
    assert namespace["verify_build"].__globals__ is namespace
    return namespace


def self_check():
    """Check path bindings without reading a build receipt or publishing aliases."""
    assert HERE == P / "N22M/validation"
    assert STAGED.name == "staged57"
    assert REPO.name == "SU2_NativeIntegrated"
    assert BUILD == P / "build590"
    helper = load_alias_helper()
    assert helper["verify_build"].__globals__["HERE"] == HERE
    assert helper["verify_build"].__globals__["BUILD"] == BUILD
    assert helper["expose"].__globals__["HERE"] == HERE
    return {"passed": True, "aliases_written": False, "build_receipt_read": False,
            "validation": str(HERE), "build_root": str(BUILD), "verifier_source": str(SOURCE)}


def verify_and_publish():
    """Verify a sealed build590 and publish only its standard validation aliases."""
    helper = load_alias_helper()
    verified = helper["verify_build"]()
    status, pins, handles = verified["status"], verified["pins"], verified["handles"]
    sys.path.insert(0, str(STAGED))
    import run_cases as checks
    checks.ROOT = HERE
    checks.REPO = REPO
    checks.BINARY = Path(status["test_binary"])
    helper["expose"]("build590_status.json", dict(status, completed_build=True))
    helper["expose"]("build590_source_pins.json", pins)
    helper["expose"]("build590_terminal_handles.json", handles)
    return {"binary": status["binary"], "binary_sha256": status["binary_sha256"],
            "test_binary": str(checks.BINARY), "test_binary_sha256": status["test_binary_sha256"]}


if __name__ == "__main__":
    print(json.dumps(self_check(), indent=2))
