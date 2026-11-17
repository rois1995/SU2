#!/usr/bin/env python3
"""Build CUT B5 Reports and finalization extraction with unchanged lifecycle."""
from pathlib import Path
ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[7]
STAGED = REPO / "integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57"
BASE = STAGED / "night_20261115/N8/A2_A3_N12/section6_8/build534/build.py"
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
TOOLS = REPO / "integration_evidence/build-integrated-v2/tools"
namespace = {"__name__": "cut_b5_candidate555_builder", "__file__": str(BASE)}
exec(compile(BASE.read_text(), str(BASE), "exec"), namespace)
namespace.update(ROOT=ROOT, REPO=REPO, STAGED=STAGED, BUILD=BUILD, TOOLS=TOOLS,
                 GRAPH=(BUILD / "build.ninja", BUILD / "compile_commands.json"))
assert REPO.name == "SU2_NativeIntegrated" and namespace["run_build"].__globals__ is namespace
if __name__ == "__main__":
    namespace["run_build"]()
