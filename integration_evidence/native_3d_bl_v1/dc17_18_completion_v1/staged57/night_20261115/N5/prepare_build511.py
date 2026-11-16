#!/usr/bin/env python3
"""Prepare build520 pins/recipe only after a primary review receipt; never builds."""

import argparse
import hashlib
import json
import subprocess
from pathlib import Path

N5 = Path(__file__).resolve().parent
STAGED = N5.parents[1]
REPO = N5.parents[5]
N4 = N5.parent / "N4"
REFERENCE = N4 / "reference_build"
RECIPE = REFERENCE / "build.py"
BUILD_DIR = N5 / "build520"
SOURCE_PIN_REFERENCE = REFERENCE / "source_pins.json"
ALLOWED_CHANGED = {
    "Common/src/adaptation/CNativeEngine3D.cpp",
    "Common/src/adaptation/CNativePredicates3D.cpp",
    "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp",
    "UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp",
    "UnitTests/SU2_CFD/adaptation/CNativeImport3D_tests.cpp",
    "UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc",
}
ALLOWED_NEW = {"Common/include/adaptation/CNativeD9Profile3D.hpp"}
SOURCE_SUFFIXES = {".c", ".cc", ".cpp", ".cxx", ".h", ".hh", ".hpp", ".hxx", ".inc", ".inl", ".py"}
RUNTIME_PINS = STAGED / "runtime_20261113_474/loaded_runtime_pins.json"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def git_sources():
    result = subprocess.run(["git", "ls-files", "--cached", "--", "Common", "SU2_CFD", "UnitTests"],
                            cwd=REPO, text=True, capture_output=True, check=True)
    return {line for line in result.stdout.splitlines() if Path(line).suffix in SOURCE_SUFFIXES}


def reference_metadata():
    status_path = REFERENCE / "build_status.json"
    binaries_path = N4 / "reference_binaries/receipt.json"
    status = json.loads(status_path.read_text())
    binaries = json.loads(binaries_path.read_text())
    runtime = json.loads(RUNTIME_PINS.read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0 and status.get("pins_verified") is True
    assert binaries.get("local_only") is True and len(runtime) > 0
    for name, relative, status_key in (("SU2_CFD", "SU2_CFD/src/SU2_CFD", "binary"),
                                       ("test_driver", "UnitTests/test_driver", "test_binary")):
        path = N4 / "reference_binaries" / name
        assert sha(path) == binaries["binaries"][name] == status[status_key + "_sha256"]
        assert status[status_key] == str(REPO / "integration_evidence/build-integrated-v3-p5-20261114" / relative)
    for path, digest in runtime.items():
        assert sha(path) == digest, f"reference runtime module changed: {path}"
    return {"build_status_path": str(status_path), "build_status_sha256": sha(status_path),
            "build_label": "N4_reference_build_build509_alias", "build_output": str(REPO / "integration_evidence/build-integrated-v3-p5-20261114"),
            "binary_sha256": binaries["binaries"]["SU2_CFD"],
            "test_binary_sha256": binaries["binaries"]["test_driver"],
            "binary_receipt": str(binaries_path), "binary_receipt_sha256": sha(binaries_path),
            "runtime_manifest": str(RUNTIME_PINS), "runtime_manifest_sha256": sha(RUNTIME_PINS),
            "runtime_module_pin_count": len(runtime)}


def source_inventory():
    reference = json.loads(SOURCE_PIN_REFERENCE.read_text())
    paths = {Path(path).resolve(strict=True) for path in reference}
    paths.update((REPO / relative).resolve(strict=True) for relative in git_sources())
    for directory in (REPO / "Common", REPO / "SU2_CFD", REPO / "UnitTests"):
        paths.update(path.resolve(strict=True) for path in directory.rglob("*")
                     if path.is_file() and path.suffix in SOURCE_SUFFIXES)
    paths.update((REPO / relative).resolve(strict=True) for relative in ALLOWED_NEW)
    paths.update(path.resolve(strict=True) for path in N5.glob("*.py"))
    build_helper = BUILD_DIR / "build.py"
    if build_helper.exists():
        paths.add(build_helper.resolve(strict=True))
    mismatches, missing = [], []
    for raw, expected in reference.items():
        path = Path(raw)
        try:
            relative = path.relative_to(REPO).as_posix()
        except ValueError:
            relative = "@external/" + path.name
        try:
            actual = sha(path)
        except OSError:
            missing.append(str(path))
            continue
        if actual != expected:
            mismatches.append(relative)
    unexpected = sorted(set(mismatches) - ALLOWED_CHANGED)
    if missing or unexpected:
        raise RuntimeError(f"reference snapshot mismatch outside allowed set: missing={missing[:8]}, unexpected={unexpected[:8]}")
    current = {str(path): sha(path) for path in sorted(paths)}
    absent = sorted(path for path in current if not Path(path).is_file())
    if absent:
        raise RuntimeError(f"source paths disappeared while hashing: {absent[:8]}")
    fingerprint = hashlib.sha256("".join(f"{path}\0{digest}\n" for path, digest in current.items()).encode()).hexdigest()
    return {"reference_key_count": len(reference), "current_source_count": len(current),
            "reference_mismatches": sorted(mismatches), "allowed_changed_sources": sorted(ALLOWED_CHANGED),
            "allowed_new_sources": sorted(ALLOWED_NEW), "source_pins": current,
            "snapshot_sha256": fingerprint, "reference_build_metadata": reference_metadata()}


def verify_recipe():
    source = RECIPE.read_text()
    compile(source, str(RECIPE), "exec")
    for expected in ("ROOT.parents[6]", "ROOT.parents[2]", "OMP_NUM_THREADS='1'",
                     "OPENBLAS_NUM_THREADS='1'", "MKL_NUM_THREADS='1'", "'-n','19'", "'-j4'",
                     "UnitTests/test_driver", "SU2_CFD/src/SU2_CFD"):
        assert expected in source, f"immutable build recipe missing {expected}"
    assert not (BUILD_DIR / "build_status.json").exists(), "build520 status already exists"
    launch = " proc=subprocess.Popen(argv,cwd=REPO,env=env,stdout=log,stderr=subprocess.STDOUT,start_new_session=True)\n"
    assert source.count(launch) == 1, "reference recipe launch changed; review before adapting"
    source = source.replace(launch, launch + " owned_handles={}\n")
    sample = "  process_rows=sample_tree(proc.pid)\n"
    assert source.count(sample) == 1, "reference process sampler changed; review handle tracking"
    source = source.replace(sample, sample + "  for row in process_rows:\n   ident=identity(row['pid'])\n   if ident:owned_handles[(ident['pid'],ident['start_ticks'])]={**ident,'name':row['name']}\n")
    wait_line = " code=proc.wait()\n"
    assert source.count(wait_line) == 1, "reference wait point changed; review terminal-handle check"
    terminal_check = wait_line
    terminal_check += " handle_rows=[]\n for item in owned_handles.values():\n  try:\n   stat=(Path('/proc')/str(item['pid'])/'stat').read_text().rsplit(')',1)[1].split()\n   alive=int(stat[19])==item['start_ticks'] and stat[0] not in ('Z','X')\n  except (OSError,IndexError,ValueError):alive=False\n  handle_rows.append({**item,'alive':alive})\n"
    terminal_check += " heavy=[]\n for proc_path in Path('/proc').glob('[0-9]*'):\n  try:\n   name=(proc_path/'comm').read_text().strip()\n   if name in ('SU2_CFD','test_driver','cc1plus','ninja','lto1'):heavy.append({'pid':int(proc_path.name),'name':name})\n  except OSError:pass\n"
    terminal_check += " terminal_ok=not any(row['alive'] for row in handle_rows)\n if not terminal_ok:code=1\n"
    source = source.replace(wait_line, terminal_check)
    finish = "save('build_status.json',result);print(json.dumps(result),flush=True)\nraise SystemExit(code or bool(changed))"
    assert source.count(finish) == 1, "reference recipe completion changed; review before adapting"
    terminal = "save('build_status.json',result);print(json.dumps(result),flush=True)\n"
    terminal += "save('terminal_handles.json',{'actual_host_proc':True,'verified_unix':time.time(),'identities':handle_rows,'all_owned_handles_terminal':terminal_ok,'heavy_processes':heavy})\n"
    terminal += "raise SystemExit(code or bool(changed))"
    source = source.replace(finish, terminal)
    compile(source, str(RECIPE), "exec")
    return source


def self_check():
    recipe = verify_recipe()
    inventory = source_inventory()
    return {"passed": True, "launched": False, "wrote_files": False,
            "reference_recipe_sha256": sha(RECIPE), "recipe_bytes": len(recipe),
            "snapshot_sha256": inventory["snapshot_sha256"],
            "reference_key_count": inventory["reference_key_count"],
            "current_source_count": inventory["current_source_count"],
            "reference_mismatches": inventory["reference_mismatches"],
            "allowed_changed_sources": inventory["allowed_changed_sources"],
            "allowed_new_sources": inventory["allowed_new_sources"],
            "reference_build_metadata": inventory["reference_build_metadata"],
            "build520_exists": BUILD_DIR.exists()}


def prepare(ready_path):
    recipe = verify_recipe()
    inventory = source_inventory()
    ready = json.loads(ready_path.read_text())
    assert ready.get("build520_ready") is True
    assert ready.get("reviewed_snapshot_sha256") == inventory["snapshot_sha256"], "primary reviewed a different source snapshot"
    assert not BUILD_DIR.exists(), f"refusing to overwrite {BUILD_DIR}"
    BUILD_DIR.mkdir()
    build_script = BUILD_DIR / "build.py"
    build_script.write_text(recipe)
    inventory["source_pins"][str(build_script.resolve())] = sha(build_script)
    pins_text = json.dumps(inventory["source_pins"], indent=2, sort_keys=True) + "\n"
    pins_path = BUILD_DIR / "source_pins.json"
    pins_path.write_text(pins_text)
    plan = {"build_label": "build520", "state": "prepared_not_launched",
            "source_snapshot_sha256": inventory["snapshot_sha256"],
            "source_pins_sha256": sha(pins_path), "source_pin_count": len(inventory["source_pins"]),
            "reference_recipe": str(RECIPE), "reference_recipe_sha256": sha(RECIPE),
            "build_output": str(REPO / "integration_evidence/build-integrated-v3-p5-20261114"),
            "binary": str(REPO / "integration_evidence/build-integrated-v3-p5-20261114/SU2_CFD/src/SU2_CFD"),
            "test_binary": str(REPO / "integration_evidence/build-integrated-v3-p5-20261114/UnitTests/test_driver"),
            "changed_sources": inventory["reference_mismatches"],
            "allowed_changed_sources": inventory["allowed_changed_sources"],
            "allowed_new_sources": inventory["allowed_new_sources"],
            "reference_build_metadata": inventory["reference_build_metadata"],
            "build_recipe_runs": False}
    (BUILD_DIR / "source_snapshot.json").write_text(json.dumps(inventory, indent=2) + "\n")
    (BUILD_DIR / "build_plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    return plan


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--prepare", action="store_true")
    parser.add_argument("--primary-ready-receipt", type=Path)
    args = parser.parse_args()
    if args.self_check:
        assert not args.prepare and not args.primary_ready_receipt
        print(json.dumps(self_check(), indent=2))
        return
    if not args.prepare or not args.primary_ready_receipt:
        parser.error("use --self-check or --prepare --primary-ready-receipt FILE")
    print(json.dumps(prepare(args.primary_ready_receipt.resolve(strict=True)), indent=2))


if __name__ == "__main__":
    main()
