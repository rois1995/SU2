#!/usr/bin/env python3
"""N11 build wrapper reusing the sealed 534 build procedure."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[6]
STAGED = ROOT.parents[2]
SOURCE = (STAGED / "night_20261115/N8/A2_A3_N12/section6_8/build534/build.py")
BUILD = REPO / "integration_evidence/build-integrated-v3-p5-20261114"
TOOLS = REPO / "integration_evidence/build-integrated-v2/tools"
GRAPH = (BUILD / "build.ninja", BUILD / "compile_commands.json")


def load_builder():
    """Load the existing builder and bind only N11-owned output paths."""
    assert SOURCE.is_file(), SOURCE
    namespace = {"__name__": "_n11_build535_delegate", "__file__": str(SOURCE)}
    exec(compile(SOURCE.read_text(), str(SOURCE), "exec"), namespace)
    namespace.update(ROOT=ROOT, REPO=REPO, STAGED=STAGED, BUILD=BUILD,
                     TOOLS=TOOLS, GRAPH=GRAPH)
    assert namespace["run_build"].__globals__ is namespace
    return namespace


def self_check():
    """Check static build bindings without reading pins or launching a build."""
    assert ROOT.name == "build535" and REPO.name == "SU2_NativeIntegrated"
    assert STAGED.name == "staged57" and SOURCE.is_file()
    assert BUILD.is_dir() and (TOOLS / "ninja").is_file()
    assert (REPO / "integration_evidence/build-integrated-v2/compile-tmp").is_dir()
    builder = load_builder()
    assert builder["run_build"].__globals__["ROOT"] == ROOT
    assert builder["source_pins"].__globals__["BUILD"] == BUILD
    return {"passed": True, "launched": False, "source_builder": str(SOURCE),
            "root": str(ROOT), "repo": str(REPO), "staged": str(STAGED),
            "build": str(BUILD), "ninja": str(TOOLS / "ninja"),
            "graph_products": [str(path) for path in GRAPH],
            "source_pins": str(ROOT / "source_pins.json"),
            "source_pins_exist": (ROOT / "source_pins.json").is_file(),
            "pins_deferred_until_root_seals": True, "workers": 4, "nice": 19}


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
    else:
        load_builder()["run_build"]()
