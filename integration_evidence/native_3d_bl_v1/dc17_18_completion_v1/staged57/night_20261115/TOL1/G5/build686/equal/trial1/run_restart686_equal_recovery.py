#!/usr/bin/env python3
"""Run the accepted isolated restart workflow against the build686 full case."""
import argparse
import importlib.util
import json
import os
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
N11 = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/N4/section6_3_after_N11')
NIGHT = N11.parents[1]
STAGED = NIGHT.parent
SECTION68 = NIGHT / "N8/A2_A3_N12/section6_8"
SECTION1 = NIGHT / "section6_1"
BUILD = Path('/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/TOL1/build686')
BASE = SECTION68 / "validation/restart534/run_disk_restart534.py"


def load(path):
    spec = importlib.util.spec_from_file_location("n11_restart_runner534", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load helper: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def runner_context():
    helper = load(BASE)
    helper.HERE = HERE
    helper.VALIDATION = HERE
    helper.SECTION6_8 = SECTION68
    helper.SECTION1 = SECTION1
    helper.STAGED = STAGED
    status = json.loads((BUILD / "build_status.json").read_text())
    helper.CFD_SHA = status["binary_sha256"]
    helper.TEST_SHA = status["test_binary_sha256"]
    original_load = helper.load_base

    def load_base():
        core = original_load()
        original_source = core.retargeted_source

        def retargeted_source():
            source = original_source()
            replacements = (
                ('sys.path.insert(0, str(Path(__file__).resolve().parents[6]))',
                 f'sys.path.insert(0, str(Path({str(STAGED)!r})))'),
                ("HERE = Path(__file__).resolve().parent", f"HERE = Path({str(HERE)!r})"),
                ("STAGED = Path(__file__).resolve().parents[6]", f"STAGED = Path({str(STAGED)!r})"),
                ('Path(__file__).resolve().parents[2] / "build534/build_status.json"',
                 f'Path({str(BUILD)!r}) / "build_status.json"'),
                ('Path(__file__).resolve().parents[2] / "build534/source_pins.json"',
                 f'Path({str(BUILD)!r}) / "source_pins.json"'),
                ('Path(__file__).resolve().parents[1] / "unchanged_build499_source_pins.json"',
                 f'Path({str(SECTION68 / "validation/unchanged_build499_source_pins.json")!r})'),
                ("run_disk_restart534.py", "run_restart686_equal_recovery.py"),
                ("validate_disk_restart534.py", "validate_restart686.py"),
            )
            for old, new in replacements:
                assert source.count(old) >= 1, old
                source = source.replace(old, new)
            for old, new in (('argv = ["nice"', 'env.pop(\'SU2_NATIVE_CONFORMING_WITNESS\', None)\nenv.update({\'SU2_NATIVE_GEOMETRY_WORK_TRACE\': \'0\', \'SU2_NATIVE_FP3_PROBE\': \'0\'})\nargv = ["nice"'), ('"mpirun", "--oversubscribe"', '"mpirun", \'-x\', \'SU2_NATIVE_GEOMETRY_WORK_TRACE\', \'-x\', \'SU2_NATIVE_FP3_PROBE\', "--oversubscribe"'), ('or k == "TMPDIR"', 'or k == "TMPDIR" or k in (\'SU2_NATIVE_GEOMETRY_WORK_TRACE\', \'SU2_NATIVE_FP3_PROBE\')')):
                assert source.count(old) == 1, old
                source = source.replace(old, new, 1)
            old_observation, new_observation = '            except (OSError,RuntimeError,ValueError) as exc:\n                # Preserve the failed observation while continuing to own/reap the bounded CFD job.\n                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})', '            except OSError as exc:\n                if exc.errno == __import__("errno").EIO:\n                    with (FOLDER / "runtime_sample_eio.jsonl").open("a") as observations:\n                        observations.write(json.dumps({"time": time.time(), "error": str(exc)}) + "\\n")\n                else:\n                    runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})\n            except (RuntimeError,ValueError) as exc:\n                runtime_error=str(exc);save(FOLDER / "runtime_observation_failure.json",{"error":runtime_error})'
            assert source.count(old_observation) == 1, old_observation
            source = source.replace(old_observation, new_observation, 1)
            old_probe, new_probe = '    return {"pid": pid, "start_ticks": proc_stat(pid)["start_ticks"],', '    values = (proc / "environ").read_bytes().split(b"\\0")\n    probe = next((entry.split(b"=", 1)[1].decode() for entry in values\n                  if entry.startswith(b"SU2_NATIVE_FP3_PROBE=")), None)\n    return {"pid": pid, "start_ticks": proc_stat(pid)["start_ticks"], "probe": probe,'
            assert source.count(old_probe) == 1, old_probe
            source = source.replace(old_probe, new_probe, 1)
            legacy_guard = 'from pathlib import Path\nexpected_restart = Path(\'/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated/integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/night_20261115/FP3_E/G5/build680/S1/trial1/disk_restart_full680_run1_event2/run.cfg\').read_bytes()\nassert __import__("hashlib").sha256(expected_restart).hexdigest() == \'c7984b6edecc090d6823a5c51493a77aaf87e62ad4b637259de35e5b3a1a44ca\'\nassert config.replace(\'ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1E-4)\', \'ADAP_HAUSD= 1E-4\', 1).encode() == b\'SOLVER= EULER\\nMATH_PROBLEM= DIRECT\\nRESTART_SOL= YES\\nMACH_NUMBER= 0.8395\\nAOA= 3.06\\nSIDESLIP_ANGLE= 0.0\\nFREESTREAM_TEMPERATURE= 288.15\\nREYNOLDS_NUMBER= 11.72E6\\nREYNOLDS_LENGTH= 0.64607\\nFLUID_MODEL= STANDARD_AIR\\nGAMMA_VALUE= 1.4\\nGAS_CONSTANT= 287.058\\nCRITICAL_TEMPERATURE= 131.00\\nCRITICAL_PRESSURE= 3588550.0\\nCRITICAL_DENSITY= 263.0\\nACENTRIC_FACTOR= 0.035\\nVISCOSITY_MODEL= SUTHERLAND\\nMU_CONSTANT= 1.716E-5\\nMU_REF= 1.716E-5\\nMU_T_REF= 273.15\\nSUTHERLAND_CONSTANT= 110.4\\nREF_ORIGIN_MOMENT_X= 0.25\\nREF_ORIGIN_MOMENT_Y= 0.00\\nREF_ORIGIN_MOMENT_Z= 0.00\\nREF_LENGTH= 0.64607\\nREF_AREA= 0.7532\\nMARKER_FAR= ( FARFIELD )\\nMARKER_SYM= ( SYMMETRY )\\nMARKER_PLOTTING= ( WING3D )\\nMARKER_MONITORING= ( WING3D )\\nNUM_METHOD_GRAD= GREEN_GAUSS\\nCFL_NUMBER= 1.0\\nCFL_ADAPT= NO\\nCFL_ADAPT_PARAM= ( 0.5, 1.25, 0.5, 20.0 )\\nRK_ALPHA_COEFF= ( 0.66667, 0.66667, 1.000000 )\\nITER= 1\\nLINEAR_SOLVER= FGMRES\\nLINEAR_SOLVER_PREC= LU_SGS\\nLINEAR_SOLVER_ERROR= 1E-4\\nLINEAR_SOLVER_ITER= 5\\nCONV_NUM_METHOD_FLOW= ROE\\nMUSCL_FLOW= YES\\nSLOPE_LIMITER_FLOW= VENKATAKRISHNAN\\nVENKAT_LIMITER_COEFF= 1.0\\nJST_SENSOR_COEFF= ( 0.5, 0.02 )\\nTIME_DISCRE_FLOW= EULER_IMPLICIT\\nCONV_RESIDUAL_MINVAL= -10\\nCONV_STARTITER= 10\\nCONV_CAUCHY_ELEMS= 100\\nCONV_CAUCHY_EPS= 1E-6\\nPARMETIS_TOLERANCE= 0.05\\nPARMETIS_EDGE_WEIGHT= 0\\nPARMETIS_POINT_WEIGHT= 1\\nMESH_FILENAME= accepted508.su2\\nMESH_FORMAT= SU2\\nMESH_OUT_FILENAME= adapted_mesh\\nSOLUTION_FILENAME= accepted508_input\\nSOLUTION_ADJ_FILENAME= solution_adj\\nOUTPUT_FILES= ( RESTART )\\nTABULAR_FORMAT= CSV\\nCONV_FILENAME= continued508_history\\nRESTART_FILENAME= continued508_restart\\nRESTART_ADJ_FILENAME= restart_adj\\nVOLUME_FILENAME= flow\\nVOLUME_ADJ_FILENAME= adjoint\\nGRAD_OBJFUNC_FILENAME= of_grad\\nSURFACE_FILENAME= surface\\nSURFACE_ADJ_FILENAME= surface_adjoint\\nOUTPUT_WRT_FREQ= 1\\nSCREEN_OUTPUT= ( INNER_ITER, RMS_DENSITY, LIFT, DRAG )\\nMESH_OUT_FORMAT= SU2\\nMGLEVEL= 0\\nSCREEN_WRT_FREQ_INNER= 10\\nWRT_RESTART_COMPACT= NO\\nCOMPUTE_METRIC= YES\\nADAP_REMESHER= NATIVE_CAVITY\\nADAP_SURFACE= YES\\nNUM_METHOD_HESS= QUADRATIC_LEAST_SQUARES\\nADAP_HESSIAN_NOISE= 0\\nADAP_SENSOR= ( MACH )\\nADAP_HMIN= 1E-6\\nADAP_HMAX= 100\\nADAP_ARMAX= 1E6\\nADAP_HGRAD= 1.3\\nADAP_HAUSD= 1E-4\\nADAP_NATIVE_MAX_WORK= 0\\nADAP_NATIVE_MAX_TIME= 5400\\nADAP_NATIVE_REPARTITION= YES\\nADAP_TRANSFER= CONSERVATIVE\\nWRT_ADAP_MESH= YES\\nWRT_PERFORMANCE= YES\\nVOLUME_OUTPUT= ( COORDINATES, SOLUTION, PRIMITIVE, METRIC, ADAP_SENSOR )\\nADAP_LOOP= NO\\nADAP_COMPLEXITY= 50000\\nADAP_NATIVE_RANKS= 6\\nADAP_ANGLE= 60\\nMARKER_EULER= ( WING3D )\\nADAP_SIZES= ( 50000 )\\nADAP_SUBITER= ( 2 )\\nADAP_FLOW_ITER= ( 1 )\\nADAP_NATIVE_MEMORY_LIMIT= 12288\\n\\nREAD_BINARY_RESTART= YES\\n\\nADAP_NATIVE_JOB_MEMORY_LIMIT= 19073\\nADAP_NATIVE_LOCAL_CORE= YES\\nADAP_NATIVE_LOCAL_SURFACE= YES\\nADAP_NORM= 1.0\\n\'\n'
            old = '(FOLDER / "run.cfg").write_text(config)'
            assert source.count(old) == 1, old
            source = source.replace(old, legacy_guard + old, 1)
            tolerance_guard = "config = config.replace('ADAP_HAUSD= 1E-4', 'ADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1E-4)', 1)\nassert config.encode() == b'SOLVER= EULER\\nMATH_PROBLEM= DIRECT\\nRESTART_SOL= YES\\nMACH_NUMBER= 0.8395\\nAOA= 3.06\\nSIDESLIP_ANGLE= 0.0\\nFREESTREAM_TEMPERATURE= 288.15\\nREYNOLDS_NUMBER= 11.72E6\\nREYNOLDS_LENGTH= 0.64607\\nFLUID_MODEL= STANDARD_AIR\\nGAMMA_VALUE= 1.4\\nGAS_CONSTANT= 287.058\\nCRITICAL_TEMPERATURE= 131.00\\nCRITICAL_PRESSURE= 3588550.0\\nCRITICAL_DENSITY= 263.0\\nACENTRIC_FACTOR= 0.035\\nVISCOSITY_MODEL= SUTHERLAND\\nMU_CONSTANT= 1.716E-5\\nMU_REF= 1.716E-5\\nMU_T_REF= 273.15\\nSUTHERLAND_CONSTANT= 110.4\\nREF_ORIGIN_MOMENT_X= 0.25\\nREF_ORIGIN_MOMENT_Y= 0.00\\nREF_ORIGIN_MOMENT_Z= 0.00\\nREF_LENGTH= 0.64607\\nREF_AREA= 0.7532\\nMARKER_FAR= ( FARFIELD )\\nMARKER_SYM= ( SYMMETRY )\\nMARKER_PLOTTING= ( WING3D )\\nMARKER_MONITORING= ( WING3D )\\nNUM_METHOD_GRAD= GREEN_GAUSS\\nCFL_NUMBER= 1.0\\nCFL_ADAPT= NO\\nCFL_ADAPT_PARAM= ( 0.5, 1.25, 0.5, 20.0 )\\nRK_ALPHA_COEFF= ( 0.66667, 0.66667, 1.000000 )\\nITER= 1\\nLINEAR_SOLVER= FGMRES\\nLINEAR_SOLVER_PREC= LU_SGS\\nLINEAR_SOLVER_ERROR= 1E-4\\nLINEAR_SOLVER_ITER= 5\\nCONV_NUM_METHOD_FLOW= ROE\\nMUSCL_FLOW= YES\\nSLOPE_LIMITER_FLOW= VENKATAKRISHNAN\\nVENKAT_LIMITER_COEFF= 1.0\\nJST_SENSOR_COEFF= ( 0.5, 0.02 )\\nTIME_DISCRE_FLOW= EULER_IMPLICIT\\nCONV_RESIDUAL_MINVAL= -10\\nCONV_STARTITER= 10\\nCONV_CAUCHY_ELEMS= 100\\nCONV_CAUCHY_EPS= 1E-6\\nPARMETIS_TOLERANCE= 0.05\\nPARMETIS_EDGE_WEIGHT= 0\\nPARMETIS_POINT_WEIGHT= 1\\nMESH_FILENAME= accepted508.su2\\nMESH_FORMAT= SU2\\nMESH_OUT_FILENAME= adapted_mesh\\nSOLUTION_FILENAME= accepted508_input\\nSOLUTION_ADJ_FILENAME= solution_adj\\nOUTPUT_FILES= ( RESTART )\\nTABULAR_FORMAT= CSV\\nCONV_FILENAME= continued508_history\\nRESTART_FILENAME= continued508_restart\\nRESTART_ADJ_FILENAME= restart_adj\\nVOLUME_FILENAME= flow\\nVOLUME_ADJ_FILENAME= adjoint\\nGRAD_OBJFUNC_FILENAME= of_grad\\nSURFACE_FILENAME= surface\\nSURFACE_ADJ_FILENAME= surface_adjoint\\nOUTPUT_WRT_FREQ= 1\\nSCREEN_OUTPUT= ( INNER_ITER, RMS_DENSITY, LIFT, DRAG )\\nMESH_OUT_FORMAT= SU2\\nMGLEVEL= 0\\nSCREEN_WRT_FREQ_INNER= 10\\nWRT_RESTART_COMPACT= NO\\nCOMPUTE_METRIC= YES\\nADAP_REMESHER= NATIVE_CAVITY\\nADAP_SURFACE= YES\\nNUM_METHOD_HESS= QUADRATIC_LEAST_SQUARES\\nADAP_HESSIAN_NOISE= 0\\nADAP_SENSOR= ( MACH )\\nADAP_HMIN= 1E-6\\nADAP_HMAX= 100\\nADAP_ARMAX= 1E6\\nADAP_HGRAD= 1.3\\nADAP_HAUSD= (WING3D, 1E-4, SYMMETRY, 1E-4, FARFIELD, 1E-4)\\nADAP_NATIVE_MAX_WORK= 0\\nADAP_NATIVE_MAX_TIME= 5400\\nADAP_NATIVE_REPARTITION= YES\\nADAP_TRANSFER= CONSERVATIVE\\nWRT_ADAP_MESH= YES\\nWRT_PERFORMANCE= YES\\nVOLUME_OUTPUT= ( COORDINATES, SOLUTION, PRIMITIVE, METRIC, ADAP_SENSOR )\\nADAP_LOOP= NO\\nADAP_COMPLEXITY= 50000\\nADAP_NATIVE_RANKS= 6\\nADAP_ANGLE= 60\\nMARKER_EULER= ( WING3D )\\nADAP_SIZES= ( 50000 )\\nADAP_SUBITER= ( 2 )\\nADAP_FLOW_ITER= ( 1 )\\nADAP_NATIVE_MEMORY_LIMIT= 12288\\n\\nREAD_BINARY_RESTART= YES\\n\\nADAP_NATIVE_JOB_MEMORY_LIMIT= 19073\\nADAP_NATIVE_LOCAL_CORE= YES\\nADAP_NATIVE_LOCAL_SURFACE= YES\\nADAP_NORM= 1.0\\n'\n"
            assert source.count(old) == 1, old
            source = source.replace(old, tolerance_guard + old, 1)
            compile(source, str(BASE), "exec")
            return source

        core.retargeted_source = retargeted_source

        def supervise(source, source_case, case):
            argv = [sys.executable, str(HERE / "run_restart686_equal_recovery.py"), "--internal",
                    "--source-case", str(source_case), "--case-dir", str(case)]
            child = subprocess.Popen(argv, cwd=HERE, start_new_session=True)
            identity = core.wait_identity(child.pid)
            wrapper = {"pid": child.pid, "start_ticks": identity["start_ticks"],
                       "started_unix": time.time(), "argv": argv,
                       "owned_cpu_ids": sorted(os.sched_getaffinity(0))}
            (case.parent / f"{case.name}_controller_launch.json").write_text(json.dumps(wrapper, indent=2) + "\n")
            code = child.wait()
            receipt = core.post_terminal_receipt(case, wrapper)
            assert receipt["all_owned_handles_terminal"], receipt
            raise SystemExit(code)

        core.supervise = supervise
        return core

    helper.load_base = load_base
    return helper


def self_check():
    helper = runner_context()
    core = helper.load_base()
    source = core.retargeted_source()
    assert "run_restart686_equal_recovery.py" in source and "validate_restart686.py" in source
    assert str(BUILD) in source and "build534" not in source
    assert str(SECTION68 / "validation/unchanged_build499_source_pins.json") in source
    validator = (HERE / "validate_restart686.py").read_text()
    assert 'log=(case / "run.log").read_text(errors="replace")' in validator
    status = json.loads((BUILD / "build_status.json").read_text())
    assert status.get("state") == "terminal" and status.get("exit_code") == 0
    return {"passed": True, "launched": False, "build": str(BUILD),
            "accepted_runner": str(BASE), "validator": str(HERE / "validate_restart686.py"),
            "build_sha256": status["binary_sha256"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--internal", action="store_true")
    parser.add_argument("--source-case", type=Path)
    parser.add_argument("--case-dir", type=Path)
    args = parser.parse_args()
    if args.self_check:
        print(json.dumps(self_check(), indent=2))
        return
    if args.source_case is None:
        parser.error("provide --source-case")
    helper = runner_context()
    core = helper.load_base()
    source_case = args.source_case.resolve(strict=True)
    reference = source_case / "accepted_reference_payloads.json"
    payloads = json.loads(reference.read_text())
    assert len(payloads["hashes"]) == 6
    assert source_case.name == "full686_run1"
    # Bind only the accepted validator's old reference path and expected hashes.
    helper.verify_n8_identity.__globals__.update(SECTION1=source_case, REFERENCE_HASHES=payloads["hashes"])
    helper.read_source_receipts(source_case)
    core.apply_owned_cpu_budget()
    case = args.case_dir.resolve() if args.case_dir else HERE / f"disk_restart_{source_case.name}_event2"
    assert HERE in case.parents and not case.exists()
    source = core.retargeted_source()
    if args.internal:
        os.environ["SU2_SECTION61_SOURCE_CASE"] = str(source_case)
        os.environ["SU2_SECTION61_RESTART_CASE"] = str(case)
        exec(compile(source, str(BASE), "exec"),
             {"__name__": "__main__", "__file__": str(HERE / "run_restart686_equal_recovery.py")})
        return
    core.supervise(source, source_case, case)


if __name__ == "__main__":
    main()
