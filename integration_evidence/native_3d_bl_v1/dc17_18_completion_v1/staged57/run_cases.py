"""Run one pinned staged-native validation case; never overwrite its receipt."""
import json
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
DIAGNOSTIC = ROOT.parent / "diagnostic56"
sys.path.insert(0, str(DIAGNOSTIC))
from run import sha, save

REPO = ROOT.parents[3]
BINARY = REPO / "integration_evidence/build-integrated-v2/UnitTests/test_driver"
CASES = {
    "connected_fix": (1, "[NativeMappedRuntime3D],[NativeMesh3D],[NativeAutomaticRepair3D],[NativeOriginalSupport3D],[NativeShapeAdmission3D]"),
    'serial_full': (1, '[NativeCollision3D],[NativeMesh3D],[NativeOriginalSupport3D],[NativeMetricFootprint3D],[NativeMetricSamples3D],[NativeCurvedMapped3D],[NativeCurvedMappedGuards3D],[NativeMappedFacetGeometry],[NativeMappedPublication3D],[NativeMappedRealized3D],[NativeMappedComponent3D],[NativeParametricGeometry],[NativeAffineSourceGeometry],[NativeFirstLayer3D],[NativeCurvedBL3D],[NativeJointAffineSource3D],[NativeGeneratedCoverage3D],[NativeMappedRuntime3D],[NativeMappedReader3D],[NativeConcurrentComposed3D],[NativeCollapseFilter3D],[NativeInteriorPlacement3D],[NativeAutomaticRepair3D],[NativeShapeAdmission3D],[NativeOptimize3D],[NativeDefault3D],[NativePolicyRuntime3D],[NativeInteractingBLVolume3D],[NativeCurvedBLVolume3D],[NativeCylinderBLVolume3D],[NativeComplexityEstimate3D],[NativeParallelBL3D],[NativeVolumeBL3D],[NativeStagedConstruction3D],[NativeConstructionHandoff3D],[NativeJointGeometry3D],[NativeConcurrentRepair3D],[NativeQualityPass3D],[NativeHeightOptimization3D],[NativeCoarsenQualityReturn3D]'),
    'mpi6': (6, '[NativeCollision3D],[NativeMesh3D],[NativeOriginalSupport3D],[NativeMetricFootprint3D],[NativeMetricSamples3D],[NativeCurvedMapped3D],[NativeCurvedMappedGuards3D],[NativeMappedFacetGeometry],[NativeMappedPublication3D],[NativeMappedRealized3D],[NativeMappedComponent3D],[NativeParametricGeometry],[NativeAffineSourceGeometry],[NativeFirstLayer3D],[NativeCurvedBL3D],[NativeJointAffineSource3D],[NativeGeneratedCoverage3D],[NativeMappedRuntime3D],[NativeMappedReader3D],[NativeConcurrentComposed3D],[NativeCollapseFilter3D],[NativeInteriorPlacement3D],[NativeAutomaticRepair3D],[NativeShapeAdmission3D],[NativeOptimize3D],[NativeDefault3D],[NativePolicyRuntime3D],[NativeStagedConstruction3D],[NativeConstructionHandoff3D],[NativeJointGeometry3D],[NativeConcurrentRepair3D],[NativeQualityPass3D],[NativeHeightOptimization3D],[NativeCoarsenQualityReturn3D]'),
    "serial": (1, "[NativeStagedConstruction3D],[NativeConstructionHandoff3D],[NativeShapeAdmission3D],"
                  "[NativeFirstLayer3D],[NativeOptimize3D],[NativeJointGeometry3D],"
                  "[NativeInteriorPlacement3D],[NativeConcurrentRepair3D],[NativeQualityPass3D],[NativeHeightOptimization3D],[NativeCoarsenQualityReturn3D]"),
    "mpi2": (2, "[NativeStagedConstruction3D],[NativeConstructionHandoff3D],[NativeInteriorPlacement3D],"
                "[NativeConcurrentRepair3D],[NativeConcurrentComposed3D],[NativeQualityPass3D],[NativeHeightOptimization3D],[NativeCoarsenQualityReturn3D]"),
    "allworker6": (6, "[NativeMappedRuntime3D],[NativeCollapseFilter3D],[NativeCollision3D],"
                      "[NativeConcurrentComposed3D],[NativeConcurrentRepair3D]"),
}

# Connected construction/quality coordination regression, shared by focused and broad campaigns.
COORDINATION = "[NativeCoordinateStar3D],[NativeMetricSplitRule3D],[NativeRepairFairness3D],[NativeConstructionSweep3D],[NativeCoarsenQualityReturn3D],[ConservativeClipRobustness]"
for name in ("serial", "serial_full", "mpi6", "connected_fix"):
    ranks, filters = CASES[name]
    CASES[name] = ranks, filters + "," + COORDINATION


for name in ("serial_full", "mpi6"):
    ranks, filters = CASES[name]
    CASES[name] = ranks, filters + (",Convex clipping:*,"
                                   "Distributed conservative projection:*,Conservative kernels:*,"
                                   "Distributed conservative kernels:*,Transfer admissibility:*,"
                                   "Distributed conservative transfer:*,Single precision transfer contract,"
                                   "[NativeCurvedLifecycle3D]")

# Geometry-based serial transfer fixtures require a halo-free mesh. Their distributed
# counterparts above exercise gathered geometry, partition equivalence and history.
ranks, filters = CASES["serial_full"]
CASES["serial_full"] = ranks, filters + ",Conservative projection:*,Conservative transfer:*"


def loaded_libraries():
    result = subprocess.run(["ldd", str(BINARY)], check=True, text=True, capture_output=True)
    paths = set()
    for line in result.stdout.splitlines():
        if "not found" in line:
            raise RuntimeError("unresolved loaded library: " + line.strip())
        match = re.search(r"(?:=>\s+)?(/[^\s]+)", line)
        if match and Path(match.group(1)).is_file():
            paths.add(str(Path(match.group(1)).resolve()))
    if not paths:
        raise RuntimeError("ldd returned no loaded library paths")
    return sorted(paths)


def sample_tree(pid):
    pending, seen, rows = [pid], set(), []
    while pending:
        current = pending.pop()
        if current in seen:
            continue
        seen.add(current)
        proc = Path("/proc") / str(current)
        try:
            pending.extend(int(child) for child in (proc / "task" / str(current) / "children").read_text().split())
            rss = int((proc / "statm").read_text().split()[1]) * os.sysconf("SC_PAGE_SIZE")
            rows.append({"pid": current, "rss_bytes": rss, "name": (proc / "comm").read_text().strip()})
        except (FileNotFoundError, ProcessLookupError, PermissionError):
            pass
    return rows


def main():
    if len(sys.argv) != 3 or not re.fullmatch(r"build[1-9][0-9]*", sys.argv[1]) or sys.argv[2] not in CASES:
        raise SystemExit("usage: run_cases.py buildN connected_fix|serial|serial_full|mpi2|mpi6|allworker6")
    build, case = sys.argv[1], sys.argv[2]
    ranks, filter_expr = CASES[case]
    build_status_path = ROOT / f"{build}_status.json"
    source_pins_path = ROOT / f"{build}_source_pins.json"
    attempt = ROOT / "cases" / f"{case}_{time.strftime('%Y%m%dT%H%M%S', time.gmtime())}_{os.getpid()}"
    attempt.mkdir(parents=True, exist_ok=False)
    status_path = attempt / "status.json"
    start = time.time()
    try:
        build_status = json.loads(build_status_path.read_text())
        if build_status.get("state") != "terminal" or build_status.get("exit_code") != 0 or not build_status.get("pins_verified"):
            raise RuntimeError(f"{build} is not a verified successful terminal build")
        source_pins = json.loads(source_pins_path.read_text())
        changed = [path for path, digest in source_pins.items() if sha(path) != digest]
        if changed:
            raise RuntimeError("build source pins changed: " + ", ".join(changed[:8]))
        runtime_manifest = ROOT.parent / "validation54/m6_euler/loaded_runtime_pins.json"
        runtime_pins = json.loads(runtime_manifest.read_text())
        changed = [path for path, digest in runtime_pins.items() if sha(path) != digest]
        if changed:
            raise RuntimeError("saved M6 loaded-runtime pins changed: " + ", ".join(changed[:8]))
        libraries = loaded_libraries()
        helper_paths = [Path(__file__).resolve(), (DIAGNOSTIC / "run.py").resolve()]
        pins = dict(source_pins)
        for path in [BINARY, *libraries, *helper_paths, build_status_path, source_pins_path, runtime_manifest]:
            pins[str(path)] = sha(path)
        pins.update(runtime_pins)
        utility_names = ["nice", "timeout"] + (["mpirun"] if ranks > 1 else [])
        utility_names.append("/usr/bin/time")
        utilities = [(Path(shutil.which(name) or "/nonexistent") if not name.startswith("/") else Path(name)).resolve()
                     for name in utility_names]
        if any(not path.is_file() for path in utilities):
            raise RuntimeError("could not resolve one or more execution utilities")
        for path in utilities:
            pins[str(path)] = sha(path)
        save(attempt / "pins.json", pins)
        env = os.environ.copy()
        env.update(OMP_NUM_THREADS="1", OPENBLAS_NUM_THREADS="1", MKL_NUM_THREADS="1")
        if case == "allworker6":
            env["SU2_NATIVE_MAPPED_ALL_WORKERS"] = "1"
        argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", "600"]
        if ranks > 1:
            argv += ["mpirun", "--bind-to", "none", "-n", str(ranks)]
        argv += [str(BINARY), filter_expr]
        save(attempt / "command.json", {
            "build": build, "case": case, "ranks": ranks, "argv": argv,
            "environment": {key: value for key, value in env.items()
                            if key.startswith(("SU2_NATIVE_", "OMPI_")) or
                            key in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS")},
            "timeout_seconds": 600,
        })
        largest_sampled_rss = 0
        with (attempt / "run.log").open("w") as log, (attempt / "resources.jsonl").open("w") as receipt:
            process = subprocess.Popen(argv, cwd=attempt, env=env, stdout=log, stderr=subprocess.STDOUT,
                                       start_new_session=True)
            save(status_path, {"state": "running", "pid": process.pid, "started_at": start})
            print(json.dumps({"state": "running", "attempt": str(attempt), "pid": process.pid}), flush=True)
            while process.poll() is None:
                rows = sample_tree(process.pid)
                largest_sampled_rss = max(largest_sampled_rss, max((row["rss_bytes"] for row in rows), default=0))
                receipt.write(json.dumps({"elapsed_seconds": time.time() - start,
                                          "processes": rows,
                                          "sampled_process_tree_rss_bytes": sum(row["rss_bytes"] for row in rows)}) + "\n")
                receipt.flush()
                time.sleep(1)
            code = process.wait()
        changed = [path for path, digest in pins.items() if sha(path) != digest]
        result = {"state": "terminal", "exit_code": code, "seconds": time.time() - start,
                  "pins_verified": not changed, "changed": changed,
                  "largest_sampled_individual_process_rss_bytes": largest_sampled_rss,
                  "rss_note": "sampled process RSS; not a whole-job peak"}
        save(status_path, result)
        print(json.dumps(result))
        raise SystemExit(code or int(bool(changed)))
    except SystemExit:
        raise
    except Exception as error:
        save(status_path, {"state": "terminal", "exit_code": 2, "seconds": time.time() - start,
                           "pins_verified": False, "error": str(error)})
        raise


if __name__ == "__main__":
    main()
