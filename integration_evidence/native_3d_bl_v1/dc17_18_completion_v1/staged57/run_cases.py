"""Run one pinned staged-native validation case; never overwrite its receipt."""
import json
import os
import re
import shutil
import struct
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
    "nodal_metric": (1, "[NativeMesh3D],[NativeDistributed3D],[NativeBridge3D],[NativeReassessment3D],[NativeCurrentNormalization3D],[NativeMetricFamily3D],[NativeBLTargetNormalization]"),
    "nodal_metric6": (6, "[NativeMesh3D],[NativeDistributed3D],[NativeBridge3D],[NativeReassessment3D],[NativeCurrentNormalization3D],[NativeMetricFamily3D],[NativeBLTargetNormalization]"),
    "m6_stage": (6, "[NativeFrozenVolume3D]"),
    "cost_checks": (1, "[NativeOverlappingAtlas3D]"),
    "normalization": (1, "[NativeWorkingSurfaceNormalization3D]"),
    "normalization6": (6, "[NativeWorkingSurfaceNormalization3D]"),
    "overlap_publication": (1, "[NativeOverlappingPublication3D]"),
    "overlap_publication6": (6, "[NativeOverlappingPublication3D]"),
    "reference_io": (1, "[NativeReference3D],[NativeAtlasReferenceIO3D]"),
    "reference_io6": (6, "[NativeReference3D],[NativeAtlasReferenceIO3D]"),
    "mapped_work_trace": (1, "[NativeMappedRuntime3D]"),
    "mapped_runtime": (1, "[NativeMappedRuntime3D]"),
    "single_certificate_checks": (1, "[NativeSingleChartCertificate3D],[NativeAcceptedFaceCertificate3D]"),
    "workflow_solved6": (6, "[NativeSolvedEuler3D]"),
    "workflow_primitive6": (6, "[NativePrimitiveRuntime3D]"),
    "workflow_resolved_m1": (6, "[NativeResolvedEuler3D]"),
    "workflow_resolved_m2": (6, "[NativeResolvedEuler3D]"),
    "workflow_resolved_m4": (6, "[NativeResolvedEuler3D]"),
    "bidirectional": (1, "[NativeBidirectionalLength3D],[NativeConstructionPassClaims3D]"),
    "bidirectional6": (6, "[NativeBidirectionalLength3D],[NativeConstructionPassClaims3D]"),
    "quality_cache": (1, "[NativeQualityPass3D]"),
    "quality_cache6": (6, "[NativeQualityPass3D]"),
    "quality_checks": (1, "[NativeOptimize3D],[NativeHeightOptimization3D],[NativeCoarsenQualityReturn3D],[NativeStageProgress3D]"),
    "stage_checks": (1, "[NativeStageProgress3D],[NativeStageCapture3D],[NativeCoarsenQualityReturn3D],[NativeConstructionSweep3D]"),
    "stage_checks6": (6, "[NativeStageProgress3D],[NativeStageCapture3D],[NativeCoarsenQualityReturn3D],[NativeConstructionSweep3D]"),
    "projection_paths": (1, "[NativeGeometryBackend3D],Entity feature reconstruction handles ordinary and genuine routes,Cumulative atlas and backend failure stop all workers before construction"),
    "projection_paths6": (6, "Entity feature reconstruction handles ordinary and genuine routes,Cumulative atlas and backend failure stop all workers before construction"),
    "geometry_backend": (1, "[NativeGeometryBackend3D]"),
    "atlas_checks": (1, "[NativeAtlas3D],[NativeCurvedAtlas3D],[NativeMappedPublication3D],"
                        "[NativeAffineGeometry],[NativeParametricGeometry],[NativeAffineSourceGeometry],"
                        "[NativeAtlasReferenceIO3D],[NativeJointAffineSource3D],[NativeStageCapture3D],"
                        "[NativeReferenceTriangulation3D],[NativeAtlasCoordinatedPublication3D],"
                        "[NativeGeometryBackend3D],[NativeCurvedMapped3D],[NativeCanonicalTrace3D],[NativeTraceCoverage3D]"),
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
COORDINATION = "[NativeCoordinateStar3D],[NativeMetricSplitRule3D],[NativeRepairFairness3D],[NativeConstructionSweep3D],[NativeCoarsenQualityReturn3D],[ConservativeClipRobustness],[NativeStageProgress3D],[NativeStageCapture3D]"
for name in ("serial", "serial_full", "mpi6", "connected_fix"):
    ranks, filters = CASES[name]
    CASES[name] = ranks, filters + "," + COORDINATION


for name in ("serial_full", "mpi6"):
    ranks, filters = CASES[name]
    filters += ",[NativeReference3D],[NativeBidirectionalLength3D],[NativeConstructionPassClaims3D]"
    CASES[name] = ranks, filters + (",[NativeAtlas3D],[NativeCurvedAtlas3D],[NativeAffineGeometry],[NativeAtlasReferenceIO3D],"
                                   "[NativeReferenceTriangulation3D],[NativeAtlasCoordinatedPublication3D],[NativeGeometryBackend3D],[NativeCanonicalTrace3D],[NativeTraceCoverage3D],"
                                   "Convex clipping:*,"
                                   "Distributed conservative projection:*,Conservative kernels:*,"
                                   "Distributed conservative kernels:*,Transfer admissibility:*,"
                                   "Distributed conservative transfer:*,Single precision transfer contract,"
                                   "[NativeCurvedLifecycle3D]")

    # Cover publication/history/restart and barycentric transfer as well as conservative kernels.
    CASES[name] = ranks, CASES[name][1] + (",[NativeCurvedTransaction3D],[NativeRuntime3D],"
                                         "[NativeCurvedRuntime3D],[NativePolicyVeto3D],"
                                         "[NativeBLPolicyRuntime3D],[NativeDistributed3D],[NativeBridge3D],[NativeReassessment3D],[NativeCurrentNormalization3D],[NativeMetricFamily3D],[NativeBLTargetNormalization],Distributed barycentric transfer:*")

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


def prepare_stage_case(attempt):
    """Use the exact archived build13 field; a bounded observer stops after four quality windows."""
    previous = ROOT / "m6_runs/euler_build13"
    archive = ROOT.parent / "diagnostic55"
    manifest_path = archive / "input_manifest.json"
    manifest = json.loads(manifest_path.read_text())
    files = {"m6.su2": (previous / "m6.su2", "mesh_sha256"),
             "m6.su2.native_ref": (previous / "m6.su2.native_ref", "reference_sha256"),
             "sensor_original.bin": (archive / "sensor_original.bin", "sensor_sha256"),
             "restart_flow_adap_00000.dat": (previous / "restart_flow_adap_00000.dat", "restart_sha256")}
    pinned = [manifest_path, previous / "run.cfg", archive / "prepare.py"]
    for name, (path, key) in files.items():
        if sha(path) != manifest[key]:
            raise RuntimeError("M6 archived-field provenance changed: " + str(path))
        pinned.append(path)
        if name != "restart_flow_adap_00000.dat":
            (attempt / name).symlink_to(path.resolve())
    shutil.copyfile(previous / "run.cfg", attempt / "run.cfg")
    pinned.append(attempt / "run.cfg")
    return pinned, {"SU2_NATIVE_FROZEN_CONFIG": str(attempt / "run.cfg"),
                    "SU2_NATIVE_FROZEN_SENSOR3D": str(attempt / "sensor_original.bin"),
                    "SU2_NATIVE_STAGE_CAPTURE": "1", "SU2_NATIVE_STAGE_CAPTURE_WINDOWS": "4"}


def validate_stage_shards(attempt, ranks):
    """Byte completeness and frozen-source identity; not mesh acceptance or exact continuation."""
    receipts = {}
    windows = None
    for rank in range(ranks):
        donor_hash = None
        for label in ("quality_entry", "observation_stop"):
            path = attempt / f"stage_{label}_rank{rank}_manifest.json"
            data = json.loads(path.read_text())
            if (data.get("rank") != rank or not data.get("complete_shard") or
                    not data.get("private_capture") or data.get("final_mesh_accepted") or
                    data.get("exact_continuation") or data.get("record_format") != 1):
                raise RuntimeError("Invalid private stage manifest: " + str(path))
            if label == "observation_stop" and (not data.get("diagnostic_observation_stop") or
                    data.get("quality_windows_observed") != 4):
                raise RuntimeError("Missing bounded quality observation stop: " + str(path))
            receipts[str(path)] = sha(path)
            for kind, count_key in (("owned", "working_rows"), ("donors", "donor_rows")):
                payload = attempt / data["files"][kind]
                record_bytes = data["owned_record_bytes" if kind == "owned" else "donor_record_bytes"]
                with payload.open("rb") as stream:
                    count = struct.unpack("=Q", stream.read(8))[0]
                if count != data[count_key] or payload.stat().st_size != 8 + count * record_bytes:
                    raise RuntimeError("Truncated stage row shard: " + str(payload))
                receipts[str(payload)] = sha(payload)
                if kind == "donors":
                    if donor_hash is not None and donor_hash != receipts[str(payload)]:
                        raise RuntimeError("Original donors changed between stage snapshots")
                    donor_hash = receipts[str(payload)]
            if data["parameters_present"]:
                payload = attempt / data["files"]["parameters"]
                if payload.stat().st_size != data["parameters_payload_bytes"]:
                    raise RuntimeError("Truncated stage geometry payload: " + str(payload))
                receipts[str(payload)] = sha(payload)
        trace = attempt / f"stage_trace_rank{rank}.jsonl"
        rows = [json.loads(line) for line in trace.read_text().splitlines()]
        if sum(row["stage"] == "window" and row["phase"] == "improvement" for row in rows) != 4:
            raise RuntimeError("Incomplete quality-stage trend: " + str(trace))
        sequence = [(row["stage"], row["phase"], row["round"]) for row in rows]
        if windows is not None and sequence != windows:
            raise RuntimeError("Stage trace rounds differ between workers")
        windows = sequence
        receipts[str(trace)] = sha(trace)
    save(attempt / "stage_output_pins.json", receipts)
    return {"complete_rank_shards": ranks, "quality_windows": 4, "original_donors_unchanged": True,
            "exact_continuation": False, "full_mesh_acceptance": False}


def main():
    if len(sys.argv) != 3 or not re.fullmatch(r"build[1-9][0-9]*", sys.argv[1]) or sys.argv[2] not in CASES:
        raise SystemExit("usage: run_cases.py buildN " + "|".join(CASES))
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
        if build_status.get("state") != "terminal" or build_status.get("exit_code") != 0 or not build_status.get("pins_verified") or build_status.get("completed_build") is False:
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
        stage_environment = {}
        if case == "mapped_work_trace":
            stage_environment["SU2_NATIVE_GEOMETRY_WORK_TRACE"] = "1"
        if case.startswith("workflow_resolved_m"):
            stage_environment["SU2_NATIVE_RESOLVED_WORKERS"] = case.rsplit("m", 1)[1]
        if case == "m6_stage":
            stage_inputs, stage_environment = prepare_stage_case(attempt)
            for path in stage_inputs:
                pins[str(path)] = sha(path)
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
        env.update(stage_environment)
        if case == "allworker6":
            env["SU2_NATIVE_MAPPED_ALL_WORKERS"] = "1"
        timeout_seconds = 2400 if case == "m6_stage" else 600
        argv = ["nice", "-n", "19", "/usr/bin/time", "-v", "timeout", str(timeout_seconds)]
        if ranks > 1:
            argv += ["mpirun", "--bind-to", "none", "-n", str(ranks)]
        argv += [str(BINARY), filter_expr]
        save(attempt / "command.json", {
            "build": build, "case": case, "ranks": ranks, "argv": argv,
            "environment": {key: value for key, value in env.items()
                            if key.startswith(("SU2_NATIVE_", "OMPI_")) or
                            key in ("OMP_NUM_THREADS", "OPENBLAS_NUM_THREADS", "MKL_NUM_THREADS")},
            "timeout_seconds": timeout_seconds,
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
        if case == "m6_stage" and code == 0 and not changed:
            try:
                result["stage_capture"] = validate_stage_shards(attempt, ranks)
            except Exception as error:
                result["diagnostic_error"] = str(error)
                result["exit_code"] = code = 1
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
