#!/usr/bin/env python3
"""Run and strictly validate candidate G2/GQ prefixes against fresh build536 refs."""
import argparse
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
PHASE = HERE.parent
N4 = PHASE.parent
NIGHT = N4.parent
STAGED = NIGHT.parent
REPO = STAGED.parents[3]
REF_DIR = N4 / "section6_3_after_N11/reference_prefix_preparation"
REF_CASES = REF_DIR / "cases"
REF_G2 = REF_CASES / "G2_reference536"
N1 = NIGHT / "section6_1"
N8V = NIGHT / "N8/A2_A3_N12/section6_8/validation"
BUILD536 = N4 / "section6_3_after_N11/build536"
REFERENCE_HELPER = REF_DIR / "run_prefix536.py"
STRICT_COMPARATOR = HERE / "compare_candidate_prefix.py"


def sha(path):
    return hashlib.file_digest(Path(path).open("rb"), "sha256").hexdigest()


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def verify_candidate_build(root):
    """Use accepted N8 build verification against the caller's candidate build root."""
    validator = load("candidate_prefix_build_validator", N8V / "validate_g2_gq.py")
    validator.HERE = HERE
    validator.BUILD = Path(root).resolve(strict=True)
    build = validator.build534()
    return validator, build


def configured(root):
    helper = load("candidate_reference_controller", REFERENCE_HELPER)
    helper.HERE = HERE
    helper.BUILD = Path(root).resolve(strict=True)
    helper.PHASE = PHASE
    runner, validator, env = helper.configured()
    assert validator.BUILD == helper.BUILD
    assert runner.BUILD == helper.BUILD
    assert env["BUILD_STATUS"] == helper.BUILD / "build_status.json"
    return helper, runner, validator, env


def validate_reference(case, gate, validator, n1):
    case = Path(case).resolve(strict=True)
    assert case.parent == REF_CASES.resolve(), case
    status = json.loads((case / "status.json").read_text())
    provenance = json.loads((case / "provenance.json").read_text())
    assert status.get("state") == "terminal" and status.get("all_owned_handles_terminal") is True
    assert status.get("gate") == gate and status.get("gate_passed") is True
    assert provenance.get("mode") == "reference_capture" and provenance.get("gate") == gate
    build_status = json.loads((BUILD536 / "build_status.json").read_text())
    assert provenance.get("binary_sha256") == build_status.get("binary_sha256")
    assert provenance.get("source_pins_sha256") == sha(BUILD536 / "source_pins.json")
    assert provenance.get("config_sha256") == sha(N1 / "full_workflow/run.cfg")
    ref_inputs = provenance.get("input_hashes", {})
    n1_inputs = n1["provenance"].get("input_hashes", {})
    for filename in ("m6.su2", "m6.su2.native_ref", "baseline_euler_seed.dat"):
        ref_values = {Path(path).name: digest for path, digest in ref_inputs.items() if Path(path).name == filename}
        n1_values = {Path(path).name: digest for path, digest in n1_inputs.items() if Path(path).name == filename}
        assert len(ref_values) == len(n1_values) == 1 and ref_values[filename] == n1_values[filename]
    accepted_receipt = json.loads((case / "fresh_build536_prefix_reference.json").read_text())
    assert accepted_receipt.get("passed") is True and accepted_receipt.get("gate") == gate
    if gate == "G2":
        validator.verify_g2_receipt(case)
        assert accepted_receipt.get("accepted_N1_reference") == str(N1 / "first_collapse")
    else:
        validator.verify_gq_receipt(case)
        assert accepted_receipt.get("accepted_N1_reference") == str(N1 / "full_workflow")
    assert len(list(case.glob("rank_output/*/rank.*/stdout"))) == 6
    return {"case": str(case), "status_sha256": sha(case / "status.json"),
            "provenance_sha256": sha(case / "provenance.json"),
            "build536_binary_sha256": build_status["binary_sha256"],
            "build536_source_pins_sha256": sha(BUILD536 / "source_pins.json")}


def compare(reference, candidate, gate):
    comparator = load("candidate_prefix_strict_comparator", STRICT_COMPARATOR)
    return comparator.compare_cases(reference, candidate, gate)


def frozen_reference_artifacts(case):
    case = Path(case).resolve(strict=True)
    logs = sorted(case.glob("rank_output/*/rank.*/stdout"))
    assert len(logs) == 6, f"expected six frozen reference rank logs: {case}"
    paths = [case / name for name in ("status.json", "provenance.json", "fresh_build536_prefix_reference.json")]
    paths.extend(logs)
    return {str(path.resolve(strict=True)): sha(path) for path in paths}


def bind_frozen_inputs(env, reference_cases):
    """Freeze helper and reference artifacts in both launch pins and provenance."""
    frozen = {str(Path(__file__).resolve()): sha(__file__),
              str(STRICT_COMPARATOR.resolve(strict=True)): sha(STRICT_COMPARATOR),
              str(REFERENCE_HELPER.resolve(strict=True)): sha(REFERENCE_HELPER)}
    for case in reference_cases:
        frozen.update(frozen_reference_artifacts(case))
    original_verify = env["verify_pins"]
    def verify_with_helpers(strict=True):
        result = original_verify(strict)
        pins = dict(result["source_pins"])
        pins.update({path: sha(path) for path in frozen})
        result["source_pins"] = pins
        return result
    env["verify_pins"] = verify_with_helpers
    original_save = env["save"]
    def save_with_helpers(path, value):
        if Path(path).name == "provenance.json":
            value["input_hashes"].update(frozen)
        return original_save(path, value)
    env["save"] = save_with_helpers
    env["SCRIPT"] = Path(__file__).resolve()
    return frozen


def verify_frozen_inputs(frozen):
    changed = {path: {"expected": digest, "actual": sha(path)}
               for path, digest in frozen.items() if sha(path) != digest}
    assert not changed, f"frozen helper/reference artifacts changed: {changed}"


def preflight():
    mem = Path("/proc/meminfo").read_text().splitlines()
    available = int(next(row.split()[1] for row in mem if row.startswith("MemAvailable:"))) // 1024
    result = {"hostname": os.uname().nodename,
              "boot_id": Path("/proc/sys/kernel/random/boot_id").read_text().strip(),
              "pid1_present": Path("/proc/1/stat").is_file(),
              "loadavg": Path("/proc/loadavg").read_text().strip(),
              "mem_available_mib": available,
              "scratch_free_bytes": shutil.disk_usage(HERE).free}
    assert result["boot_id"] and result["pid1_present"]
    assert available >= 19073 and result["scratch_free_bytes"] > 2_000_000_000
    return result


class LaunchStop(Exception):
    pass


def self_check(build_root):
    comparator = load("candidate_prefix_comparator_selfcheck", STRICT_COMPARATOR)
    assert comparator.self_check()["passed"]
    validator, build = verify_candidate_build(build_root)
    helper, runner, validator, env = configured(build_root)
    assert env["BUILD_STATUS"] == Path(build_root).resolve() / "build_status.json"
    assert env["GROUP"] == PHASE and env["A2"] == PHASE
    assert env["PURPOSE_SOURCE_CHECK"] == PHASE / "source_check.md"
    assert validator.N1_FULL == N1 / "full_workflow"
    assert validator.BUILD == Path(build_root).resolve()
    assert 'GROUP / "source_check.md"' in runner.patched_controller()
    n1 = validator.accepted_n1()
    ref_meta = validate_reference(REF_G2, "G2", validator, n1)
    frozen = bind_frozen_inputs(env, [ref_meta["case"]])
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    case = HERE / "cases" / f".selfcheck_{os.getpid()}"
    original = subprocess.Popen
    captured = {}
    def stub(*args, **kwargs):
        argv = args[0] if args else kwargs.get("args")
        argv = list(map(str, argv)) if isinstance(argv, (list, tuple)) else [str(argv)]
        if argv and (argv[0] == "nice" or "mpirun" in argv):
            captured.update(argv=argv, env=kwargs.get("env", {}), cwd=str(kwargs.get("cwd")),
                            start_new_session=kwargs.get("start_new_session"))
            raise LaunchStop()
        return original(*args, **kwargs)
    subprocess.Popen = stub
    try:
        runner.CASES.mkdir(exist_ok=True)
        sys.argv = [str(N4 / "run_prefix_controller.py"), str(case), "--gate", "G2"]
        before = env["verify_pins"]()
        assert set(frozen).issubset(before["source_pins"])
        try:
            env["main"]()
        except LaunchStop:
            pass
    finally:
        subprocess.Popen = original
    provenance = json.loads((case / "provenance.json").read_text())
    assert str(PHASE / "source_check.md") in provenance["input_hashes"]
    assert str(N8V.parent / "source_check.md") not in provenance["input_hashes"]
    assert set(frozen).issubset(provenance["input_hashes"])
    verify_frozen_inputs(frozen)
    if case.exists():
        shutil.rmtree(case)
    assert captured and captured["start_new_session"] is True
    mpi = captured["argv"].index("mpirun")
    assert captured["argv"][mpi + 4] == "6"
    assert str(build["status"]["binary"]) in captured["argv"]
    assert captured["env"].get("OMPI_MCA_osc") == "pt2pt"
    return {"passed": True, "launched": False, "candidate_build_root": str(Path(build_root).resolve()),
            "binary_sha256": build["status"]["binary_sha256"],
            "test_binary_sha256": build["status"]["test_binary_sha256"],
            "strict_comparator_sha256": sha(STRICT_COMPARATOR),
            "frozen_reference_artifacts": sorted(path for path in frozen
                                                  if path.startswith(str(REF_G2.resolve()) + os.sep)),
            "frozen_candidate_inputs": sorted(path for path in frozen
                                              if path.startswith(str(HERE.resolve()) + os.sep) or
                                              path == str(REFERENCE_HELPER.resolve())),
            "reference_case_argument_required": True,
            "controller_setup_stopped_at": "Popen stub before numerical execution",
            "command": captured["argv"], "environment_osc": captured["env"]["OMPI_MCA_osc"]}


def launch(gate, case_name, build_root, reference_case, g2_reference=None):
    assert gate in ("G2", "GQ") and case_name
    build_root = Path(build_root).resolve(strict=True)
    validator, build = verify_candidate_build(build_root)
    helper, runner, validator, env = configured(build_root)
    host = preflight()
    n1 = validator.accepted_n1()
    reference = validate_reference(reference_case, gate, validator, n1)
    if gate == "GQ":
        g2_reference = Path(g2_reference or REF_G2).resolve(strict=True)
        reference_g2 = validate_reference(g2_reference, "G2", validator, n1)
    else:
        reference_g2 = None
    frozen = bind_frozen_inputs(env, [reference["case"]] +
                                ([reference_g2["case"]] if reference_g2 is not None else []))
    case = (HERE / "cases" / case_name).resolve()
    assert case.parent == (HERE / "cases").resolve() and not case.exists()
    os.environ["OMPI_MCA_osc"] = "pt2pt"
    runner.CASES.mkdir(exist_ok=True)
    sys.argv = [str(N4 / "run_prefix_controller.py"), str(case), "--gate", gate]
    env["verify_pins"]()
    try:
        env["main"]()
    except SystemExit as exc:
        code = exc.code if isinstance(exc.code, int) else int(bool(exc.code))
        if code != 0:
            raise
    verify_frozen_inputs(frozen)
    status = json.loads((case / "status.json").read_text())
    assert status.get("state") == "terminal" and status.get("all_owned_handles_terminal") is True
    assert status.get("gate") == gate and status.get("gate_passed") is True
    validator.require_terminal_case(case, gate)
    validator.check_candidate_identity(case, build, n1, gate)
    if gate == "G2":
        validator.verify_g2_receipt(case)
    else:
        validator.verify_gq_receipt(case)
    g2_result = compare(reference_g2["case"], case, "G2") if gate == "GQ" else None
    if g2_result is not None:
        (case / "strict_g2_reference_comparison.json").write_text(json.dumps(
            {"gate": "G2", "passed": g2_result["passed"], "comparison": g2_result,
             "comparator_sha256": sha(STRICT_COMPARATOR)}, indent=2) + "\n")
        assert g2_result["passed"], g2_result
    result = compare(reference["case"], case, gate)
    path = case / "strict_fresh_reference_comparison.json"
    path.write_text(json.dumps({"gate": gate, "passed": result["passed"],
                                "comparison": result, "comparator_sha256": sha(STRICT_COMPARATOR)}, indent=2) + "\n")
    assert result["passed"], result
    receipt = {"passed": True, "gate": gate, "case": str(case),
               "candidate_build_root": str(build_root),
               "candidate_binary_sha256": build["status"]["binary_sha256"],
               "candidate_source_pins_sha256": sha(build_root / "source_pins.json"),
               "fresh_reference": reference, "fresh_g2_reference": reference_g2,
               "frozen_helper_and_reference_inputs": sorted(frozen),
               "strict_comparator_sha256": sha(STRICT_COMPARATOR),
               "strict_comparison_sha256": sha(path), "host_preflight": host}
    receipt_path = case / "candidate_prefix_acceptance.json"
    receipt_path.write_text(json.dumps(receipt, indent=2) + "\n")
    return receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("self-check", "G2", "GQ"))
    parser.add_argument("--build-root", required=True)
    parser.add_argument("--case-name")
    parser.add_argument("--reference-case")
    parser.add_argument("--g2-reference")
    args = parser.parse_args()
    if args.action == "self-check":
        result = self_check(args.build_root)
    else:
        assert args.case_name and args.reference_case
        result = launch(args.action, args.case_name, args.build_root, args.reference_case, args.g2_reference)
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
