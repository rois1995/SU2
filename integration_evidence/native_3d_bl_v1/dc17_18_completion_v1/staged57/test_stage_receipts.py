"""Lightweight completeness checks for bounded native-stage capture receipts."""
import importlib.util
import json
import struct
import tempfile
from pathlib import Path


RUNNER = Path(__file__).with_name("run_cases.py")
spec = importlib.util.spec_from_file_location("stage_run_cases", RUNNER)
run_cases = importlib.util.module_from_spec(spec)
spec.loader.exec_module(run_cases)


def write_manifest(root, rank, label, *, owned_count=0, donor_bytes=b"", parameters=None):
    stem = f"stage_{label}_rank{rank}"
    owned = f"{stem}_owned.bin"
    donors = f"{stem}_donors.bin"
    (root / owned).write_bytes(struct.pack("=Q", owned_count) + b"x" * owned_count)
    (root / donors).write_bytes(struct.pack("=Q", len(donor_bytes)) + donor_bytes)
    data = {
        "complete_shard": True,
        "private_capture": True,
        "rank": rank,
        "record_format": 1,
        "final_mesh_accepted": False,
        "exact_continuation": False,
        "working_rows": owned_count,
        "donor_rows": len(donor_bytes),
        "owned_record_bytes": 1,
        "donor_record_bytes": 1,
        "files": {"owned": owned, "donors": donors},
        "parameters_present": parameters is not None,
        "parameters_payload_bytes": 0 if parameters is None else len(parameters),
    }
    if label == "observation_stop":
        data.update(diagnostic_observation_stop=True, quality_windows_observed=4)
    if parameters is not None:
        name = f"{stem}_parameters.bin"
        (root / name).write_bytes(parameters)
        data["files"]["parameters"] = name
    (root / f"{stem}_manifest.json").write_text(json.dumps(data))


def write_rank(root, rank=0, *, donors=b"d", windows=4, parameters=None,
               quality_owned=0, stop_owned=0):
    write_manifest(root, rank, "quality_entry", owned_count=quality_owned, donor_bytes=donors)
    write_manifest(root, rank, "observation_stop", owned_count=stop_owned, donor_bytes=donors,
                   parameters=parameters)
    traces = [{"stage": "window", "phase": "improvement", "round": (i+1)*64} for i in range(windows)]
    (root / f"stage_trace_rank{rank}.jsonl").write_text("".join(json.dumps(x) + "\n" for x in traces))


def rejects(mutator, ranks=1):
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_rank(root)
        mutator(root)
        try:
            run_cases.validate_stage_shards(root, ranks)
        except Exception as error:
            return error
        raise AssertionError("validator accepted malformed stage receipts")


def test_valid_and_rejected_cases():
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_rank(root)
        result = run_cases.validate_stage_shards(root, 1)
        assert result == {"complete_rank_shards": 1, "quality_windows": 4,
                          "original_donors_unchanged": True, "exact_continuation": False,
                          "full_mesh_acceptance": False}
        pins = json.loads((root / "stage_output_pins.json").read_text())
        assert len(pins) == 7 and all(len(digest) == 64 for digest in pins.values())

    truncated_row_error = rejects(lambda root: (root / "stage_quality_entry_rank0_owned.bin").write_bytes(b"\x00" * 7))
    assert isinstance(truncated_row_error, struct.error), "truncated rows should be rejected (currently via struct.error)"
    missing_rank_error = rejects(lambda root: None, ranks=2)
    assert isinstance(missing_rank_error, FileNotFoundError), "missing rank shard should be rejected (currently via FileNotFoundError)"
    rejects(lambda root: (root / "stage_observation_stop_rank0_donors.bin").write_bytes(
        struct.pack("=Q", 1) + b"z"))
    rejects(lambda root: write_rank(root, windows=3))
    rejects(lambda root: (write_rank(root, parameters=b"xx"),
                          (root / "stage_observation_stop_rank0_parameters.bin").write_bytes(b"x")))
    with tempfile.TemporaryDirectory() as temp:
        root = Path(temp)
        write_rank(root, 0)
        write_rank(root, 1)
        assert run_cases.validate_stage_shards(root, 2)["complete_rank_shards"] == 2
        path = root / "stage_trace_rank1.jsonl"
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        rows[-1]["round"] += 1
        path.write_text("".join(json.dumps(row)+"\n" for row in rows))
        try:
            run_cases.validate_stage_shards(root, 2)
        except RuntimeError as error:
            assert "rounds differ" in str(error)
        else:
            raise AssertionError("validator accepted mismatched worker rounds")


if __name__ == "__main__":
    test_valid_and_rejected_cases()
    print("stage receipt self-check passed")
