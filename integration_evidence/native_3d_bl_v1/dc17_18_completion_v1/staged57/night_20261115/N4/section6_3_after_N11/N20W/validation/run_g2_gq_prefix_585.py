#!/usr/bin/env python3
"""Run one accepted N8 G2/GQ prefix through build585 and the 6.9 timer rule."""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

P = Path(__file__).resolve().parents[2]
HERE = P / "N20W/validation"
NIGHT = P.parent.parent
N14 = NIGHT / "N14/section6_12"
SECTION = P
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
N8 = NIGHT / "N8"
A2 = N8 / "A2_A3_N12/section6_8"
N10 = NIGHT / "N10"
BUILD = P / "build585"
BASE = A2 / "validation/run_g2_gq_prefix.py"
TIMER_HELPER = NIGHT / "N11/section6_8_preparation/drafts/compare_timer_census_delta_69.py"
CASE_DIR = HERE / "cases"


def load_validation():
    """Import the build585 validator binding used by the inherited controller."""
    spec = importlib.util.spec_from_file_location("validate_g2_gq", HERE / "validate_g2_gq.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    assert module.BUILD == BUILD and module.COMPARATOR68 == TIMER_HELPER
    return module


def load_runner():
    """Retarget the accepted prefix controller and bind it to N14 paths."""
    source = BASE.read_text()
    assert "build534" in source
    source = source.replace("build534", "build585")
    source = source.replace('extra_path = HERE / "unchanged_build499_source_pins.json"',
                            'extra_path = A2 / "validation/unchanged_build499_source_pins.json"')
    namespace = {"__name__": "n14_prefix_runner", "__file__": str(Path(__file__).resolve())}
    exec(compile(source, str(BASE), "exec"), namespace)
    namespace.update(HERE=HERE, A2=A2, GROUP=SECTION, STAGED=STAGED, NIGHT=NIGHT,
                     REPO=REPO, BUILD=BUILD, N4_DIR=NIGHT / "N4",
                     N4_SCRIPT=NIGHT / "N4/run_prefix_controller.py",
                     N68_COMPARATOR=TIMER_HELPER, CASES=CASE_DIR,
                     N1_FULL=NIGHT / "section6_1/full_workflow")
    return namespace


def publish_aliases():
    """Verify the terminal build and expose the expected build585 aliases."""
    alias_path = P / "build585_aliases.py"
    spec = importlib.util.spec_from_file_location("n14_prefix_build_aliases", alias_path)
    alias = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(alias)
    return alias.verify_and_publish()


def self_check():
    """Check dynamic path bindings and timer comparator selection without launch."""
    validation = load_validation()
    runner = load_runner()
    controller = runner["patched_controller"]()
    compile(controller, str(runner["N4_SCRIPT"]), "exec")
    env, imported = runner["namespace"]()
    assert imported is validation or imported.BUILD == BUILD
    assert env["BUILD"] == BUILD and env["PURPOSE"] == HERE
    assert env["N68_COMPARATOR"] == TIMER_HELPER
    assert validation.BUILD == BUILD and validation.COMPARATOR68 == TIMER_HELPER
    assert env["BUILD_STATUS"] == BUILD / "build_status.json"
    assert env["SOURCE_PINS"] == BUILD / "source_pins.json"
    assert (A2 / "validation/unchanged_build499_source_pins.json").is_file()
    return {"passed": True, "launched": False, "gate_interface": ["G2", "GQ"],
            "build": str(BUILD), "timer_comparator": str(TIMER_HELPER),
            "prefix_controller": str(runner["N4_SCRIPT"])}


def main():
    """Verify build aliases, bind validators, then run one requested prefix."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--case-name")
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if not args.gate or not args.case_name:
        parser.error("launch requires --gate and --case-name")
    publish_aliases()
    load_validation()
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    sys.argv = [str(BASE), "--gate", args.gate, "--case-name", args.case_name]
    load_runner()["main"]()


if __name__ == "__main__":
    main()
