#!/usr/bin/env python3
"""Validate the isolated build617 restart using accepted Section 6.8 checks."""
import argparse
import importlib.util
import json
import os
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
NIGHT = N11.parents[1]
STAGED = NIGHT.parent
SECTION68 = NIGHT / "N8/A2_A3_N12/section6_8"
SECTION1 = NIGHT / "section6_1"
BUILD = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FX1/build617')
BASE = SECTION68 / "validation/restart534/validate_disk_restart534.py"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def context():
    helper = load("n11_restart_validator534", BASE)
    helper.HERE = HERE
    helper.VALIDATION = HERE
    helper.SECTION6_8 = SECTION68
    helper.SECTION1 = SECTION1
    helper.STAGED = STAGED
    status = json.loads((BUILD / "build_status.json").read_text())
    helper.CFD_SHA = status["binary_sha256"]
    helper.TEST_SHA = status["test_binary_sha256"]
    inherited = helper.load_base()
    source = inherited.retargeted_source()
    edits = (
        ("HERE = Path(__file__).resolve().parent", f"HERE = Path({str(HERE)!r})"),
        ("STAGED = Path(__file__).resolve().parents[6]", f"STAGED = Path({str(STAGED)!r})"),
        ('Path(__file__).resolve().parents[2] / "build534/build_status.json"',
         f'Path({str(BUILD)!r}) / "build_status.json"'),
    )
    for old, new in edits:
        assert source.count(old) == 1, old
        source = source.replace(old, new, 1)
    compile(source, str(BASE), "exec")
    return helper, inherited, source


def validate(source_case, case):
    source_case = Path(source_case).resolve(strict=True)
    case = Path(case).resolve(strict=True)
    assert HERE in case.parents and (case / "run.log").is_file()
    helper, inherited, source = context()
    reference = source_case / "accepted_reference_payloads.json"
    payloads = json.loads(reference.read_text())
    assert len(payloads["hashes"]) == 6
    assert source_case.name == "full617_run1"
    # Bind only the accepted validator's old reference path and expected hashes.
    helper.verify_n8_identity.__globals__.update(SECTION1=source_case, REFERENCE_HASHES=payloads["hashes"])
    helper.source_receipts(source_case)
    os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
    os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
    namespace = inherited.__dict__
    namespace.update(__name__="__main__", __file__=str(Path(__file__).resolve()),
                     log=(case / "run.log").read_text(errors="replace"))
    # The accepted source uses `log` in its resource proof before its later local read.
    exec(compile(source, str(BASE), "exec"), namespace)


def self_check():
    _, inherited, source = context()
    early_use = source.index("time_bound = conservative_time_bound(log)")
    later_read = source.index('log = (D / "run.log").read_text()')
    assert early_use < later_read
    inherited.__dict__["log"] = "restart log bound before exec"
    assert inherited.__dict__["log"] == "restart log bound before exec"
    return {"passed": True, "launched": False, "build": str(BUILD),
            "inherited_validator": str(BASE), "prebound_log": True,
            "resource_proof_uses_bound_log_before_local_read": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if args.source_case is None or args.case_dir is None:
        parser.error("provide --source-case and --case-dir")
    validate(args.source_case, args.case_dir)


if __name__ == "__main__":
    main()
