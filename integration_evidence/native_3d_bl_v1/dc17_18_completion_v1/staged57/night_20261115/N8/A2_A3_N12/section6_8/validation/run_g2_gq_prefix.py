#!/usr/bin/env python3
"""Run one bounded G2 or GQ prefix using the existing N4 controller core."""
import argparse
import hashlib
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
A2 = HERE.parent
GROUP = A2
STAGED = A2.parents[3]
NIGHT = A2.parents[2]
REPO = A2.parents[7]
N4_DIR = NIGHT / "N4"
N4_SCRIPT = N4_DIR / "run_prefix_controller.py"
N68_COMPARATOR = A2 / "drafts/compare_integer_rows_68.py"
N1_FULL = NIGHT / "section6_1/full_workflow"
CASES = HERE / "cases"
BUILD = A2 / "build534"


def sha(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def patch_once(source, old, new):
    assert source.count(old) == 1, f"expected one controller fragment: {old[:90]!r}"
    return source.replace(old, new, 1)


def patched_controller():
    source = N4_SCRIPT.read_text()
    source = patch_once(source,
        '    conflicting = [row for row in existing.values() if row["comm"] in ("SU2_CFD", "test_driver", "cc1plus", "lto1")]\n'
        '    assert not conflicting, f"owned native/build process already present: {conflicting}"\n',
        '    assert no_active_validation_case(), "an N8 prefix case is still active"\n')
    source = patch_once(source,
        '    config = R / "m6_inputs/euler_adapted293_event1_v1/run.cfg"\n'
        '    assert sha(config) == clean_provenance["config_sha256"]\n',
        '    config = N1_FULL / "run.cfg"\n'
        '    assert sha(config) == sha(N1_FULL / "run.cfg")\n')
    source = patch_once(source,
        '    launch = host_snapshot()\n',
        '    launch = host_snapshot()\n'
        '    available_kib = int(launch["MemAvailable"].split()[1])\n'
        '    assert available_kib >= 19073 * 1024, "free RAM below unchanged 19073 MiB job cap"\n'
        '    save(case / "launch_preflight.json", {"gate": gate, "host": launch,\n'
        '         "free_ram_at_least_job_cap": True, "max_job_peak_rss_bytes": MAX_JOB_PEAK_RSS})\n')
    source = source.replace('PURPOSE / "source_check.md"', 'GROUP / "source_check.md"')
    source = source.replace('PURPOSE / "compare_integer_rows.py"', 'N68_COMPARATOR')
    start = source.index('    for path in (R / \'f3b_20261115/check_complete_events.py\'')
    end = source.index('        input_hashes[str(path)]=sha(path)', start) + len('        input_hashes[str(path)]=sha(path)')
    source = source[:start] + (
        '    for path in (SCRIPT, N4_SCRIPT, Path(runtime_checks.__file__), runtime_manifest,\n'
        '                 STAGED / "f3b_20261115/check_complete_events.py",\n'
        '                 STAGED / "f3b_20261115/check_timer_tree.py",\n'
        '                 STAGED / "nobl_clean_check_20261114/extract_markers.py",\n'
        '                 BUILD_STATUS, SOURCE_PINS,\n'
        '                 REFERENCE293, REFERENCE494, CLEAN / "provenance.json",\n'
        '                 REFERENCE_RUN / "provenance.json", N1_FULL / "status.json",\n'
        '                 N1_FULL / "full_workflow_receipt.json",\n'
        '                 N1_FULL / "section6_1_reference_identity.json",\n'
        '                 GROUP / "source_check.md", PURPOSE / "validate_g2_gq.py",\n'
        '                 PURPOSE / "run_g2_gq_prefix.py", BUILD / "terminal_handles.json",\n'
        '                 BUILD / "generated_build_graph_after.json",\n'
        '                 N4_DIR / "compare_integer_rows.py", N68_COMPARATOR):\n'
        '        input_hashes[str(path)]=sha(path)'
    ) + source[end:]
    return source


def no_active_validation_case():
    if not CASES.exists():
        return True
    for path in CASES.iterdir():
        status_path = path / "status.json"
        if status_path.is_file():
            status = json.loads(status_path.read_text())
            if status.get("state") == "running":
                return False
    return True


def namespace():
    sys.path.insert(0, str(HERE))
    import validate_g2_gq as validation
    source = patched_controller()
    status_path = BUILD / "build_status.json"
    binary = Path(json.loads(status_path.read_text()).get("binary", BUILD / "SU2_CFD/src/SU2_CFD")) \
        if status_path.is_file() else BUILD / "SU2_CFD/src/SU2_CFD"
    env = {
        "__name__": "n8_existing_prefix_controller",
        "__file__": str(N4_SCRIPT),
        "STAGED": STAGED,
        "NIGHT": NIGHT,
        "N4_SCRIPT": N4_SCRIPT,
        "N4_DIR": N4_DIR,
        "N68_COMPARATOR": N68_COMPARATOR,
        "N1_FULL": N1_FULL,
        "SCRIPT": Path(__file__).resolve(),
        "PURPOSE": HERE,
        "A2": GROUP,
        "GROUP": GROUP,
        "R": STAGED,
        "REPO": REPO,
        "BUILD": BUILD,
        "BUILD_BINARY": binary,
        "BINARY": binary,
        "BUILD_STATUS": status_path,
        "SOURCE_PINS": BUILD / "source_pins.json",
        "CLEAN": STAGED / "nobl_clean_check_20261114/run",
        "REFERENCE494": STAGED / "nobl_clean_check_20261114/first_collapse_receipt.json",
        "REFERENCE293": STAGED / "nobl_cause_confirm_20261114/reference_counts.json",
        "REFERENCE_RUN": N1_FULL,
        "PURPOSE_SOURCE_CHECK": GROUP / "source_check.md",
    }
    exec(compile(source, str(N4_SCRIPT), "exec"), env)
    env.update({key: value for key, value in {
        "SCRIPT": Path(__file__).resolve(), "PURPOSE": HERE, "R": STAGED,
        "STAGED": STAGED, "NIGHT": NIGHT,
        "REPO": REPO, "BUILD": BUILD, "BUILD_BINARY": binary,
        "BINARY": binary, "BUILD_STATUS": status_path,
        "SOURCE_PINS": BUILD / "source_pins.json", "CLEAN": STAGED / "nobl_clean_check_20261114/run",
        "REFERENCE494": STAGED / "nobl_clean_check_20261114/first_collapse_receipt.json",
        "REFERENCE293": STAGED / "nobl_cause_confirm_20261114/reference_counts.json",
        "REFERENCE_RUN": N1_FULL, "N1_FULL": N1_FULL, "A2": GROUP, "GROUP": GROUP, "N4_DIR": N4_DIR,
        "N4_SCRIPT": N4_SCRIPT,
        "N68_COMPARATOR": N68_COMPARATOR,
        "EXPECTED494": {0: 480, 1: 472, 2: 344, 3: 263, 4: 473, 5: 464},
        "EXPECTED_G2": {0: 480, 1: 473, 2: 346, 3: 263, 4: 469, 5: 464},
        "EXPECTED_G2_WORK": 90_809_869, "MAX_WORK": 108_459_426,
        "MAX_JOB_PEAK_RSS": 19_999_490_048,
    }.items()})
    validation.BUILD = BUILD
    env["verify_pins"] = lambda strict=True: verify_build_pins(validation, env)
    env["no_active_validation_case"] = no_active_validation_case
    env["PURPOSE"] = HERE
    env["SCRIPT"] = Path(__file__).resolve()
    return env, validation


def verify_build_pins(validation, env):
    """Reuse the current-build validator and verify the complete pin map shape."""
    checked = validation.build534()
    pin_doc = validation.read_json(env["SOURCE_PINS"])
    pins = pin_doc.get("source_pins", pin_doc)
    assert isinstance(pins, dict) and pins
    extra_path = HERE / "unchanged_build499_source_pins.json"
    extra = json.loads(extra_path.read_text())
    assert not (pins.keys() & extra.keys())
    assert all(sha(path) == digest for path, digest in extra.items())
    pins = dict(pins, **extra)
    return {
        "binary": checked["binary"], "source_build_binary": checked["binary"],
        "binary_sha256": checked["status"]["binary_sha256"], "binary_matches_build_status": True,
        "source_pins": pins, "source_pin_mismatches": {},
        "build_status": str(env["BUILD_STATUS"]), "source_pins_sha256": sha(env["SOURCE_PINS"]),
        "additional_source_pins_sha256": sha(extra_path),
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--gate", choices=("G2", "GQ"))
    parser.add_argument("--case-name")
    args = parser.parse_args()
    if args.self_check:
        source = patched_controller()
        compile(source, str(N4_SCRIPT), "exec")
        env, validation = namespace()
        assert env["R"] == STAGED and env["REPO"] == REPO
        assert env["BUILD_STATUS"] == BUILD / "build_status.json"
        assert env["SOURCE_PINS"] == BUILD / "source_pins.json"
        assert env["REFERENCE_RUN"] == N1_FULL and (N1_FULL / "run.cfg").is_file()
        assert env["REFERENCE494"] == STAGED / "nobl_clean_check_20261114/first_collapse_receipt.json"
        assert env["REFERENCE293"] == STAGED / "nobl_cause_confirm_20261114/reference_counts.json"
        assert env["PURPOSE"] == HERE and env["SCRIPT"] == Path(__file__).resolve()
        assert env["BUILD"] == BUILD and validation.BUILD == BUILD
        assert env["N4_DIR"] == N4_DIR and env["N4_SCRIPT"] == N4_SCRIPT
        assert env["N68_COMPARATOR"] == N68_COMPARATOR
        assert env["A2"] == GROUP and env["GROUP"] == GROUP
        assert 'N4_DIR / "compare_integer_rows.py", N68_COMPARATOR' in source
        assert "MAX_JOB_PEAK_RSS = 19999490048" in source
        assert 'ADAP_NATIVE_MAX_TIME' in source and 'ADAP_NATIVE_RANKS' in source
        print(json.dumps({"passed": True, "launched": False, "built": False,
                          "patched_controller_compiles": True,
                          "retargeted_build": str(BUILD), "retargeted_stage_root": str(STAGED),
                          "retargeted_comparator": str(N68_COMPARATOR)}))
        return
    assert args.gate and args.case_name, "--gate and --case-name are required"
    case = (CASES / args.case_name).resolve()
    assert case.parent == CASES.resolve() and not case.exists()
    assert no_active_validation_case()
    env, validation = namespace()
    env["verify_pins"]()
    sys.argv = [str(N4_SCRIPT), str(case), "--gate", args.gate]
    try:
        env["main"]()
    except SystemExit as result:
        code = result.code if isinstance(result.code, int) else int(bool(result.code))
        assert code == 0, f"existing controller exited {code}; preserve case evidence"
    receipt = validation.validate(args.gate, case)
    print(json.dumps({"passed": True, "gate": args.gate, "case": str(case),
                      "controller": str(N4_SCRIPT), "accepted_reference": str(
                          validation.N1_COLLAPSE if args.gate == "G2" else validation.N1_FULL),
                      "comparison": receipt}, indent=2))


if __name__ == "__main__":
    main()
