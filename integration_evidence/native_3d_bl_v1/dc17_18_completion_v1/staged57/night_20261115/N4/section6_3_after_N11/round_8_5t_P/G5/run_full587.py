#!/usr/bin/env python3
"""Run the accepted two-event full-workflow gates with terminal build587."""
import argparse
import hashlib
import importlib
import importlib.util
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = HERE.parents[1]
NIGHT = N11.parents[1]
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
SECTION68 = NIGHT / "N8/A2_A3_N12/section6_8"
SECTION1 = NIGHT / "section6_1"
SECTION2 = NIGHT / "section6_2"
BUILD = N11 / "build587"
PURPOSE = HERE
CASE = PURPOSE / "full587"
FULL_VALIDATOR = SECTION68 / "validation/validate_n8.py"
FULL_CONTROLLER = SECTION2 / "run_full_section6_2.py"
POST_VALIDATOR = SECTION68 / "reports/validate_full_post534.py"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def prepare_build():
    aliases = load("n11_build587_aliases_full", N11 / "build587_aliases.py")
    result = aliases.verify_and_publish()
    return json.loads((BUILD / "build_status.json").read_text()), result


def load_full_validator():
    """Load accepted validation with an exact line token for the new LAW_SELECTION tag."""
    module = load("n11_validate_n8_exact_tokens", FULL_VALIDATOR)
    source = FULL_VALIDATOR.read_text()
    old, new = "if marker not in raw:", 'if marker + " " not in raw:'
    assert source.count(old) == 1
    namespace = module.__dict__
    exec(compile(source.replace(old, new, 1), str(FULL_VALIDATOR), "exec"), namespace)
    module.HERE = HERE
    module.BUILD = BUILD
    module.prepare_build = lambda: prepare_build()[0]
    return module


def postprocessor_source():
    source = POST_VALIDATOR.read_text()
    old_roots = ("HERE = Path(__file__).resolve().parent\nGROUP = HERE.parent\n"
                 'VALIDATION = GROUP / "validation"\nCASE = VALIDATION / "full_workflow"')
    new_roots = ("HERE = N11_VALIDATION / \"reports\"\nGROUP = N11_ROOT\n"
                 "VALIDATION = N11_VALIDATION\nCASE = FULL_CASE")
    assert source.count(old_roots) == 1
    source = source.replace(old_roots, new_roots, 1)
    old_loader = ('    spec = importlib.util.spec_from_file_location("post534_original", VALIDATION / "validate_n8.py")\n'
                  '    module = importlib.util.module_from_spec(spec)\n'
                  '    spec.loader.exec_module(module)')
    assert source.count(old_loader) == 1
    source = source.replace(old_loader, "    module = load_full_validator()", 1)
    old_prepare = "    module.prepare_build()"
    new_prepare = ("    module.HERE = VALIDATION\n"
                   "    module.BUILD = BUILD\n"
                   "    module.prepare_build = prepare_build_587\n"
                   "    module.prepare_build()")
    assert source.count(old_prepare) == 1
    source = source.replace(old_prepare, new_prepare, 1)
    old_pin = '"frozen_parent_helper_sha256": sha(VALIDATION / "validate_n8.py")'
    new_pin = f'"frozen_parent_helper_sha256": sha(Path({str(FULL_VALIDATOR)!r}))'
    assert source.count(old_pin) == 1
    source = source.replace(old_pin, new_pin, 1)
    return source


def postvalidate(case):
    """Execute accepted postprocessor logic with build587 and this frozen case."""
    namespace = {"__name__": "__main__", "__file__": str(POST_VALIDATOR),
                 "N11_VALIDATION": HERE, "N11_ROOT": N11,
                 "FULL_CASE": Path(case).resolve(strict=True), "BUILD": BUILD,
                 "Path": Path, "load_full_validator": load_full_validator,
                 "prepare_build_587": lambda: prepare_build()[0]}
    source = postprocessor_source()
    exec(compile(source, str(POST_VALIDATOR), "exec"), namespace)


def run(case=CASE):
    case = Path(case).resolve()
    assert case.parent == PURPOSE and not case.exists(), f"refusing to reuse full-run directory: {case}"
    status, _ = prepare_build()
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    assert status.get("pins_verified") is True and not status.get("changed")
    wrapper = load("n11_section62_full587", FULL_CONTROLLER)
    wrapper.HERE, wrapper.STAGED, wrapper.REPO = PURPOSE, STAGED, REPO
    wrapper.OLD = STAGED / "p7p8_r3_20261115"
    runner = wrapper.load_runner(status["binary"])
    runner.update(SCRIPT=Path(__file__).resolve(), PURPOSE=PURPOSE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=BUILD / "build_status.json", SOURCE_PINS=BUILD / "source_pins.json",
                  BINARY=Path(status["binary"]).resolve())
    assert runner["sha"](runner["BINARY"]) == status["binary_sha256"]
    for entry in (str(PURPOSE), str(HERE), str(wrapper.OLD)):
        while entry in sys.path:
            sys.path.remove(entry)
    sys.path[0:0] = [str(PURPOSE), str(HERE), str(wrapper.OLD)]
    sys.argv = [str(Path(__file__).resolve()), str(case), "--full"]
    try:
        runner["main"]()
    except SystemExit as exc:
        if exc.code not in (None, 0):
            raise
    postvalidate(case)
    receipt = json.loads((case / "n8_payload_identity.json").read_text())
    assert receipt.get("passed") is True and receipt.get("workflow_gates_passed") is True
    return receipt


def wall_law_line(line):
    return "NATIVE3D_WALL_LAW " in line


def controller_dependencies():
    """Resolve every accepted controller helper and frozen full-run input path."""
    return (
        PURPOSE / "check_connected_events.py", PURPOSE / "check_complete_events.py",
        PURPOSE / "check_timer_tree.py", PURPOSE / "unchanged_build499_source_pins.json",
        STAGED / "f3b_20261115/check_timer_tree.py",
        STAGED / "nobl_clean_check_20261114/run/provenance.json",
        STAGED / "nobl_clean_check_20261114/run/run.cfg",
        STAGED / "nobl_clean_check_20261114/first_collapse_receipt.json",
        STAGED / "nobl_cause_confirm_20261114/reference_counts.json",
        STAGED / "nobl_clean_check_20261114/extract_markers.py",
        STAGED / "m6_inputs/euler_adapted293_event1_v1/run.cfg",
        STAGED / "m6_inputs/euler_original_force_seed_v1/m6.su2",
        STAGED / "m6_inputs/euler_original_force_seed_v1/m6.su2.native_ref",
        STAGED / "m6_inputs/euler_original_force_seed_v1/baseline_euler_seed.dat",
        STAGED / "runtime_20261113_474/loaded_runtime_pins.json",
        STAGED / "run_cases.py",
        SECTION1 / "accepted_reference_payloads.json", FULL_VALIDATOR,
        FULL_CONTROLLER, POST_VALIDATOR, BUILD / "build_status.json",
        BUILD / "source_pins.json", N11 / "build587_aliases.py",
    )


def controller_preflight():
    """Resolve the exact runner globals and all prelaunch inputs without starting it."""
    status = json.loads((BUILD / "build_status.json").read_text())
    wrapper = load("n11_section62_full587_preflight", FULL_CONTROLLER)
    wrapper.HERE, wrapper.STAGED, wrapper.REPO = PURPOSE, STAGED, REPO
    wrapper.OLD = STAGED / "p7p8_r3_20261115"
    runner = wrapper.load_runner(status["binary"])
    runner.update(SCRIPT=Path(__file__).resolve(), PURPOSE=PURPOSE, R=STAGED, REPO=REPO,
                  BUILD_STATUS=BUILD / "build_status.json", SOURCE_PINS=BUILD / "source_pins.json",
                  BINARY=Path(status["binary"]).resolve())
    assert runner["PURPOSE"] == PURPOSE and runner["R"] == STAGED and runner["REPO"] == REPO
    assert runner["BUILD_STATUS"] == BUILD / "build_status.json"
    assert runner["SOURCE_PINS"] == BUILD / "source_pins.json"
    assert runner["CLEAN"] == STAGED / "nobl_clean_check_20261114/run"
    assert runner["REFERENCE494"] == STAGED / "nobl_clean_check_20261114/first_collapse_receipt.json"
    assert runner["REFERENCE293"] == STAGED / "nobl_cause_confirm_20261114/reference_counts.json"
    dependencies = controller_dependencies()
    assert all(path.is_file() for path in dependencies), dependencies
    modules = pinned_controller_modules()
    assert all(Path(module.__file__).resolve() == expected.resolve()
               for module, expected in modules), modules
    return {"purpose": str(runner["PURPOSE"]), "script": str(runner["SCRIPT"]),
            "build_status": str(runner["BUILD_STATUS"]),
            "dependency_count": len(dependencies), "dependencies": [str(p) for p in dependencies]}


def pinned_controller_modules():
    """Import the helpers the controller resolves by bare module name, pinned to PURPOSE."""
    names = ("check_connected_events", "check_complete_events", "check_timer_tree")
    expected = tuple(PURPOSE / f"{name}.py" for name in names)
    purpose = str(PURPOSE)
    while purpose in sys.path:
        sys.path.remove(purpose)
    sys.path.insert(0, purpose)
    try:
        modules = tuple(importlib.import_module(name) for name in names)
    finally:
        sys.path.remove(purpose)
    resolved = tuple((module, path) for module, path in zip(modules, expected))
    assert all(Path(module.__file__).resolve() == path.resolve()
               for module, path in resolved), resolved
    return resolved


def self_check():
    """Check inherited paths, acceptance hashes, fixed timer storage and exit handling."""
    assert HERE.name == "G5" and N11.name == "section6_3_after_N11" and BUILD == N11 / "build587"
    assert PURPOSE.is_dir() and CASE.parent == PURPOSE and not CASE.exists()
    assert FULL_VALIDATOR.is_file() and FULL_CONTROLLER.is_file() and POST_VALIDATOR.is_file()
    assert json.loads((BUILD / "build_status.json").read_text()).get("state") == "terminal"
    assert 'row.get("fixedstorage") == "4120"' in FULL_VALIDATOR.read_text()
    assert wall_law_line("NATIVE3D_WALL_LAW rank=0 boundary=exit")
    assert not wall_law_line("NATIVE3D_WALL_LAW_SELECTION rank=0 source=primary")
    adapted = postprocessor_source()
    compile(adapted, str(POST_VALIDATOR), "exec")
    assert "module = load_full_validator()" in adapted
    assert f'"frozen_parent_helper_sha256": sha(Path({str(FULL_VALIDATOR)!r}))' in adapted
    validator = load_full_validator()
    assert validator.full_wall_timer_report.__globals__ is validator.__dict__
    assert validator.HERE == HERE and validator.BUILD == BUILD
    assert validator.SECTION1 == SECTION1 and validator.SECTION2 == SECTION2
    dependencies = controller_dependencies()
    assert all(path.is_file() for path in dependencies), dependencies
    try:
        raise SystemExit(0)
    except SystemExit as exc:
        assert exc.code == 0
    hashes = json.loads((SECTION1 / "accepted_reference_payloads.json").read_text())["hashes"]
    assert len(hashes) == 6
    return {"passed": True, "launched": False, "build": str(BUILD),
            "case": str(CASE), "purpose": str(PURPOSE),
            "controller_preflight": controller_preflight(),
            "accepted_payload_count": len(hashes),
            "timer_fixedstorage": 4120, "postprocessor": str(POST_VALIDATOR)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--case", type=Path, default=CASE)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    print(json.dumps(run(args.case), indent=2))


if __name__ == "__main__":
    main()
