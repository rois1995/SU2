"""Assess the bounded sequential surface-transaction trajectory on saved M6 data."""
import hashlib
import json
import struct
import time
from pathlib import Path

R = Path(__file__).resolve().parent


def sha(path):
    with Path(path).open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def owned_rows(path):
    with Path(path).open("rb") as stream:
        count = struct.unpack("<Q", stream.read(8))[0]
        for _ in range(count):
            row = stream.read(416)
            assert len(row) == 416
            yield struct.unpack_from("<Q", row, 8)[0], hashlib.sha256(row).digest()
        assert not stream.read(1)


def terminal_receipt(case):
    receipt = json.loads((case / "terminal_handles.json").read_text())
    identities = receipt["sampled_identities"] + [receipt["wrapper_excluded"]]
    checked = []
    for row in identities:
        try:
            fields = Path("/proc", str(row["pid"]), "stat").read_text().rsplit(")", 1)[1].split()
            terminal = int(fields[19]) != row["start_ticks"] or fields[0] == "Z"
            state = fields[0]
        except FileNotFoundError:
            terminal, state = True, "absent"
        assert terminal, row
        checked.append(dict(**row, terminal=True, state=state))
    return dict(actual_host_proc=True, all_owned_handles_terminal=True,
                wrapper_verified_terminal=True, identities=checked, checked_unix=time.time())


def main():
    cases = list((R / "cases").glob("curved473_surface_transaction_serial_*"))
    assert cases, "no 470 transaction case found"
    case = max(cases, key=lambda path: path.stat().st_mtime)
    status = json.loads((case / "status.json").read_text())
    assert status["state"] == "terminal" and status["exit_code"] == 0 and status["pins_verified"]
    pins = json.loads((case / "pins.json").read_text())
    assert all(sha(path) == digest for path, digest in pins.items())
    handles = terminal_receipt(case)
    (case / "terminal_handles_actual.json").write_text(json.dumps(handles, indent=2) + "\n")

    records = [json.loads(line) for line in (case / "actual_bl_cavity_planning.jsonl").read_text().splitlines()]
    prefix = [row for row in records if row["stage"] == "split_continuation"]
    assert len(prefix) == 1 and prefix[0]["private_published"] and prefix[0]["new_cells"] == 19
    inputs = [row for row in records if row["stage"] == "transaction_input"]
    states = [row for row in records if row["stage"] == "transaction_state"]
    operations = [row for row in records if row["stage"] == "transaction_operation"]
    final_views = [row for row in records if row["stage"] == "transaction_final_observation"]
    results = [row for row in records if row["stage"] == "transaction_result"]
    assert len(inputs) == len(final_views) == len(results) == 1 and states
    assert inputs[0]["initial_observation_cells"] == 36
    assert inputs[0]["replay_steps"] == 16
    replay_steps=[row for row in records if row["stage"]=="transaction_replay_step"]
    replay_checkpoints=[row for row in records if row["stage"]=="transaction_replay_checkpoint"]
    assert len(replay_steps)==16 and all(row["exact_coordinate_match"] for row in replay_steps)
    assert [row["step"] for row in replay_steps]==list(range(16))
    assert len(replay_checkpoints)==1 and replay_checkpoints[0]["exact_topology_coordinates_physical_support"]
    assert replay_checkpoints[0]["prefix_commits"]==16 and replay_checkpoints[0]["cells"]==2
    assert any(row["operator"]=="ReconstructSplitPatch" for row in operations)
    direct=operations[16];assert direct["operator"]=="SplitSurfaceStrip" and not direct["constructed"]
    assert "inverts inherited volume connectivity" in direct["reason"]
    assert inputs[0]["observation_is_transaction"] is False
    assert inputs[0]["requested_height_unchanged"] is True
    result = results[0]
    final_view = final_views[0]
    assert 1 <= result["attempts"] <= 24
    assert result["attempts"] == len(operations)
    assert result["commits"] == sum(bool(row["published"]) for row in operations)
    assert [row["pass"] for row in states] == list(range(len(states)))
    assert states[0]["cells"] == 36
    assert final_view["cells"] == result["final_observation_cells"]
    assert final_view["geometry_bad"] == result["geometry_bad"]
    assert final_view["after_first_row"] == result["first_row_reached"]
    assert result["mesh_to_model_guidance_closed"] == (result["geometry_bad"] == 0)
    assert result["surface_geometry_certified"] is False
    assert result["original_mesh_accepted"] is False

    pub_delta = [0, 0, 0]
    published = []
    for index, row in enumerate(operations, 1):
        assert row["attempt"] == index
        assert row["old_cells"] <= 64 and row["new_cells"] <= 128
        assert row["measurements_known"] == row["constructed"]
        assert row["reserved"] and row["native_input_topology"]
        if row["published"]:
            assert row["measurements_known"]
            assert all(row[key] for key in (
                "constructed", "native_input_topology", "metric", "parameters",
                "contact_protocol", "contact", "preserved_heights", "provisional_height", "reserved"))
            pub_delta[0] += row["new_q_bad"] - row["old_q_bad"]
            pub_delta[1] += row["new_J_bad"] - row["old_J_bad"]
            pub_delta[2] += row["new_geometry_bad"] - row["old_geometry_bad"]
            published.append(row)
    expected_delta = [result["sum_local_q_delta"], result["sum_local_J_delta"],
                      result["sum_local_geometry_delta"]]
    assert pub_delta == expected_delta
    for state in states:
        local = [0, 0, 0]
        for row in published[:state["commits"]]:
            local[0] += row["new_q_bad"] - row["old_q_bad"]
            local[1] += row["new_J_bad"] - row["old_J_bad"]
            local[2] += row["new_geometry_bad"] - row["old_geometry_bad"]
        assert local == [state["sum_local_q_delta"], state["sum_local_J_delta"],
                         state["sum_local_geometry_delta"]]

    # These are transaction-local delta sums; scope expansion means they are not a global field census.
    state_summary = [{key: row[key] for key in ("pass", "cells", "commits", "attempts", "q_bad", "J_bad",
                                                  "geometry_bad", "geometry_excess", "scope_added_old_cells",
                                                  "sum_local_q_delta", "sum_local_J_delta", "sum_local_geometry_delta")}
                     for row in states]
    owned_export = case / "correction_unchanged_owned.bin"
    assert owned_export.exists()
    original = R / "cases/curved451_strip_reuse_saved_target_surface_mpi6_20261112T185700_120424"
    expected = {}
    for rank in range(6):
        for identity, digest in owned_rows(original / f"stage_surface_rank{rank}_owned.bin"):
            assert identity not in expected
            expected[identity] = digest
    matched = 0
    for identity, digest in owned_rows(owned_export):
        assert expected.pop(identity) == digest
        matched += 1
    assert not expected and matched == 303622

    report = dict(
        case=str(case), status=status, all_source_runtime_input_pins_verified=len(pins),
        all_actual_handles_terminal=True, states=states, final_observation=final_view,
        operations=operations, result=result,
        local_delta_check=dict(published_operations=len([row for row in operations if row["published"]]),
                               summed_q_j_geometry_deltas=pub_delta,
                               scope_added_old_cells=result["scope_added_old_cells"],
                               state_summary=state_summary),
        owned_records_byte_identical_to_saved451=matched,
        owned_export_sha256=sha(owned_export), script_sha256=sha(__file__),
        limits=["Serial private trajectory only; no production or MPI qualification.",
                "Observed region is a scope for geometry tracking, not one transaction cavity.",
                "Summed q/J/geometry values are local published-transaction deltas, not whole-field churn.",
                "Mesh-to-model guidance closure is not final bidirectional geometry certification.",
                "Tangential suitability, first-row readiness, accepted BL/RANS workflow, and mesh acceptance are not established."],
        production_changed=True, production_qualified=False, goal_complete=False)
    (R / "surface_transaction473_assessment.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(dict(states=len(states), operations=len(operations), result=result, owned_records=matched), indent=2))


if __name__ == "__main__":
    main()
