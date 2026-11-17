#!/usr/bin/env python3
"""Reuse the pinned house builder for the N13 test-only measurement."""
from pathlib import Path
ROOT = Path(__file__).resolve().parent
STAGED = ROOT.parents[3]
REPO = STAGED.parents[3]
BASE = STAGED / "night_20261115/N8/A2_A3_N12/section6_8/build534/build.py"
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
TOOLS = REPO / "integration_evidence/build-integrated-v2/tools"
namespace = {"__name__": "m25_test_builder", "__file__": str(BASE)}
exec(compile(BASE.read_text(), str(BASE), "exec"), namespace)
namespace.update(ROOT=ROOT, REPO=REPO, STAGED=STAGED, BUILD=BUILD, TOOLS=TOOLS,
                 GRAPH=(BUILD / "build.ninja", BUILD / "compile_commands.json"))
assert REPO.name == "SU2_NativeIntegrated" and namespace["run_build"].__globals__ is namespace
if __name__ == "__main__":
    namespace["run_build"]()
