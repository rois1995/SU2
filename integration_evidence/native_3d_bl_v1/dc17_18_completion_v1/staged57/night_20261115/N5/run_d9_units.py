#!/usr/bin/env python3
"""Run one descriptor-pinned hidden D9 unit measurement after primary readiness."""

import argparse
import json
import math
import re
import os
import sys
import signal
import shutil
import time
from pathlib import Path

import run_measurements as core

N5 = core.N5
STAGED = core.STAGED


def strict_json(path):
    def reject_constant(value):
        raise ValueError(f"nonstandard JSON number: {value}")
    data = json.loads(path.read_text(), parse_constant=reject_constant)
    def check(value):
        if isinstance(value, float):
            assert math.isfinite(value), f"non-finite JSON number in {path}"
        elif isinstance(value, dict):
            for item in value.values():
                check(item)
        elif isinstance(value, list):
            for item in value:
                check(item)
    check(data)
    return data


ACTION_RANKS = {"serial_stack": {1}, "geometry": {1}, "counter_check": {1},
                "operators": {1}, "locality": {2, 4, 6}, "actual_operators": {1}}
ACTION_TAGS = {"serial_stack": "[.][NativeD9SerialStack]", "geometry": "[.][NativeD9Geometry]",
               "counter_check": "[.][NativeD9CounterCheck]", "operators": "[.][NativeD9Operators]",
               "locality": "[.][NativeD9Locality]", "actual_operators": "[.][NativeD9ActualOperators]"}
MEMORY_LIMIT = 19999490048


def validate_descriptor(path):
    descriptor = strict_json(path)
    assert descriptor.get("schema") == "n5_d9_measurement_v1"
    action = descriptor.get("action")
    env = descriptor.get("environment")
    hashes = descriptor.get("input_file_sha256")
    assert action in ACTION_RANKS
    assert isinstance(env, dict) and isinstance(hashes, dict)
    assert descriptor.get("ranks") in ACTION_RANKS[action]
    assert descriptor.get("test_filter") == ACTION_TAGS[action]
    resolved_inputs = set()
    for raw, expected in hashes.items():
        assert re.fullmatch(r"[0-9a-f]{64}", expected)
        path_input = Path(raw).resolve(strict=True)
        assert core.sha(path_input) == expected, f"descriptor input hash mismatch: {path_input}"
        resolved_inputs.add(path_input)
    if action == "serial_stack":
        assert descriptor["ranks"] == 1
        assert set(env) == {"SU2_D9_STACK_MODE", "SU2_D9_STACK_JSON"}
        assert env["SU2_D9_STACK_MODE"] in {"M12_control", "M12_meet", "M10_n1", "M10_n2", "M10_n4"}
        assert env["SU2_D9_STACK_JSON"] == "d9_stack_measurement.json"
    elif action == "geometry":
        assert descriptor["ranks"] == 1
        assert set(env) <= {"SU2_D9_REFERENCE", "SU2_D9_PARAMETERS", "SU2_D9_WALL_MARKERS"}
        ref = Path(env["SU2_D9_REFERENCE"]).resolve(strict=True)
        assert ref in resolved_inputs
        if env.get("SU2_D9_PARAMETERS"):
            assert Path(env["SU2_D9_PARAMETERS"]).resolve(strict=True) in resolved_inputs
    elif action in ("actual_operators", "locality"):
        assert set(env) == {"SU2_NATIVE_D9_MESH"}
        mesh = Path(env["SU2_NATIVE_D9_MESH"]).resolve(strict=True)
        sidecar = Path(str(mesh) + ".native_ref").resolve(strict=True)
        assert mesh in resolved_inputs and sidecar in resolved_inputs
    else:
        assert not env and not hashes
    assert descriptor.get("case_name", "").startswith("n5_d9_")
    return descriptor

def pin_descriptor(runtime, descriptor_path, descriptor):
    old_save = runtime.save
    pinned = {str(descriptor_path): core.sha(descriptor_path)}
    pinned.update({str(Path(p).resolve()): digest for p, digest in descriptor["input_file_sha256"].items()})
    def save_with_descriptor(path, value):
        if Path(path).name == "pins.json":
            value.update(pinned)
        old_save(path, value)
    runtime.save = save_with_descriptor


def validate_mpi_resource_receipt(path, ranks):
    data = strict_json(path)
    assert data.get("schema") == "n5_d9_unit_resource_guard_v2"
    assert data.get("expected_ranks") == ranks
    assert data.get("all_expected_rank_peaks_seen") is True
    assert data.get("all_owned_handles_terminal") is True
    assert data.get("resource_guard_triggered") is False
    assert data.get("live_hwm_lower_bound") is True
    assert data.get("gnu_time_child_maximum_is_job_peak") is False
    rank_peaks = data.get("rank_peak_hwm_bytes")
    assert isinstance(rank_peaks, dict) and set(rank_peaks) == {str(rank) for rank in range(ranks)}
    total = sum(rank_peaks.values())
    assert data.get("sum_rank_peak_hwm_bytes") == total
    assert total <= MEMORY_LIMIT
    return data


def parse_rank_peak(text):
    peaks = re.findall(r'Maximum resident set size \(kbytes\):\s*(\d+)', text)
    assert peaks, "rank GNU time peak is missing"
    return int(peaks[-1]) * 1024


def wrap_rank_timing_argv(argv, test_binary, shell):
    args = list(argv)
    if "mpirun" not in args or str(test_binary) not in args:
        return args
    index = args.index(str(test_binary))
    rank_shell = 'exec /usr/bin/time -v -o "d9_rank_${OMPI_COMM_WORLD_RANK:?}_peak.txt" "$@"'
    return args[:index] + [shell, "-c", rank_shell, "n5-rank-time"] + args[index:]


def install_rank_timing(runtime, test_binary):
    shell = shutil.which("sh")
    assert shell, "POSIX sh is unavailable"
    shell_path = Path(shell).resolve(strict=True)
    old_save = runtime.save
    def save_with_shell_pin(path, value):
        if Path(path).name == "pins.json":
            value[str(shell_path)] = core.sha(shell_path)
        old_save(path, value)
    runtime.save = save_with_shell_pin
    old_popen = runtime.subprocess.Popen
    def popen_with_rank_timing(argv, *args, **kwargs):
        original = list(argv) if isinstance(argv, (list, tuple)) else argv
        wrapped = wrap_rank_timing_argv(argv, test_binary, str(shell_path))
        if wrapped != list(argv):
            cwd = Path(kwargs["cwd"])
            core.dump(cwd / "d9_rank_timing_wrapper.json", {
                "schema": "n5_d9_rank_timing_wrapper_v1",
                "original_argv": original,
                "wrapped_argv": wrapped,
                "shell_path": str(shell_path),
                "shell_sha256": core.sha(shell_path),
                "rank_file_template": "d9_rank_${OMPI_COMM_WORLD_RANK}_peak.txt",
                "rank_environment_required": "OMPI_COMM_WORLD_RANK",
            })
        return old_popen(wrapped, *args, **kwargs)
    runtime.subprocess.Popen = popen_with_rank_timing
    return old_popen, old_save


def record_stack_artifacts(data, attempt, artifacts):
    assert data.get("mode")
    assert all(key in data for key in ("timing_seconds", "phases", "room", "positive_validity_and_topology"))
    phase_log = attempt / "d9_stack_phase_log.txt"
    assert phase_log.is_file()
    artifacts[phase_log.name] = core.sha(phase_log)
    if data["phases"].get("G_exception"):
        assert data["phases"]["G_returned"] is False and data["phases"]["G"] is None
        assert data["columns"] is None and data["columns_by_wall"] is None


def produced_artifacts(action, attempt, ranks, resource_path):
    if action == "serial_stack":
        paths = [attempt / "d9_stack_measurement.json"]
    elif action == "geometry":
        paths = [attempt / "d9_geometry_summary.json"]
    elif action == "operators":
        paths = [attempt / "d9_native_operator_costs.json"]
    elif action == "actual_operators":
        paths = [attempt / "native_d9_actual_operators_m1.json"]
    elif action == "locality":
        paths = [attempt / f"native_d9_locality_m{ranks}.json"]
        paths.extend(attempt / f"native_d9_locality_owners_rank{rank}.json" for rank in range(ranks))
    else:
        paths = []
    for path in paths:
        assert path.is_file() and path.stat().st_size > 0, f"required artifact missing: {path}"
        if action != "serial_stack":
            assert isinstance(strict_json(path), dict), f"artifact is not a JSON object: {path}"
    log_path = attempt / "run.log"
    assert log_path.is_file()
    log = log_path.read_text(errors="replace")
    assert "No test cases matched" not in log
    expected_test = {"operators": "NativeD9Operators", "locality": "NativeD9Locality",
                     "actual_operators": "NativeD9ActualOperators"}.get(action)
    if expected_test:
        assert expected_test in log, f"requested hidden test is absent from run.log: {expected_test}"
    if ranks == 1:
        peaks = re.findall(r'Maximum resident set size \(kbytes\):\s*(\d+)', log)
        assert peaks and int(peaks[-1]) * 1024 <= MEMORY_LIMIT, "serial GNU time peak exceeds job gate"
    else:
        live = validate_mpi_resource_receipt(resource_path, ranks)
        rank_files = [attempt / f"d9_rank_{rank}_peak.txt" for rank in range(ranks)]
        rank_peaks = [max(parse_rank_peak(path.read_text(errors="replace")),
                          live["rank_peak_hwm_bytes"][str(rank)]) for rank, path in enumerate(rank_files)]
        rank_sum = sum(rank_peaks)
        assert rank_sum <= MEMORY_LIMIT, "summed final per-rank GNU time peaks exceed job gate"
        summary_path = attempt / "d9_rank_peak_summary.json"
        core.dump(summary_path, {
            "schema": "n5_d9_rank_peak_summary_v1",
            "rank_peak_bytes": {str(rank): peak for rank, peak in enumerate(rank_peaks)},
            "sum_rank_peak_bytes": rank_sum,
            "maximum_job_bytes": MEMORY_LIMIT,
            "within_limit": rank_sum <= MEMORY_LIMIT,
            "meaning": "sum of per-rank maximum of final GNU-time RSS and observed VmHWM; not simultaneous whole-job peak",
            "rank_files": {path.name: core.sha(path) for path in rank_files},
        })
        wrapper_path = attempt / "d9_rank_timing_wrapper.json"
        wrapper = strict_json(wrapper_path)
        assert wrapper.get("schema") == "n5_d9_rank_timing_wrapper_v1"
        assert len(wrapper.get("wrapped_argv", [])) > len(wrapper.get("original_argv", []))
        assert wrapper.get("rank_environment_required") == "OMPI_COMM_WORLD_RANK"
        pins = strict_json(attempt / "pins.json")
        assert pins.get(wrapper["shell_path"]) == wrapper.get("shell_sha256")
    artifacts = {path.name: core.sha(path) for path in [*paths, log_path]}
    if ranks > 1:
        artifacts[resource_path.name] = core.sha(resource_path)
        artifacts[wrapper_path.name] = core.sha(wrapper_path)
        artifacts[summary_path.name] = core.sha(summary_path)
        artifacts.update({path.name: core.sha(path) for path in rank_files})
        resources_path = attempt / "resources.jsonl"
        assert resources_path.is_file() and resources_path.stat().st_size > 0
        artifacts[resources_path.name] = core.sha(resources_path)
    if action in ("serial_stack", "geometry", "operators", "actual_operators"):
        data = strict_json(paths[0])
        schemas = {"serial_stack": "D9_stack_v1", "geometry": "d9_geometry_measurement_v1",
                   "operators": "d9_native_operator_costs_v1"}
        if action in schemas:
            assert data.get("schema") == schemas[action]
        if action == "serial_stack":
            record_stack_artifacts(data, attempt, artifacts)
        elif action == "geometry":
            assert data.get("status") in {"observed", "unsupported_reference_has_no_geometry_authority",
                                           "unsupported_missing_parameter_state"}
            if data["status"] == "observed":
                for name in ("d9_geometry_edges.csv", "d9_geometry_faces.csv", "d9_geometry_projections.csv"):
                    path = attempt / name
                    assert path.is_file() and path.stat().st_size > 0, f"geometry output missing: {path}"
                    artifacts[name] = core.sha(path)
    return artifacts


class UnitResourceGuard:
    def __init__(self, runtime, path, expected_ranks):
        self.original = runtime.sample_tree
        self.path = path
        self.expected_ranks = expected_ranks
        self.handles = {}
        self.peaks = {}
        self.rank_for_handle = {}
        self.rank_peaks = {}
        self.unranked = set()
        self.exceeded = False
        runtime.sample_tree = self.sample

    @staticmethod
    def identity(pid):
        try:
            proc = Path("/proc") / str(pid)
            fields = (proc / "stat").read_text().rsplit(")", 1)[1].split()
            return {"pid": pid, "start_ticks": int(fields[19]), "state": fields[0],
                    "pgid": int(fields[2]), "sid": int(fields[3])}
        except (OSError, ValueError, IndexError):
            return None

    def rank(self, pid):
        if self.expected_ranks == 1:
            return 0
        try:
            values = (Path("/proc") / str(pid) / "environ").read_bytes().split(b"\0")
            env = dict(item.split(b"=", 1) for item in values if b"=" in item)
            for key in (b"OMPI_COMM_WORLD_RANK", b"PMI_RANK", b"PMIX_RANK", b"SLURM_PROCID"):
                if key in env:
                    value = int(env[key])
                    return value if 0 <= value < self.expected_ranks else None
        except (OSError, ValueError):
            pass
        return None

    def sample(self, root):
        rows = self.original(root)
        for row in rows:
            identity = self.identity(row["pid"])
            if not identity:
                continue
            key = (identity["pid"], identity["start_ticks"])
            self.handles[key] = dict(identity, name=row["name"])
            if row["name"] != "test_driver":
                continue
            rank = self.rank(row["pid"])
            self.rank_for_handle[key] = rank
            try:
                status = (Path("/proc") / str(row["pid"]) / "status").read_text()
                hwm = next(line for line in status.splitlines() if line.startswith("VmHWM:"))
                peak = int(hwm.split()[1]) * 1024
                self.peaks[key] = max(self.peaks.get(key, 0), peak)
                if rank is None:
                    self.unranked.add(str(row["pid"]))
                else:
                    self.rank_peaks[rank] = max(self.rank_peaks.get(rank, 0), peak)
            except (OSError, StopIteration):
                pass
        rank_sum = sum(self.rank_peaks.values())
        self.exceeded |= (rank_sum > MEMORY_LIMIT or len(self.rank_peaks) > self.expected_ranks
                         or bool(self.unranked) or len(self.peaks) > 6)
        if self.exceeded:
            self.signal_owned(signal.SIGTERM)
        return rows

    def signal_owned(self, signum):
        for saved in reversed(list(self.handles.values())):
            current = self.identity(saved["pid"])
            if not self.same_live_identity(saved, current):
                continue
            try:
                os.kill(saved["pid"], signum)
            except ProcessLookupError:
                pass

    @staticmethod
    def same_live_identity(saved, current):
        return bool(current and current["start_ticks"] == saved["start_ticks"]
                    and current["state"] not in ("Z", "X"))

    def finish(self):
        if self.exceeded:
            deadline = time.monotonic() + 10
            while time.monotonic() < deadline:
                if not any(self.same_live_identity(item, self.identity(item["pid"]))
                           for item in self.handles.values()):
                    break
                time.sleep(.2)
            self.signal_owned(signal.SIGKILL)
        handles = []
        for (pid, ticks), saved in self.handles.items():
            actual = self.identity(pid)
            alive = bool(actual and actual["start_ticks"] == ticks and actual["state"] not in ("Z", "X"))
            handles.append(dict(saved, alive=alive, mpi_rank=self.rank_for_handle.get((pid, ticks))))
        observed = set(self.rank_peaks)
        expected = set(range(self.expected_ranks))
        data = {"schema": "n5_d9_unit_resource_guard_v2", "actual_host_proc": True, "unix": time.time(),
                "expected_ranks": self.expected_ranks, "rank_peak_hwm_bytes": {str(k): v for k, v in sorted(self.rank_peaks.items())},
                "sum_rank_peak_hwm_bytes": sum(self.rank_peaks.values()), "unranked_test_driver_pids": sorted(self.unranked),
                "all_expected_rank_peaks_seen": observed == expected and not self.unranked,
                "identities": handles, "all_owned_handles_terminal": not any(row["alive"] for row in handles),
                "maximum_job_bytes": MEMORY_LIMIT, "resource_guard_triggered": self.exceeded,
                "rank_peak_basis": "per-rank /proc VmHWM maxima sampled while test_driver lives",
                "live_hwm_lower_bound": True, "gnu_time_child_maximum_is_job_peak": False,
                "peak_note": "named sum of observed per-rank test_driver VmHWM maxima; live lower-bound observation, not GNU time maximum-child RSS"}
        core.dump(self.path, data)
        assert data["all_owned_handles_terminal"], "owned fixture handles remain live"
        if self.expected_ranks > 1:
            assert data["all_expected_rank_peaks_seen"], "missing MPI rank VmHWM observation"
        assert not self.exceeded, "fixture exceeded unchanged night resource limit"


def run(descriptor_path):
    core.verify_run_ready()
    descriptor_path = descriptor_path.resolve(strict=True)
    descriptor = validate_descriptor(descriptor_path)
    receipt = N5 / "unit_measurement_receipts" / (descriptor["case_name"] + ".json")
    assert not receipt.exists(), f"refusing to repeat existing measurement action: {receipt}"
    sys.path.insert(0, str(STAGED))
    import run_cases as runtime
    status, _, _, _, test_binary = core.verify_build()
    runtime.ROOT = N5
    runtime.BINARY = test_binary
    ranks = descriptor["ranks"]
    runtime.CASES[descriptor["case_name"]] = (ranks, descriptor["test_filter"])
    pin_descriptor(runtime, descriptor_path, descriptor)
    for key in tuple(os.environ):
        if key.startswith("SU2_D9_") or key == "SU2_NATIVE_D9_MESH":
            os.environ.pop(key)
    os.environ.update(descriptor["environment"])
    sys.argv = [str(STAGED / "run_cases.py"), core.BUILD520.name, descriptor["case_name"]]
    core.dump(N5 / (descriptor['case_name'] + '_preflight.json'),
              {'unix': time.time(), 'loadavg': Path('/proc/loadavg').read_text(),
               'MemAvailable': next(line for line in Path('/proc/meminfo').read_text().splitlines()
                                    if line.startswith('MemAvailable:'))})
    resource_path = N5 / (descriptor["case_name"] + "_resource_handles.json")
    old_popen, old_save = install_rank_timing(runtime, test_binary)
    guard = UnitResourceGuard(runtime, resource_path, ranks)
    try:
        runtime.main(STAGED / "runtime_20261113_474/loaded_runtime_pins.json")
    except SystemExit as result:
        if result.code not in (None, 0):
            raise
    finally:
        try:
            guard.finish()
        finally:
            runtime.subprocess.Popen = old_popen
            runtime.save = old_save
    attempts = sorted((N5 / "cases").glob(descriptor["case_name"] + "_*"), key=lambda p: p.stat().st_mtime)
    assert attempts
    attempt = attempts[-1]
    result = strict_json(attempt / "status.json")
    assert result.get("state") == "terminal" and result.get("exit_code") == 0 and result.get("pins_verified") is True
    artifacts = produced_artifacts(descriptor["action"], attempt, ranks, resource_path)
    resource = strict_json(resource_path)
    receipt.parent.mkdir(parents=True, exist_ok=True)
    assert not receipt.exists(), f"refusing to overwrite {receipt}"
    core.dump(receipt, {"schema": "n5_d9_unit_action_v1", "case": descriptor["case_name"],
                        "descriptor": str(descriptor_path), "descriptor_sha256": core.sha(descriptor_path),
                        "attempt": str(attempt), "ranks": ranks,
                        "attempt_status_sha256": core.sha(attempt / "status.json"),
                        "artifacts_sha256": artifacts, "resource_guard": resource,
                        "rank_peak_summary": strict_json(attempt / "d9_rank_peak_summary.json") if ranks > 1 else None,
                        "source_pins_sha256": core.sha(core.BUILD520 / "source_pins.json"),
                        "test_binary_sha256": status["test_binary_sha256"], "passed": True})
    return strict_json(receipt)


def self_check():
    assert "NativeD9CounterCheck" in (core.REPO / "UnitTests/Common/adaptation/CNativeDistributed3D_tests.cpp").read_text()
    assert "NativeD9SerialStack" in (core.REPO / "UnitTests/SU2_CFD/adaptation/fixtures/native_first_row_dc12.inc").read_text()
    assert "NativeD9Geometry" in (core.REPO / "UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp").read_text()
    sample = ["nice", "-n", "19", "timeout", "600", "mpirun", "-n", "2", "/x/test_driver", "filter"]
    wrapped = wrap_rank_timing_argv(sample, Path("/x/test_driver"), "/bin/sh")
    assert wrapped[wrapped.index("mpirun") + 3:wrapped.index("mpirun") + 7] == [
        "/bin/sh", "-c", 'exec /usr/bin/time -v -o "d9_rank_${OMPI_COMM_WORLD_RANK:?}_peak.txt" "$@"', "n5-rank-time"]
    assert parse_rank_peak("Maximum resident set size (kbytes): 123\n") == 123 * 1024
    saved = {"pid": 1, "start_ticks": 5}
    assert UnitResourceGuard.same_live_identity(saved, {"start_ticks": 5, "state": "R"})
    assert not UnitResourceGuard.same_live_identity(saved, {"start_ticks": 6, "state": "R"})
    assert not UnitResourceGuard.same_live_identity(saved, {"start_ticks": 5, "state": "Z"})
    return {"passed": True, "launched": False, "rank_wrapper_selfcheck": True,
            "counter_check_artifact": "run.log",
            "measurement_json_is_required_for": ["serial_stack", "geometry"], "unit_timeout_seconds": 600,
            "new_actions": {"operators": [1], "locality": [2, 4, 6], "actual_operators": [1]},
            "locality_outputs": "native_d9_locality_mN.json plus native_d9_locality_owners_rankR.json",
            "actual_operators_input_hashes": ["SU2_NATIVE_D9_MESH", "adjacent .native_ref"],
            "mpi_memory_source": "gate sums each rank GNU-time maximum RSS; live per-rank VmHWM remains the early-stop lower-bound monitor"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-check", action="store_true")
    parser.add_argument("--descriptor", type=Path)
    args = parser.parse_args()
    if args.self_check:
        assert args.descriptor is None
        print(json.dumps(self_check(), indent=2))
        return
    if args.descriptor is None:
        parser.error("provide --descriptor FILE or --self-check")
    print(json.dumps(run(args.descriptor), indent=2))


if __name__ == "__main__":
    main()
