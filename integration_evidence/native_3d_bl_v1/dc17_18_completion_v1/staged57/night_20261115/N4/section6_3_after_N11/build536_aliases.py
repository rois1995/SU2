#!/usr/bin/env python3
"""Retarget the frozen build535 verifier to the accepted baseline build536."""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent / "reference_validation"
PHASE = HERE.parent
N4 = PHASE.parent
STAGED = N4.parents[1]
REPO = STAGED.parents[3]
BUILD = PHASE / "build536"
SOURCE = (STAGED / "night_20261115/N8/A2_A3_N12/section6_8/validation/build534_aliases.py")


def load_verifier():
    """Load the accepted alias verifier with real build536 and new output roots."""
    source = SOURCE.read_text()
    assert "build534" in source
    source = source.replace("build534", "build536")
    anchor = "STAGED = A2.parents[3]"
    assert source.count(anchor) == 1
    source = source.replace(anchor, "STAGED = A2.parents[2]")
    namespace = {"__name__": "n4_build536_alias_delegate", "__file__": str(SOURCE)}
    exec(compile(source, str(SOURCE), "exec"), namespace)
    namespace.update(HERE=HERE, A2=PHASE, STAGED=STAGED, REPO=REPO, BUILD=BUILD)
    assert namespace["main"].__globals__ is namespace
    return namespace


def self_check():
    """Check actual function globals and baseline inputs without publishing aliases."""
    helper = load_verifier()
    status = json.loads((BUILD / "build_status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    assert status["binary_sha256"] == "a144d90cdc50b6ca5d77e23f929294c271a7c15628b28136b65303f15e4083b1"
    assert status["test_binary_sha256"] == "b5fb04ffd903d94357467a35d8adc5e9c3ce32ce5dd562bd2539f55aa66f52f3"
    assert helper["verify_build"].__globals__["BUILD"] == BUILD
    assert helper["main"].__globals__["HERE"] == HERE
    return {"passed": True, "published": False, "validation": str(HERE),
            "build_root": str(BUILD), "test_binary": status["test_binary"],
            "test_binary_sha256": status["test_binary_sha256"]}


def verify_and_publish():
    """Verify build536 pins and publish only this new phase's aliases."""
    helper = load_verifier()
    return helper["main"]()


if __name__ == "__main__":
    print(json.dumps(self_check(), indent=2))
