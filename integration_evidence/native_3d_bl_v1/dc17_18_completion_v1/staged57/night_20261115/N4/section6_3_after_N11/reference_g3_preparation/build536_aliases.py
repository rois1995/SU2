#!/usr/bin/env python3
"""Verify accepted build536 and publish aliases only in this new G3 folder."""
import hashlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
N4 = PHASE.parent
NIGHT = N4.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
BUILD = PHASE / "build536"
ALIAS_SOURCE = PHASE / "build536_aliases.py"
EXPECTED_BINARY = "a144d90cdc50b6ca5d77e23f929294c271a7c15628b28136b65303f15e4083b1"
EXPECTED_TEST = "b5fb04ffd903d94357467a35d8adc5e9c3ce32ce5dd562bd2539f55aa66f52f3"


def sha(path):
    """Hash one exact build input or output."""
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verifier():
    """Retarget the existing read-only build verifier without changing it."""
    spec = importlib.util.spec_from_file_location("n4_build536_alias_source", ALIAS_SOURCE)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    helper = module.load_verifier()
    helper.update(HERE=HERE, A2=PHASE, STAGED=STAGED, REPO=REPO, BUILD=BUILD)
    assert helper["verify_build"].__globals__ is helper
    assert helper["main"].__globals__ is helper
    return helper


def verify_build():
    """Require the accepted CFD/test hashes, graph, source pins, and terminal handles."""
    helper = verifier()
    result = helper["verify_build"]()
    status = result["status"]
    assert status["binary_sha256"] == EXPECTED_BINARY
    assert status["test_binary_sha256"] == EXPECTED_TEST
    assert result["handles"]["actual_host_proc"] is True
    assert result["handles"]["all_owned_handles_terminal"] is True
    return result


def publish():
    """Publish the verified build aliases into this new preparation folder."""
    helper = verifier()
    result = helper["verify_build"]()
    status = result["status"]
    assert status["binary_sha256"] == EXPECTED_BINARY
    assert status["test_binary_sha256"] == EXPECTED_TEST
    receipt = helper["main"]()
    return {"receipt": receipt, "status": status, "handles": result["handles"]}


def main():
    """Provide the import-compatible alias entry point used by the accepted safeguard."""
    return publish()["receipt"]


def self_check():
    """Check the real build verifier and its retargeted function globals without publishing."""
    result = verify_build()
    assert verifier()["HERE"] == HERE
    return {"passed": True, "published": False, "build": str(BUILD),
            "binary_sha256": result["status"]["binary_sha256"],
            "test_binary_sha256": result["status"]["test_binary_sha256"],
            "terminal_owned_handles": len(result["handles"]["identities"])}


if __name__ == "__main__":
    print(json.dumps(self_check(), indent=2))
