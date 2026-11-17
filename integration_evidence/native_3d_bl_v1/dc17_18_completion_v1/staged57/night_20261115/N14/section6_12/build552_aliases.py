#!/usr/bin/env python3
"""N11 alias wrapper reusing the 534 verifier after build terminal."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent / "validation"
N11 = HERE.parent
STAGED = N11.parents[2]
REPO = STAGED.parents[3]
BUILD = N11 / "build552"
SOURCE = (STAGED / "night_20261115/N8/A2_A3_N12/section6_8/validation/build534_aliases.py")


def load_alias_helper():
    """Load the existing verifier with N11 validation and build locations."""
    assert SOURCE.is_file(), SOURCE
    namespace = {"__name__": "_n11_build552_alias_delegate", "__file__": str(SOURCE)}
    exec(compile(SOURCE.read_text(), str(SOURCE), "exec"), namespace)
    namespace.update(HERE=HERE, BUILD=BUILD)
    assert namespace["verify_build"].__globals__ is namespace
    return namespace


def self_check():
    """Check alias output roots without verifying or writing build aliases."""
    assert HERE.parent == N11 and STAGED.name == "staged57"
    assert REPO.name == "SU2_NativeIntegrated" and SOURCE.is_file()
    assert BUILD == N11 / "build552"
    helper = load_alias_helper()
    assert helper["verify_build"].__globals__["HERE"] == HERE
    assert helper["verify_build"].__globals__["BUILD"] == BUILD
    assert helper["expose"].__globals__["HERE"] == HERE
    return {"passed": True, "aliases_written": False, "validation": str(HERE),
            "build_root": str(BUILD), "verifier_source": str(SOURCE),
            "expected_aliases": [str(HERE / name) for name in (
                "build552_status.json", "build552_source_pins.json",
                "build552_terminal_handles.json")]}


def verify_and_publish():
    """Verify a terminal N11 build and expose aliases for reused house checks."""
    helper = load_alias_helper()
    verified = helper["verify_build"]()
    status, pins, handles = (verified["status"], verified["pins"],
                             verified["handles"])
    sys.path.insert(0, str(STAGED))
    import run_cases as checks
    checks.ROOT = HERE
    checks.REPO = REPO
    checks.BINARY = Path(status["test_binary"])
    helper["expose"]("build552_status.json", dict(status, completed_build=True))
    helper["expose"]("build552_source_pins.json", pins)
    helper["expose"]("build552_terminal_handles.json", handles)
    return {"binary": status["binary"], "binary_sha256": status["binary_sha256"],
            "test_binary": str(checks.BINARY),
            "test_binary_sha256": status["test_binary_sha256"]}


if __name__ == "__main__":
    print(json.dumps(self_check(), indent=2))
